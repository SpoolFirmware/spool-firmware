#pragma once

#include "FreeRTOS.h"
#include "queue.h"
#include "fix16.h"
#include "core/magic_config.h"

#define X_AND_Y 2

#define for_each_axis(iter) for (uint8_t iter = 0; iter < NR_AXES; iter++)
#define for_each_stepper(iter) \
    for (uint8_t iter = 0; iter < NR_STEPPERS; iter++)


void motionInit(void);
fix16_t vecUnit(const int32_t a[NR_AXES], fix16_t out[X_AND_Y]);
