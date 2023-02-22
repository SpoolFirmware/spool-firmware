#include "config_private.h"
#include "lib/hal/common/adc.h"
#include "lib/hal/common/timer.h"
#include "platform/platform.h"
#include "lib/thermistor/thermistors.h"

/* ------------ HAL Drivers --------------- */
// ADC
static struct ADCConfig adcCfg;
static struct ADCDriver adc;

// TIM8 (PWM for Heater / BED / FAN)
static struct TimerConfig pwmTimerCfg = {
    .timerTargetFrequency = 1000,
    .interruptEnable = false,
};
struct TimerDriver pwmTimer1;

/* ----------- Pin Defines ---------------- */
const static struct IOLine tBedPin = { .group = DRF_BASE(DRF_GPIOC), .pin = 3 };
const static struct IOLine tH0Pin = { .group = DRF_BASE(DRF_GPIOA), .pin = 0 };

const static struct IOLine BedPWMPin = { .group = DRF_BASE(DRF_GPIOC),
                                         .pin = 9 };
const static struct IOLine TH0PWMPin = { .group = DRF_BASE(DRF_GPIOC),
                                         .pin = 8 };
const static struct IOLine FAN1PWMPin = { .group = DRF_BASE(DRF_GPIOC),
                                          .pin = 7 };
const static struct IOLine FAN0PWMPin = { .group = DRF_BASE(DRF_GPIOC),
                                          .pin = 6 };

/* ----------- ADC Streams ---------------- */
const static uint32_t BedADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC1) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 13);
const static uint32_t TH0ADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC1) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 0);

void privThermalInit(void)
{
    // ADC for Temperature Measurement
    halGpioSetMode(tBedPin,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ANALOG)); // Bed
    halGpioSetMode(tH0Pin,
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) |
                       DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ANALOG)); // Hotend

    halADCInit(&adc, &adcCfg);

    // Heater / FAN Setup
    const uint32_t halGpioModeOutputPP =
        DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT50) |
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ALT_PUSH_PULL);
    halGpioSetMode(BedPWMPin, halGpioModeOutputPP);
    halGpioSetMode(TH0PWMPin, halGpioModeOutputPP);
    halGpioSetMode(FAN1PWMPin, halGpioModeOutputPP);
    halGpioSetMode(FAN0PWMPin, halGpioModeOutputPP);

    // TIM8
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _TIM8EN, _ENABLED);
    halTimerConstruct(&pwmTimer1, DRF_BASE(DRF_TIM8));
    pwmTimerCfg.clkDomainFrequency = halClockApb2TimerFreqGet(&halClockConfig);
    halTimerStart(&pwmTimer1, &pwmTimerCfg);

    REG_WR32(DRF_REG(_TIM8, _CCER), 0);
    REG_WR32(DRF_REG(_TIM8, _CCR1), 0);
    REG_WR32(DRF_REG(_TIM8, _CCR2), 0);
    REG_WR32(DRF_REG(_TIM8, _CCR3), 0);
    REG_WR32(DRF_REG(_TIM8, _CCR4), 0);
    REG_WR32(DRF_REG(_TIM8, _CCMR1_OUTPUT),
             DRF_DEF(_TIM8, _CCMR1_OUTPUT, _OC1M, _PWM_MODE1) |
                 DRF_DEF(_TIM8, _CCMR1_OUTPUT, _OC1PE, _ENABLED) |
                 DRF_DEF(_TIM8, _CCMR1_OUTPUT, _OC2M, _PWM_MODE1) |
                 DRF_DEF(_TIM8, _CCMR1_OUTPUT, _OC2PE, _ENABLED));
    REG_WR32(DRF_REG(_TIM8, _CCMR2_OUTPUT),
             DRF_DEF(_TIM8, _CCMR2_OUTPUT, _OC3M, _PWM_MODE1) |
                 DRF_DEF(_TIM8, _CCMR2_OUTPUT, _OC3PE, _ENABLED) |
                 DRF_DEF(_TIM8, _CCMR2_OUTPUT, _OC4M, _PWM_MODE1) |
                 DRF_DEF(_TIM8, _CCMR2_OUTPUT, _OC4PE, _ENABLED));
    REG_WR32(DRF_REG(_TIM8, _CCER), DRF_DEF(_TIM8, _CCER, _CC1E, _SET) |
                                        DRF_DEF(_TIM8, _CCER, _CC2E, _SET) |
                                        DRF_DEF(_TIM8, _CCER, _CC3E, _SET) |
                                        DRF_DEF(_TIM8, _CCER, _CC4E, _SET));
    REG_WR32(DRF_REG(_TIM8, _BDTR), DRF_DEF(_TIM8, _BDTR, _MOE, _ENABLED));
    halTimerStartContinous(&pwmTimer1, 100 - 1);
}

void privThermalPostInit(void)
{
    halADCStart(&adc);
    REG_FLD_SET_DRF(_ADC1, _SMPR2, _SMP0, _CYCLES71_5);
}

fix16_t platformReadTemp(int8_t idx)
{
    uint16_t u16Val = 0;
    switch (idx) {
    case -1:
        u16Val = halADCConvertSingle(&adc, BedADCStream);
        break;
    case 0:
        u16Val = halADCConvertSingle(&adc, TH0ADCStream);
        break;
    default:
        panic();
    }

    return thermistorEvaulate(&t100k_4k7_table, u16Val);
}

void platformSetHeater(int8_t idx, uint8_t pwm)
{   
    if (idx == 0) {
        REG_WR32(DRF_REG(_TIM8, _CCR3), pwm);
    } else if (idx == -1) { // BED
        REG_WR32(DRF_REG(_TIM8, _CCR4), pwm);
    } else {
        panic();
    }
}

void platformSetFan(int8_t idx, uint8_t pwm)
{
    if (idx == 0) {
        REG_WR32(DRF_REG(_TIM8, _CCR1), pwm);
    }
}