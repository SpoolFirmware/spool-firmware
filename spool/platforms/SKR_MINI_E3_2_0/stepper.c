#include "config_private.h"

const size_t NR_STEPPERS = 2;

const struct {
    struct IOLine en;
    struct IOLine step;
    struct IOLine dir;
    bool invertEn;
    bool invertDir;
} steppers[] = {
    {
        .en = { .group = DRF_BASE(DRF_GPIOB), .pin = 14 },
        .step = { .group = DRF_BASE(DRF_GPIOB), .pin = 13 },
        .dir = { .group = DRF_BASE(DRF_GPIOB), .pin = 12 },
        .invertEn = true,
        .invertDir = false,
    },
    {
        .en = { .group = DRF_BASE(DRF_GPIOB), .pin = 11 },
        .step = { .group = DRF_BASE(DRF_GPIOB), .pin = 10 },
        .dir = { .group = DRF_BASE(DRF_GPIOB), .pin = 2 },
        .invertEn = true,
        .invertDir = false,
    },
};

struct UARTDriver stepperUart = { 0 };
const static struct UARTConfig stepperUartCfg = {
    .baudrate = 115200,
    .useRxInterrupt = false,
    .useTxInterrupt = false,
    .useTx = true,
    .useRx = true,
};

static void sSetupStepperTimers(void)
{
    // Setup TIMER 6 with 2x
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _TIM6EN, _ENABLED);
    REG_WR32(DRF_REG(_TIM6, _PSC),
             halClockApb1TimerFreqGet(&halClockConfig) / 10000000 - 1);
    REG_WR32(DRF_REG(_TIM6, _ARR), 19);
    REG_WR32(DRF_REG(_TIM6, _CR1), 0);
    REG_WR32(DRF_REG(_TIM6, _CNT), 0);
    REG_WR32(DRF_REG(_TIM6, _CR1), DRF_DEF(_TIM6, _CR1, _OPM, _ENABLED));
    REG_WR32(DRF_REG(_TIM6, _DIER), DRF_DEF(_TIM6, _DIER, _UIE, _ENABLED));
    halIrqPrioritySet(IRQ_TIM6, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_TIM6);

    // Step Executor Timer
    REG_FLD_SET_DRF(_RCC, _APB1ENR, _TIM7EN, _ENABLED);
    halIrqPrioritySet(IRQ_TIM7, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_TIM7);

    /* actual PSC is value + 1 */
    REG_WR32(DRF_REG(_TIM7, _PSC), (halClockApb1TimerFreqGet(&halClockConfig) /
                                    (platformGetStepperTimerFreq() * 2)) -
                                       1);
    REG_WR32(DRF_REG(_TIM7, _ARR), 1);
    REG_WR32(DRF_REG(_TIM7, _CNT), 0);
    REG_WR32(DRF_REG(_TIM7, _DIER), DRF_DEF(_TIM7, _DIER, _UIE, _ENABLED));
    REG_WR32(DRF_REG(_TIM7, _CR1), 0);
}

void privStepperInit(void)
{
    // Configure GPIO Pins
    platformDisableStepper(0xFF);
    for (size_t i = 0; i < ARRAY_LENGTH(steppers); i++) {
        halGpioSetMode(steppers[i].en,
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSH_PULL));
        halGpioSetMode(steppers[i].step,
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSH_PULL));
        halGpioSetMode(steppers[i].dir,
                       DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
                           DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSH_PULL));
    }
    sSetupStepperTimers();

    REG_FLD_SET_DRF(_RCC, _APB1ENR, _UART4EN, _ENABLED);
    halUartInit(&stepperUart, &stepperUartCfg, DRF_BASE(DRF_UART4),
                halClockApb1FreqGet(&halClockConfig));
}

IRQ_HANDLER_TIM7(void)
{
    static uint32_t lastCntrReload = 0;
    size_t loopCntr = 0;
    if (FLD_TEST_DRF(_TIM7, _SR, _UIF, _UPDATE_PENDING,
                     REG_RD32(DRF_REG(_TIM7, _SR)))) {
stepperTimerHandler_begin:
        REG_WR32(DRF_REG(_TIM7, _SR),
                 ~DRF_DEF(_TIM7, _SR, _UIF, _UPDATE_PENDING));
        /* execute current job */
        uint32_t requestedTicks = executeStep((lastCntrReload + 1) / 2);
        uint32_t cntrReload = 2 * requestedTicks;
        if (cntrReload == 0)
            cntrReload = 1;
        if ((lastCntrReload = REG_RD32(DRF_REG(_TIM7, _CNT))) > cntrReload) {
            REG_WR32(DRF_REG(_TIM7, _CNT), 0);
            if ((++loopCntr) < 4) {
                goto stepperTimerHandler_begin;
            }
        }
        lastCntrReload = cntrReload;
        REG_WR32(DRF_REG(_TIM7, _ARR), cntrReload);
    }
    halIrqClear(IRQ_TIM7);
}

void platformEnableStepper(uint8_t stepperMask)
{
    for (size_t i = 0; i < ARRAY_LENGTH(steppers); i++) {
        if (stepperMask & BIT(i)) {
            if (steppers[i].invertEn) {
                halGpioClear(steppers[i].en);
            } else {
                halGpioSet(steppers[i].en);
            }
        }
    }
    REG_WR32(DRF_REG(_TIM7, _CR1), DRF_DEF(_TIM7, _CR1, _CEN, _ENABLED));
}

void platformDisableStepper(uint8_t stepperMask)
{
    for (size_t i = 0; i < ARRAY_LENGTH(steppers); i++) {
        if (stepperMask & BIT(i)) {
            if (steppers[i].invertEn) {
                halGpioSet(steppers[i].en);
            } else {
                halGpioClear(steppers[i].en);
            }
        }
    }
}

void platformSetStepperDir(uint8_t dirMask)
{
    for (uint8_t i = 0; i < ARRAY_LENGTH(steppers); ++i) {
        if (((dirMask & BIT(i)) == 0) ^ steppers[i].invertDir) {
            halGpioSet(steppers[i].dir);
        } else {
            halGpioClear(steppers[i].dir);
        }
    }
}

static void setStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioSet(steppers[i].step);
        }
    }
}

static void clrStepper(uint8_t stepperMask)
{
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        if (stepperMask & BIT(i)) {
            halGpioClear(steppers[i].step);
        }
    }
}

void platformStepStepper(uint8_t stepperMask)
{
    setStepper(stepperMask);
    REG_WR32(DRF_REG(_TIM6, _CR1), DRF_DEF(_TIM6, _CR1, _CEN, _ENABLED) |
                                       DRF_DEF(_TIM6, _CR1, _OPM, _ENABLED));
}

IRQ_HANDLER_TIM6(void)
{
    if (FLD_TEST_DRF(_TIM6, _SR, _UIF, _UPDATE_PENDING,
                     REG_RD32(DRF_REG(_TIM6, _SR)))) {
        REG_WR32(DRF_REG(_TIM6, _SR),
                 ~DRF_DEF(_TIM6, _SR, _UIF, _UPDATE_PENDING));
        clrStepper(0xFF);
    }
    halIrqClear(IRQ_TIM6);
}

uint32_t platformGetStepperTimerFreq(void)
{
    return 50000;
}
