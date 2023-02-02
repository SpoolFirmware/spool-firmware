#include "error.h"
#include "hal/hal.h"
#include "drf.h"
#include "usart.h"
#include "manual/mcu.h"

void halUartInit(struct UARTDriver *pDriver, const struct UARTConfig *pCfg,
                 size_t deviceBase, uint32_t clkSpeed)
{
    pDriver->cfg = *pCfg;
    pDriver->deviceBase = deviceBase;
    // Calculate and set the baudrate factor
    pDriver->brr = (clkSpeed + (pCfg->baudrate / 2)) / pCfg->baudrate;
}

void halUartStart(struct UARTDriver *pDriver)
{
    REG_WR32(pDriver->deviceBase + DRF_USART1_BRR, pDriver->brr);

    uint32_t cr1 = DRF_DEF(_USART1, _CR1, _UE, _ENABLED);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _TE, pDriver->cfg.useTx, cr1);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _RE, pDriver->cfg.useRx, cr1);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _RXNEIE, pDriver->cfg.useRxInterrupt, cr1);
    // TODO: useTxInterrupt
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR1, cr1);

    REG_WR32(pDriver->deviceBase + DRF_USART1_CR2, 0);
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR3, 0);

    REG_WR32(pDriver->deviceBase + DRF_USART1_SR, 0);
    REG_RD32(pDriver->deviceBase + DRF_USART1_SR);
    REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
}

void halUartSendByte(struct UARTDriver *pDriver, uint8_t byte)
{
    while (FLD_TEST_DRF(_USART1, _SR, _TXE, _CLR,
                        REG_RD32(pDriver->deviceBase + DRF_USART1_SR)))
        ;
    REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
}

int halUartRecvByte(struct UARTDriver *pDriver, uint8_t *pByte)
{
    if (FLD_TEST_DRF(_USART1, _SR, _RXNE, _SET,
                     REG_RD32(pDriver->deviceBase + DRF_USART1_SR))) {
        *pByte = REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
        return 0;
    }
    return 1;
}
