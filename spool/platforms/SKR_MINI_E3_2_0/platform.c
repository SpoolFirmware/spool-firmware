#include "FreeRTOS.h"
#include "stream_buffer.h"

#include "config_private.h"

// TIMER 6 is used for stepper scheduler
// TIMER 7 is used for stepper pulse width

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

const static struct IOLine endStops[] = {
    { .group = DRF_BASE(DRF_GPIOC), .pin = 0 },
    { .group = DRF_BASE(DRF_GPIOC), .pin = 1 },
    { .group = DRF_BASE(DRF_GPIOC), .pin = 2 },
};

const struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA), .pin = 1 };
__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);
    halGpioInit(&gpioConfig);

    // Initialize comm first, this gives us dbgPrintf
    privCommInit();
    privStepperInit();

    // Configure ENDStops
    for (size_t i = 0; i < ARRAY_LENGTH(endStops); i++) {
        halGpioSetMode(endStops[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _FLOATING));
    }
}

void platformPostInit(void)
{
    privCommPostInit();
    privStepperPostInit();
}

bool platformGetEndstop(uint8_t axis)
{
    if (axis < ARRAY_LENGTH(endStops))
        return !halGpioRead(endStops[axis]);
    return true;
}

fix16_t platformReadTemp(int8_t idx)
{
    return F16(0);
}

void platformSetHeater(int8_t idx, uint8_t pwm)
{
}

void platformSetFan(int8_t idx, uint8_t pwm)
{
}

void __warn(const char *file, int line, const char *err)
{
}

void __warn_on_err(const char *file, int line, status_t err)
{
}
