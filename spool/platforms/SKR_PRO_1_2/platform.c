#include "FreeRTOS.h"
#include "configuration.h"
#include "platform_private.h"
#include "misc.h"
#include "platforms/common/platform.h"
#include "drivers/spi_sw.h"
#include "drivers/st7920.h"
#include "ui/st7920.h"

const struct HalClockConfig halClockConfig = {
    .hseFreqHz = 8000000,
    .q = 7,
    .p = 2,
    .n = 168,
    .m = 4,

    .apb2Prescaler = 2,
    .apb1Prescaler = 4,
    .ahbPrescaler = 1,
};

const static struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA), .pin = 7 };

const static struct IOLine dispSpiSwCs = { .group = DRF_BASE(DRF_GPIOD),
                                           .pin = 10 };
const static struct IOLine dispSpiSwMosi = { .group = DRF_BASE(DRF_GPIOD),
                                             .pin = 11 };
const static struct IOLine dispSpiSwSclk = { .group = DRF_BASE(DRF_GPIOG),
                                             .pin = 2 };

static struct SpiSw dispSpiSw;
static struct St7920 st7920;

const static uint32_t stepperFrequency = 100000;

struct TimerConfig stepperExecutionTimerCfg = {
    .timerTargetFrequency = 100000 * 2,
    .clkDomainFrequency = 0,
    .interruptEnable = true,
};
struct TimerDriver stepperExecutionTimer = { 0 };

struct TimerDriver wallClockTimer;

const static struct IOLine endstops[NR_AXIS] = {
    { .group = DRF_BASE(DRF_GPIOB), .pin = 10 }, /* X */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 12 }, /* Y */
    { .group = DRF_BASE(DRF_GPIOG), .pin = 8 },  /* Z */
    { 0 }, // E
};

const static struct IOLine step[NR_STEPPER] = {
    /* X */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 9 },
    /* Y */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 11 },
    /* Z */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 13 },
    /* E */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 14 },
};

const static struct IOLine dir[NR_STEPPER] = {
    /* X */
    { .group = DRF_BASE(DRF_GPIOF), .pin = 1 },
    /* Y */
    { .group = DRF_BASE(DRF_GPIOE), .pin = 8 },
    /* Z */
    { .group = DRF_BASE(DRF_GPIOC), .pin = 2 },
    /* E */
    { .group = DRF_BASE(DRF_GPIOA), .pin = 0 },
};

const static struct IOLine en[NR_STEPPER] = {
    /* X */
    { .group = DRF_BASE(DRF_GPIOF), .pin = 2 },
    /* Y */
    { .group = DRF_BASE(DRF_GPIOD), .pin = 7 },
    /* Z */
    { .group = DRF_BASE(DRF_GPIOC), .pin = 0 },
    /* E */
    { .group = DRF_BASE(DRF_GPIOC), .pin = 3 },
};

void platformEnableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPER; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioClear(en[i]);
        }
    }
}

void platformDisableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPER; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioSet(en[i]);
        }
    }
}

void platformSetStepperDir(uint8_t dirMask)
{
    for (uint8_t i = 0; i < NR_STEPPER; ++i) {
        if (dirMask & BIT(i)) {
            halGpioSet(dir[i]);
        } else {
            halGpioClear(dir[i]);
        }
    }
}

static void setStepper(uint8_t stepperMask, uint8_t stepMask)
{
    for (uint8_t i = 0; i < NR_STEPPER; ++i) {
        if (stepperMask & BIT(i)) {
            if (stepMask & BIT(i)) {
                halGpioSet(step[i]);
            } else {
                halGpioClear(step[i]);
            }
        }
    }
}

static uint32_t lastCntrReload = 0;
IRQ_HANDLER_TIM7(void)
{
    if (halTimerPending(&stepperExecutionTimer)) {
VectorB4_begin:
        halTimerIrqClear(&stepperExecutionTimer);
        halTimerChangeReloadValue(&stepperExecutionTimer, 65535);
        /* execute current job */
        uint32_t requestedTicks = executeStep((lastCntrReload + 1) / 2);
        uint32_t cntrReload = 2 * requestedTicks;
        if (cntrReload == 0)
            cntrReload = 1;
        if ((lastCntrReload = halTimerGetCount(&stepperExecutionTimer)) > cntrReload) {
            halTimerZeroCount(&stepperExecutionTimer);
            goto VectorB4_begin;
        }
        lastCntrReload = cntrReload;
        halTimerChangeReloadValue(&stepperExecutionTimer, cntrReload);
    }
    halIrqClear(IRQ_TIM7);
}

static void reloadTimer(void)
{
    REG_WR32(DRF_REG(_TIM2, _CR1), DRF_DEF(_TIM2, _CR1, _CEN, _ENABLED) |
                                       DRF_DEF(_TIM2, _CR1, _OPM, _ENABLED));
}

void platformStepStepper(uint8_t stepperMask)
{
    setStepper(stepperMask, stepperMask);
    reloadTimer();
}

