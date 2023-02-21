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

static fix16_t vecDot(const fix16_t a[X_AND_Y], const fix16_t b[X_AND_Y])
{
    fix16_t res = 0;
    for (uint8_t i = 0; i < X_AND_Y; ++i) {
        res = fix16_add(res, fix16_mul(a[i], b[i]));
    }
    return res;
}

static void populateBlock(const struct PlannerJob *prev, struct PlannerJob *new,
                          const int32_t plan[NR_STEPPER],
                          const uint32_t max_v[NR_STEPPER],
                          const uint32_t acc[NR_STEPPER], bool stop)
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

    /* TODO correct for acceleration */
    new->a = acc[maxStepper];

    uint32_t v = (uint32_t)((float)new->steppers[maxStepper].x / timeEst);
    new->vSq = v *v;

    /* cos(pi - theta) = -cos(theta) */
    fix16_t cosTheta = -vecDot(prev->unit_vec, new->unit_vec);

    if (cosTheta <= JUNCTION_INHERIT_VEL_THRES) {
        /* essentially the same direction */
        /* TODO handle junction velocity like marlin */
        uint32_t viSq = min(prev->vfSq, new->vSq);
        new->viSq = viSq;
    } else if (new->len <= JUNCTION_SMOOTHING_DIST_THRES(maxStepper) &&
               cosTheta <= JUNCTION_SMOOTHING_THRES) {
        /* is this where we do rounded corners? */
        /* for now, we pretend cos is linear */
        uint32_t viSq =
            min((uint32_t)((float)prev->vfSq * abs(fix16_to_float(cosTheta))),
                new->vSq);
        new->viSq = viSq;
    } else {
        /* is a minimum speed helpful here? why? */
        new->viSq = motionGetMinVelocity(maxStepper);
    }

    if (stop) {
        calcReverse(new);
        return;
    }

    uint32_t accelerationX = (new->vSq - new->viSq) / (2 * new->a);
    if (new->x < accelerationX) {
        new->vSq = new->x * 2 * new->a + new->viSq;
        new->vfSq = new->vSq;
    } else {
        new->accelerationX = accelerationX;
        new->vfSq = new->vSq;
    }
}

/* curr needs to be a move */
static void calcReverse(struct PlannerJob *curr)
{
    uint32_t accelerationX = (curr->vSq - curr->viSq) / (2 * curr->a);
    uint32_t decelerationX = (curr->vSq - curr->vfSq) / (2 * curr->a);
    uint32_t sum = accelerationX + decelerationX;

    if (curr->x < sum) {
        int32_t direct = (int32_t)accelerationX - (int32_t)decelerationX;
        if (int32_less_abs((int32_t)curr->x, direct)) {
            /* cannot decelerate to desired speed */
            curr->accelerationX = 0;
            curr->decelerationX = curr->x;
            curr->viSq = curr->x * 2 * curr->a + curr->vfSq;
            curr->vSq = curr->viSq;
        } else {
            curr->accelerationX = (uint32_t)(direct + (int32_t)curr->x) / 2;
            curr->decelerationX = curr->x - curr->accelerationX;
            curr->vSq = curr->accelerationX * 2 * curr->a + curr->viSq;
            /* here, due to integer math, vfSq may dip slightly below 0*/
            curr->vfSq =
                (uint32_t)max(0, -(int32_t)(curr->decelerationX * 2 * curr->a) +
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
                   const fix16_t unit_vec[X_AND_Y],
                   const uint32_t max_v[NR_STEPPER],
                   const uint32_t acc[NR_STEPPER], fix16_t len, bool stop)
{
    const struct PlannerJob *prev = findPrevMovePlan();
    struct PlannerJob *tail = __plannerEnqueue(k);

    for (uint8_t i = 0; i < X_AND_Y; ++i)
        tail->unit_vec[i] = unit_vec[i];
    tail->len = len;
    populateBlock(prev, tail, plan, max_v, acc, stop);
    reversePass();
}
