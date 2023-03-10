#include "config_private.h"
#include "manual/irq.h"
#include "stream_buffer.h"

struct UARTDriver dbgUart = { 0 };
const static struct UARTConfig dbgUartCfg = {
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

void privCommInit(void)
{
    // Setup dbgUart (UART5)
    halGpioSetMode(halGpioLineConstruct(DRF_BASE(DRF_GPIOC), 12),
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL));
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _UART5EN, _ENABLED);

    halUartInit(&dbgUart, &dbgUartCfg, DRF_BASE(DRF_UART5),
                halClockApb1FreqGet(&halClockConfig));
    halUartStart(&dbgUart);

    // Setup cmdUart (GCode)
    halGpioSetMode(halGpioLineConstruct(DRF_BASE(DRF_GPIOA), 2),
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL));
    halGpioSetMode(halGpioLineConstruct(DRF_BASE(DRF_GPIOA), 3),
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _FLOATING));
    // Enable UART2
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _USART2EN, _ENABLED);
    halUartInit(&cmdUart, &cmdUartCfg, DRF_BASE(DRF_USART2),
                halClockApb1FreqGet(&halClockConfig));
}

void privCommPostInit(void)
{
    halUartStart(&cmdUart);
    halIrqPrioritySet(IRQ_USART2, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_USART2);
}

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return halUartRecvBytes(&cmdUart, (uint8_t *)pBuffer, bufferSize,
                            ticksToWait);
}

void platformSendResponse(const char *pBuffer, size_t len)
{
    halUartSend(&cmdUart, (const uint8_t *)pBuffer, len);
}

void platformDbgPutc(char c)
{
    halUartSendByte(&dbgUart, (uint8_t)c);
}

IRQ_HANDLER_USART2(void)
{
    halUartIrqHandler(&cmdUart);
    halIrqClear(IRQ_USART1);
}
