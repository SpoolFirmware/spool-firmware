#include "spi_sw.h"
#include "platform/platform.h"

static inline void waitTimeoutUs(uint32_t us)
{
    uint64_t expiration = platformGetTimeUs() + (uint64_t)us;
    while (platformGetTimeUs() < expiration)
        ;
}

void spiInit(struct SpiSw *spi, struct IOLine sclk, struct IOLine cs,
             struct IOLine mosi)
{
    spi->sclk = sclk;
    spi->cs = cs;
    spi->mosi = mosi;
    halGpioSet(sclk);
    halGpioClear(cs);
    halGpioClear(mosi);
}

void spiSendByte(const struct SpiSw *spi, uint8_t byte)
{
    uint8_t mask = 0b10000000;
    for (uint8_t i = 0; i < 8; ++i, mask >>= 1) {
        halGpioClear(spi->sclk);
        waitTimeoutUs(0);
        if (byte & mask) {
            halGpioSet(spi->mosi);
        } else {
            halGpioClear(spi->mosi);
        }
        waitTimeoutUs(0);
        halGpioSet(spi->sclk);
        waitTimeoutUs(0);
    }
}

/* TODO decide if enter critical is good */
void spiSendCmd(const struct SpiSw *spi, uint8_t *cmd, uint8_t size)
{
    halGpioSet(spi->cs);
    waitTimeoutUs(0);
    for (uint8_t i = 0; i < size; ++i, ++cmd) {
        spiSendByte(spi, *cmd);
    }
    halGpioClear(spi->cs);
}