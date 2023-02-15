#pragma once
#include "fix16.h"

#define NR_STEPPERS 4

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)
#define STEPPER_C BIT(2)

#define STEPPER_A_IDX 0
#define STEPPER_B_IDX 1
#define STEPPER_C_IDX 2

#define STEPS_PER_MM   80
#define STEPS_PER_MM_Z 800

#define STEPS_PER_MM_E 195

#define VEL_Z 5
#define ACC_Z 50

#define ACC_E 1000
#define VEL_E 50

#define VEL  100
#define JERK 10
#define ACC  1000

#define HOMING_VEL   50
#define HOMING_VEL_Z 5

#define MAX_X 160
#define MAX_Y 160
#define MAX_Z 120

#define ENDSTOP_X 0
#define ENDSTOP_Y 1
#define ENDSTOP_Z 2
#define X_AXIS    0
#define Y_AXIS    1
#define Z_AXIS    2
#define E_AXIS    3

#define NR_AXES   4

const static uint32_t V_MAX_STEPS = VEL * STEPS_PER_MM;
const static uint32_t ACCEL_STEPS = ACC * STEPS_PER_MM;
