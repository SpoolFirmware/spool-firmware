#include "platform/platform.h"
#include "hal/clock.h"
#include "hal/stm32f401/clock.h"

void platformInit(struct PlatformConfig *config)
{
    struct HalClockConfig halClockConfig = {
        .hseFreq = 25,
        .q = 7,
        .p = 4,
        .n = 336,
        .m = 25,

        .apb2Prescaler = 0,
        .apb1Prescaler = 2,
        .ahbPrescaler = 0,
    };

    halClockInit(&halClockConfig);
}
