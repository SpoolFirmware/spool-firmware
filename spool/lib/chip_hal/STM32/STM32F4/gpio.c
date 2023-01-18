#include "gpio.h"

/*!
 * For STM32 GPIO block, line.group is the base addr of the GPIO bank, line.pin is the pin #
 */

void chipHalGpioSetMode(struct IOLine line, GPIOMode mode)
{
    // SetMode
    uint32_t moder = line.group + DRF_GPIO_MODER;
    uint32_t val = REG_RD32(moder);
    REG_WR32(moder, FLD_IDX_SET_DRF_NUM(_GPIO, _MODER, _MODER, line.pin, mode, val));
}

void chipHalGpioSet(struct IOLine line)
{
    REG_WR32(line.group + DRF_GPIO_BSRR, DRF_IDX_DEF(_GPIO, _BSRR, _BS, line.pin, _SET));
}

void chipHalGpioClear(struct IOLine line)
{
    REG_WR32(line.group + DRF_GPIO_BSRR, DRF_IDX_DEF(_GPIO, _BSRR, _BR, line.pin, _RESET));
}
