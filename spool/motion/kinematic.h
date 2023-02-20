#pragma once

#include <stdint.h>
#include "fix16.h"
#include "motion/motion.h"
#include "core/magic_config.h"

void planMove(const int32_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
                fix16_t unitVec[X_AND_Y], fix16_t *len);

void planCoreXy(const int32_t movement[NR_AXES],
                         int32_t plan[NR_STEPPERS], fix16_t unit_vec[X_AND_Y],
                         fix16_t *len);

void planI3(const int32_t movement[NR_AXES], int32_t plan[NR_STEPPERS],
              fix16_t unit_vec[X_AND_Y], fix16_t *len);