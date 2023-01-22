#include "platform/platform.h"
#include "hal/clock.h"
#include "hal/stm32/clock.h"

void platformInit(struct PlatformConfig *config)
{
    struct HalClockConfig halClockConfig = {
        .hseFreq = 8,
        .q = 7,
        .p = 2,
        .n = 168,
        .m = 4,

        .apb2Prescaler = 2,
        .apb1Prescaler = 4,
        .ahbPrescaler = 0,
    };

    halClockInit(&halClockConfig);
}
