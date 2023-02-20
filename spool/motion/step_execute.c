#include "motion/step_plan_ng.h"
#include <stdbool.h>

#include "misc.h"
#include "string.h"

#include "core/spool.h"

#include "step_execute.h"
#include "step_schedule.h"

static struct StepperJob job = { 0 };
static uint32_t sIterationCompleted = 0;
// Line Drawing
static uint32_t sAccelerationTime;
static uint32_t sDecelerationTime;
static int32_t sDeltaError[NR_STEPPER]; // "error"
static uint32_t sIncrementRatio[NR_STEPPER]; // "m". This expect slope <= 1
static uint32_t sStepsExecuted[NR_STEPPER]; // steps executed
static uint32_t sStepSize; // "x" step size
static uint32_t
    sStepCounter; // Counter to keep track of the desired axis movement.

static inline void sDiscardJob(void)
{
    job.type = StepperJobUndef;
}

static uint16_t sCalcInterval(struct StepperJob *pJob)
{
    int64_t stepRate;

    if (sIterationCompleted < pJob->accelerateUntil) { // Accelerating
        stepRate = (((int64_t)sAccelerationTime * pJob->accel_steps_s2) /
                    platformGetStepperTimerFreq()) +
                   pJob->entry_steps_s;
        if (stepRate > pJob->cruise_steps_s) {
            stepRate = pJob->cruise_steps_s;
        }
    } else if (sIterationCompleted > pJob->decelerateAfter) { // Decelertaing
        stepRate = (int64_t)pJob->cruise_steps_s -
                   (((int64_t)sDecelerationTime * pJob->accel_steps_s2) /
                    platformGetStepperTimerFreq());
        if (stepRate < pJob->exit_steps_s) { // Never go below exit velocity
            stepRate = pJob->exit_steps_s;
        }
    } else { // Cruise
        stepRate = pJob->cruise_steps_s;
    }

    if (stepRate < motionGetMinVelocity(STEPPER_A)) { // TODO: CONFIG: MIN_STEP_RATE
        stepRate = motionGetMinVelocity(STEPPER_A);
    }

    uint32_t interval = (platformGetStepperTimerFreq() / stepRate);

    if (sIterationCompleted < pJob->accelerateUntil) {
        sAccelerationTime += interval;
    } else if (sIterationCompleted > pJob->decelerateAfter) {
        sDecelerationTime += interval;
    }

    return interval;
}

uint16_t executeStep(uint16_t ticksElapsed)
{
    /* Check endstops first */
    if (job.type != StepperJobUndef && job.type != StepperJobRun) {
        for (uint8_t i = 0; i < NR_AXIS; ++i) {
            if (platformGetEndstop(i)) {
                if (job.type == StepperJobHomeX && i == X_AXIS) {
                    sDiscardJob();
                    notifyHomeISR(sStepCounter);
                    return 0;
                }
                if (job.type == StepperJobHomeY && i == Y_AXIS) {
                    sDiscardJob();
                    notifyHomeISR(sStepCounter);
                    return 0;
                }
                if (job.type == StepperJobHomeZ && i == Z_AXIS) {
                    sDiscardJob();
                    notifyHomeISR(sStepCounter);
                    return 0;
                }
            }
        }
    }

    // Executes Steps *FIRST*
    if (job.type != StepperJobUndef) {
        uint8_t stepperMask = 0;
        for (uint8_t i = 0; i < NR_STEPPER; i++) {
            sDeltaError[i] += sIncrementRatio[i];
            if (sDeltaError[i] >= 0) {
                sDeltaError[i] -= sStepSize;
                if (sStepsExecuted[i] < job.blocks[i].totalSteps) {
                    sStepsExecuted[i]++;
                    stepperMask |= BIT(i);
                }
            }
        }
        platformStepStepper(stepperMask);

        sIterationCompleted += 1;
        if (sIterationCompleted >= job.totalStepEvents) {
            bool finished = true;
            for_each_stepper(i) {
                if (sStepsExecuted[i] < job.blocks[i].totalSteps) {
                    finished = false;
                }
            }
            if (finished)
                sDiscardJob();
        }
        
        // TODO: THIS IS KIND OF BAD, maybe generic
        if (stepperMask & BIT(STEPPER_C)) {
            sStepCounter++;
        }
    }

    // Now we can do other things
    if (job.type == StepperJobUndef) {
        if (xQueueReceiveFromISR(StepperExecutionQueue, &job, NULL) != pdTRUE) {
            return platformGetStepperTimerFreq() / 1000; // 1ms
        }
        // Acquired new block
        sStepCounter = 0;
        for_each_stepper(i) {
            if (platformMotionInvertStepper[i])
                job.stepDirs ^= BIT(i);
        }
        platformSetStepperDir(job.stepDirs);

        sIterationCompleted = 0;
        for_each_stepper(i)
            sStepsExecuted[i] = 0;
        sAccelerationTime = sDecelerationTime = 0;
        sStepSize = job.totalStepEvents * 2;
        for (int i = 0; i < NR_STEPPER; i++) {
            sDeltaError[i] = -((int32_t)job.totalStepEvents);
            sIncrementRatio[i] = job.blocks[i].totalSteps * 2;
        }
    }

    return sCalcInterval(&job);
}
