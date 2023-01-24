#pragma once

#include "FreeRTOS.h"
#include "platform/platform.h"
#include "task.h"
#include "queue.h"

#define STEP_QUEUE_SIZE 20

struct StepTick {
    uint8_t stepper_mask;
    uint8_t dir_mask;
};

QueueHandle_t stepTaskInit(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);

void scheduleSteps(QueueHandle_t queueHandle);
