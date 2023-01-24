#include "step_schedule.h"
#include <stdint.h>
#include "platform/platform.h"
#include "bitops.h"

struct job {
    uint16_t aInterval;
    uint16_t bInterval;
    uint16_t aSteps;
    uint16_t bSteps;
    uint8_t stepperDirs;
};

struct job queueBuf[STEP_QUEUE_SIZE];

StaticQueue_t stepQueue;

// uint32_t posX;
// uint32_t posY;

static struct job jobs[2] = {
    {
        .aInterval = 10,
        .aSteps = 10000,
        .bInterval = 10,
        .bSteps = 10000,
        .stepperDirs = BIT(0) | BIT(1)
    },
    {
        .aInterval = 10,
        .aSteps = 10000,
        .bInterval = 10,
        .bSteps = 10000,
        .stepperDirs = 0
    },
};

QueueHandle_t stepTaskInit(void)
{
    QueueHandle_t handle = xQueueCreateStatic(
        STEP_QUEUE_SIZE, sizeof(struct job), (uint8_t *)queueBuf, &stepQueue);
    // xQueueSend(handle, &jobs[0], 10);
    // xQueueSend(handle, &jobs[1], 10);
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
    static struct job job = { 0 };
    static uint16_t aCntr = 0;
    static uint16_t bCntr = 0;
    if (job.aSteps == 0 && job.bSteps == 0) {
        if (xQueueReceiveFromISR(queueHandle, &job, NULL) == pdTRUE) {
            halGpioToggle(platformGetStatusLED());
            setStepperDir(job.stepperDirs);
            aCntr = job.aInterval;
            bCntr = job.bInterval;
            return;
        } else {
            return;
        }
    }

    uint32_t stepper_mask = 0;
    if (aCntr == 0) {
        stepper_mask |= STEPPER_A;

        aCntr = job.aInterval;
        job.aSteps--;
    }
    if (bCntr == 0) {
        stepper_mask |= STEPPER_B;
        
        bCntr = job.bInterval;
        job.bSteps--;
    }
    aCntr--;
    bCntr--;

    if (stepper_mask == 0) {
        return;
    }
    stepStepper(stepper_mask);
}
