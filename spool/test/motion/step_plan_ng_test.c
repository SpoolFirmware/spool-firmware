#include "step_plan_ng.h"
#include "compiler.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

struct Move {
    fix16_t x;
    fix16_t y;
};

const static uint32_t max_v[NR_STEPPERS] = {
    VEL * STEPS_PER_MM, VEL *STEPS_PER_MM,
    VEL * STEPS_PER_MM, VEL *STEPS_PER_MM,
    };

#define M(x, y)        \
    {                  \
        F16(x), F16(y) \
    }
const static struct Move movements[] = {
    M(100, 10),  M(5, 10),     M(1, 10),     M(-1, 10),  M(-10, 10),
    M(10, -100), M(10, -12), M(0.2, 0.2), M(10, -10),
};

const static int nr_movements = ARRAY_SIZE(movements);

void testPlanContinuous(const struct Move *move)
{
    int32_t plan[NR_STEPPERS];
    const fix16_t movement[NR_AXES] = { move->x, move->y };
    fix16_t unitVec[NR_AXES];
    fix16_t len;

    planCoreXy(movement, plan, unitVec, &len);
    printf("(x,y) %f, %f => (a,b) %d, %d\n", fix16_to_float(move->x),
           fix16_to_float(move->y), plan[0], plan[1]);

    __enqueuePlan(StepperJobRun, plan, unitVec, max_v, STEPPER_ACC, len, true);
}

void printPlan(void)
{
    struct PlannerJob job;
    memset(&job, 0, sizeof(job));
    __dequeuePlan(&job);

    printf("===\n");
    for_each_stepper(i) {
        struct PlannerBlock *s = &job.steppers[i];
    }
    struct PlannerJob *s = &job;
        printf("x=%d acc=%d dec=%d vi=%f v=%f vf=%f\n", s->x, s->accelerationX,
               s->decelerationX, sqrtf((float)s->viSq), sqrtf((float)s->vSq),
               sqrtf((float)s->vfSq));
}

int main()
{
    for (int i = 0; i < nr_movements; ++i)
        testPlanContinuous(&movements[i]);

    for (int i = 0; i < nr_movements; ++i)
        printPlan();
    return 0;
}
