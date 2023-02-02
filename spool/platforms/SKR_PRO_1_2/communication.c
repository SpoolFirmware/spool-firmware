#include "platform_private.h"
#include "stream_buffer.h"
#include "dbgprintf.h"
#include "error.h"

struct UARTDriver printUart = { 0 };
const static struct UARTConfig uart1Cfg = {
    .baudrate = 115200,
    .useRxInterrupt = 0,
    .useTxInterrupt = 0,
    .useTx = 1,
    .useRx = 0,
};

struct UARTDriver cmdUart = { 0 };
const static struct UARTConfig cmdUartCfg = {
    .baudrate = 115200,
    .useRxInterrupt = 1,
    .useTxInterrupt = 0,
    .useTx = 1,
    .useRx = 0,
};

static char uartCommandBuffer[128];
static StreamBufferHandle_t cmdmgmtBufferHandle;
static StaticStreamBuffer_t cmdmgmtBufferMeta;

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return xStreamBufferReceive(cmdmgmtBufferHandle, pBuffer, bufferSize,
                                ticksToWait);
}

void platformSendResponse(const char *pBuffer, size_t len)
{
    halUartSend(&cmdUart, (const uint8_t *)pBuffer, len);
}

void communicationInit(void)
{
    struct IOLine dbgUartTx = { .group = GPIOD, .pin = 8 };
    halGpioSetMode(dbgUartTx,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));
    halUartInit(&printUart, &uart1Cfg, DRF_BASE(DRF_USART3),
                halClockApb1FreqGet(&halClockConfig));
    // printUart is started early here
    halUartStart(&printUart);

    struct IOLine commUartTx = { .group = GPIOB, .pin = 6 };
    struct IOLine commUartRx = { .group = GPIOB, .pin = 7 };
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

    // Setup cmdStream
    cmdmgmtBufferHandle = xStreamBufferCreateStatic(
        sizeof(uartCommandBuffer), 1, (uint8_t *)uartCommandBuffer,
        &cmdmgmtBufferMeta);
}

void communicationPostInit(void)
{
    halUartStart(&cmdUart);
    halIrqPrioritySet(IRQ_USART1, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_USART1);
}

IRQ_HANDLER_USART1(void)
{
    uint8_t byte;
    if (!halUartRecvByte(&cmdUart, &byte)) {
        xStreamBufferSendFromISR(cmdmgmtBufferHandle, &byte, 1, NULL);
    }
    halIrqClear(IRQ_USART1);
}

void __warn(const char *file, int line, const char *err)
{
    dbgPrintf("WARN %s in %s:%d\n", err, file, line);
}

void __warn_on_err(const char *file, int line, status_t err)
{
    dbgPrintf("WARN ERR %d in %s:%d\n", err, file, line);
}

void platformDbgPutc(char c)
{
    halUartSendByte(&printUart, (uint8_t)c);
}