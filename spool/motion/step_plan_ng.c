#include "step_plan_ng.h"
#include <math.h>
#include "string.h"
#include "compiler.h"
#include "error.h"
#include "number.h"
#include "dbgprintf.h"
#include "bitops.h"

struct StepperPlanBuf {
    uint32_t size;
    /* consumer, if buf is not empty, points to the slot before the head */
    uint32_t head;
    /* producer, if buf is not full, points to the first available slot */
    uint32_t tail;
    struct PlannerJob buf[MOTION_LOOKAHEAD];
};

static struct StepperPlanBuf stepperPlanBuf;
const static struct PlannerJob empty = {
    .type = StepperJobRun,
};

void plannerInit()
{
    stepperPlanBuf.head = MOTION_LOOKAHEAD - 1;
}

uint32_t plannerSize(void)
{
    return stepperPlanBuf.size;
}

uint32_t plannerCapacity(void)
{
    return MOTION_LOOKAHEAD;
}

uint32_t plannerAvailableSpace(void)
{
    return MOTION_LOOKAHEAD - plannerSize();
}

static inline uint32_t planBufNextIdx(uint32_t i)
{
    return (i == MOTION_LOOKAHEAD - 1 ? 0 : i + 1);
}

static inline uint32_t planBufPrevIdx(uint32_t i)
{
    return (i == 0 ? MOTION_LOOKAHEAD - 1 : i - 1);
}

void plannerDequeue(struct PlannerJob *out)
{
    BUG_ON(plannerSize() == 0);
    stepperPlanBuf.size--;
    stepperPlanBuf.head = planBufNextIdx(stepperPlanBuf.head);
    *out = stepperPlanBuf.buf[stepperPlanBuf.head];
}

static void calcReverse(struct PlannerJob *);

static fix16_t vecDot(const fix16_t a[NR_AXIS], const fix16_t b[NR_AXIS])
{
    fix16_t res = 0;
    for_each_axis(i) {
        res = fix16_add(res, fix16_mul(a[i], b[i]));
    }
    return res;
}

static void populateBlock(const struct PlannerJob *prev, struct PlannerJob *new,
                          const int32_t plan[NR_STEPPER],
                          const uint32_t max_v[NR_STEPPER],
                          const int32_t accMM[NR_STEPPER], bool stop)
{
    float timeEst = 0;
    uint32_t maxStepper = 0;

    for_each_stepper(i) {
        if (plan[i] >= 0)
            new->stepDirs |= BIT(i);
        new->steppers[i].x = (uint32_t)abs(plan[i]);
    }
    for_each_stepper(i) {
        struct PlannerBlock *s = &new->steppers[i];
        if (max_v[i] != 0) {
            timeEst = max((float)s->x / (float)max_v[i], timeEst);
        }
        if (s->x > new->x) {
            new->x = s->x;
            maxStepper = i;
        }
    }

    if (timeEst == 0) {
        /* empty block */
        return;
    }

    uint32_t v = (uint32_t)((float)new->steppers[maxStepper].x / timeEst);
    new->vSq = v * v;

    /* TODO correct for acceleration */
    new->accMM = accMM[maxStepper];
    for_each_stepper(i) {
        if ((new->steppers[i].x > 0) && (accMM[i] < new->accMM)) {
            const int32_t maxPossible = (accMM[i] * new->x / new->steppers[i].x);
            if (new->accMM > maxPossible) {
                new->accMM = maxPossible;
            }
        }
    }

    /* cos(pi - theta) = -cos(theta) */
    fix16_t cosTheta = -vecDot(prev->unit_vec, new->unit_vec);

    if (new->type != StepperJobRun || prev == &empty) {
        new->viSq = 0;
        new->vfSq = 0;
        stop = true;
    } else if (cosTheta > JUNCTION_STOP_VEL_THRES) {
        // Opposite direction, almost stop
        const int32_t minVi = motionGetMinVelocity(maxStepper);
        new->viSq = (minVi * minVi);
    } else {
        fix16_t junctionUnitVec[NR_AXIS];
        fix16_t mag = F16(0);
        for_each_axis(i) {
            junctionUnitVec[i] = new->unit_vec[i] - prev->unit_vec[i];
            mag = fix16_add(mag, fix16_sq(junctionUnitVec[i]));
        }
        mag = fix16_sqrt(mag);
        for_each_axis(i) {
            junctionUnitVec[i] = fix16_div(junctionUnitVec[i], mag);
        }

        // Limit acceleration by axis??
        int32_t jaccMMs2 = new->accMM;
        for_each_axis(i) {
            if (new->unit_vec[i]) {
                if (fix16_mul_int32(fix16_abs(new->unit_vec[i]), jaccMMs2) > (int32_t)accMM[i]) {
                    jaccMMs2 = fix16_mul_int32(
                        fix16_div(F16(1.0), fix16_abs(new->unit_vec[i])),
                        (int32_t)accMM[i]);
                }
            }
        }

        // avoid div by 0
        if (cosTheta < F16(-0.999)) {
            cosTheta = F16(-0.999);
        }
        const fix16_t sinThetaDiv2 =
            fix16_sqrt(fix16_mul(F16(0.5), fix16_sub(F16(1.0), cosTheta)));
        const float oneMinusSinThetaDiv2f =
            fix16_to_float(fix16_sub(F16(1.0), sinThetaDiv2));

        // viSq = (new->a * ((0.05f * sinThetaDiv2) / (1.0f - sinThetaDiv2));
        uint32_t viSq =
            (uint32_t)(fix16_mul_int32(fix16_mul(F16(0.05f), sinThetaDiv2),
                                       jaccMMs2) /
                       oneMinusSinThetaDiv2f);
        viSq *= platformMotionStepsPerMMAxis[maxStepper] * platformMotionStepsPerMMAxis[maxStepper];

        // Special treatment to small segments < 1mm.
        if (new->lenMM < F16(1) && cosTheta < JUNCTION_SMOOTHING_THRES) {
            const fix16_t theta = fix16_acos(cosTheta);
            const uint32_t limit_sqr =
                fix16_mul_int32(fix16_div(new->lenMM, theta), jaccMMs2) * platformMotionStepsPerMMAxis[maxStepper] * platformMotionStepsPerMMAxis[maxStepper];
            if (limit_sqr < viSq) {
                viSq = limit_sqr;
            }
        }
        const uint32_t prevCurVSqMin = min(prev->vSq, new->vSq);
        new->viSq = min(viSq, prevCurVSqMin);
    }

    new->accSteps = new->accMM * platformMotionStepsPerMM[maxStepper];

    if (stop) {
        calcReverse(new);
        return;
    }

    uint32_t accelerationX = (new->vSq - new->viSq) / (2 * new->accSteps);
    if (new->x < accelerationX) {
        new->vSq = new->x * 2 * new->accSteps + new->viSq;
        new->vfSq = new->vSq;
    } else {
        new->accelerationX = accelerationX;
        new->vfSq = new->vSq;
    }
}

