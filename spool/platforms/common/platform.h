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
/* ------------------- Types to be implemented by platform ------------------ */
enum Axis;
enum Stepper;
/* ----------------- Configuration Types ------------------------------------ */
enum KinematicKind {
    KinematicKindUndef = 0,
    KinematicKindI3, /* Uses STEPPER_A,B,C for X,Y,Z */
    KinematicKindCoreXY, /* Uses STEPPER_A,B for corexy, C for Z */
};

struct XYPositionMM {
    int32_t x_mm;
    int32_t y_mm;
};

struct PIDConifg {

};

/* ----------------- Variables to be implemented by platform ---------------- */
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
extern const int32_t platformMotionDefaultAcc[];
extern const int32_t platformMotionHomingVel[];
extern const int32_t platformMotionHomingAcc[];

/* Steps, TODO per stepper/per-axis steps, it doesn't make sense */
extern const int32_t platformMotionStepsPerMM[];
extern const int32_t platformMotionStepsPerMMAxis[];
extern const int32_t platformMotionMinVel[];
/* ---------------- Functions to be implemented by platform ----------------- */
/**
 * @brief Initialization Function, called before all
 * 
 * @param config 
 */
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
#define PLATFORM_FAN_IDX_HOTEND     (-1)
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
