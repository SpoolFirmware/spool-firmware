#include "platform_private.h"
#include "stream_buffer.h"
#include "dbgprintf.h"
#include "error.h"

struct UARTDriver printUart = { 0 };
const static struct UARTConfig uart1Cfg = {
    .baudrate = 115200,
    .useRxInterrupt = false,
    .useTxInterrupt = false,
    .useTx = true,
    .useRx = false,
};

struct UARTDriver cmdUart = { 0 };
const static struct UARTConfig cmdUartCfg = {
    .baudrate = 115200,
    .useRxInterrupt = true,
    .useTxInterrupt = true,
    .useTx = true,
    .useRx = true,
};

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return halUartRecvBytes(&cmdUart, (uint8_t *)pBuffer, bufferSize, ticksToWait);
}

void platformSendResponse(const char *pBuffer, size_t len)
{
    halUartSend(&cmdUart, (const uint8_t *)pBuffer, len);
}

void communicationInit(void)
{
    struct IOLine dbgUartTx = { .group = DRF_BASE(DRF_GPIOD), .pin = 8 };
    halGpioSetMode(dbgUartTx,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));
    halUartInit(&printUart, &uart1Cfg, DRF_BASE(DRF_USART3),
                halClockApb1FreqGet(&halClockConfig));
    // printUart is started early here
    halUartStart(&printUart);

    struct IOLine commUartTx = { .group = DRF_BASE(DRF_GPIOB), .pin = 6 };
    struct IOLine commUartRx = { .group = DRF_BASE(DRF_GPIOB), .pin = 7 };
    halGpioSetMode(commUartTx,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));
    halGpioSetMode(commUartRx,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));
    halUartInit(&cmdUart, &cmdUartCfg, DRF_BASE(DRF_USART1),
                halClockApb2FreqGet(&halClockConfig));
}

void communicationPostInit(void)
{
    halUartStart(&cmdUart);
    halIrqPrioritySet(IRQ_USART1, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_USART1);
}

// GCODE UART IRQ HANDLER
IRQ_HANDLER_USART1(void)
{
    halUartIrqHandler(&cmdUart);
    halIrqClear(IRQ_USART1);
}

void platformDbgPutc(char c)
{
    halUartSendByte(&printUart, (uint8_t)c);
}
