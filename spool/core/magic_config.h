#pragma once
#include "fix16.h"

#define NR_STEPPERS 2

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)
#define STEPPER_A_IDX 0
#define STEPPER_B_IDX 1

const static uint32_t STEPS_PER_MM = 80;

#define VEL     100
#define JERK    10
#define ACC     300

const static uint32_t V_MAX_STEPS = VEL * STEPS_PER_MM;
const static uint32_t ACCEL_STEPS = ACC * STEPS_PER_MM;
