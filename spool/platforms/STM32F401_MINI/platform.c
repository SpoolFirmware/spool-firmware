#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/stm32/hal.h"

const static struct IOLine statusLED = {.group = GPIOC, .pin = 13};

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

    uint32_t rcc = REG_RD32(DRF_REG(_RCC,_AHB1ENR));
    rcc = FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOCEN, _ENABLED, rcc);
    REG_WR32(DRF_REG(_RCC,_AHB1ENR), rcc);

    halGpioSetMode(statusLED, 
        DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT)
        | DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL)
        | DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH)
        );
}

void stepX(void)
{
}

void platformMotorStep(uint16_t motor_mask)
{
}

__attribute__((always_inline))
inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}
