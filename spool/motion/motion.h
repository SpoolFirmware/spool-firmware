#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "fix16.h"
#include "configuration.h"

#define X_AND_Y 2

#define for_each_axis(iter) for (uint8_t iter = 0; iter < NR_AXIS; iter++)
#define for_each_stepper(iter) \
    for (uint8_t iter = 0; iter < NR_STEPPER; iter++)

void motionInit(void);
fix16_t vecUnit(const float a[NR_AXIS], fix16_t out[NR_AXIS]);

int32_t motionGetMaxVelocity(uint8_t stepper);
void motionSetMaxVelocity(uint8_t stepper, int32_t maxVel);
int32_t motionGetDefaultAcceleration(uint8_t stepper);
int32_t motionGetHomingVelocity(uint8_t stepper);
int32_t motionGetHomingAcceleration(uint8_t stepper);
int32_t motionGetMinVelocity(uint8_t stepper);

#define PLATFORM_MOTION_STEPS_PER_MM(STEPPER) PLATFORM_MOTION_STEPS_PER_MM_##STEPPER
#define PLATFORM_MOTION_STEPS_PER_MM_AXIS(AXIS) PLATFORM_MOTION_STEPS_PER_MM_##AXIS
#define PLATFORM_MOTION_LIMIT(AXIS) PLATFORM_MOTION_LIMIT_##AXIS
