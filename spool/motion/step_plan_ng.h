#pragma once
#include "fix16.h"
#include "motion/motion.h"
#include "platform/platform.h"
#include <stdbool.h>
#include "error.h"
#include "compiler.h"

const static uint32_t VEL_CHANGE_THRESHOLD = 10;

/* maximum size of the plan buffer */
#define MOTION_LOOKAHEAD 15

#define PLANNING_TASK_NOTIFY_SLOT 1

/* MAGIC PLANNER CONSTANTS */
/* TODO MAKE CONFIGURABLE */
#define SECONDS_IN_MIN 60

#define JUNCTION_STOP_VEL_THRES F16(0.99999f)
/* for distances less than DIST, and angles cos greater than octagon (from marlin)
 * we smooth out the velocity */
#define JUNCTION_SMOOTHING_DIST_THRES(STEPPER) F16(1 * platformMotionStepsPerMM[STEPPER])
#define JUNCTION_SMOOTHING_THRES      F16(-0.707f)

/* suboptimal arrangement, do we really care about the axes */
/* re comment above: no */
enum JobType {
    /* default value is to not run */
    StepperJobUndef = 0,
    /* moves */
    StepperJobRun,
    StepperJobHomeX,
    StepperJobHomeY,
    StepperJobHomeZ,
    /* not moves */
    StepperJobEnableSteppers,
    StepperJobDisableSteppers,
    StepperJobSync,
};

struct PlannerJob {
    enum JobType type;
    union {
        struct {
            //! number of steps on the "longest" axis.
            uint32_t x; 
            uint32_t accelerationX;
            uint32_t decelerationX;
            //! acceleration in steps?
            int32_t accMM;
            int32_t accSteps;
            uint32_t viSq;
            uint32_t vSq;
            uint32_t vfSq;
            fix16_t lenMM;
            fix16_t unit_vec[NR_AXIS];
            uint32_t steppers[NR_STEPPER];
            uint8_t stepDirs;
        }; /* StepperJobRun, StepperJobHomeX, StepperJobHomeY, StepperJobHomeZ
            */
        struct {
            TaskHandle_t notify;
            uint32_t seq;
        }; /* StepperJobSync */
    };
};

static inline bool plannerJobIsMove(const struct PlannerJob *j)
{
    switch (j->type) {
    case StepperJobRun:
    case StepperJobHomeX:
    case StepperJobHomeY:
    case StepperJobHomeZ:
        return true;
    case StepperJobSync:
    case StepperJobEnableSteppers:
    case StepperJobDisableSteppers:
        return false;
    default:
        panic();
        break;
    }
}

void plannerInit_legacy(void);

uint32_t plannerAvailableSpace(void);
uint32_t plannerCapacity(void);
uint32_t plannerSize(void);

void plannerDequeue(struct PlannerJob *out);
void plannerEnqueue(enum JobType k);
void plannerEnqueueNotify(enum JobType k, TaskHandle_t notify, uint32_t seq);
void plannerEnqueueMove(enum JobType k, const int32_t plan[NR_STEPPER],
                   const fix16_t unit_vec[NR_AXIS],
                   const uint32_t max_v[NR_STEPPER],
                   const int32_t acc[NR_STEPPER], fix16_t len, bool stop);