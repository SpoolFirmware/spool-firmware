#include <stdbool.h>

#include "misc.h"
#include "string.h"
#include "error.h"
#include "dbgprintf.h"

#include "core/spool.h"

#include "step_execute.h"
#include "step_schedule.h"

#include "motion/motion.h"
#include "lib/planner/planner.h"

static struct ExecutorJob job = { 0 };
static uint32_t sIterationCompleted = 0;
// Line Drawing
static uint32_t sAccelerationTime;
static uint32_t sDecelerationTime;
static int32_t sDeltaError[NR_STEPPER]; // "error"
static int32_t sIncrementRatio[NR_STEPPER]; // "m". This expect slope <= 1
static uint32_t sStepsExecuted[NR_STEPPER]; // steps executed
static int32_t sStepSize; // "x" step size
static int32_t sStepCounter; // Counter track the number of steps moved.

static inline void sDiscardJob(void)
{
    job.type = StepperJobUndef;
}

static uint16_t sCalcInterval(struct MoveSteps *pJob)
{
    int32_t stepRate;

    if (sIterationCompleted < pJob->accelerateUntil) { // Accelerating
        stepRate =
            (int32_t)(((uint32_t)sAccelerationTime * pJob->accelerate_stepss2) /
                      platformGetStepperTimerFreq()) +
            pJob->entry_steps_s;
        if (stepRate > pJob->cruise_steps_s) {
            stepRate = pJob->cruise_steps_s;
        }
    } else if (sIterationCompleted > pJob->decelerateAfter) { // Decelertaing
        stepRate =
            (int32_t)pJob->cruise_steps_s -
            (int32_t)(((int64_t)sDecelerationTime * pJob->accelerate_stepss2) /
                      platformGetStepperTimerFreq());
        if (stepRate < pJob->exit_steps_s) { // Never go below exit velocity
            stepRate = pJob->exit_steps_s;
        }
    } else { // Cruise
        stepRate = pJob->cruise_steps_s;
    }

    // TODO: CONFIG:MIN_STEP_RATE
    const int32_t maxAxisMinVel =
        fix16_mul_int32(motionGetMinVelocityMM(pJob->max_axis),
                        platformMotionStepsPerMMAxis[pJob->max_axis]);
    if (stepRate < maxAxisMinVel) {
        stepRate = maxAxisMinVel;
    }
    configASSERT(stepRate > 0);

    uint32_t interval = (platformGetStepperTimerFreq() / stepRate);

    if (sIterationCompleted < pJob->accelerateUntil) {
        sAccelerationTime += interval;
    } else if (sIterationCompleted > pJob->decelerateAfter) {
        sDecelerationTime += interval;
    }

    return interval;
}

__attribute__((noinline)) uint16_t executeStep(uint16_t ticksElapsed)
{
    /* Check endstops first */
    if (job.type != StepperJobUndef && job.type != StepperJobRun) {
        for (uint8_t i = 0; i < NR_AXIS; ++i) {
            if ((job.type == StepperJobHomeX && i == X_AXIS) ||
                (job.type == StepperJobHomeY && i == Y_AXIS) ||
                (job.type == StepperJobHomeZ && i == Z_AXIS)) {
                if (platformGetEndstop(i)) {
                    sDiscardJob();
                    notifyHomeISR(sStepCounter);
                    return 0;
                }
            }
        }
    }

    // Executes Steps *FIRST*
    if (job.type != StepperJobUndef) {
        struct MoveSteps *pMove = &job.moveSteps;
        uint8_t stepperMask = 0;
        for (uint8_t i = 0; i < NR_STEPPER; i++) {
            sDeltaError[i] += sIncrementRatio[i];
            if (sDeltaError[i] >= 0) {
                sDeltaError[i] -= sStepSize;
                if (sStepsExecuted[i] < pMove->delta_x_steps[i]) {
                    sStepsExecuted[i]++;
                    stepperMask |= BIT(i);
                }
            }
        }
        platformStepStepper(stepperMask);

        sIterationCompleted += 1;
        if (sIterationCompleted >= pMove->delta_x_steps[pMove->max_axis]) {
            bool finished = true;
            for_each_stepper(i) {
                if (sStepsExecuted[i] < pMove->delta_x_steps[i]) {
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
        switch (job.type) {
        /* move */
        case StepperJobRun:
        case StepperJobHomeX:
        case StepperJobHomeY:
        case StepperJobHomeZ:
            break;
        /* no move */
        case StepperJobEnableSteppers:
            platformEnableStepper(0xFF);
            sDiscardJob();
            return 0;
        case StepperJobDisableSteppers:
            platformDisableStepper(0xFF);
            sDiscardJob();
            return 0;
        case StepperJobSync:
            xTaskNotifyFromISR(job.sync.notify, job.sync.seq, eSetValueWithOverwrite,
                               NULL);
            sDiscardJob();
            return 0;
        case StepperJobUndef:
        default:
            panic();
            break;
        }

        // All other return directly
        sStepCounter = 0;
        for_each_stepper(i) {
            if (platformMotionInvertStepper[i])
                job.moveSteps.step_dirs ^= BIT(i);
        }
        platformSetStepperDir((uint8_t) job.moveSteps.step_dirs);

        sIterationCompleted = 0;
        for_each_stepper(i)
            sStepsExecuted[i] = 0;
        sAccelerationTime = sDecelerationTime = 0;
        const uint32_t totalStepEvents = job.moveSteps.delta_x_steps[job.moveSteps.max_axis];
        sStepSize = totalStepEvents * 2;
        for (int i = 0; i < NR_STEPPER; i++) {
            sDeltaError[i] = -((int32_t)totalStepEvents);
            sIncrementRatio[i] = job.moveSteps.delta_x_steps[i] * 2;
        }
    }

    return sCalcInterval(&job.moveSteps);
}
