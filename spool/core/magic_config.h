#pragma once
#include "fix16.h"

#define NR_STEPPERS 2

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)

#define MICROSTEPPING 32
#define STEPS_PER_REV 200
#define MM_PER_ROT 31
#define MM_TO_STEP(x) ((x) * MICROSTEPPING * STEPS_PER_ROT / MM_PER_ROT)
