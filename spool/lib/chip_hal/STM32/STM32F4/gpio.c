#include "gpio.h"

/*!
 * For STM32 GPIO block, line.group is the base addr of the GPIO bank, line.pin is the pin #
 */

void chipHalGpioSetMode(struct IOLine line, GPIOMode mode)
{
    // SetMode
    uint32_t moder = line.group + DRF_GPIOA_MODER_OFFSET;
    uint32_t val = REG_RD32(moder);
    val &= ~(DRF_MASK(DRF_GPIOA_MODER_MODER0) << (line.pin * 2));
    val |= (DRF_VAL(_GPIO,_MODE,_MODE, mode) << (line.pin * 2));
    REG_WR32(moder, val);
}

void chipHalGpioSet(struct IOLine line)
{
    uint32_t bsrr_reg = line.group + DRF_GPIOC_BSRR_OFFSET;
    REG_WR32(bsrr_reg, DRF_DEF(_GPIOA, _BSRR, _BS0, _SET) << line.pin);
}

void chipHalGpioClear(struct IOLine line)
{
    uint32_t bsrr_reg = line.group + DRF_GPIOC_BSRR_OFFSET;
    REG_WR32(bsrr_reg, DRF_DEF(_GPIOA, _BSRR, _BR0, _SET) << line.pin);
}
