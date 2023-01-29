#pragma once
#include "fix16.h"

#define NR_STEPPERS 2

#define STEPPER_A     BIT(0)
#define STEPPER_B     BIT(1)
#define STEPPER_A_IDX 0
#define STEPPER_B_IDX 1

const static uint32_t STEPS_PER_MM = 160;

#define VEL  100
#define JERK 10
#define ACC  1000

#define HOMING_VEL 5

#define MAX_X 200
#define MAX_Y 200

#define ENDSTOP_X 0
#define ENDSTOP_Y 1
#define NR_AXES 2

const static uint32_t V_MAX_STEPS = VEL * STEPS_PER_MM;
const static uint32_t ACCEL_STEPS = ACC * STEPS_PER_MM;
