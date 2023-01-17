#include "drf/drf.h"
#include "chip_hal/gpio.h"

__attribute__((always_inline))
struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin)
{
    struct IOLine line = {group, pin};
    return line;
}

__attribute__((always_inline))
void chipHalGpioSet(struct IOLine line)
{

}

__attribute__((always_inline))
void chipHalGpioClear(struct IOLine line)
{

}