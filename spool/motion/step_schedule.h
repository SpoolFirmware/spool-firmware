#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "step_plan_ng.h"

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

typedef struct MotionBlock {
    uint32_t totalSteps;
} motion_block_t;

typedef struct StepperJob {
    enum JobType type;
    union {
        struct {
            motion_block_t blocks[NR_STEPPER];
            // v2 Stuff (Line Drawing Meme)
            uint32_t totalStepEvents;
            uint32_t accelerateUntil;
            uint32_t decelerateAfter;

            uint32_t entry_steps_s;
            uint32_t cruise_steps_s;
            uint32_t exit_steps_s;
            uint32_t accel_steps_s2;

            uint8_t stepDirs;
        };
        struct {
            TaskHandle_t notify;
            uint32_t seq;
        };
    };
} job_t;

void motionPlannerTaskInit(void);

void notifyHomeISR(uint32_t stepsMoved);

portTASK_FUNCTION_PROTO(stepScheduleTask, pvParameters);
