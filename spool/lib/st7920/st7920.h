#pragma once
#include "hal/hal.h"
#include "spi_sw.h"

enum St7920State {
    St7920Undef,
    St7920Initialized,
};

enum St7920RS {
    St7920InstructionWrite = 0,
    St7920DataWrite = 1,
};

struct St7920 {
    struct SpiSw spi;
    enum St7920State state;
};

void st7920Init(struct St7920 *drv, const struct SpiSw *spi);
/* in pixels, not st7920 addresses */

/**
 * @brief writes the necessary part of the row
 * 
 * @param drv st7920
 * @param x0 start pixel
 * @param x1 end pixel (incl)
 * @param y row
 * @param buf row buffer
 */
void st7920WriteTile(struct St7920 *drv, uint8_t x0, uint8_t x1, uint8_t y,
                const uint8_t *buf);