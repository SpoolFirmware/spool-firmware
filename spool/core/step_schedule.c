#include "step_schedule.h"
#include <stdint.h>
#include "platform/platform.h"
#include "bitops.h"

struct job {
    uint32_t aInterval;
    uint32_t bInterval;
    int32_t aSteps;
    int32_t bSteps;
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
    },
    {
    .aInterval = 10,
    .aSteps = -10000,
    .bInterval = 10,
    .bSteps = -10000,
    },
};

QueueHandle_t stepTaskInit(void)
{
    QueueHandle_t handle = xQueueCreateStatic(STEP_QUEUE_SIZE,
                                              sizeof(struct job),
                                              (uint8_t *)queueBuf, &stepQueue);
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

#define abs(x) (((x) >= 0) ? (x) : -(x))

void scheduleSteps(QueueHandle_t queueHandle)
{
    struct job tick = {0};
    int32_t aAcc = 0;
    uint32_t a = 0;
    int32_t bAcc = 0;
    uint32_t b = 0;
    BaseType_t woken;
    if (abs(aAcc) == abs(tick.aSteps) && abs(bAcc) == abs(tick.bSteps)) {
        if (xQueueReceiveFromISR(queueHandle, &tick, &woken) == pdTRUE) {
            a = tick.aInterval;
            b = tick.bInterval;
        } else
            return;
    }

    uint32_t stepper_mask = 0;
    uint32_t dir_mask = 0;
    if (a == 0) {
        stepper_mask |= STEPPER_A;
    }
    if (b == 0) {
        stepper_mask |= STEPPER_B;
    }
    a--;
    b--;
    if (stepper_mask == 0) {
        return;
    }
    if (tick.aSteps >= 0) {
        dir_mask |= STEPPER_A;
    }
    if (tick.bSteps >= 0) {
        dir_mask |= STEPPER_B;
    }

    stepStepper(stepper_mask, dir_mask);
}