#include "gpio.h"

/*!
 * For STM32 GPIO block, line.group is the base addr of the GPIO bank, line.pin is the pin #
 */


#define LINE_REG(LINE, REG) ((LINE.group) + (DRF_GPIO_##REG))

void halGpioSetMode(struct IOLine line, GPIOMode mode)
{
    uint32_t val;
    // SetMode
    const uint32_t pinMode = DRF_VAL(_HAL_GPIO, _MODE, _MODE, mode);
    const uint32_t outType = DRF_VAL(_HAL_GPIO, _MODE, _TYPE, mode);
    const uint32_t outSpeed = DRF_VAL(_HAL_GPIO, _MODE, _SPEED, mode);
    const uint32_t afNumber = DRF_VAL(_HAL_GPIO, _MODE, _AF, mode);
    uint32_t pullType = DRF_VAL(_HAL_GPIO, _MODE, _PUPD, mode);

    // MODER
    const uint32_t modeR = line.group + DRF_GPIO_MODER;
    val = REG_RD32(modeR);
    REG_WR32(modeR, FLD_IDX_SET_DRF_NUM(_GPIO, _MODER, _MODER, line.pin, pinMode, val));

    if (mode == DRF_HAL_GPIO_MODE_MODE_OUTPUT || mode == DRF_HAL_GPIO_MODE_MODE_AF) 
    {
        // OTYPER
        const uint32_t oTypeR = line.group + DRF_GPIO_OTYPER;
        val = REG_RD32(oTypeR);
        REG_WR32(oTypeR, FLD_IDX_SET_DRF_NUM(_GPIO, _OTYPER, _OT, line.pin, outType, val));
        // OSPEEDR
        const uint32_t oSpeedR = line.group + DRF_GPIO_OSPEEDR;
        val = REG_RD32(oSpeedR);
        REG_WR32(oSpeedR, FLD_IDX_SET_DRF_NUM(_GPIO, _OSPEEDR, _OSPEEDR, line.pin, outSpeed, val));
    }

    if (mode == DRF_HAL_GPIO_MODE_MODE_ANALOG)
    {
        pullType = DRF_HAL_GPIO_MODE_PUPD_FLOATING;
    }
    // PUPDR
    const uint32_t oTypeR = line.group + DRF_GPIO_PUPDR;
    val = REG_RD32(oTypeR);
    REG_WR32(oTypeR, FLD_IDX_SET_DRF_NUM(_GPIO, _PUPDR, _PUPDR, line.pin, pullType, val));

    //AF
    if (pinMode == DRF_HAL_GPIO_MODE_MODE_AF)
    {
        if (line.pin < DRF_GPIO_AFRL_AFRL__COUNT)
        {
            // AFRL (0..7)
            const uint32_t afRL = line.group + DRF_GPIO_AFRL;
            val = REG_RD32(afRL);
            REG_WR32(afRL, FLD_IDX_SET_DRF_NUM(_GPIO, _AFRL, _AFRL, line.pin, afNumber, val));
        }
        else
        {
            // AFRH (8..15)
            const uint32_t afRH = line.group + DRF_GPIO_AFRH;
            val = REG_RD32(afRH);
            REG_WR32(afRH, FLD_IDX_SET_DRF_NUM(_GPIO, _AFRH, _AFRH, line.pin - DRF_GPIO_AFRL_AFRL__COUNT, afNumber, val));
        }
    }
    
}

__attribute__((always_inline))
inline uint8_t halGpioRead(struct IOLine line)
{
    return DRF_IDX_VAL(_GPIO, _IDR, _IDR, line.pin, REG_RD32(line.group + DRF_GPIO_IDR));
}

__attribute((always_inline))
inline void halGpioToggle(struct IOLine line)
{
    REG_WR32(LINE_REG(line, ODR), REG_RD32(LINE_REG(line, ODR)) ^ DRF_IDX_DEF(_GPIO, _ODR, _ODR, line.pin, _HIGH));
}

__attribute__((always_inline))
inline void halGpioSet(struct IOLine line)
{
    REG_WR32(line.group + DRF_GPIO_BSRR, DRF_IDX_DEF(_GPIO, _BSRR, _BS, line.pin, _SET));
}

__attribute__((always_inline))
inline void halGpioClear(struct IOLine line)
{
    REG_WR32(line.group + DRF_GPIO_BSRR, DRF_IDX_DEF(_GPIO, _BSRR, _BR, line.pin, _RESET));
}
