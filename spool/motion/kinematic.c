#include "misc.h"
#include "platform/platform.h"
#include "core/spool.h"
#include "motion/kinematic.h"

/* --------------- Private Function Declarations ----------------------- */
/* ------------ Public Function Definitions ---------------------------- */
void planMove(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER],
              fix16_t unitVec[X_AND_Y], fix16_t *len)
{
    switch (PLATFORM_FEATURE_ENABLED(Kinematic)) {
    case KinematicKindCoreXY:
        planCoreXy(movement, plan, unitVec, len);
        break;
    case KinematicKindI3:
        planI3(movement, plan, unitVec, len);
        break;
    default:
        panic();
        break;
    }
}

/* ------------ Private Function Definitions ------------------- */
void planI3(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER],
            fix16_t unit_vec[X_AND_Y], fix16_t *len)
{
    for_each_stepper(i) {
        plan[i] = movement[i];
    }

    *len = vecUnit(movement, unit_vec);
}

void planCoreXy(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER],
                fix16_t unit_vec[X_AND_Y], fix16_t *len)
{
    _Static_assert(NR_AXIS >= X_AND_Y,
                   "number of axes insufficient for corexy");

    int32_t aX = movement[X_AXIS] + movement[Y_AXIS];
    int32_t bX = movement[X_AXIS] - movement[Y_AXIS];
    for_each_stepper(i) {
        if (i < X_AND_Y)
            continue;
        plan[i] = movement[i];
    }
    plan[STEPPER_A] = aX;
    plan[STEPPER_B] = bX;

    *len = vecUnit(movement, unit_vec);
}
