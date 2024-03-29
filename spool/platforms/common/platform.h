#pragma once

#include "hal/hal.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "queue.h"
#include "fix16.h"
#include "spool/lib/planner/planner.h"

struct PlatformConfig {
    uint8_t _rsvd;
};
/* ------------------- Types to be implemented by platform ------------------ */
enum Axis;
enum Stepper;
/* ----------------- Configuration Types ------------------------------------ */
struct XYPositionMM {
    int32_t x_mm;
    int32_t y_mm;
};

enum DisplayKind {
    DisplayKindNone = 0,
    DisplayKindSt7920,
};

struct PIDConifg {};

/* ----------------- Variables to be implemented by platform ---------------- */
extern const enum DisplayKind platformFeatureDisplay;
extern const bool platformFeatureBedLeveling;
extern const enum KinematicKind platformFeatureKinematic;

extern const float platformFeatureZOffset;
extern const struct XYPositionMM platformFeatureZHomingPos;
extern const enum Stepper platformFeatureExtruderStepper;

extern const struct XYPositionMM platformBedLevelingCorners[2];

/* Motion, in mm, mm/s, mm/s^2 */
extern const int32_t platformMotionLimits[];
extern const bool platformMotionInvertStepper[];
extern const int32_t platformMotionDefaultMaxVel[];
extern const int32_t platformMotionMaxAccAxis[];
extern const int32_t platformMotionDefaultAcc[];
extern const int32_t platformMotionHomingVel[];
extern const int32_t platformMotionHomingAcc[];

extern const fix16_t platformJunctionDeviation; 

/* Steps, TODO per stepper/per-axis steps, it doesn't make sense */
extern const int32_t platformMotionStepsPerMM[];
extern const int32_t platformMotionStepsPerMMAxis[];
extern const fix16_t platformMotionMinVelMM[];

extern const struct IOLine sdCSPin;
/* ---------------- Functions to be implemented by platform ----------------- */
/**
 * @brief Initialization Function, called before all
 *
 * @param config
 */
void platformInit(struct PlatformConfig *config);
void platformPostInit(void);
struct IOLine platformGetStatusLED(void);

/**
 * @brief Returns a pointer to the SPIDevice driver object
 * 
 * @return struct SPIDevice* 
 */
struct SPIDevice *platformGetSDSPI(void);

// Timing
/**
 * @return Number of us since reset
 */
uint64_t platformGetTimeUs(void);
uint64_t platformGetTimeUsIrqsafe(void);

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
#define PLATFORM_FAN_IDX_HOTEND (-1)
void platformSetFan(int8_t idx, uint8_t pwm);

// Communication
size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait);
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
