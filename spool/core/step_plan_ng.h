#pragma once
#include "fix16.h"
#include "magic_config.h"
#if 0
#include "FreeRTOS.h"
#include "task.h"
#endif
#include <stdbool.h>

// static const fix16_t VEL_FIX = F16(VEL);
// static const fix16_t HOMING_VEL_FIX = F16(HOMING_VEL);
const static fix16_t ACC_FIX = F16(ACC);
const static fix16_t STEPS_PER_MM_FIX = F16(STEPS_PER_MM);

const static int32_t VEL_CHANGE_THRESHOLD = 10;

const static int32_t STEPPER_ACC[NR_STEPPERS] = {
    ACC * STEPS_PER_MM,
    ACC * STEPS_PER_MM,
};

const static int32_t STEPPER_STEPS_PER_MM[NR_STEPPERS] = {
    STEPS_PER_MM,
    STEPS_PER_MM,
};

#define MOTION_LOOKAHEAD 10

#define PLANNING_TASK_NOTIFY_SLOT 1

#define SECONDS_IN_MIN 60

struct PlannerBlock {
    int32_t x;
    int32_t accelerationX;
    int32_t decelerationX;
    int32_t a;
    uint32_t viSq;
    uint32_t vSq;
    int32_t v;
    uint32_t vfSq;
};

/* suboptimal arrangement, do we really care about the axes */
enum JobType {
    /* default value is to not run */
    StepperJobUndef = 0,
    StepperJobRun,
    StepperJobHomeX,
    StepperJobHomeY,
    StepperJobDisableSteppers,
};

struct PlannerJob {
    struct PlannerBlock steppers[NR_STEPPERS];
    enum JobType type;
};

#if 0
void initPlanner(TaskHandle_t producer, TaskHandle_t consumer);
#endif
void initPlanner(void);

void planCoreXy(const fix16_t movement[NR_AXES], int32_t plan[NR_STEPPERS]);

// void dequeuePlan(struct PlannerJob *out);
// void enqueuePlan(const int32_t plan[NR_STEPPERS],
//                  const int32_t max_v[NR_STEPPERS], bool more_entries);

uint32_t plannerAvailableSpace(void);
uint32_t plannerSize(void);

void __dequeuePlan(struct PlannerJob *out);
void __enqueuePlan(enum JobType k, const int32_t plan[NR_STEPPERS],
                   const int32_t max_v[NR_STEPPERS], bool more_entries);

#define for_each_axis(iter) for (uint8_t iter = 0; iter < NR_AXES; iter++)

#define for_each_stepper(iter) \
    for (uint8_t iter = 0; iter < NR_STEPPERS; iter++)
