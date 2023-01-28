#include "step_execute.h"
#include "step_schedule.h"
#include "magic_config.h"
#include "bitops.h"
#include "error.h"
#include <stdbool.h>

static QueueHandle_t queueHandle;

void stepExecuteSetQueue(QueueHandle_t queueHandle_)
{
    queueHandle = queueHandle_;
}

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

static uint64_t getFreqSquared(void) {
    return ((uint64_t)getStepperTimerFreq()) * getStepperTimerFreq();
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
            return (uint16_t)(getFreqSquared() / (pBlock->cruiseSteps - (ACCEL_STEPS * pBlock->ticksCurState)));
        case BlockStateAccelerating:
            return (uint16_t)(getFreqSquared() / (pBlock->entryVel_steps_s + (ACCEL_STEPS * pBlock->ticksCurState)));
        case BlockStateCruising:
            return (uint16_t)(getStepperTimerFreq() / pBlock->cruiseVel_steps_s);
        default:
            break;
    }
    return 1;
}

static uint32_t min(uint32_t a, uint32_t b)
{
    return a < b ? a : b;
}

void executeStep(void)
{
    static struct StepperJob job = { 0 };
    static uint16_t counter[NR_STEPPERS] = { 0 };
    uint32_t stepper_mask = 0;

    if (stepperJobFinished(&job)) {
        if (xQueueReceiveFromISR(queueHandle, &job, NULL) != pdTRUE) {
            return;
        }
        setStepperDir(job.stepDirs);
        for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
            counter[i] = 0;
            job.blocks[i].blockState = BlockStateAccelerating;
            job.blocks[i].stepsExecuted = 0;
            job.blocks[i].ticksCurState = 1;
        }
    }

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        motion_block_t *pBlock = &job.blocks[i];
        if (pBlock->stepsExecuted < pBlock->totalSteps) {
            if (counter[i] == 0) {
                pBlock->stepsExecuted += 1;
                stepper_mask |= BIT(i);
                switch(pBlock->blockState) {
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
                        if (pBlock->stepsExecuted >= pBlock->accelerationSteps + pBlock->cruiseSteps) {
                            pBlock->blockState = BlockStateDecelerating;
                            pBlock->ticksCurState = 1;
                        }
                        break;
                    default:
                        break;
                }
                counter[i] = sCalcInterval(pBlock);
                if (counter[i] > 40) counter[i] = 40;
                pBlock->ticksCurState += counter[i];
            } else {
                counter[i]--;
            }
        }
    }
    stepStepper(stepper_mask);
}
