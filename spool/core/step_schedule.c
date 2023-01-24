#include "step_schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "bitops.h"

#define NR_STEPPERS 2

struct StepperJob {
    uint16_t interval[NR_STEPPERS];
    uint16_t steps[NR_STEPPERS];
    uint8_t stepperDirs;
};

bool stepperJobFinished(const struct StepperJob *pStepperJob)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (pStepperJob->steps[i] != 0) {
            return false;
        }
    }
    return true;
}

struct StepperJob queueBuf[STEP_QUEUE_SIZE];

StaticQueue_t stepQueue;

// uint32_t posX;
// uint32_t posY;



#define MICROSTEPPING 32
#define STEPS_PER_ROT 200
#define MM_PER_ROT 31
#define MM_TO_STEP(x) ((x) * MICROSTEPPING * STEPS_PER_ROT / MM_PER_ROT)

static const struct StepperJob jobs[2] = {
    {
        .interval = { 10, 10 },
        .steps = { 10000, 10000 },
        .stepperDirs = BIT(0) | BIT(1),
    },
    {
        .interval = { 10, 10 },
        .steps = { 10000, 10000 },
        .stepperDirs = 0,
    },
    };

QueueHandle_t stepTaskInit(void)
{
    QueueHandle_t handle = xQueueCreateStatic(
        STEP_QUEUE_SIZE, sizeof(struct StepperJob), (uint8_t *)queueBuf, &stepQueue);
    return handle;
}

/* 1/32 microstepping */
/* 200 steps per rotation */
/* 0.00589048125mm per microstepping */
// static uint32_t stepPerMM = 170; /* ish, TODO fix inaccuracy */
// static uint32_t maxVelX = 10; /* mm/s */
// static uint32_t maxAccX = 1000; /* */

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    int x = 0;

    enableStepper(STEPPER_A | STEPPER_B);
    for (;;) {
        if (xQueueSend(queue, &jobs[x], 10) == pdTRUE) {
            x = !x;
        }
    }
}

__attribute__((noinline)) void scheduleSteps(QueueHandle_t queueHandle)
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
