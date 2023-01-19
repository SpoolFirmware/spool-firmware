#include "gpio.h"

__attribute__((weak))
struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin)
{    
    struct IOLine line = {group, pin};
    return line;
}
