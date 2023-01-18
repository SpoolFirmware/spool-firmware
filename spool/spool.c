//
// Created by codetector on 1/14/23.
//

#include "chip_hal/hal.h"
#include "chip_hal/stm32f401/gpio.h"
#include "spool_config.h"
#include <stdint.h>


void main()
{
    REG_WR32(DRF_REG(_RCC,_AHB1ENR), FLD_SET_DRF(_RCC,_AHB1ENR,_GPIOCEN, _ENABLED, REG_RD32(DRF_REG(_RCC,_AHB1ENR))));

    struct IOLine led = chipHalGpioLineConstruct(GPIOC, 13);
    chipHalGpioSetMode(led, DRF_DEF(_GPIO, _MODE, _MODE, _OUTPUT));
    chipHalGpioClear(led);
    // chipHalGpioSet();
    for(;;)
    {}
}
