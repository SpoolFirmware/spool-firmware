#include "step_plan_ng.h"
#include <math.h>
#include "string.h"
#include "compiler.h"
#include "error.h"
#include "number.h"
#include "dbgprintf.h"

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

void planCoreXy(const fix16_t movement[NR_AXES], int32_t plan[NR_STEPPERS])
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
}

void __dequeuePlan(struct PlannerJob *out)
{
    BUG_ON(plannerSize() == 0);
    stepperPlanBuf.size--;
    stepperPlanBuf.head = (stepperPlanBuf.head + 1) % MOTION_LOOKAHEAD;
    *out = stepperPlanBuf.buf[stepperPlanBuf.head];
}

static void calcReverse(struct PlannerJob *);

static void populateBlock(const struct PlannerJob *prev, struct PlannerJob *new,
                          bool continuous)
{
    float time_est = 0;
    for_each_stepper(i) {
        struct PlannerBlock *s = &new->steppers[i];
        s->a = fix16_copy_sign(STEPPER_ACC[i], s->x);
        if (s->v != 0) {
            time_est = max((float)s->x / (float)s->v, time_est);
        }
    }
    if (time_est == 0) {
        /* empty block */
        for_each_stepper(i) {
            struct PlannerBlock *s = &new->steppers[i];
            s->accelerationX = 0;
            s->decelerationX = 0;
            s->viSq = 0;
            s->vSq = 0;
            s->vfSq = 0;
        }
        return;
    }

    for_each_stepper(i) {
        struct PlannerBlock *s = &new->steppers[i];

        int32_t v = (int32_t)((float)s->x / time_est);
        s->vSq = (uint32_t)(v * v);

        if (s->x == 0) {
            /* doesn't move */
            s->viSq = 0;
            continue;
        }
        if (int32_same_sign(s->x, prev->steppers[i].x)) {
            /* prev block slows down or maintain speed */
            s->viSq = min(prev->steppers[i].vfSq, s->vSq);
        } else /* prev block needs to halt */
            s->viSq = 0;

        if (!continuous)
            continue;

        int32_t accelerationX = ((int32_t)(s->vSq - s->viSq)) / (2 * s->a);
        if (int32_less_abs(s->x, accelerationX)) {
            s->accelerationX = s->x;
            s->vSq = (uint32_t)ASSERT_GE0(s->x * 2 * s->a + (int32_t)s->viSq);
            s->vfSq = s->vSq;
        } else {
            s->accelerationX = accelerationX;
            s->vfSq = s->vSq;
        }
    }

    /* compute reverse with vf=0 to stop at the end */
    if (!continuous)
        calcReverse(new);
}

static void calcReverse(struct PlannerJob *curr)
{
    for_each_stepper(i) {
        struct PlannerBlock *s = &curr->steppers[i];

        int32_t accelerationX = ((int32_t)(s->vSq - s->viSq)) / (2 * s->a);
        int32_t decelerationX = ((int32_t)(s->vfSq - s->vSq)) / (2 * -s->a);
        int32_t sum = accelerationX + decelerationX;

        if (int32_less_abs(s->x, sum)) {
            if (int32_less_abs(s->x, accelerationX - decelerationX)) {
                /* cannot decelerate to desired speed */
                s->accelerationX = 0;
                s->decelerationX = s->x;
                s->viSq =
                    (uint32_t)ASSERT_GE0(s->x * 2 * s->a + (int32_t)s->vfSq);
                s->vSq = s->viSq;
                s->v = (int32_t)sqrtf((float)s->viSq);
            } else {
                s->accelerationX = accelerationX * s->x / sum;
                s->decelerationX = decelerationX * s->x / sum;

                s->vSq = (uint32_t)ASSERT_GE0(s->accelerationX * 2 * s->a +
                                              (int32_t)s->viSq);
                s->vfSq = (uint32_t)ASSERT_GE0(s->decelerationX * -2 * s->a +
                                               (int32_t)s->vSq);
            }
        } else {
            s->accelerationX = accelerationX;
            s->decelerationX = decelerationX;
        }
    }
}

static bool recalculate(const struct PlannerJob *next, struct PlannerJob *curr)
{
    bool dirty = false;
    for_each_stepper(i) {
        if (abs((int32_t)curr->steppers[i].vfSq -
                (int32_t)next->steppers[i].viSq) >= VEL_CHANGE_THRESHOLD) {
            dirty = true;
            WARN_ON(
                !int32_same_sign(curr->steppers[i].x, next->steppers[i].x) &&
                next->steppers[i].viSq != 0);

            curr->steppers[i].vfSq = next->steppers[i].viSq;
        } else if (!int32_same_sign(curr->steppers[i].x, next->steppers[i].x) &&
                   curr->steppers[i].vfSq != 0) {
            /* not sure if this would happen. it shouldn't */
            WARN();
            dbgPrintf("currx%d nextx%d vfSq%d next viSq%d\n", curr->steppers[i].x,
                      next->steppers[i].x, curr->steppers[i].vfSq,
                      next->steppers[i].viSq);
            dirty = true;
            curr->steppers[i].vfSq = 0;
        }
    }
    return dirty;
}

static void reversePass(void)
{
    uint32_t i = stepperPlanBuf.tail;
    struct PlannerJob *next = NULL;
    while (i != stepperPlanBuf.head) {
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
    }
}

void __enqueuePlan(enum JobType k, const int32_t plan[NR_STEPPERS],
                   const int32_t max_v[NR_STEPPERS], bool more_entries)
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
    for_each_stepper(i) {
        tail->steppers[i].x = plan[i];
        tail->steppers[i].v = int32_copy_sign(max_v[i], plan[i]);
    }
    tail->type = k;
    populateBlock(prev, tail, more_entries);
    reversePass();
    stepperPlanBuf.tail = (stepperPlanBuf.tail + 1) % MOTION_LOOKAHEAD;
    stepperPlanBuf.size++;
}
