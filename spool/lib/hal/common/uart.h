#pragma once

#include <stdint.h>

struct UARTConfig
{
    uint32_t baudrate;
};

struct UARTDriver;

// halUartInit is defined in port specific manner
void halUartStart(struct UARTDriver *pDriver);
void halUartSendByte(struct UARTDriver *pDriver, uint8_t byte);
void halUartSend(struct UARTDriver *pDriver, const uint8_t *pBuffer, uint32_t len);
