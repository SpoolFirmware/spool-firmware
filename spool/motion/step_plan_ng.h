#pragma once
#include "fix16.h"
#include "core/magic_config.h"
#include <stdbool.h>
#include "error.h"
#include "compiler.h"

const static uint32_t VEL_CHANGE_THRESHOLD = 10;

const static uint32_t STEPPER_ACC[] = {
    ACC * STEPS_PER_MM,
    ACC *STEPS_PER_MM,
    ACC_Z *STEPS_PER_MM_Z,
    ACC_E *STEPS_PER_MM_E,
};
STATIC_ASSERT(ARRAY_SIZE(STEPPER_ACC) == NR_STEPPERS);

const static uint32_t STEPPER_STEPS_PER_MM[] = {
    STEPS_PER_MM,
    STEPS_PER_MM,
    STEPS_PER_MM_Z,
    STEPS_PER_MM_E,
};
STATIC_ASSERT(ARRAY_SIZE(STEPPER_STEPS_PER_MM) == NR_STEPPERS);

#define MOTION_LOOKAHEAD 10

#define PLANNING_TASK_NOTIFY_SLOT 1

/* MAGIC PLANNER CONSTANTS */
/* TODO MAKE CONFIGURABLE */
#define SECONDS_IN_MIN 60

#define JUNCTION_INHERIT_VEL_THRES F16(0.99f)
/* for distances less than DIST, and angles cos greater than octagon (from marlin)
 * we smooth out the velocity */
#define JUNCTION_SMOOTHING_DIST_THRES F16(1)
#define JUNCTION_SMOOTHING_THRES      F16(0.7071067812f)

struct PlannerBlock {
    uint32_t x;
};

/* suboptimal arrangement, do we really care about the axes */
/* re comment above: no */
enum JobType {
    /* default value is to not run */
    StepperJobUndef = 0,
    StepperJobRun,
    StepperJobHomeX,
    StepperJobHomeY,
    StepperJobHomeZ,
    StepperJobDisableSteppers,
};

struct PlannerJob {
    uint32_t x;
    uint32_t accelerationX;
    uint32_t decelerationX;
    uint32_t a;
    uint32_t viSq;
    uint32_t vSq;
    uint32_t vfSq;
    fix16_t len;
    fix16_t unit_vec[NR_AXES];
    struct PlannerBlock steppers[NR_STEPPERS];
    enum JobType type;
    uint8_t stepDirs;
};

void initPlanner(void);

void planCoreXy(const int32_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
                fix16_t unit_vec[NR_AXES], fix16_t *len);

uint32_t plannerAvailableSpace(void);
uint32_t plannerSize(void);

void __dequeuePlan(struct PlannerJob *out);
void __enqueuePlan(enum JobType k, const int32_t plan[NR_STEPPERS],
                   const fix16_t unit_vec[NR_AXES],
                   const uint32_t max_v[NR_STEPPERS],
                   const uint32_t acc[NR_STEPPERS], fix16_t len, bool stop);

#define for_each_axis(iter) for (uint8_t iter = 0; iter < NR_AXES; iter++)

#define for_each_stepper(iter) \
    for (uint8_t iter = 0; iter < NR_STEPPERS; iter++)
