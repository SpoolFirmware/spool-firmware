#include "drf/drf.h"
#include "chip_hal/gpio.h"
#include "manual/stm32f401.h"

struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin)
{
    struct IOLine line = {group};
    return line;
}

void chipHalGpioSet(struct IOLine line)
{
    (*(volatile uint32_t*)(line.group)) = 1;
}

void chipHalGpioClear(struct IOLine line)
{
    (*(volatile uint32_t*)line.group) = 1;
}
