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
    /* consumer */
    uint32_t head;
    /* producer */
    uint32_t tail;
    struct PlannerJob buf[MOTION_LOOKAHEAD];
};

static struct StepperPlanBuf stepperPlanBuf;
const static struct PlannerJob empty;

void initPlanner()
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

static fix16_t vecUnit(const int32_t a[NR_AXES], fix16_t out[X_AND_Y])
{
    float accum = 0;
    float af[NR_AXES];
    for (uint8_t i = 0; i < X_AND_Y; ++i) {
        float x = (float)a[i];
        accum += x * x;
        af[i] = x;
    }
    float len = sqrtf(accum);
    accum = 1.0f / len;
    for (uint8_t i = 0; i < X_AND_Y; ++i) {
        out[i] = fix16_from_float(af[i] * accum);
    }
    return fix16_from_float(len);
}

void planI3(const int32_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
            fix16_t unit_vec[X_AND_Y], fix16_t *len)
{
    for_each_stepper(i) {
        plan[i] = movement[i];
    }

    *len = vecUnit(movement, unit_vec);
}

void planCoreXy(const int32_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
                fix16_t unit_vec[X_AND_Y], fix16_t *len)
{
    _Static_assert(NR_AXES >= X_AND_Y,
                   "number of axes insufficient for corexy");

    int32_t aX = movement[X_AXIS] + movement[Y_AXIS];
    int32_t bX = movement[X_AXIS] - movement[Y_AXIS];
    for_each_stepper(i) {
        if (i < X_AND_Y)
            continue;
        plan[i] = movement[i];
    }
    plan[STEPPER_A_IDX] = aX;
    plan[STEPPER_B_IDX] = bX;

    *len = vecUnit(movement, unit_vec);
}

void __dequeuePlan(struct PlannerJob *out)
{
    BUG_ON(plannerSize() == 0);
    stepperPlanBuf.size--;
    stepperPlanBuf.head = (stepperPlanBuf.head + 1) % MOTION_LOOKAHEAD;
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

// static void vecSub(const fix16_t a[NR_AXES], const fix16_t b[NR_AXES],
//                       fix16_t out[NR_AXES])
// {
//     for_each_axis(i) {
//         out[i] = a[i] - b[i];
//     }
// }

static void populateBlock(const struct PlannerJob *prev, struct PlannerJob *new,
                          const int32_t plan[NR_STEPPERS],
                          const uint32_t max_v[NR_STEPPERS],
                          const uint32_t acc[NR_STEPPERS], bool stop)
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
    } else if (new->len <= JUNCTION_SMOOTHING_DIST_THRES &&
               cosTheta <= JUNCTION_SMOOTHING_THRES) {
        /* is this where we do rounded corners? */
        /* for now, we pretend cos is linear */
        uint32_t viSq =
            min((uint32_t)((float)prev->vfSq * abs(fix16_to_float(cosTheta))),
                new->vSq);
        new->viSq = viSq;
    } else {
        /* is a minimum speed helpful here? why? */
        new->viSq = MIN_STEP_RATE[maxStepper];
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
    uint32_t i = stepperPlanBuf.tail;
    uint32_t size = plannerSize();
    struct PlannerJob *next = NULL;

    while (size > 0) {
        if (next == NULL) {
            next = &stepperPlanBuf.buf[i];
        } else {
            struct PlannerJob *curr = &stepperPlanBuf.buf[i];
            if (recalculate(next, curr)) {
                calcReverse(curr);
                next = curr;
            } else {
                break;
            }
        }

        if (i == 0) {
            i = MOTION_LOOKAHEAD - 1;
        } else {
            i--;
        }
        size--;
    }
}

void __enqueuePlan(enum JobType k, const int32_t plan[NR_STEPPERS],
                   const fix16_t unit_vec[X_AND_Y],
                   const uint32_t max_v[NR_STEPPERS],
                   const uint32_t acc[NR_STEPPERS], fix16_t len, bool stop)
{
    BUG_ON(plannerAvailableSpace() == 0);
    const struct PlannerJob *prev;
    if (plannerSize() == 0) {
        prev = &empty;
    } else {
        prev = &stepperPlanBuf
                    .buf[(stepperPlanBuf.tail == 0 ? MOTION_LOOKAHEAD - 1 :
                                                     stepperPlanBuf.tail - 1)];
    }
    struct PlannerJob *tail = &stepperPlanBuf.buf[stepperPlanBuf.tail];
    memset(tail, 0, sizeof(*tail));
    for (uint8_t i = 0; i < X_AND_Y; ++i)
        tail->unit_vec[i] = unit_vec[i];
    tail->type = k;
    tail->len = len;
    populateBlock(prev, tail, plan, max_v, acc, stop);
    reversePass();
    stepperPlanBuf.tail = (stepperPlanBuf.tail + 1) % MOTION_LOOKAHEAD;
    stepperPlanBuf.size++;
}
