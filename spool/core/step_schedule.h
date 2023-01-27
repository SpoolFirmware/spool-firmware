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
} motion_block_t;

typedef struct StepperJob {
    motion_block_t blocks[NR_STEPPERS];
    uint8_t stepDirs;
} job_t;


#define STEP_QUEUE_SIZE 20

QueueHandle_t stepTaskInit(void);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
