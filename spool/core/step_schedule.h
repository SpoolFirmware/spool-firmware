#pragma once
#include "FreeRTOS.h"
#include "platform/platform.h"
#include "task.h"
#include "queue.h"
#include "magic_config.h"

struct PrinterState {
    fix16_t x;
    fix16_t y;
};

struct StepperJob {
    uint16_t interval[NR_STEPPERS];
    uint16_t steps[NR_STEPPERS];
    uint8_t stepperDirs;
};

#define STEP_QUEUE_SIZE 20

QueueHandle_t stepTaskInit(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
