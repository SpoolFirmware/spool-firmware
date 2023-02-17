#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "fix16.h"

typedef struct {
    // Target State
    fix16_t setPoint;
    // Configuration State
    fix16_t kp;
    fix16_t ki;
    fix16_t kd;

    fix16_t outputMin;
    fix16_t outputMax;
    fix16_t effectiveRange;

    ///
    /// For SetPoint -> Output Calculation
    /// Out = (setPoint + b) * m
    ///
    fix16_t m;
    fix16_t b;

    // Internal State
    fix16_t maxOverKi;
    fix16_t iState;

    fix16_t inputHistory[20];
    uint8_t head;
} pid_t;

void pidInit(pid_t *pPid);
void pidReset(pid_t *pPid);
bool pidStable(pid_t *pPid);
fix16_t pidUpdateLoop(pid_t *pPid, fix16_t input);
