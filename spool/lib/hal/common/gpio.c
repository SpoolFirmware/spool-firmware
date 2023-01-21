#include "gpio.h"

__attribute__((weak))
struct IOLine halGpioLineConstruct(uint32_t group, uint32_t pin)
{    
    struct IOLine line = {group, pin};
    return line;
}
