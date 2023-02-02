#pragma once

#include <stdint.h>
#include <stdbool.h>

struct UARTConfig
{
    uint32_t baudrate;
    bool useRxInterrupt : 1;
    bool useTxInterrupt : 1;
    bool useTx : 1;
    bool useRx : 1;
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
