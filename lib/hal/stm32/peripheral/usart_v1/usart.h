#pragma once
#include <stddef.h>
#include <stdint.h>

#include "hal/hal.h"

struct UARTDriver
{
    size_t deviceBase;
    uint32_t brr;
    struct UARTConfig cfg;
};

void halUartInit(struct UARTDriver *pDriver, const struct UARTConfig *pCfg,
                 size_t deviceBase, uint32_t clkSpeed);
