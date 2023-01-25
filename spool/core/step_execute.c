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

static bool stepperJobFinished(const struct StepperJob *pStepperJob)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (pStepperJob->steps[i] != 0) {
            return false;
        }
    }
    return true;
}

__attribute__((noinline)) void executeStep()
{
    static struct StepperJob job = { 0 };
    static uint16_t counter[NR_STEPPERS] = { 0 };
    halGpioSet(platformGetStatusLED());
    if (stepperJobFinished(&job)) {
        if (xQueueReceiveFromISR(queueHandle, &job, NULL) == pdTRUE) {
            setStepperDir(job.stepperDirs);
            for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
                counter[i] = job.interval[i];
            }
        }
        return;
    }

    uint32_t stepper_mask = 0;
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (job.steps[i] != 0) {
            if (counter[i] == 0) {
                stepper_mask |= BIT(i);
                counter[i] = job.interval[i];
                job.steps[i]--;
            } else {
                counter[i]--;
            }
        }
    }
    if (stepper_mask == 0) {
        return;
    }
    stepStepper(stepper_mask);
    halGpioClear(platformGetStatusLED());
}