IRQ_HANDLER_TIM2(void)
{
    if (FLD_TEST_DRF(_TIM2, _SR, _UIF, _UPDATE_PENDING,
                     REG_RD32(DRF_REG(_TIM2, _SR)))) {
        REG_WR32(DRF_REG(_TIM2, _SR),
                 ~DRF_DEF(_TIM2, _SR, _UIF, _UPDATE_PENDING));
        setStepper(0xFF, 0);
    }
    halIrqClear(IRQ_TIM2);
}

static void setupTimer()
{
    // Set Interrupt Priority
    // 28 is TIM2
    // halGpioSet(statusLED);
    halIrqPrioritySet(IRQ_TIM2, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_TIM2);

    // Enable Counter
    /* TIM2 runs off APB1, which runs at 42, but 84 due to x2 in the clock graph??? */
    REG_WR32(DRF_REG(_TIM2, _PSC),
             halClockApb1TimerFreqGet(&halClockConfig) / 10000000 - 1);
    // .1us / tick
    REG_WR32(DRF_REG(_TIM2, _ARR), 19);
    REG_WR32(DRF_REG(_TIM2, _CNT), 0);
    REG_WR32(DRF_REG(_TIM2, _CR1), DRF_DEF(_TIM2, _CR1, _OPM, _ENABLED));
    REG_WR32(DRF_REG(_TIM2, _DIER), DRF_DEF(_TIM2, _DIER, _UIE, _ENABLED));

    // schedule steps on different timer
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _TIM7EN, _ENABLED);
    stepperExecutionTimerCfg.clkDomainFrequency = halClockApb1TimerFreqGet(&halClockConfig);
    stepperExecutionTimerCfg.interruptEnable = true;
    halTimerConstruct(&stepperExecutionTimer, DRF_BASE(DRF_TIM7));
    halTimerStart(&stepperExecutionTimer, &stepperExecutionTimerCfg);

    halIrqPrioritySet(IRQ_TIM7, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_TIM7);
    halTimerStartContinous(&stepperExecutionTimer, 1);
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);

    // Enable AHB Peripherials
    uint32_t ahb1enr = REG_RD32(DRF_REG(_RCC, _AHB1ENR));
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOAEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOBEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOCEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIODEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOEEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOFEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOGEN, _ENABLED, ahb1enr);
    REG_WR32(DRF_REG(_RCC, _AHB1ENR), ahb1enr);

    // Enable APB1 Peripherials
    uint32_t apb1enr = REG_RD32(DRF_REG(_RCC, _APB1ENR));
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _USART3EN, _ENABLED, apb1enr);
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM2EN, _ENABLED, apb1enr);
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM3EN, _ENABLED, apb1enr);
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM5EN, _ENABLED, apb1enr);
    REG_WR32(DRF_REG(_RCC, _APB1ENR), apb1enr);

    uint32_t apb2enr = REG_RD32(DRF_REG(_RCC, _APB2ENR));
    apb2enr = FLD_SET_DRF(_RCC, _APB2ENR, _USART1EN, _ENABLED, apb2enr);
    REG_WR32(DRF_REG(_RCC, _APB2ENR), apb2enr);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    for (uint8_t i = 0; i < NR_STEPPER; ++i) {
        halGpioSetMode(step[i],
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                           DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
        halGpioSetMode(en[i],
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                           DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
        halGpioSetMode(dir[i],
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                           DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    }

    for (uint8_t i = 0; i < NR_AXIS; ++i) {
        halGpioSetMode(endstops[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT));
    }

    halGpioSetMode(dispSpiSwCs,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(dispSpiSwMosi,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(dispSpiSwSclk,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));

    halTimerConstruct(&wallClockTimer, DRF_BASE(DRF_TIM5));
    halTimerStart(&wallClockTimer, &(struct TimerConfig){
        .timerTargetFrequency = 1000000,
        .clkDomainFrequency = halClockApb1TimerFreqGet(&halClockConfig),
        .interruptEnable = true
    });

    communicationInit();
    thermalInit();

    // Start Wallclock
    halIrqPrioritySet(IRQ_TIM5, configKERNEL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_TIM5);
    halTimerStartContinous(&wallClockTimer, 10000);
}

void platformPostInit(void)
{
    setupTimer();
    communicationPostInit();
    thermalPostInit();

    spiInit(&dispSpiSw, dispSpiSwSclk, dispSpiSwCs, dispSpiSwMosi);
    st7920Init(&st7920, &dispSpiSw);
    uiSt7920Init(&st7920);
}

volatile static uint64_t wallClockTimeUs = 0;
IRQ_HANDLER_TIM5(void)
{
    wallClockTimeUs += 10000;
    halTimerIrqClear(&wallClockTimer);
    halIrqClear(IRQ_TIM5);
}

uint64_t platformGetTimeUs(void)
{
    taskENTER_CRITICAL();
    uint64_t time = wallClockTimeUs + halTimerGetCount(&wallClockTimer);
    taskEXIT_CRITICAL();
    return time;
}

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

const uint32_t platformGetStepperTimerFreq(void)
{
    return stepperFrequency;
}

__attribute__((always_inline)) inline bool platformGetEndstop(uint8_t axis)
{
    return !halGpioRead(endstops[axis]);
}
