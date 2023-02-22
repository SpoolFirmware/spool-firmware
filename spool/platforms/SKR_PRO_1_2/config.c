#include "platform/platform.h"
#include "configuration.h"
#include "compiler.h"
#include "error.h"

const bool platformFeatureBedLeveling = true;
const enum KinematicKind platformFeatureKinematic = KinematicKindCoreXY;

const float platformFeatureZOffset = 0.9;
const struct ZHomingPos platformFeatureZHomingPos = {
    .x_mm = 30,
    .y_mm = 30,
};

const int32_t platformMotionLimits[] = {
    PLATFORM_MOTION_LIMIT_X_AXIS,
    PLATFORM_MOTION_LIMIT_Y_AXIS,
    PLATFORM_MOTION_LIMIT_Z_AXIS,
    0,
};

const bool platformMotionInvertStepper[] = {
    false,
    false,
    false,
    false,
};

const int32_t platformMotionDefaultMaxVel[] = {
    120,
    120,
    5,
    45,
};
const int32_t platformMotionDefaultAcc[] = {
    1000,
    1000,
    50,
    1000,
};
const int32_t platformMotionHomingVel[] = {
    50,
    50,
    3,
    0,
};
const int32_t platformMotionHomingAcc[] = {
    1000,
    1000,
    500,
    0,
};
const int32_t platformMotionStepsPerMM[] = {
    PLATFORM_MOTION_STEPS_PER_MM_STEPPER_A,
    PLATFORM_MOTION_STEPS_PER_MM_STEPPER_B,
    PLATFORM_MOTION_STEPS_PER_MM_STEPPER_C,
    PLATFORM_MOTION_STEPS_PER_MM_STEPPER_D,
};
const int32_t platformMotionStepsPerMMAxis[] = {
    PLATFORM_MOTION_STEPS_PER_MM_X_AXIS,
    PLATFORM_MOTION_STEPS_PER_MM_Y_AXIS,
    PLATFORM_MOTION_STEPS_PER_MM_Z_AXIS,
    PLATFORM_MOTION_STEPS_PER_MM_E_AXIS,
};
const int32_t platformMotionMinVel[] = {
    0.25 * PLATFORM_MOTION_STEPS_PER_MM_STEPPER_A,
    0.25 * PLATFORM_MOTION_STEPS_PER_MM_STEPPER_B,
    0.25 * PLATFORM_MOTION_STEPS_PER_MM_STEPPER_C,
    0.25 * PLATFORM_MOTION_STEPS_PER_MM_STEPPER_D,
};

const enum Stepper platformFeatureExtruderStepper = STEPPER_D;

/* Sanity Checks */
STATIC_ASSERT(ARRAY_SIZE(platformMotionInvertStepper) == NR_AXIS);
STATIC_ASSERT(ARRAY_SIZE(platformMotionLimits) == NR_AXIS);
STATIC_ASSERT(ARRAY_SIZE(platformMotionDefaultMaxVel) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionDefaultAcc) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionHomingVel) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionHomingAcc) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionMinVel) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionStepsPerMM) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionStepsPerMMAxis) == NR_AXIS);
