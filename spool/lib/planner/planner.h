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
    fix16_t acc_v[MAX_AXIS];
    bool stop;
};

struct MoveSteps {
    int32_t delta_x_steps[MAX_STEPPERS];
    uint32_t max_axis;
    uint32_t accelerate_steps;
    uint32_t decelerate_steps;
    uint32_t accelerate_stepss2;
};

typedef struct Planner *PlannerHandle;

// Implemented In Rust
PlannerHandle plannerInit(PlannerHandle handle, uint32_t numAxis, uint32_t numStepper);

bool plannerEnqueue(PlannerHandle handle, const struct PlannerMove *plannerMove);

bool plannerDequeue(PlannerHandle handle, struct MoveSteps *plannerMove);

uint32_t plannerFreeCapacity(const PlannerHandle handle);

// Implemented In C
void *portMallocAligned(size_t size, size_t align);
