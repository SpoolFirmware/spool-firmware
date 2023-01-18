//
// Created by codetector on 1/14/23.
//

#include "chip_hal/hal.h"
#include "chip_hal/stm32f401/gpio.h"
#include "spool_config.h"
#include <stdint.h>

void main()
{
    chipHalClockInit();

    REG_WR32(DRF_REG(_RCC,_AHB1ENR), FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOCEN, _ENABLED, REG_RD32(DRF_REG(_RCC,_AHB1ENR))));

    struct IOLine led = chipHalGpioLineConstruct(GPIOC, 13);
    chipHalGpioSetMode(led, 
        DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
        DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH)
    );
    for(;;)
    {
        chipHalGpioClear(led);
        chipHalGpioSet(led);
    }
}
