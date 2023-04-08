#include <math.h>
#include "motion.h"
#include "core/spool.h"
#include "platform/platform.h"
#include "step_schedule.h"
#include "step_execute.h"
#include "lib/planner/planner.h"

PlannerHandle PLANNER;

static fix16_t motionMaxVel[NR_STEPPER];

void motionInit(void)
{
    StepperExecutionQueue = xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct ExecutorJob));
    if (StepperExecutionQueue == NULL) {
        panic();
    }
    for_each_stepper(i) {
        motionMaxVel[i] = F16((float)platformMotionDefaultMaxVel[i] / 2);
    }
    platformDisableStepper(0xFF);
    motionPlannerTaskInit();
	PLANNER = plannerInit(0, 0, (const uint32_t *)platformMotionStepsPerMM);
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

fix16_t motionGetMaxVelocityMM(uint8_t stepper)
{
    return motionMaxVel[stepper];
}

void motionSetMaxVelocityMM(uint8_t stepper, fix16_t maxVel)
{
    motionMaxVel[stepper] = maxVel;
}

fix16_t motionGetDefaultAccelerationMM(uint8_t stepper)
{
    return F16(platformMotionDefaultAcc[stepper]);
}

fix16_t motionGetHomingVelocityMM(uint8_t stepper)
{
    return F16(platformMotionHomingVel[stepper]);
}

fix16_t motionGetHomingAccelerationMM(uint8_t stepper)
{
    return F16(platformMotionHomingAcc[stepper]);
}

fix16_t motionGetMinVelocityMM(uint8_t stepper)
{
    return F16(platformMotionMinVelSteps[stepper]);
}
