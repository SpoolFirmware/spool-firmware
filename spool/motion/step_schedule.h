#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "lib/planner/planner.h"

struct PrinterMove {
    int32_t x; /* unit: steps */
    int32_t y;
    int32_t z;
    int32_t e;
};

struct PrinterState {
    int32_t x; /* unit: steps */
    int32_t y;
    int32_t z;
    int32_t e;
    bool continuousMode;
    bool homedX : 1, homedY : 1, homedZ : 1;
};

void motionPlannerTaskInit(void);

void notifyHomeISR(uint32_t stepsMoved);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
