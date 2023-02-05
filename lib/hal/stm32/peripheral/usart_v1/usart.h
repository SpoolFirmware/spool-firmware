#pragma once
#include <stddef.h>
#include <stdint.h>

#include "hal/hal.h"

#include "FreeRTOS.h"
#include "stream_buffer.h"

struct UARTDriver
{
    struct UARTConfig cfg;
    size_t deviceBase;
    uint32_t brr;
    StreamBufferHandle_t txBuffer;
    StreamBufferHandle_t rxBuffer;
};

void halUartInit(struct UARTDriver *pDriver, const struct UARTConfig *pCfg,
                 size_t deviceBase, uint32_t clkSpeed);
void halUartIrqHandler(struct UARTDriver *pDriver);
