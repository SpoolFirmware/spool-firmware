#include "error.h"
#include "hal/hal.h"
#include "drf.h"
#include "usart.h"
#include "manual/mcu.h"

void halUartInit(struct UARTDriver *pDriver, const struct UARTConfig *pCfg,
                 size_t deviceBase, uint32_t clkSpeed)
{
    pDriver->deviceBase = deviceBase;
    // Calculate and set the baudrate factor
    pDriver->brr =
        (clkSpeed << DRF_SIZE(DRF_USART1_BRR_DIV_Fraction)) / pCfg->baudrate;
}

void halUartStart(struct UARTDriver *pDriver)
{
    REG_WR32(pDriver->deviceBase + DRF_USART1_BRR, pDriver->brr);
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR1,
             DRF_DEF(_USART1, _CR1, _UE, _ENABLED) |
                 DRF_DEF(_USART1, _CR1, _TE, _ENABLED) |
                 DRF_DEF(_USART1, _CR1, _RE, _ENABLED));
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR2, 0);
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR3, 0);
}

void halUartSendByte(struct UARTDriver *pDriver, uint8_t byte)
{
    while (FLD_TEST_DRF(_USART1, _SR, _TXE, _CLR,
                        REG_RD32(pDriver->deviceBase + DRF_USART1_SR)));
    REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
}
