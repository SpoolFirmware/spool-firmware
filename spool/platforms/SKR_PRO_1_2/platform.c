#include "FreeRTOS.h"
#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"
#include "bitops.h"
#include "spool_config.h"
#include "manual/mcu.h"

const static struct HalClockConfig halClockConfig = {
    .hseFreqHz = 8000000,
    .q = 7,
    .p = 2,
    .n = 168,
    .m = 4,

    .apb2Prescaler = 2,
    .apb1Prescaler = 4,
    .ahbPrescaler = 1,
};

struct UARTDriver printUart = {0};

// const static struct IOLine statusLED = { .group = GPIOA, .pin = 7 };
const static struct IOLine statusLED = { .group = GPIOE, .pin = 0 };

#define IRQ_TIM2 28
#define IRQ_TIM3 29
#define NR_STEPPERS 2

#define STEPPER_A BIT(0)
#define STEPPER_B BIT(1)

const static struct IOLine step[NR_STEPPERS] = {
    /* X */
    { .group = GPIOE, .pin = 9 },
    /* Y */
    { .group = GPIOE, .pin = 11 },
};

const static struct IOLine dir[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 1 },
    /* Y */
    { .group = GPIOE, .pin = 8 },
};

const static struct IOLine en[NR_STEPPERS] = {
    /* X */
    { .group = GPIOF, .pin = 2 },
    /* Y */
    { .group = GPIOD, .pin = 7 },
};

void enableStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioClear(en[i]);
        } else {
            halGpioSet(en[i]);
        }
    }
}

void setStepperDir(uint8_t dirMask)
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
void VectorB4() {
VectorB4_begin:
    if(FLD_TEST_DRF(_TIM3, _SR, _UIF, _UPDATE_PENDING, REG_RD32(DRF_REG(_TIM3, _SR)))){
        REG_WR32(DRF_REG(_TIM3, _SR), ~DRF_DEF(_TIM3, _SR, _UIF, _UPDATE_PENDING));
        /* execute current job */
        uint32_t requestedTicks = executeStep((lastCntrReload + 1) / 2);
        uint32_t cntrReload = 2 * requestedTicks;
        if (cntrReload == 0) cntrReload = 1;
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

void stepStepper(uint8_t stepperMask)
{
    setStepper(stepperMask, stepperMask);
    reloadTimer();
}

void VectorB0() {
    if(FLD_TEST_DRF(_TIM2, _SR, _UIF, _UPDATE_PENDING, REG_RD32(DRF_REG(_TIM2, _SR)))){
        REG_WR32(DRF_REG(_TIM2, _SR), ~DRF_DEF(_TIM2, _SR, _UIF, _UPDATE_PENDING));
        setStepper(STEPPER_A | STEPPER_B, 0);
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
    REG_WR32(DRF_REG(_TIM2, _PSC), halClockApb1TimerFreqGet(&halClockConfig) / 10000000 - 1);
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
    REG_WR32(DRF_REG(_TIM3, _PSC), halClockApb1TimerFreqGet(&halClockConfig) / getStepperTimerFreq() / 2 - 1);
    /* 10khz */
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
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOFEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOEEN, _ENABLED, ahb1enr);
    ahb1enr = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIODEN, _ENABLED, ahb1enr);
    REG_WR32(DRF_REG(_RCC, _AHB1ENR), ahb1enr);

    // Enable APB1 Peripherials
    uint32_t apb1enr = REG_RD32(DRF_REG(_RCC, _APB1ENR));
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM2EN, _ENABLED, apb1enr);
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM3EN, _ENABLED, apb1enr);
    REG_WR32(DRF_REG(_RCC, _APB1ENR), apb1enr);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
    halGpioSetMode(step[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                              DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                              DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(en[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                              DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                              DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    halGpioSetMode(dir[i], DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                              DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                              DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH));
    }
}

void platformPostInit()
{
    setupTimer();
}

__attribute__((always_inline)) 
inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

uint32_t getStepperTimerFreq(void)
{
    return 100000;
}
