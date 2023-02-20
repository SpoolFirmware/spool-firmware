#include "gpio.h"
#include "manual/mcu.h"
#include "drf.h"

#define LINE_REG(LINE, REG) ((LINE.group) + (DRF_GPIO_##REG))

void halGpioInit(const struct HalGPIOConfig *cfg)
{
    uint32_t groupEnableMask = (1U << 7U) - 1;
    uint32_t maskedEnableBits = cfg->groupEnable & groupEnableMask;

    // RESET GPIO & AFIO
    REG_WR32(DRF_REG(_RCC, _APB2RSTR),
             (maskedEnableBits << DRF_SHIFT(DRF_RCC_APB2RSTR_IOPARST)) |
                 DRF_DEF(_RCC, _APB2RSTR, _AFIORST, _RESET));
    REG_WR32(DRF_REG(_RCC, _APB2RSTR), 0);

    uint32_t apb2enr = REG_RD32(DRF_REG(_RCC, _APB2ENR));
    apb2enr = FLD_SET_DRF(_RCC, _APB2ENR, _AFIOEN, _ENABLED, apb2enr);

    apb2enr &= ~(groupEnableMask << DRF_SHIFT(DRF_RCC_APB2ENR_IOPAEN));
    apb2enr |= maskedEnableBits << DRF_SHIFT(DRF_RCC_APB2ENR_IOPAEN);

    REG_WR32(DRF_REG(_RCC, _APB2ENR), apb2enr);
}

void halGpioSetMode(struct IOLine line, GPIOMode mode)
{
    if (!line.group) return;
    uint32_t cr_addr = line.group;
    uint32_t position = line.pin;
    if (line.pin < DRF_GPIO_CRL_MODE__COUNT) {
        cr_addr += DRF_GPIO_CRL;
    } else {
        cr_addr += DRF_GPIO_CRH;
        position -= DRF_GPIO_CRL_MODE__COUNT;
    }

    uint32_t cr = REG_RD32(cr_addr);
    cr = FLD_IDX_SET_DRF_NUM(_GPIO, _CRL, _MODE, position, DRF_VAL(_HAL_GPIO, _MODE, _MODE, mode), cr);
    cr = FLD_IDX_SET_DRF_NUM(_GPIO, _CRL, _CNF, position, DRF_VAL(_HAL_GPIO, _MODE, _TYPE, mode), cr);
    REG_WR32(cr_addr, cr);
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
