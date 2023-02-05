#include "error.h"
#include "hal/hal.h"
#include "drf.h"
#include "usart.h"
#include "manual/mcu.h"

#include "task.h"

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
    if (pDriver->cfg.useTxInterrupt) {
        UNIMPLEMENTED("Send with TxInterrupt");
    } else {
        taskENTER_CRITICAL();
        {
            while (FLD_TEST_DRF(_USART1, _SR, _TXE, _CLR,
                                REG_RD32(pDriver->deviceBase + DRF_USART1_SR)))
                ;
            REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
        }
        taskEXIT_CRITICAL();
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
    return xStreamBufferReceive(pDriver->rxBuffer, pBuffer, bufferSize, ticksToWait);
}

void halUartIrqHandler(struct UARTDriver *pDriver)
{
    uint32_t sr = REG_RD32(pDriver->deviceBase + DRF_USART1_SR);
    uint8_t byte;
    if (pDriver->cfg.useRxInterrupt) {
        if (FLD_TEST_DRF(_USART1, _SR, _RXNE, _SET, sr)) {
            byte = REG_RD32(pDriver->deviceBase + DRF_USART1_DR);
            xStreamBufferSendFromISR(pDriver->rxBuffer, &byte, 1, NULL);
        }
    }
    if (pDriver->cfg.useTxInterrupt) {
        if (!xStreamBufferIsEmpty(pDriver->txBuffer) &&
            FLD_TEST_DRF(_USART1, _SR, _TXE, _SET, sr)) {
            configASSERT(xStreamBufferReceiveFromISR(pDriver->txBuffer, &byte,
                                                     1, NULL) == pdTRUE);
            REG_WR32(pDriver->deviceBase + DRF_USART1_DR, byte);
        }
    }
}
