#include "platform/platform.h"
#include "configuration.h"
#include "compiler.h"
#include "error.h"

const enum DisplayKind platformFeatureDisplay = DisplayKindSt7920;
const bool platformFeatureBedLeveling = true;
const enum KinematicKind platformFeatureKinematic = KinematicKindCoreXY;

const fix16_t platformJunctionDeviation = F16(0.015);
const float platformFeatureZOffset = 1.0;
const struct XYPositionMM platformFeatureZHomingPos = {
    .x_mm = 30,
    .y_mm = 30,
};

const struct XYPositionMM platformBedLevelingCorners[2] = {
    {.x_mm = 5, .y_mm = 10},
    {.x_mm = 135, .y_mm = 160},
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
    true,
};

const int32_t platformMotionDefaultMaxVel[] = {
    200,
    200,
    10,
    35,
};

const int32_t platformMotionMaxAccAxis[] = {
    1500,
    1500,
    300,
    1000,
};

const int32_t platformMotionDefaultAcc[] = {
    1500,
    1500,
    300,
    1000,
};
const int32_t platformMotionHomingVel[] = {
    50,
    50,
    8,
    0,
};
const int32_t platformMotionHomingAcc[] = {
    800,
    800,
    300,
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
const fix16_t platformMotionMinVelMM[] = {
    F16(0.1),
    F16(0.1),
    F16(0.1),
    F16(0.1),
};

const enum Stepper platformFeatureExtruderStepper = STEPPER_D;

/* Sanity Checks */
STATIC_ASSERT(ARRAY_SIZE(platformMotionInvertStepper) == NR_AXIS);
STATIC_ASSERT(ARRAY_SIZE(platformMotionLimits) == NR_AXIS);
STATIC_ASSERT(ARRAY_SIZE(platformMotionDefaultMaxVel) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionMaxAccAxis) == NR_AXIS);
STATIC_ASSERT(ARRAY_SIZE(platformMotionDefaultAcc) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionHomingVel) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionHomingAcc) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionMinVelMM) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionStepsPerMM) == NR_STEPPER);
STATIC_ASSERT(ARRAY_SIZE(platformMotionStepsPerMMAxis) == NR_AXIS);
