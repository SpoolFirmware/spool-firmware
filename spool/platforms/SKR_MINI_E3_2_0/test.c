#include "config_private.h"
#include "misc.h"
#include "manual/mcu.h"
#include "manual/irq.h"
#include "hal/stm32/hal.h"
#include "dbgprintf.h"

static struct SPIDevice sdSPI = {.base=DRF_BASE(DRF_SPI1)};

const struct IOLine sdCSPin = {.group=DRF_BASE(DRF_GPIOA), .pin=4};
const struct IOLine sck = {.group=DRF_BASE(DRF_GPIOA), .pin=5};
const struct IOLine miso = {.group=DRF_BASE(DRF_GPIOA), .pin=6};
const struct IOLine mosi = {.group=DRF_BASE(DRF_GPIOA), .pin=7};

struct SPIDevice *platformGetSDSPI(void)
{
    return &sdSPI;
}

void privTestInit(void)
{
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _SPI1EN, _ENABLED);

    halGpioSetMode(sdCSPin, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSH_PULL));
    halGpioSetMode(miso, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _FLOATING));
    halGpioSetMode(mosi, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL));
    halGpioSetMode(sck, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL));

    halSpiInit(&sdSPI, &(struct SPIConfig){
        .clkSpeed=halClockApb2FreqGet(&halClockConfig),
        .baudrate=250000,
        .mode=SPIMode_Master,
        .duplexMode=SPIDuplexMode_FullDuplex,
        .frameFormat=SPIFrameFormat_MSBFirst,
    });
}

IRQ_HANDLER_SPI1(void)
{
    halSpiLLServiceInterrupt(&sdSPI);
    halIrqClear(IRQ_SPI1);
}

void privTestPostInit(void)
{
    halGpioSet(sdCSPin);
    halSpiStart(&sdSPI);
    halIrqPrioritySet(IRQ_SPI1, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_SPI1);
}