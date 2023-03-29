#include "gpio.h"
#include "bitops.h"
#include "error.h"
#include "dbgprintf.h"

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

static uint32_t gpioGroupToIdx(uint32_t group)
{
    /* stolen from chibios
     * Port index is obtained assuming that GPIO ports are placed at regular
     * 0x400 intervals in memory space. So far this is true for all
     * devices.
     */
    return (((uint32_t)group - (uint32_t)DRF_BASE(DRF_GPIOA)) >> 10u) & 0xFu;
}

/* copied from
 * ChibiOS/os/hal/ports/STM32/LLD/GPIOv2/hal_pal_lld.c */
/* need to enable syscfg peripheral */
void halGpioEnableInterrupt(struct IOLine line, enum GpioExtiMode mode)
{
    uint32_t padMask = BIT(line.pin);
    uint32_t groupNum = gpioGroupToIdx(line.group);
    uint32_t rtsr = REG_RD32(DRF_REG(_EXTI, _RTSR));
    uint32_t ftsr = REG_RD32(DRF_REG(_EXTI, _FTSR));
    /* check if channel already in use */
    BUG_ON((rtsr & padMask) && (ftsr & padMask));

    uint32_t crReg = DRF_REG(_SYSCFG, _EXTICR1) + line.pin / 4 * 4;
    uint32_t crShift = line.pin % 4 * 4;
    uint32_t crMask = ~(DRF_MASK(DRF_SYSCFG_EXTICR1_EXTI0) << crShift);
    REG_WR32(crReg, (REG_RD32(crReg) & crMask) | (groupNum << crShift));

    if (mode & GpioExtiModeRisingEdge) {
        REG_WR32(DRF_REG(_EXTI, _RTSR), rtsr | padMask);
    } else {
        REG_WR32(DRF_REG(_EXTI, _RTSR), rtsr & ~padMask);
    }
    if (mode & GpioExtiModeFallingEdge) {
        REG_WR32(DRF_REG(_EXTI, _FTSR), ftsr | padMask);
    } else {
        REG_WR32(DRF_REG(_EXTI, _FTSR), ftsr & ~padMask);
    }

    uint32_t imr = REG_RD32(DRF_REG(_EXTI, _IMR));
	/* 1 is not masked */
    REG_WR32(DRF_REG(_EXTI, _IMR), imr | padMask); // TODO
    uint32_t emr = REG_RD32(DRF_REG(_EXTI, _EMR));
    REG_WR32(DRF_REG(_EXTI, _EMR), emr & ~padMask); // TODO
}

void halGpioDisableInterrupt(struct IOLine line, enum GpioExtiMode mode)
{
    // uint32_t padMask = BIT(line.pin);
    // uint8_t groupNum = gpioGroupToIdx(line.group);
    // uint32_t rtsr = REG_RD32(DRF_REG(_EXTI, _RTSR));
    // uint32_t ftsr = REG_RD32(DRF_REG(_EXTI, _FTSR));

    UNIMPLEMENTED("gpio v2 disable interrupt");
}
