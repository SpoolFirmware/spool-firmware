#include "step_execute.h"
#include "step_schedule.h"
#include "magic_config.h"
#include "bitops.h"
#include <stdbool.h>

static QueueHandle_t queueHandle;

void stepExecuteSetQueue(QueueHandle_t queueHandle_)
{
    queueHandle = queueHandle_;
}

static bool stepperJobFinished(const struct StepperJob *pJob)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        motion_block_t *pBlock = &(pJob->blocks[i]);
        if (pBlock->stepsExecuted < pBlock->totalSteps) {
            return false;
        }
    }
    return true;
}

/*!
 * Derived from v = 1/2 a t^2
 *
 * Since v is essentially step frequency (scaled by steps/mm), this can be simplified into
 *
 * interval = 1/(v0 - (accel_steps_s2 * ticks_elapsed_in_stage))
 */
static uint16_t sCalcInterval(motion_block_t *pBlock)
{
    switch(pBlock->blockState) {
        case BlockStateDecelerating:
            return (uint32_t)(((uint64_t)getStepperTimerFreq() * getStepperTimerFreq()) >> 10UL) / (pBlock->cruiseVel_steps_s - (ACCEL_STEPS >> 10UL * (pBlock->stepsExecuted - pBlock->cruiseSteps - pBlock->accelerationSteps)));
            break;
        case BlockStateAccelerating:
            return (uint32_t)(((uint64_t)getStepperTimerFreq() * getStepperTimerFreq()) >> 10UL) / (pBlock->entryVel_steps_s + (ACCEL_STEPS >> 10UL * pBlock->stepsExecuted));
            break;
        case BlockStateCruising:
            return getStepperTimerFreq() / pBlock->cruiseVel_steps_s;
        default:
            break;
    }
    return 1;
}

void executeStep(void)
{
    static struct StepperJob job = { 0 };
    static uint16_t counter[NR_STEPPERS] = { 0 };

    uint32_t stepper_mask = 0;
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        motion_block_t *pBlock = &job.blocks[i];
        if (pBlock->totalSteps < pBlock->stepsExecuted) {
            if (counter[i] == 0) {
                pBlock->stepsExecuted += 1;
                switch(pBlock->blockState) {
                    case BlockStateAccelerating:
                        if (pBlock->stepsExecuted >= pBlock->accelerationSteps) {
                            if (pBlock->cruiseSteps != 0) {
                                pBlock->blockState = BlockStateCruising;
                            } else {
                                pBlock->blockState = BlockStateDecelerating;
                            }
                        }
                        break;
                    case BlockStateCruising:
                        if (pBlock->stepsExecuted >= pBlock->accelerationSteps + pBlock->cruiseSteps) {
                            pBlock->blockState = BlockStateDecelerating;
                        }
                        break;
                    default:
                        break;
                }
                stepper_mask |= BIT(i);
                counter[i] = sCalcInterval(pBlock);
            } else {
                counter[i]--;
            }
        }
    }
    stepStepper(stepper_mask);

    if (stepperJobFinished(&job)) {
        if (xQueueReceiveFromISR(queueHandle, &job, NULL) != pdTRUE) {
            return;
        }
        setStepperDir(job.stepDirs);
        for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
            counter[i] = 0;
            job.blocks[i].blockState = BlockStateAccelerating;
            job.blocks[i].stepsExecuted = 0;
        }
    }

}
