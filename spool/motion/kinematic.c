#include "misc.h"
#include "platform/platform.h"
#include "core/spool.h"
#include "motion/kinematic.h"

/* --------------- Private Function Declarations ----------------------- */
/* ------------ Public Function Definitions ---------------------------- */
void planMove(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER])
{
    switch (PLATFORM_FEATURE_ENABLED(Kinematic)) {
    case KinematicKindCoreXY:
        planCoreXy(movement, plan);
        break;
    case KinematicKindI3:
        planI3(movement, plan);
        break;
    default:
        panic();
        break;
    }
}

/* ------------ Private Function Definitions ------------------- */
void planI3(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER])
{
    for_each_stepper(i) {
        plan[i] = movement[i];
    }

    // *len = vecUnit(movement_mm, unit_vec);
}

void planCoreXy(const int32_t movementSteps[NR_AXIS], int32_t plan[NR_STEPPER])
{
    _Static_assert(NR_AXIS >= X_AND_Y,
                   "number of axes insufficient for corexy");

    int32_t aX = movementSteps[X_AXIS] + movementSteps[Y_AXIS];
    int32_t bX = movementSteps[X_AXIS] - movementSteps[Y_AXIS];
    for_each_stepper(i) {
        if (i < X_AND_Y)
            continue;
        plan[i] = movementSteps[i];
    }
    plan[STEPPER_A] = aX;
    plan[STEPPER_B] = bX;

    // *len = vecUnit(movement_mm, unit_vec);
}
