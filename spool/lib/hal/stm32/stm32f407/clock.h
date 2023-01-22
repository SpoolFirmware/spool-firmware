#include <hal/clock.h>

/*
 * Flash/Voltage depends on AHB frequency
 * Required: AHB runs at 168MHz
 */
struct HalClockConfig {
    uint32_mhz_t hseFreq;

    uint32_t q;
    uint32_t p;
    uint32_t n;
    uint32_t m;

    uint32_t apb2Prescaler;
    uint32_t apb1Prescaler;
    uint32_t ahbPrescaler;
};
