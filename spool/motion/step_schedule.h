#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "core/magic_config.h"
#include "step_plan_ng.h"

struct PrinterState {
    int32_t x; /* unit: steps */
    int32_t y;
    int32_t z;
    int32_t e;
    int32_t feedrate;
    bool continuousMode;
};

typedef struct MotionBlock {
    uint32_t totalSteps;
} motion_block_t;

typedef struct StepperJob {
    motion_block_t blocks[NR_STEPPERS];
    enum JobType type;
    uint8_t stepDirs;

    // v2 Stuff (Line Drawing Meme)
    uint32_t totalStepEvents;
    uint32_t accelerateUntil;
    uint32_t decelerateAfter;

    uint32_t entry_steps_s;
    uint32_t cruise_steps_s;
    uint32_t exit_steps_s;
    uint32_t accel_steps_s2;
} job_t;

void motionPlannerTaskInit(void);

void notifyHomeXISR(void);
void notifyHomeYISR(void);
void notifyHomeZISR(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
