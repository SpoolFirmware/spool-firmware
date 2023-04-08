#pragma once

#include "fix16.h"
#include "configuration.h"
#include "lib/planner/planner.h"

extern PlannerHandle PLANNER;

#define X_AND_Y 2

#define for_each_axis(iter) for (uint8_t iter = 0; iter < NR_AXIS; iter++)
#define for_each_stepper(iter) \
    for (uint8_t iter = 0; iter < NR_STEPPER; iter++)

void motionInit(void);
fix16_t vecUnit(const fix16_t vec[NR_AXIS], fix16_t unit_vec[NR_AXIS]);

fix16_t motionGetMaxVelocityMM(uint8_t stepper);
void motionSetMaxVelocityMM(uint8_t stepper, fix16_t maxVel);
fix16_t motionGetDefaultAccelerationMM(uint8_t stepper);
fix16_t motionGetHomingVelocityMM(uint8_t stepper);
fix16_t motionGetHomingAccelerationMM(uint8_t stepper);
fix16_t motionGetMinVelocityMM(uint8_t stepper);

#define PLATFORM_MOTION_STEPS_PER_MM(STEPPER) PLATFORM_MOTION_STEPS_PER_MM_##STEPPER
#define PLATFORM_MOTION_STEPS_PER_MM_AXIS(AXIS) PLATFORM_MOTION_STEPS_PER_MM_##AXIS
#define PLATFORM_MOTION_LIMIT(AXIS) PLATFORM_MOTION_LIMIT_##AXIS
