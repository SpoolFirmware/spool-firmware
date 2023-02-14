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

#define X_AND_Y 2

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

static fix16_t vecUnit(const fix16_t a[NR_AXES], fix16_t out[NR_AXES])
{
    float accum = 0;
    float af[NR_AXES];
    for_each_axis(i) {
        float x = fix16_to_float(a[i]);
        accum += x * x;
        af[i] = x;
    }
    float len = sqrtf(accum);
    accum = 1.0f / len;
    for_each_axis(i) {
        out[i] = fix16_from_float(af[i] * accum);
    }
    return fix16_from_float(len);
}

void planCoreXy(const fix16_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
                fix16_t unit_vec[NR_AXES], fix16_t *len)
{
    _Static_assert(NR_AXES >= X_AND_Y,
                   "number of axes insufficient for corexy");

    fix16_t aX = fix16_add(movement[X_AXIS], movement[Y_AXIS]);
    fix16_t bX = fix16_sub(movement[X_AXIS], movement[Y_AXIS]);
    for_each_stepper(i) {
        if (i < X_AND_Y)
            continue;
        plan[i] = fix16_mul_int32(movement[i], STEPPER_STEPS_PER_MM[i]);
    }
    plan[STEPPER_A_IDX] =
        fix16_mul_int32(aX, STEPPER_STEPS_PER_MM[STEPPER_A_IDX]);
    plan[STEPPER_B_IDX] =
        fix16_mul_int32(bX, STEPPER_STEPS_PER_MM[STEPPER_B_IDX]);

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

static fix16_t vecDot(const fix16_t a[NR_AXES], const fix16_t b[NR_AXES])
{
    fix16_t res = 0;
    for_each_axis(i) {
        res += fix16_mul(a[i], b[i]);
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
                          const int32_t max_v[NR_STEPPERS],
                          const int32_t acc[NR_STEPPERS], bool stop)
{
    float timeEst = 0;
    uint32_t maxStepper = 0;
    for_each_stepper(i) {
        if (plan[i] > 0)
            new->stepDirs |= BIT(i);
        new->steppers[i].x = abs(plan[i]);
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

    /* TODO correct for acceleration */
    new->a = acc[maxStepper];
    if (timeEst == 0) {
        /* empty block */
        return;
    }

    uint32_t v = (uint32_t)((float)new->steppers[maxStepper].x / timeEst);
    new->vSq = v *v;

    fix16_t cosTheta = vecDot(prev->unit_vec, new->unit_vec);
    if (cosTheta >= JUNCTION_INHERIT_VEL_THRES) {
        /* essentially the same direction */
        /* TODO handle junction velocity like marlin */
        uint32_t viSq = min(prev->vfSq, new->vSq);
        new->viSq = viSq;
    } else if (new->len <= JUNCTION_SMOOTHING_DIST_THRES &&
               cosTheta >= JUNCTION_SMOOTHING_THRES) {
        /* is this where we do rounded corners? */
        /* for now, we pretend cos is linear */
        uint32_t viSq = min(prev->vfSq * cosTheta, new->vSq);
        new->a = new->a *cosTheta;
        new->viSq = viSq;
    } else {
        /* is a minimum speed helpful here? why? */
        new->viSq = 0;
    }

    if (stop) {
        calcReverse(new);
        return;
    }

    int32_t accelerationX = ((int32_t)(new->vSq - new->viSq)) / (2 * new->a);
    if (int32_less_abs(new->x, accelerationX)) {
        new->vSq =
            (uint32_t)ASSERT_GE0(new->x * 2 * new->a + (int32_t) new->viSq);
        new->vfSq = new->vSq;
    } else {
        new->accelerationX = accelerationX;
        new->vfSq = new->vSq;
    }
}

static void calcReverse(struct PlannerJob *curr)
{
    int32_t accelerationX =
        ((int32_t)(curr->vSq - curr->viSq)) / (2 * (int32_t)curr->a);
    int32_t decelerationX =
        ((int32_t)(curr->vfSq - curr->vSq)) / (2 * -(int32_t)curr->a);
    int32_t sum = accelerationX + decelerationX;

    if (int32_less_abs(curr->x, sum)) {
        int32_t direct = accelerationX - decelerationX;
        if (int32_less_abs(curr->x, direct)) {
            /* cannot decelerate to desired speed */
            curr->accelerationX = 0;
            curr->decelerationX = curr->x;
            curr->viSq = (uint32_t)max(
                0, ASSERT_GE0(curr->x * 2 * curr->a + (int32_t)curr->vfSq));
            curr->vSq = curr->viSq;
        } else {
            curr->accelerationX = (direct + curr->x) / 2;
            curr->decelerationX = curr->x - curr->accelerationX;
            curr->vSq =
                (uint32_t)max(0, ASSERT_GE0(curr->accelerationX * 2 * curr->a +
                                            (int32_t)curr->viSq));
            /* here, due to integer math, vfSq may dip slightly below 0*/
            curr->vfSq =
                (uint32_t)max(0, -(int32_t)curr->decelerationX * 2 * curr->a +
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
                   const fix16_t unit_vec[NR_AXES],
                   const int32_t max_v[NR_STEPPERS],
                   const int32_t acc[NR_STEPPERS], fix16_t len, bool stop)
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
    tail->type = k;
    for_each_axis(i)
        tail->unit_vec[i] = unit_vec[i];
    tail->len = len;
    populateBlock(prev, tail, plan, max_v, acc, stop);
    reversePass();
    stepperPlanBuf.tail = (stepperPlanBuf.tail + 1) % MOTION_LOOKAHEAD;
    stepperPlanBuf.size++;
}
