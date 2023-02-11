#include <stdbool.h>

#include "misc.h"
#include "string.h"

#include "spool.h"

#include "step_execute.h"
#include "step_schedule.h"
#include "magic_config.h"

static bool stepperJobFinished(const struct StepperJob *pJob)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        const motion_block_t *pBlock = &(pJob->blocks[i]);
        if (pBlock->stepsExecuted < pBlock->totalSteps) {
            return false;
        }
    }
    return true;
}

static uint64_t getFreqSquared(void)
{
    return ((uint64_t)platformGetStepperTimerFreq()) * platformGetStepperTimerFreq();
}

/*!
 * Derived from delta_v = 1/2 a t^2
 *
 * Since del_v is essentially step frequency (scaled by steps/mm),
 * this can be simplified into
 *
 * interval = freq_squared / (v0 * freq + (accel_steps_s2 * ticks_elapsed_in_stage))
 */
static uint16_t sCalcInterval(motion_block_t *pBlock)
{
    uint64_t deltaVel = ACCEL_STEPS * (uint64_t)pBlock->ticksCurState;
    switch (pBlock->blockState) {
    case BlockStateDecelerating: {
        uint64_t initialVel =
            ((uint64_t)pBlock->cruiseVel_steps_s) * platformGetStepperTimerFreq();
        if (deltaVel > initialVel) {
            return 10;
        } else {
            return (uint16_t)(getFreqSquared() / (initialVel - deltaVel));
        }
    }
    case BlockStateAccelerating: {
        uint64_t initialVel =
            (uint64_t)pBlock->entryVel_steps_s * platformGetStepperTimerFreq();
        return (uint16_t)(getFreqSquared() / (initialVel + deltaVel));
    }
    case BlockStateCruising:
        return (uint16_t)(platformGetStepperTimerFreq() / pBlock->cruiseVel_steps_s);
    default:
        break;
    }
    return 1;
}

uint16_t executeStep(uint16_t ticksElapsed)
{
    static struct StepperJob job = { 0 };
    static uint16_t counter[NR_STEPPERS] = { 0 };

    /* Check endstops first */
    for (uint8_t i = 0; i < NR_AXES; ++i) {
        if (platformGetEndstop(i)) {
            if (job.type == StepperJobHomeX && i == ENDSTOP_X) {
                memset(&job, 0, sizeof(job));
                memset(&counter, 0, sizeof(counter));
                notifyHomeXISR();
                return 0;
            }
            if (job.type == StepperJobHomeY && i == ENDSTOP_Y) {
                memset(&job, 0, sizeof(job));
                memset(&counter, 0, sizeof(counter));
                notifyHomeYISR();
                return 0;
            }
        }
    }

    if (job.type != StepperJobUndef) {
        // Executes Steps *FIRST*
        uint8_t stepper_mask = 0;
        for (uint8_t i = 0; i < NR_STEPPERS; i++) {
            motion_block_t *pBlock = &job.blocks[i];
            if (pBlock->stepsExecuted < pBlock->totalSteps) {
                if (counter[i] > ticksElapsed) {
                    counter[i] -= ticksElapsed;
                } else {
                    counter[i] = 0;
                    pBlock->stepsExecuted += 1;
                    stepper_mask |= BIT(i);
                }
            }
        }
        platformStepStepper(stepper_mask);
    }

    // Now we can do other things
    if (stepperJobFinished(&job)) {
        if (xQueueReceiveFromISR(StepperExecutionQueue, &job, NULL) != pdTRUE) {
            return 0;
        }
        platformSetStepperDir(job.stepDirs);
        for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
            counter[i] = 0;
            job.blocks[i].blockState = BlockStateAccelerating;
            job.blocks[i].stepsExecuted = 0;
            job.blocks[i].ticksCurState = 1;
        }
    }

    uint16_t sleepTime = 40;
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        motion_block_t *pBlock = &job.blocks[i];
        if (pBlock->stepsExecuted < pBlock->totalSteps) {
            if (counter[i] == 0) {
                // Move the block state if required
                switch (pBlock->blockState) {
                case BlockStateAccelerating:
                    if (pBlock->stepsExecuted >= pBlock->accelerationSteps) {
                        if (pBlock->cruiseSteps != 0) {
                            pBlock->blockState = BlockStateCruising;
                        } else {
                            pBlock->blockState = BlockStateDecelerating;
                            pBlock->ticksCurState = 1;
                        }
                    }
                    break;
                case BlockStateCruising:
                    if (pBlock->stepsExecuted >=
                        pBlock->accelerationSteps + pBlock->cruiseSteps) {
                        pBlock->blockState = BlockStateDecelerating;
                        pBlock->ticksCurState = 1;
                    }
                    break;
                default:
                    break;
                }

                // Calculate the next step interval
                counter[i] = sCalcInterval(pBlock);
                if (counter[i] > 40)
                    counter[i] = 40;
            }
            // Update next wakeup time
            pBlock->ticksCurState += ticksElapsed;
            if (counter[i] < sleepTime) {
                sleepTime = counter[i];
            }
        }
    }

    return sleepTime;
}
