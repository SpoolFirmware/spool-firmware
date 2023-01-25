#include "step_schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "bitops.h"

struct StepperJob queueBuf[STEP_QUEUE_SIZE];

StaticQueue_t stepQueue;

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
    xTaskCreate(stepScheduleTask, "stepSchedule", 1024, (void *)handle,
                tskIDLE_PRIORITY + 2, (TaskHandle_t *)NULL);
    return handle;
}

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
