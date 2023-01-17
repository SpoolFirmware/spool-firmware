//
// Created by codetector on 1/14/23.
//

#include "chip_hal/hal.h"
#include "chip_hal/stm32f401/gpio.h"
#include "spool_config.h"
#include <stdint.h>


void main()
{
    struct IOLine led = chipHalGpioLineConstruct(GPIOA, 1);
    chipHalGpioSet(led);
    // chipHalGpioSet();
    for(;;)
    {}
}
