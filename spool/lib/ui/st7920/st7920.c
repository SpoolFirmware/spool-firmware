#include "st7920.h"
#include <stdint.h>

const static struct St7920 *st7920;

void uiSt7920Init(const struct St7920 *drv)
{
    st7920 = drv;
}

void uiWriteTile(uint8_t x0, uint8_t x1, uint8_t y, const uint8_t *buf)
{
    st7920WriteTile(st7920, x0, x1, y, buf);
}