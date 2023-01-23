#pragma once

#include "hal/hal.h"
#include <stdint.h>

struct PlatformConfig;

void platformInit(struct PlatformConfig *config);
struct IOLine platformGetStatusLED(void);

void stepX(void);
void platformMotorStep(uint8_t motor_mask, uint8_t dir_mask);
void enableStepper(uint8_t stepperMask);
void setStepper(uint8_t stepperMask, uint8_t dirMask, uint8_t stepMask);
void disableStepper(uint8_t stepperMask);
