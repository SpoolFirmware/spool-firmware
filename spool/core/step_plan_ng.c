#include "step_plan_ng.h"
#include <math.h>

#include "string.h"
#include "compiler.h"
#if 0
#include "semphr.h"
#endif
#include "error.h"
// #include <stdio.h>
#include "number.h"

struct StepperPlanBuf {
    uint32_t idle;
    /* consumer */
    uint32_t head;
    /* producer */
    uint32_t tail;
    struct PlannerJob buf[MOTION_LOOKAHEAD];
};

#define X_AND_Y 2

static struct StepperPlanBuf stepperPlanBuf;
const static struct PlannerJob empty;
#if 0
static TaskHandle_t p;
static TaskHandle_t c;
static StaticSemaphore_t planBufMutexBuf;
static SemaphoreHandle_t planBufMutex;

void initPlanner(TaskHandle_t p_, TaskHandle_t c_)
{
    p = p_;
    c = c_;
    for (uint8_t i = 0; i < MOTION_LOOKAHEAD; ++i)
        xTaskNotifyGive(p);

    planBufMutex = xSemaphoreCreateMutexStatic(&planBufMutexBuf);
    BUG_ON(planBufMutex == NULL);
    stepperPlanBuf.head = MOTION_LOOKAHEAD - 1;
}
#endif

void initPlanner()
{
    stepperPlanBuf.head = MOTION_LOOKAHEAD - 1;
}

uint32_t plannerSize(void)
{
    if (stepperPlanBuf.head <= stepperPlanBuf.tail) {
        return stepperPlanBuf.tail - stepperPlanBuf.head;
    }
    return MOTION_LOOKAHEAD - 1 - stepperPlanBuf.head + stepperPlanBuf.tail;
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
        plan[i] = 0;
    }
    plan[STEPPER_A_IDX] =
        fix16_mul_int32(aX, STEPPER_STEPS_PER_MM[STEPPER_A_IDX]);
    plan[STEPPER_B_IDX] =
        fix16_mul_int32(bX, STEPPER_STEPS_PER_MM[STEPPER_B_IDX]);
}

void __dequeuePlan(struct PlannerJob *out)
{
    BUG_ON(plannerSize() == 0);
    stepperPlanBuf.head = (stepperPlanBuf.head + 1) % MOTION_LOOKAHEAD;
    // memcpy(out, &stepperPlanBuf.buf[stepperPlanBuf.head], sizeof(*out));
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
        // printf("viSq %d, v%d, prev%d\n", s->viSq, s->vSq,
        //        prev->steppers[i].vfSq);

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
        // if (s->vfSq > s->vSq) {
        //     printf("x %d vf %d v %d\n", s->x, s->vfSq, s->vSq);
        // }

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

            // printf("recalc vf %d next vi %d\n", curr->steppers[i].vfSq,
            //        next->steppers[i].viSq);
            curr->steppers[i].vfSq = next->steppers[i].viSq;
        } else if (!int32_same_sign(curr->steppers[i].x, next->steppers[i].x) &&
                   curr->steppers[i].vfSq != 0) {
            /* not sure if this would happen. it shouldn't */
            WARN();
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
    stepperPlanBuf.head = MOTION_LOOKAHEAD - 1;
    const struct PlannerJob *prev;
    if ((stepperPlanBuf.head + 1) % MOTION_LOOKAHEAD == stepperPlanBuf.tail) {
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
}

#if 0
void dequeuePlan(struct PlannerJob *out)
{
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    xSemaphoreTake(planBufMutex, portMAX_DELAY);
    __dequeuePlan(out);
    xSemaphoreGive(planBufMutex);
    xTaskNotifyGive(p);
}

void enqueuePlan(const int32_t plan[NR_STEPPERS],
                 const int32_t max_v[NR_STEPPERS], bool more_entries)
{
    ulTaskNotifyTake(pdFALSE, portMAX_DELAY);
    xSemaphoreTake(planBufMutex, portMAX_DELAY);

    __enqueuePlan(plan, max_v, more_entries);

    xSemaphoreGive(planBufMutex);
    xTaskNotifyGive(c);
}
#endif
