#include "platform_private.h"
#include "lib/thermistor/thermistors.h"
#include "semphr.h"
#include "error.h"

/* ---------------------- GPIO Lines ---------------------------------------- */
const static struct IOLine th0 = { .group = DRF_BASE(DRF_GPIOF),
                                   .pin = 3 }; // BED
const static struct IOLine th1 = { .group = DRF_BASE(DRF_GPIOF),
                                   .pin = 4 }; // E0

const static struct IOLine heater0 = { .group = DRF_BASE(DRF_GPIOB), .pin = 1 }; // E0
const static struct IOLine bedHeater = { .group = DRF_BASE(DRF_GPIOD), .pin = 12 }; // E0
const static struct IOLine extruderFan = { .group = DRF_BASE(DRF_GPIOC), .pin = 8 }; // E0 Fan
const static struct IOLine fan0 = { .group = DRF_BASE(DRF_GPIOE), .pin = 5 }; // Part Fan

/* ---------------------- HAL Drivers & Configurations --------------------- */
struct TimerConfig pwmTimer1Cfg = {
    .timerTargetFrequency = 1000,
    .clkDomainFrequency = 0,
    .interruptEnable = false,
};
struct TimerDriver pwmTimer1;

struct TimerConfig pwmTimer4Cfg = {
    .timerTargetFrequency = 1000,
    .clkDomainFrequency = 0,
    .interruptEnable = false,
};
struct TimerDriver pwmTimer4;

static struct ADCDriver adc;
static struct ADCConfig adcCfg = { .adcClkMaxFreq = 32000000 };
/* ------------------ Private Variables ------------------------------------- */
const static uint32_t BedADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC3) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 9);
const static uint32_t TH0ADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC3) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 14);

/* -------------- Function Prototypes --------------------------------------- */
/**
 * @brief Configures and enables the timers that drive all PWM signals
 */
static void s_configurePWMTimer(void);

void thermalInit(void)
{
    // ADC Setup
    halGpioSetMode(th0, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _ANALOG)); // Bed TH0
    halGpioSetMode(th1,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _ANALOG)); // Hotend TH1
    adcCfg.adcParentClockSpeed = halClockApb2FreqGet(&halClockConfig);
    halADCInit(&adc, &adcCfg);

    halGpioSetMode(heater0,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 1)); // AF1 for TIM1
    halGpioSetMode(bedHeater,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 2)); // AF2 for TIM4
    // TODO: PWM
    halGpioSetMode(extruderFan,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                       DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                       DRF_NUM(_HAL_GPIO, _MODE, _AF, 1));
    halGpioSetMode(fan0, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                             DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                             DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                             DRF_NUM(_HAL_GPIO, _MODE, _AF, 1));
}

static void s_configurePWMTimer(void)
{
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _TIM1EN, _ENABLED);
    halTimerConstruct(&pwmTimer1, DRF_BASE(DRF_TIM1));
    pwmTimer1Cfg.clkDomainFrequency = halClockApb2TimerFreqGet(&halClockConfig);
    halTimerStart(&pwmTimer1, &pwmTimer1Cfg);

    REG_WR32(DRF_REG(_TIM1, _CCER), 0);
    REG_WR32(DRF_REG(_TIM1, _CCMR2_OUTPUT),
             DRF_DEF(_TIM1, _CCMR2_OUTPUT, _OC3M, _PWM_MODE1) |
                 DRF_DEF(_TIM1, _CCMR2_OUTPUT, _OC3PE, _ENABLED));
    REG_WR32(DRF_REG(_TIM1, _CCER), DRF_DEF(_TIM1, _CCER, _CC3NE, _SET));
    REG_WR32(DRF_REG(_TIM1, _CCR3), 0);
    REG_WR32(DRF_REG(_TIM1, _BDTR), DRF_DEF(_TIM1, _BDTR, _MOE, _ENABLED));
    halTimerStartContinous(&pwmTimer1, 100 - 1);

    REG_FLD_SET_DRF(_RCC, _APB1ENR, _TIM4EN, _ENABLED);
    halTimerConstruct(&pwmTimer4, DRF_BASE(DRF_TIM4));
    pwmTimer4Cfg.clkDomainFrequency = halClockApb1TimerFreqGet(&halClockConfig);
    halTimerStart(&pwmTimer4, &pwmTimer4Cfg);

    REG_WR32(DRF_REG(_TIM4, _CCER), 0);
    REG_WR32(DRF_REG(_TIM4, _CCMR1_OUTPUT),
             DRF_DEF(_TIM4, _CCMR1_OUTPUT, _OC1M, _PWM_MODE1) |
                 DRF_DEF(_TIM4, _CCMR1_OUTPUT, _OC1PE, _ENABLED));
    REG_WR32(DRF_REG(_TIM4, _CCER), DRF_DEF(_TIM4, _CCER, _CC1E, _SET));
    REG_WR32(DRF_REG(_TIM4, _CCR3), 0);
    halTimerStartContinous(&pwmTimer4, 100 - 1);
}

void thermalPostInit(void)
{
    halADCStart(&adc);
    s_configurePWMTimer();
}

fix16_t platformReadTemp(int8_t idx)
{
    uint16_t u16Value = 0;
    if (idx == -1) {
        u16Value = halADCConvertSingle(&adc, BedADCStream);
    } else if (idx == 0) {
        u16Value = halADCConvertSingle(&adc, TH0ADCStream);
    } else {
        panic();
    }
    return thermistorEvaulate(&t100k_4k7_table, u16Value);
}

void platformSetHeater(int8_t idx, uint8_t pwm)
{
    if (idx == 0) {
        REG_WR32(DRF_REG(_TIM1, _CCR3), pwm);
    } else if (idx == -1) { // BED
        REG_WR32(DRF_REG(_TIM4, _CCR1), pwm);
    } else {
        panic();
    }
}

void platformSetFan(int8_t idx, uint8_t pwm)
{
    struct IOLine fan = {};
    switch (idx) {
    case -1:
        fan = extruderFan;
        break;
    case 0:
        fan = fan0;
        break;
    default:
        panic();
        return;
    }
    if (pwm) {
        halGpioSet(fan);
    } else {
        halGpioClear(fan);
    }
}
