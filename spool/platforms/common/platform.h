#pragma once

#include "hal/hal.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "queue.h"

struct PlatformConfig;

void platformInit(struct PlatformConfig *config);
void platformPostInit(void);
struct IOLine platformGetStatusLED(void);

void enableStepper(uint8_t stepperMask);
void stepStepper(uint8_t stepperMask);
void disableStepper(uint8_t stepperMask);

void setStepperDir(uint8_t dirMask);
void executeStep(void);
uint32_t getStepperTimerFreq(void);
