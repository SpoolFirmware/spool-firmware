#pragma once
#include "hal/hal.h"

struct SpiSw {
    struct IOLine sclk;
    struct IOLine cs;
    struct IOLine mosi;
};

/* gpio pins need to be output */
void swSpiInit(struct SpiSw *spi, struct IOLine sclk, struct IOLine cs,
             struct IOLine mosi);

void swSpiSendCmd(const struct SpiSw *spi, uint8_t *cmd, uint8_t size);

void swSpiSendByte(const struct SpiSw *spi, uint8_t byte);
