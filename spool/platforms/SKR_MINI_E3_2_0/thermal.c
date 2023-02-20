#include "config_private.h"
#include "lib/hal/common/adc.h"
#include "platform/platform.h"
#include "lib/thermistor/thermistors.h"

static struct ADCConfig adcCfg;
static struct ADCDriver adc;

const static struct IOLine tBedPin = {.group = DRF_BASE(DRF_GPIOC), .pin = 3};
const static struct IOLine tH0Pin = {.group = DRF_BASE(DRF_GPIOA), .pin = 0};

const static uint32_t BedADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC1) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 13);
const static uint32_t TH0ADCStream = DRF_DEF(_HAL_ADC, _STREAM, _ADC, _ADC1) |
                                     DRF_NUM(_HAL_ADC, _STREAM, _CHN, 0);

void privThermalInit(void)
{
    halGpioSetMode(tBedPin, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) | 
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ANALOG)); // Bed
    halGpioSetMode(tH0Pin, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _INPUT) | 
        DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _ANALOG)); // Hotend
    
    halADCInit(&adc, &adcCfg);
}

void privThermalPostInit(void)
{
    halADCStart(&adc);
    REG_FLD_SET_DRF(_ADC1, _SMPR2, _SMP0, _CYCLES71_5);
}

fix16_t platformReadTemp(int8_t idx)
{
    uint16_t u16Val = 0;
    switch(idx) {
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
}

void platformSetFan(int8_t idx, uint8_t pwm)
{
}