#include "config_private.h"

struct UARTDriver dbgUart = { 0 };
const static struct UARTConfig dbgUartCfg = {
    .baudrate = 115200,
    .useRxInterrupt = false,
    .useTxInterrupt = false,
    .useTx = true,
    .useRx = false,
};

void privCommInit(void)
{
    halGpioSetMode(halGpioLineConstruct(DRF_BASE(DRF_GPIOC), 12),
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL));
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _UART5EN, _ENABLED);

    halUartInit(&dbgUart, &dbgUartCfg, DRF_BASE(DRF_UART5), halClockApb1FreqGet(&halClockConfig));
    halUartStart(&dbgUart);
}

void privCommPostInit(void)
{
}

void platformDbgPutc(char c)
{
    if (c == '\n')
        halUartSendByte(&dbgUart, '\r');
    halUartSendByte(&dbgUart, (uint8_t) c);
}
