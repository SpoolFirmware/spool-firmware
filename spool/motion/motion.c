#include <math.h>
#include "motion.h"
#include "core/spool.h"
#include "platform/platform.h"
#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"

static int32_t motionMaxVelSteps[NR_STEPPER];

void motionInit(void)
{
    StepperExecutionQueue = xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct StepperJob));
    if (StepperExecutionQueue == NULL) {
        panic();
    }
    for_each_stepper(i) {
        motionMaxVelSteps[i] = (int32_t)platformMotionDefaultMaxVel[i] *
                          platformMotionStepsPerMM[i];
    }
    platformDisableStepper(0xFF);
    motionPlannerTaskInit();
    initPlanner();
}

fix16_t vecUnit(const int32_t a[NR_AXIS], fix16_t out[X_AND_Y])
{
    float accum = 0;
    float af[NR_AXIS];
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

int32_t motionGetMaxVelocity(uint8_t stepper)
{
    return motionMaxVelSteps[stepper];
}

void motionSetMaxVelocity(uint8_t stepper, int32_t maxVel)
{
    motionMaxVelSteps[stepper] = maxVel;
}

int32_t motionGetDefaultAcceleration(uint8_t stepper)
{
    return (int32_t)platformMotionDefaultAcc[stepper] *
           platformMotionStepsPerMM[stepper];
}

int32_t motionGetHomingVelocity(uint8_t stepper)
{
    return (int32_t)platformMotionHomingVel[stepper] *
           platformMotionStepsPerMM[stepper];
}

int32_t motionGetHomingAcceleration(uint8_t stepper)
{
    return (int32_t)platformMotionHomingAcc[stepper] *
           platformMotionStepsPerMM[stepper];
}

int32_t motionGetMinVelocity(uint8_t stepper)
{
    return (int32_t)platformMotionMinVel[stepper] *
           platformMotionStepsPerMM[stepper];
}
