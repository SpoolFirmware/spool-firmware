#include <stddef.h>
#include "drf.h"
#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"
#include "manual/irq.h"
#include "manual/mcu.h"
#include "stream_buffer.h"

#include "FreeRTOS.h"

const static struct HalClockConfig halClockConfig = {
    .hseFreqHz = 8000000,
    .pllXtPre = 1,
    .usbPre1_5X = 1, // 0 -> 1, 1 -> 1.5
    .pllMul = 9,

    .ahbPrescaler = 1,
    .apb1Prescaler = 2,
    .apb2Prescaler = 1,
    .adcPrescaler = 6,
};

const static struct HalGPIOConfig gpioConfig = {
    .groupEnable = EnableGPIOA | EnableGPIOB | EnableGPIOC,
};

const static struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA),
                                         .pin = 1 };

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);
    halGpioInit(&gpioConfig);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSH_PULL));
}

void platformPostInit(void)
{
    halGpioSet(statusLED);
}

void enableStepper(uint8_t stepperMask)
{
}

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return 0;
}

void platformDbgPutc(char c)
{
    (void)c;
}
