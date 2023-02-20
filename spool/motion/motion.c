#include <math.h>
#include "motion.h"
#include "core/spool.h"
#include "platform/platform.h"
#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"

void motionInit(void)
{
    StepperExecutionQueue = xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct StepperJob));
    if (StepperExecutionQueue == NULL) {
        panic();
    }
    platformDisableStepper(0xFF);
    motionPlannerTaskInit();
    initPlanner();
}

fix16_t vecUnit(const int32_t a[NR_AXES], fix16_t out[X_AND_Y])
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
