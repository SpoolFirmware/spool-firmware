#pragma once

#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "fix16.h"

struct PlatformConfig {
    uint8_t _rsvd;
};
/* ----------------- Variables to be implemented by platform ---------------- */

/* ---------------- Functions to be implemented by platform ----------------- */
void platformInit(struct PlatformConfig *config);
void platformPostInit(void);
struct IOLine platformGetStatusLED(void);

// Steppers
void platformEnableStepper(uint8_t stepperMask);
void platformDisableStepper(uint8_t stepperMask);
void platformStepStepper(uint8_t stepperMask);
void platformSetStepperDir(uint8_t dirMask);
const uint32_t platformGetStepperTimerFreq(void);

// Sensors
bool platformGetEndstop(uint8_t axis);
// -1 is BED
fix16_t platformReadTemp(int8_t idx);

// -1 is BED
void platformSetHeater(int8_t idx, uint8_t pwm);

// -1 is Hotend Fan
void platformSetFan(int8_t idx, uint8_t pwm);

// Communication
size_t platformRecvCommand(char *pBuffer, size_t bufferSize, TickType_t ticksToWait);
void platformSendResponse(const char *pBuffer, size_t len);

// DEBUGGING
void platformDbgPutc(char c);

/* ---------------------- Functions platform needs to call ------------------ */
/*!
 * @param ticksElapsed  number of ticksElapsed since last invocation.
 *
 * @note    This function should be invoked by the platform in a timer ISR that
 *          runs at @ref platformGetStepperTimerFreq.
 *
 * @returns number of desired ticks till the next step
 */
uint16_t executeStep(uint16_t ticksElapsed);
