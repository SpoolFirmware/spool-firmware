#pragma once
#include <stdint.h>

/*
 * Flash/Voltage depends on AHB frequency
 * Required: AHB runs at 84MHz
 */
struct HalClockConfig {
    uint32_t hseFreqHz;

    uint8_t pllXtPre; //! 1 or 2
    uint8_t usbPre1_5X; // 0 -> 1, 1 -> 1.5
    uint16_t pllMul;

    uint32_t ahbPrescaler;
    uint32_t apb1Prescaler;
    uint32_t apb2Prescaler;
    uint32_t adcPrescaler;
};

uint32_t halClockSysclkFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockAhbFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb1TimerFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb1FreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb2TimerFreqGet(const struct HalClockConfig *cfg);
uint32_t halClockApb2FreqGet(const struct HalClockConfig *cfg);
