#include "uart.h"

__attribute__((weak)) void halUartSend(struct UARTDriver *pDriver,
                                       const uint8_t *pBuffer, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++)
    {
        halUartSendByte(pDriver, pBuffer[i]);
    }
}
