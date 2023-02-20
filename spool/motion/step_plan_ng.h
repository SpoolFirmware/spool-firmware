#pragma once
#include "fix16.h"
#include "motion/motion.h"
#include "platform/platform.h"
#include <stdbool.h>
#include "error.h"
#include "compiler.h"

const static uint32_t VEL_CHANGE_THRESHOLD = 10;

#define MOTION_LOOKAHEAD 10

#define PLANNING_TASK_NOTIFY_SLOT 1

/* MAGIC PLANNER CONSTANTS */
/* TODO MAKE CONFIGURABLE */
#define SECONDS_IN_MIN 60

#define JUNCTION_INHERIT_VEL_THRES F16(-0.99999f)
/* for distances less than DIST, and angles cos greater than octagon (from marlin)
 * we smooth out the velocity */
#define JUNCTION_SMOOTHING_DIST_THRES(STEPPER) F16(1 * platformMotionStepsPerMM[STEPPER])
#define JUNCTION_SMOOTHING_THRES      F16(-0.5f)

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
    fix16_t unit_vec[X_AND_Y];
    struct PlannerBlock steppers[NR_STEPPER];
    enum JobType type;
    uint8_t stepDirs;
};

void initPlanner(void);

uint32_t plannerAvailableSpace(void);
uint32_t plannerSize(void);

void __dequeuePlan(struct PlannerJob *out);
void __enqueuePlan(enum JobType k, const int32_t plan[NR_STEPPER],
                   const fix16_t unit_vec[X_AND_Y],
                   const uint32_t max_v[NR_STEPPER],
                   const uint32_t acc[NR_STEPPER], fix16_t len, bool stop);