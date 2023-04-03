#include <math.h>
#include "motion.h"
#include "core/spool.h"
#include "platform/platform.h"
#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"
#include "lib/planner/planner.h"


static int32_t motionMaxVelSteps[NR_STEPPER];

void motionInit(void)
{
    StepperExecutionQueue = xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct StepperJob));
    if (StepperExecutionQueue == NULL) {
        panic();
    }
    for_each_stepper(i) {
        motionMaxVelSteps[i] = (int32_t)platformMotionDefaultMaxVel[i] / 2 *
                          platformMotionStepsPerMM[i];
    }
    platformDisableStepper(0xFF);
    motionPlannerTaskInit();
    // TODO enable when RS side is done
    plannerInit_legacy();
    // plannerInit(NULL, 0, 0);
}

fix16_t vecUnit(const fix16_t vec[NR_AXIS], fix16_t unit_vec[NR_AXIS])
{
    float accum = 0;
    for_each_axis(i) {
        const float componentFloat = fix16_to_float(vec[i]);
        accum += componentFloat * componentFloat;
    }
    fix16_t len = fix16_from_float(sqrtf(accum));
    const fix16_t lenInverse = fix16_div(F16(1.0), len);
    for_each_axis(i) {
        unit_vec[i] = fix16_mul(vec[i], lenInverse);
    }
    return len;
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
    return (int32_t)platformMotionMinVelSteps[stepper];
}
