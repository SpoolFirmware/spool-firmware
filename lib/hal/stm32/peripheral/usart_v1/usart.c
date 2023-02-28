#include "error.h"
#include "hal/hal.h"
#include "drf.h"
#include "usart.h"
#include "manual/mcu.h"
#include "task.h"

void sUartNotify(struct UARTDriver *pDriver)
{
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR1,
             FLD_SET_DRF(_USART1, _CR1, _TXEIE, _ENABLED,
                         REG_RD32(pDriver->deviceBase + DRF_USART1_CR1)));
}

void halUartInit(struct UARTDriver *pDriver, const struct UARTConfig *pCfg,
                 size_t deviceBase, uint32_t clkSpeed)
{
    pDriver->cfg = *pCfg;
    pDriver->deviceBase = deviceBase;
    // Calculate and set the baudrate factor
    pDriver->brr = (clkSpeed + (pCfg->baudrate / 2)) / pCfg->baudrate;
    if (pDriver->cfg.useTxInterrupt) {
        pDriver->txBuffer = xStreamBufferCreate(64, 1);
        configASSERT(pDriver->txBuffer);
    }
    if (pDriver->cfg.useRxInterrupt) {
        pDriver->rxBuffer = xStreamBufferCreate(64, 1);
        configASSERT(pDriver->rxBuffer);
    }
}

void halUartStart(struct UARTDriver *pDriver)
{
    REG_WR32(pDriver->deviceBase + DRF_USART1_BRR, pDriver->brr);

    uint32_t cr1 = DRF_DEF(_USART1, _CR1, _UE, _ENABLED);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _TE, pDriver->cfg.useTx, cr1);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _RE, pDriver->cfg.useRx, cr1);
    cr1 = FLD_SET_DRF_NUM(_USART1, _CR1, _RXNEIE, pDriver->cfg.useRxInterrupt,
                          cr1);
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR1, cr1);

    REG_WR32(pDriver->deviceBase + DRF_USART1_CR2, 0);
    REG_WR32(pDriver->deviceBase + DRF_USART1_CR3, 0);

    REG_WR32(pDriver->deviceBase + DRF_USART1_SR, 0);
    REG_RD32(pDriver->deviceBase + DRF_USART1_SR);
    REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
}

void halUartSendByte(struct UARTDriver *pDriver, uint8_t byte)
{
    if (pDriver->cfg.useTxInterrupt) {
        xStreamBufferSend(pDriver->txBuffer, &byte, 1, portMAX_DELAY);
        sUartNotify(pDriver);
    } else {
        while (FLD_TEST_DRF(_USART1, _SR, _TXE, _CLR,
                            REG_RD32(pDriver->deviceBase + DRF_USART1_SR)))
            ;
        REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
    }
}

void halUartSend(struct UARTDriver *pDriver, const uint8_t *pBuffer,
                 uint32_t len)
{
    if (pDriver->cfg.useTxInterrupt) {
        xStreamBufferSend(pDriver->txBuffer, pBuffer, len, portMAX_DELAY);
        sUartNotify(pDriver);
    } else {
        for (uint32_t i = 0; i < len; i++) {
            halUartSendByte(pDriver, pBuffer[i]);
        }
    }
}

int halUartRecvByteBlocking(struct UARTDriver *pDriver, uint8_t *pByte,
                            TickType_t timeout)
{
    int status = 1;
    if (pDriver->cfg.useRxInterrupt) {
        {
            if (xStreamBufferReceive(pDriver->rxBuffer, pByte, 1, timeout) ==
                pdTRUE) {
                status = 0;
            }
        }
    } else {
        TickType_t exitTime = xTaskGetTickCount() + timeout;
        while ((xTaskGetTickCount() < exitTime) &&
               (FLD_TEST_DRF(_USART1, _SR, _RXNE, _CLR,
                             REG_RD32(pDriver->deviceBase + DRF_USART1_SR))))
            ;
        if (FLD_TEST_DRF(_USART1, _SR, _RXNE, _SET,
                         REG_RD32(pDriver->deviceBase + DRF_USART1_SR))) {
            *pByte = REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
            status = 0;
        }
    }
    return status;
}

int halUartRecvByte(struct UARTDriver *pDriver, uint8_t *pByte)
{
    return halUartRecvByteBlocking(pDriver, pByte, 0);
}

size_t halUartRecvBytes(struct UARTDriver *pDriver, uint8_t *pBuffer,
                        size_t bufferSize, TickType_t ticksToWait)
{
    configASSERT(pDriver->cfg.useRxInterrupt); // TODO not supported otherwise
    return xStreamBufferReceive(pDriver->rxBuffer, pBuffer, bufferSize,
                                ticksToWait);
}

void halUartIrqHandler(struct UARTDriver *pDriver)
{
    uint32_t sr = REG_RD32(pDriver->deviceBase + DRF_USART1_SR);
    uint8_t byte;
    if (pDriver->cfg.useRxInterrupt &&
        FLD_TEST_DRF(_USART1, _SR, _RXNE, _SET, sr)) {
        byte = REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
        xStreamBufferSendFromISR(pDriver->rxBuffer, &byte, 1, NULL);
    }
    if (pDriver->cfg.useTxInterrupt &&
        FLD_TEST_DRF(_USART1, _SR, _TXE, _SET, sr)) {
        if ((!xStreamBufferIsEmpty(pDriver->txBuffer))) {
            configASSERT(xStreamBufferReceiveFromISR(pDriver->txBuffer, &byte,
                                                     1, NULL) == pdTRUE);
            REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
        } else {
            REG_WR32(
                pDriver->deviceBase + DRF_USART1_CR1,
                FLD_SET_DRF(_USART1, _CR1, _TXEIE, _DISABLED,
                            REG_RD32(pDriver->deviceBase + DRF_USART1_CR1)));
        }
    }
}

void halUartReset(struct UARTDriver *pDriver, bool resetRx, bool resetTx)
{
    while (FLD_TEST_DRF(_USART1, _SR, _TC, _CLR,
                REG_RD32(pDriver->deviceBase + DRF_USART1_SR)))
        ;
    if (resetRx) {
        REG_WR32(pDriver->deviceBase + DRF_USART1_SR, 0);
        if (pDriver->cfg.useRxInterrupt) {
            xStreamBufferReset(pDriver->rxBuffer);
        }
    }

    if (resetTx && pDriver->cfg.useTxInterrupt) {
        xStreamBufferReset(pDriver->txBuffer);
    }
}

void halUartWaitForIdle(struct UARTDriver *pDriver)
{
}
