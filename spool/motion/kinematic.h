#pragma once

#include <stdint.h>
#include "fix16.h"
#include "motion/motion.h"
#include "configuration.h"

void planMove(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER]);

void planCoreXy(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER]);

void planI3(const int32_t movement[NR_AXIS], int32_t plan[NR_STEPPER]);