/* curr needs to be a move */
static void calcReverse(struct PlannerJob *curr)
{
    WARN_ON(curr->vSq == 0);
    int32_t accelerationX = (curr->vSq - curr->viSq) / (2 * curr->accSteps);
    int32_t decelerationX = (curr->vSq - curr->vfSq) / (2 * curr->accSteps);
    int32_t sum = accelerationX + decelerationX;

    if (curr->x < sum) {
        int32_t direct = (int32_t)accelerationX - (int32_t)decelerationX;
        if (int32_less_abs((int32_t)curr->x, direct)) {
            /* cannot decelerate to desired speed */
            curr->accelerationX = 0;
            curr->decelerationX = curr->x;
            curr->viSq = curr->x * 2 * curr->accSteps + curr->vfSq;
            curr->vSq = curr->viSq;
        } else {
            curr->accelerationX = (uint32_t)(direct + (int32_t)curr->x) / 2;
            curr->decelerationX = curr->x - curr->accelerationX;
            curr->vSq = curr->accelerationX * 2 * curr->accSteps + curr->viSq;
            /* here, due to integer math, vfSq may dip slightly below 0*/
            curr->vfSq =
                (uint32_t)max(0, -(int32_t)(curr->decelerationX * 2 * curr->accSteps) +
                                     (int32_t)curr->vSq);
        }
    } else {
        curr->accelerationX = accelerationX;
        curr->decelerationX = decelerationX;
    }
}

/* requires both arguments to be moves */
static bool recalculate(const struct PlannerJob *next, struct PlannerJob *curr)
{
    bool dirty = false;
    if (abs((int32_t)curr->vfSq - (int32_t)next->viSq) >=
        VEL_CHANGE_THRESHOLD) {
        dirty = true;

        curr->vfSq = next->viSq;
    }
    return dirty;
}

static void reversePass(void)
{
    uint32_t i = planBufPrevIdx(stepperPlanBuf.tail);
    uint32_t size = plannerSize();
    struct PlannerJob *next = NULL;

    while (size > 0) {
        if (next == NULL) {
            next = &stepperPlanBuf.buf[i];
        } else {
            struct PlannerJob *curr = &stepperPlanBuf.buf[i];
            if (plannerJobIsMove(curr)) {
                if (recalculate(next, curr)) {
                    calcReverse(curr);
                    next = curr;
                } else {
                    break;
                }
            } else {
                /* this one does not have a move, lookahead another one */
            }
        }

        i = planBufPrevIdx(i);
        size--;
    }
}

/* returns the job just enqueued */
static struct PlannerJob *__plannerEnqueue(enum JobType k)
{
    BUG_ON(plannerAvailableSpace() == 0);
    struct PlannerJob *tail = &stepperPlanBuf.buf[stepperPlanBuf.tail];
    memset(tail, 0, sizeof(*tail));

    tail->type = k;

    stepperPlanBuf.tail = (stepperPlanBuf.tail + 1) % MOTION_LOOKAHEAD;
    stepperPlanBuf.size++;
    return tail;
}
void plannerEnqueue(enum JobType k)
{
    __plannerEnqueue(k);
}

void plannerEnqueueNotify(enum JobType k, TaskHandle_t notify, uint32_t seq)
{
    struct PlannerJob *job = __plannerEnqueue(k);

    job->notify = notify;
    job->seq = seq;
}

/* requires planner to be non-empty */
static const struct PlannerJob *findPrevMovePlan(void)
{
    uint32_t i = planBufPrevIdx(stepperPlanBuf.tail);
    uint32_t size = plannerSize();

    while (size > 0) {
        struct PlannerJob *curr = &stepperPlanBuf.buf[i];
        if (plannerJobIsMove(curr)) {
            return curr;
        } else {
            /* this one does not have a move, lookahead another one */
        }

        i = planBufPrevIdx(i);
        size--;
    }
    return &empty;
}

void plannerEnqueueMove(enum JobType k, const int32_t plan[NR_STEPPER],
                        const fix16_t unit_vec[NR_AXIS],
                        const uint32_t max_v[NR_STEPPER],
                        const int32_t acc[NR_STEPPER], fix16_t lenMM,
                        bool stop)
{
    const struct PlannerJob *prev = findPrevMovePlan();
    struct PlannerJob *tail = __plannerEnqueue(k);

    for_each_axis(i) {
        tail->unit_vec[i] = unit_vec[i];
    }
    tail->lenMM = lenMM;
    populateBlock(prev, tail, plan, max_v, acc, stop);
    reversePass();
}
