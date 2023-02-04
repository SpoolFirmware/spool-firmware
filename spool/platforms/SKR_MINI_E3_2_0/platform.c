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

const struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA),
                                         .pin = 1 };

static void sSetupStepperTimers(void)
{
    // Setup TIMER 6 with 2x
}

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);
    halGpioInit(&gpioConfig);

    privCommInit();
}

void platformPostInit(void)
{
    privCommPostInit();
}

IRQ_HANDLER_TIM6(void)
{
    halIrqClear(IRQ_TIM6);
}

IRQ_HANDLER_TIM7(void)
{
   halIrqClear(IRQ_TIM7);
}

void platformEnableStepper(uint8_t stepperMask)
{
}

void platformDisableStepper(uint8_t stepperMask)
{
}

void __warn(const char *file, int line, const char *err)
{
}

void __warn_on_err(const char *file, int line, status_t err)
{
}

