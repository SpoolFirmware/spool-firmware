#pragma once
#include "hal/clock.h"

/*
 * Flash/Voltage depends on AHB frequency
 * Required: AHB runs at 84MHz
 */
struct HalClockConfig {
    uint32_t hseFreqHz;

    uint32_t q;
    uint32_t p;
    uint32_t n;
    uint32_t m;

    uint32_t apb2Prescaler;
    uint32_t apb1Prescaler;
    uint32_t ahbPrescaler;
};

uint32_t halClockSysclkFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockAhbFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb1TimerFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb2TimerFreqGet(const struct HalClockConfig *cfg);
