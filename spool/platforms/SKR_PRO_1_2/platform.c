#include "FreeRTOS.h"
#include "platform_private.h"
#include "bitops.h"

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

// const static struct IOLine statusLED = { .group = GPIOA, .pin = 7 };
const static struct IOLine statusLED = { .group = GPIOE, .pin = 0 };

const static struct IOLine heater0 = { .group = GPIOB, .pin = 1 };
const static struct IOLine fan0 = { .group = GPIOC, .pin = 8 };

struct TimerConfig pwmTimerCfg = {
    .timerTargetFrequency = 1000,
    .clkDomainFrequency = 0,
    .interruptEnable = false,
};
struct TimerDriver pwmTimer0 = { 0 };

#define NR_STEPPERS 3

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)
#define STEPPER_C BIT(2)

#define NR_AXES 3

const static struct IOLine endstops[NR_AXES] = {
    { .group = GPIOB, .pin = 10 }, /* X */
    { .group = GPIOE, .pin = 12 }, /* Y */
    { .group = DRF_BASE(DRF_GPIOG), .pin = 8 }, /* Z */
};

const static struct IOLine step[NR_STEPPERS] = {
    /* X */
    { .group = GPIOE, .pin = 9 },
    /* Y */
    { .group = GPIOE, .pin = 11 },
    /* Z */
    { .group = GPIOE, .pin = 13 },
};

const static struct IOLine dir[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 1 },
    /* Y */
    { .group = GPIOE, .pin = 8 },
    /* Z */
    { .group = GPIOC, .pin = 2 },
};

const static struct IOLine en[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 2 },
    /* Y */
    { .group = GPIOD, .pin = 7 },
    /* Z */
    { .group = GPIOC, .pin = 0 },
};

void platformEnableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioClear(en[i]);
        }
    }
}

void platformDisableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioSet(en[i]);
        }
    }
}

void platformSetStepperDir(uint8_t dirMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (dirMask & BIT(i)) {
            halGpioSet(dir[i]);
        } else {
            halGpioClear(dir[i]);
        }
    }
}

static void setStepper(uint8_t stepperMask, uint8_t stepMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
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
IRQ_HANDLER_TIM3(void)
{
    if (FLD_TEST_DRF(_TIM3, _SR, _UIF, _UPDATE_PENDING,
                     REG_RD32(DRF_REG(_TIM3, _SR)))) {
VectorB4_begin:
        REG_WR32(DRF_REG(_TIM3, _SR),
                 ~DRF_DEF(_TIM3, _SR, _UIF, _UPDATE_PENDING));
        /* execute current job */
        uint32_t requestedTicks = executeStep((lastCntrReload + 1) / 2);
        uint32_t cntrReload = 2 * requestedTicks;
        if (cntrReload == 0)
            cntrReload = 1;
        if ((lastCntrReload = REG_RD32(DRF_REG(_TIM3, _CNT))) > cntrReload) {
            REG_WR32(DRF_REG(_TIM3, _CNT), 0);
            goto VectorB4_begin;
        }
        lastCntrReload = cntrReload;
        REG_WR32(DRF_REG(_TIM3, _ARR), cntrReload);
    }
    halIrqClear(IRQ_TIM3);
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
        setStepper(STEPPER_A | STEPPER_B | STEPPER_C, 0);
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
    halIrqPrioritySet(IRQ_TIM3, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_TIM3);

    // Enable Counter
    /* TIM3 runs off APB1, which runs at 42, but 84 due to x2 in the clock graph??? */
    /* actual PSC is value + 1 */
    REG_WR32(DRF_REG(_TIM3, _PSC), halClockApb1TimerFreqGet(&halClockConfig) /
                                           platformGetStepperTimerFreq() / 2 -
                                       1);
    REG_WR32(DRF_REG(_TIM3, _ARR), 1);
    REG_WR32(DRF_REG(_TIM3, _CNT), 0);
    REG_WR32(DRF_REG(_TIM3, _DIER), DRF_DEF(_TIM3, _DIER, _UIE, _ENABLED));
    REG_WR32(DRF_REG(_TIM3, _CR1), DRF_DEF(_TIM3, _CR1, _CEN, _ENABLED));
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
    REG_WR32(DRF_REG(_RCC, _APB1ENR), apb1enr);

    uint32_t apb2enr = REG_RD32(DRF_REG(_RCC, _APB2ENR));
    apb2enr = FLD_SET_DRF(_RCC, _APB2ENR, _USART1EN, _ENABLED, apb2enr);
    apb2enr = FLD_SET_DRF(_RCC, _APB2ENR, _TIM1EN, _ENABLED, apb2enr);
    REG_WR32(DRF_REG(_RCC, _APB2ENR), apb2enr);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
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

    for (uint8_t i = 0; i < NR_AXES; ++i) {
        halGpioSetMode(endstops[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT));
    }

    /* TODO-codetector Cleanup */
    halGpioSetMode(heater0, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                                DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                                DRF_NUM(_HAL_GPIO, _MODE, _AF, 1));
    halGpioSetMode(fan0, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                                DRF_NUM(_HAL_GPIO, _MODE, _AF, 1));
    halTimerConstruct(&pwmTimer0, DRF_BASE(DRF_TIM1));
    pwmTimerCfg.clkDomainFrequency = halClockApb2TimerFreqGet(&halClockConfig);
    halTimerStart(&pwmTimer0, &pwmTimerCfg);

    REG_WR32(DRF_REG(_TIM1, _CCER), 0);
    REG_WR32(DRF_REG(_TIM1, _CCMR2_OUTPUT),
             DRF_DEF(_TIM1, _CCMR2_OUTPUT, _OC3M, _PWM_MODE1) |
             DRF_DEF(_TIM1, _CCMR2_OUTPUT, _OC3PE, _ENABLED));
    REG_WR32(DRF_REG(_TIM1, _CCER),
             /* DRF_DEF(_TIM1, _CCER, _CC3NP, _SET) | */
             DRF_DEF(_TIM1, _CCER, _CC3NE, _SET));
    REG_WR32(DRF_REG(_TIM1, _CCR3), 0);
    REG_WR32(DRF_REG(_TIM1, _BDTR), DRF_DEF(_TIM1, _BDTR, _MOE, _ENABLED));
    halTimerStartContinous(&pwmTimer0, 100-1);
    /* TODO ^ END */

    communicationInit();
    thermalInit();
}

void platformPostInit(void)
{
    setupTimer();
    communicationPostInit();
    thermalPostInit();
}

void platformSetHeater(uint8_t idx, uint8_t pwm)
{
    REG_WR32(DRF_REG(_TIM1, _CCR3), pwm);
}

void platformSetFan(uint8_t idx, uint8_t pwm)
{
    if (pwm) {
        halGpioSet(fan0);
    } else {
        halGpioClear(fan0);
    }
}

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

uint32_t platformGetStepperTimerFreq(void)
{
    return 100000;
}

__attribute__((always_inline)) inline bool platformGetEndstop(uint8_t axis)
{
    return !halGpioRead(endstops[axis]);
}
