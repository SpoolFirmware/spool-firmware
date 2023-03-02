#include "st7920.h"
#include "misc.h"
#include "platform/platform.h"
#include "dbgprintf.h"
#include "compiler.h"

static inline void waitTimeoutUs(uint32_t us)
{
    uint64_t expiration = platformGetTimeUs() + (uint64_t)us;
    while (platformGetTimeUs() < expiration)
        ;
}

/**
 * @brief convert horizontal pixel to tile index
 * st7920 addresses every other byte for horizontal pixels
 * IOW each tile has 16 bits
 * @param x horizontal coord in pixels
 * @return uint8_t tile index
 */
static inline uint8_t st7920XToTile(uint8_t x)
{
    return x >> 4;
}

static void st7920SendByteRaw(const struct St7920 *drv, uint8_t b)
{
    uint8_t spiCmd[] = {
        b & 0b11110000, /* upper 4 bits */
        b << 4, /* lower 4 bits */
    };
    spiSendCmd(&drv->spi, spiCmd, ARRAY_SIZE(spiCmd));
}

static void st7920SendByte(const struct St7920 *drv, enum St7920RS rs,
                           uint8_t b)
{
    uint8_t spiCmd[] = {
        0b11111000 |
            (rs << 1), /* sync 5 bits, then RW (does not apply), RS, zero */
        b & 0b11110000, /* upper 4 bits */
        b << 4, /* lower 4 bits */
    };
    spiSendCmd(&drv->spi, spiCmd, ARRAY_SIZE(spiCmd));
}

// static void st7920DisplayClear(const struct St7920 *drv)
// {
//     for (int y = 0; y < 64; ++y) {
//         st7920SendByte(drv, St7920InstructionWrite, 0x80 | y);
//         st7920SendByte(drv, St7920InstructionWrite, 0x80);
//         uint8_t dataWrite = 0xFA;
//         spiSendCmd(&drv->spi, &dataWrite, 1);
//         for (int x = 0; x < 8; ++x) {
//             dataWrite = 0x10;
//             spiSendCmd(&drv->spi, &dataWrite, 0);
//             spiSendCmd(&drv->spi, &dataWrite, 0);
//             spiSendCmd(&drv->spi, &dataWrite, 0);
//             spiSendCmd(&drv->spi, &dataWrite, 0);
//         }
//     }
// }

void st7920WriteTile(const struct St7920 *drv, uint8_t x0, uint8_t x1, uint8_t y,
                     /* row addr */
                     const uint8_t *buf)
{
    uint8_t tileStart = st7920XToTile(x0);
    uint8_t tileEnd = st7920XToTile(x1);
    st7920SendByte(drv, St7920InstructionWrite, 0x80 | y);
    st7920SendByte(drv, St7920InstructionWrite, 0x80 | tileStart);
    uint8_t dataWrite = 0xFA;
    spiSendCmd(&drv->spi, &dataWrite, 1);
    for (int i = tileStart; i <= tileEnd; ++i) {
        /* each tile is two bytes*/
        uint8_t byteIdx = i << 1;
        st7920SendByteRaw(drv, buf[byteIdx]);
        st7920SendByteRaw(drv, buf[byteIdx + 1]);
    }
}

void st7920Init(struct St7920 *drv, const struct SpiSw *spi)
{
    if (drv->state == St7920Initialized) {
        PR_WARN("st7920 already initialized\n");
    }

    drv->spi = *spi;

    waitTimeoutUs(40000);

    /* function set 8 bit, with 0x8 for some reason */
    st7920SendByte(drv, St7920InstructionWrite, 0x38);
    waitTimeoutUs(100);

    /* display control off */
    st7920SendByte(drv, St7920InstructionWrite, 0x08);
    waitTimeoutUs(100);

    /* entry mode */
    st7920SendByte(drv, St7920InstructionWrite, 0x06);
    waitTimeoutUs(100);

    /* return home */
    st7920SendByte(drv, St7920InstructionWrite, 0x02);
    waitTimeoutUs(100);

    /* clear screen*/
    st7920SendByte(drv, St7920InstructionWrite, 0x01);
    waitTimeoutUs(10000);

    /* function set 8 bit with extended graphics */
    st7920SendByte(drv, St7920InstructionWrite, 0x3E);
    waitTimeoutUs(10000);

    /* function set 8 bit with extended graphics, again because one bit can be set at a time */
    st7920SendByte(drv, St7920InstructionWrite, 0x3E);
    waitTimeoutUs(10000);

    /* reset graphics ver to 0 */
    st7920SendByte(drv, St7920InstructionWrite, 0x80);
    waitTimeoutUs(100);

    /* reset graphics hor to 0 */
    st7920SendByte(drv, St7920InstructionWrite, 0x80);
    waitTimeoutUs(100);

    // /* clear screen */
    // st7920DisplayClear(drv);

    /* display control on */
    st7920SendByte(drv, St7920InstructionWrite, 0x0C);
    waitTimeoutUs(100);

    drv->state = St7920Initialized;
}