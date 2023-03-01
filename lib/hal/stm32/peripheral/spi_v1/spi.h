#pragma once

#include "hal/spi.h"
#include "FreeRTOS.h"
#include "semphr.h"

struct SPIDevice {
    StaticSemaphore_t smphr;
    SemaphoreHandle_t smphrHandle;
    uint32_t base;

    const void* txBuffer;
    void* rxBuffer;
    uint32_t rxCnt, txCnt;
};


void halSpiLLServiceInterrupt(struct SPIDevice *pDev);
