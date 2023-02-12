#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "core/magic_config.h"
#include "step_plan_ng.h"

struct PrinterState {
    fix16_t x;
    fix16_t y;
    fix16_t z;
    fix16_t e;
    fix16_t feedrate;
    bool continuousMode;
};

enum MotionBlockState {
    BlockStateAccelerating = 0,
    BlockStateCruising,
    BlockStateDecelerating,
};

typedef struct MotionBlock {
    uint32_t totalSteps;
    uint32_t accelerationSteps;
    uint32_t cruiseSteps;
    uint32_t decelerationSteps;

    uint32_t entryVel_steps_s;
    uint32_t cruiseVel_steps_s;
    uint32_t exitVel_steps_s;

    uint32_t stepsExecuted;
    enum MotionBlockState blockState;
    uint32_t ticksCurState;
} motion_block_t;

typedef struct StepperJob {
    motion_block_t blocks[NR_STEPPERS];
    enum JobType type;
    uint8_t stepDirs;
} job_t;

void motionPlannerTaskInit(void);

void notifyHomeXISR(void);
void notifyHomeYISR(void);
void notifyHomeZISR(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
