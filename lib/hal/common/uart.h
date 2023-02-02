#pragma once

#include <stdint.h>

struct UARTConfig
{
    uint32_t baudrate;
    uint8_t useRxInterrupt : 1;
    uint8_t useTxInterrupt : 1;
    uint8_t useTx : 1;
    uint8_t useRx : 1;
};

struct UARTDriver;

// halUartInit is defined in port specific manner
void halUartStart(struct UARTDriver *pDriver);
void halUartSendByte(struct UARTDriver *pDriver, uint8_t byte);
void halUartSend(struct UARTDriver *pDriver, const uint8_t *pBuffer, uint32_t len);
/*!
 * @return 0 if a byte is recv'd.
 */
int halUartRecvByte(struct UARTDriver *pDriver, uint8_t *pByte);
