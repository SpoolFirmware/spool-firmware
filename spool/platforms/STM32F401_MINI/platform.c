#include "platform/platform.h"
#include "chip_hal/clock.h"
#include "chip_hal/stm32f401/clock.h"

void platformInit(struct PlatformConfig *config)
{
    struct ChipHalClockConfig chipHalClockConfig = {
        .hseFreq = 25,
        .q = 7,
        .p = 4,
        .n = 336,
        .m = 25,

        .apb2Prescaler = 0,
        .apb1Prescaler = 2,
        .ahbPrescaler = 0,
    };

    chipHalClockInit(&chipHalClockConfig);
}
