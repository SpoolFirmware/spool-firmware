#pragma once
#include <stdint.h>
#include "hal/adc.h"

#define DRF_HAL_ADC_STREAM_ADC      31 : 24
#define DRF_HAL_ADC_STREAM_ADC_ADC1 0U
#define DRF_HAL_ADC_STREAM_ADC_ADC2 1U
#define DRF_HAL_ADC_STREAM_ADC_ADC3 2U
#define DRF_HAL_ADC_STREAM_CHN      23 : 0

struct ADCConfig {
    uint32_t adcParentClockSpeed;
    uint32_t adcClkMaxFreq;
};

struct ADCDriver {
    uint32_t commonCCR;
};
