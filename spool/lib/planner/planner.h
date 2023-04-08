#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "fix16.h"

#define MAX_STEPPERS 4
#define MAX_AXIS 4

enum JobType {
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

struct PlannerMove {
    uint32_t job_type;
    int32_t motor_steps[MAX_STEPPERS];
    fix16_t delta_x[MAX_AXIS];
    fix16_t min_v[MAX_AXIS];
    fix16_t max_v[MAX_AXIS];
    fix16_t acc[MAX_AXIS];
    bool stop;
};

struct MoveSteps {
    uint32_t delta_x_steps[MAX_STEPPERS];
    uint32_t step_dirs;

    uint32_t max_axis;
    uint32_t accelerateUntil;
    uint32_t decelerateAfter;
    
    //! Acceleration
    uint32_t accelerate_stepss2;

    uint32_t entry_steps_s;
    uint32_t cruise_steps_s;
    uint32_t exit_steps_s;
};

struct SyncJob {
	void *notify;
	uint32_t seq;
};

struct ExecutorJob {
	enum JobType type;
	union {
		struct MoveSteps moveSteps;
		struct SyncJob sync;
	};
};

typedef struct Planner *PlannerHandle;

// Implemented In Rust
PlannerHandle plannerInit(uint32_t numAxis, uint32_t numStepper, const uint32_t stepsPerMM[MAX_STEPPERS]);

bool plannerEnqueue(PlannerHandle handle, const struct PlannerMove *plannerMove);
bool plannerEnqueueSync(PlannerHandle handle, const struct SyncJob *syncJob);
bool plannerEnqueueOther(PlannerHandle handle, uint32_t jobType);
bool plannerDequeue(PlannerHandle handle, struct ExecutorJob *job);
uint32_t plannerFreeCapacity(const PlannerHandle handle);
bool plannerIsEmpty(const PlannerHandle handle);

// Implemented In C
void *portMallocAligned(size_t size, size_t align);
