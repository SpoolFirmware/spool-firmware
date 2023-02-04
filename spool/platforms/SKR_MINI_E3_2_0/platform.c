#include "FreeRTOS.h"
#include "stream_buffer.h"

#include "config_private.h"


const struct HalClockConfig halClockConfig = {
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

const struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA),
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

    privCommInit();
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
    UNIMPLEMENTED("platformRecvCommand unimplemented");
}

void platformSendResponse(const char *pBuffer, size_t len)
{
    UNIMPLEMENTED("platformSendResponse unimplemented");
}

void __warn(const char *file, int line, const char *err)
{
}

void __warn_on_err(const char *file, int line, status_t err)
{
}

