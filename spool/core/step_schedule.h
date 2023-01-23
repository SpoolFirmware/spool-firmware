#pragma once

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

#define STEP_QUEUE_SIZE 1000

struct StepTick {
    uint8_t stepper_mask;
    uint8_t dir_mask;
    uint8_t freq;
};

QueueHandle_t stepTaskInit(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
