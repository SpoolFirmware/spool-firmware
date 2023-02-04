#pragma once

#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"

struct PlatformConfig {
    uint8_t _rsvd;
};
/* ----------------- Variables to be implemented by platform ---------------- */

/* ---------------- Functions to be implemented by platform ----------------- */
void platformInit(struct PlatformConfig *config);
void platformPostInit(void);
struct IOLine platformGetStatusLED(void);

void enableStepper(uint8_t stepperMask);
void stepStepper(uint8_t stepperMask);
void disableStepper(uint8_t stepperMask);

void setStepperDir(uint8_t dirMask);

bool platformGetEndstop(uint8_t axis);

/*!
 * @param ticksElapsed  number of ticksElapsed since last invocation.
 * @returns number of desired ticks till the next step
 */
uint16_t executeStep(uint16_t ticksElapsed);
uint32_t getStepperTimerFreq(void);

size_t platformRecvCommand(char *pBuffer, size_t bufferSize, TickType_t ticksToWait);
void platformSendResponse(const char *pBuffer, size_t len);

void platformDbgPutc(char c);
