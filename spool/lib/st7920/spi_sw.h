#pragma once
#include "hal/hal.h"

struct SpiSw {
    struct IOLine sclk;
    struct IOLine cs;
    struct IOLine mosi;
};

/* gpio pins need to be output */
void spiInit(struct SpiSw *spi, struct IOLine sclk, struct IOLine cs,
             struct IOLine mosi);

void spiSendCmd(const struct SpiSw *spi, uint8_t *cmd, uint8_t size);

void spiSendByte(const struct SpiSw *spi, uint8_t byte);
