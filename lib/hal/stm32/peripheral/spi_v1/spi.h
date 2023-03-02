#pragma once

#include "hal/spi.h"
#include "FreeRTOS.h"
#include "semphr.h"

struct SPIDevice {
    const uint32_t base;
    struct {
        StaticSemaphore_t smphr;
        SemaphoreHandle_t smphrHandle;

        const void *txBuffer;
        void *rxBuffer;
        uint32_t rxCnt, txCnt;
    } inner;
};

void halSpiLLServiceInterrupt(struct SPIDevice *pDev);
