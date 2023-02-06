#pragma once

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

struct TimerConfig {
    uint32_t timerTargetFrequency;
    uint32_t clkDomainFrequency;
    bool interruptEnable;
};

struct TimerDriver;

void halTimerConstruct(struct TimerDriver *pDriver, size_t deviceBase);
void halTimerStart(struct TimerDriver *pDriver, const struct TimerConfig *pConfig);
void halTimerStop(struct TimerDriver *pDriver);
bool halTimerPending(const struct TimerDriver *pDriver);
void halTimerIrqClear(const struct TimerDriver *pDriver);

void halTimerStartContinous(struct TimerDriver *pDriver, uint32_t reloadValue);
void halTimerChangeReloadValue(struct TimerDriver *pDriver, uint32_t reloadValue);
void halTimerStartOneShot(struct TimerDriver *pDriver, uint32_t reloadValue);
