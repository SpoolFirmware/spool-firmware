#include "FreeRTOS.h"
#include "stream_buffer.h"
#include "configuration.h"
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
    .groupEnable = EnableGPIOA | EnableGPIOB | EnableGPIOC | EnableGPIOD,
};

const static struct IOLine endStops[] = {
    { .group = DRF_BASE(DRF_GPIOC), .pin = 0 },
    { .group = DRF_BASE(DRF_GPIOC), .pin = 1 },
    { .group = DRF_BASE(DRF_GPIOC), .pin = 2 },
};

struct TimerDriver wallClockTimer;

const struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA), .pin = 1 };
__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);
    halGpioInit(&gpioConfig);
    REG_FLD_SET_DRF_NUM(_AFIO, _MAPR, _SWJ_CFG, 0x2); // Disable JTAG, leave only SWD

    // Configure ENDStops
    for (size_t i = 0; i < ARRAY_LENGTH(endStops); i++) {
        halGpioSetMode(endStops[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _FLOATING));
    }

    REG_FLD_SET_DRF(_RCC, _APB1ENR, _TIM5EN, _ENABLED);
    halTimerConstruct(&wallClockTimer, DRF_BASE(DRF_TIM5));
    halTimerStart(&wallClockTimer, &(struct TimerConfig){
        .timerTargetFrequency = 10000000,
        .clkDomainFrequency = halClockApb1TimerFreqGet(&halClockConfig),
        .interruptEnable = true
    });
    
    // Initialize comm first, this gives us dbgPrintf
    privCommInit();
    privStepperInit();
    privThermalInit();
    privTestInit();
}

void platformPostInit(void)
{
    privCommPostInit();
    privStepperPostInit();
    privThermalPostInit();
    privTestPostInit();

    // Start Wallclock
    halIrqPrioritySet(IRQ_TIM5, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_TIM5);
    halTimerStartContinous(&wallClockTimer, 10000);
}

static uint64_t wallClockTimeUs = 0;
IRQ_HANDLER_TIM5(void)
{
    wallClockTimeUs += 10000;
    halTimerIrqClear(&wallClockTimer);
    halIrqClear(IRQ_TIM5);
}

uint64_t platformGetTimeUs(void)
{
    return wallClockTimeUs + (10000 - halTimerGetCount(&wallClockTimer));
    privTestPostInit();
}

bool platformGetEndstop(uint8_t axis)
{
    configASSERT(axis < ARRAY_LENGTH(endStops));
    bool endStop = halGpioRead(endStops[axis]);
    if (axis == Z_AXIS)
        endStop = !endStop;
    if (axis < ARRAY_LENGTH(endStops))
        return endStop;
    return true;
}
