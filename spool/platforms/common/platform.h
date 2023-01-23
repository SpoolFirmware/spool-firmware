#pragma once

#include "hal/hal.h"

struct PlatformConfig;

void platformInit(struct PlatformConfig *config);
struct IOLine platformGetStatusLED(void);

void stepX(void);
void platformMotorStep(uint8_t motor_mask, uint8_t dir_mask);
