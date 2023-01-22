#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/stm32/hal.h"

const static struct IOLine statusLED = { .group = GPIOA, .pin = 7 };
const static struct IOLine xStep = { .group = GPIOE, .pin = 9 };
const static struct IOLine xDir = { .group = GPIOF, .pin = 1 };
const static struct IOLine xEn = { .group = GPIOF, .pin = 2 };

void spinMotor(void)
{
    halGpioClear(xEn);
    halGpioClear(xDir);
}

void stepX(void)
{
    halGpioSet(xStep);
    asm(".rept 400 ; nop ; .endr");
    halGpioClear(xStep);
}

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

    uint32_t rcc = REG_RD32(DRF_REG(_RCC, _AHB1ENR));
    rcc = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOAEN, _ENABLED, rcc);
    rcc = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOFEN, _ENABLED, rcc);
    rcc = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOEEN, _ENABLED, rcc);
    REG_WR32(DRF_REG(_RCC, _AHB1ENR), rcc);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    halGpioSetMode(xStep, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                              DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                              DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    halGpioSetMode(xDir, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                             DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                             DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    halGpioSetMode(xEn, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                            DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                            DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));

    spinMotor();
}

__attribute__((always_inline)) struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}
