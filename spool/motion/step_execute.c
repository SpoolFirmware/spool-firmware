#include <stdbool.h>

#include "misc.h"
#include "string.h"

#include "core/spool.h"

#include "step_execute.h"
#include "step_schedule.h"
#include "core/magic_config.h"

static uint64_t getFreqSquared(void)
{
    return ((uint64_t)platformGetStepperTimerFreq()) * platformGetStepperTimerFreq();
}

static struct StepperJob job = {0};
static uint32_t sIterationCompleted = 0;
// Line Drawing
static uint32_t sAccelerationTime;
static uint32_t sDecelerationTime;
static int32_t sDeltaError[NR_STEPPERS]; // "error"
static uint32_t sIncrementRatio[NR_STEPPERS]; // "m". This expect slope <= 1
static uint32_t sStepSize; // "x" step size

static inline void sDiscardJob(void)
{
    job.type = StepperJobUndef;
}

static uint16_t sCalcInterval(struct StepperJob *pJob)
{
    int64_t stepRate;

    if (sIterationCompleted < pJob->accelerateUntil) { // Accelerating
        stepRate = ((int64_t)sAccelerationTime * pJob->accel_steps_s2) + pJob->entry_steps_s;
        if (stepRate > pJob->cruise_steps_s) {
            stepRate = pJob->cruise_steps_s;
        }
    } else if (sIterationCompleted > pJob->decelerateAfter) {  // Decelertaing
        stepRate = (int64_t)pJob->cruise_steps_s - ((int64_t)sDecelerationTime * pJob->accel_steps_s2);
        if (stepRate < pJob->exit_steps_s) { // Never go below exit velocity
            stepRate = pJob->exit_steps_s;
        }
    } else { // Cruise
        stepRate = pJob->cruise_steps_s;
    }

    if (stepRate < 100) { // TODO: CONFIG: MIN_STEP_RATE
        stepRate = 100;
    }

    return ((uint32_t)stepRate / platformGetStepperTimerFreq());
}

uint16_t executeStep(uint16_t ticksElapsed)
{
    static uint16_t counter[NR_STEPPERS] = { 0 };

    /* Check endstops first */
    if (job.type != StepperJobUndef && job.type != StepperJobRun) {
        for (uint8_t i = 0; i < NR_AXES; ++i) {
            if (platformGetEndstop(i)) {
                if (job.type == StepperJobHomeX && i == ENDSTOP_X) {
                    sDiscardJob();
                    notifyHomeXISR();
                    return 0;
                }
                if (job.type == StepperJobHomeY && i == ENDSTOP_Y) {
                    sDiscardJob();
                    notifyHomeYISR();
                    return 0;
                }
                if (job.type == StepperJobHomeZ && i == ENDSTOP_Z) {
                    sDiscardJob();
                    notifyHomeZISR();
                    return 0;
                }
            }
        }
    }

    // Executes Steps *FIRST*
    if (job.type != StepperJobUndef) {
        uint8_t stepperMask = 0;
        for (uint8_t i = 0; i < NR_STEPPERS; i++) {
            sDeltaError[i] += sIncrementRatio[i];
            if (sDeltaError[i] >= 0) {
                sDeltaError[i] -= sStepSize;
                stepperMask |= BIT(i);
            }
        }
        platformStepStepper(stepperMask);

        sIterationCompleted += 1;
        if (sIterationCompleted >= job.totalStepEvents) {
            sDiscardJob();
        }
    }

    // Now we can do other things
    if (job.type == StepperJobUndef) {
        if (xQueueReceiveFromISR(StepperExecutionQueue, &job, NULL) != pdTRUE) {
            return platformGetStepperTimerFreq() / 1000; // 1ms
        }
        // Acquired new block
        platformSetStepperDir(job.stepDirs);

        // TODO: remove once supported in planner?
        if (job.totalStepEvents == 0)  {
            uint8_t maxStepAxisIndex = 0;
            for (int i = 0; i < NR_STEPPERS; i++) {
                if (job.blocks[i].totalSteps >= job.totalStepEvents) {
                    maxStepAxisIndex = i;
                    job.totalStepEvents = job.blocks[i].totalSteps;
                }
            }
            job.totalStepEvents = job.blocks[maxStepAxisIndex].totalSteps;
            job.accelerateUntil = job.blocks[maxStepAxisIndex].accelerationSteps;
            job.decelerateAfter = job.accelerateUntil + job.blocks[maxStepAxisIndex].cruiseSteps;
            job.entry_steps_s = job.blocks[maxStepAxisIndex].entryVel_steps_s;
            job.cruise_steps_s = job.blocks[maxStepAxisIndex].cruiseVel_steps_s;
            job.exit_steps_s = job.blocks[maxStepAxisIndex].exitVel_steps_s;
            job.accel_steps_s2 = ACCEL_STEPS; // TODO: FIX THIS IS WRONG 
        }

        sIterationCompleted = 0;
        sAccelerationTime = sDecelerationTime = 0;
        sStepSize = job.totalStepEvents * 2;
        for (int i = 0; i < NR_STEPPERS; i++) {
            sDeltaError[i] = -((int32_t) job.totalStepEvents);
            sIncrementRatio[i] = job.blocks[i].totalSteps * 2;
        }
    }

    return sCalcInterval(&job);
}
