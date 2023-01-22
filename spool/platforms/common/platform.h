#pragma once

#include "hal/hal.h"

struct PlatformConfig;

void platformInit(struct PlatformConfig *config);
struct IOLine platformGetStatusLED(void);

void stepX(void);
void platformMotorStep(uint16_t motor_mask);
