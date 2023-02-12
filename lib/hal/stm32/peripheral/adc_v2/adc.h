#pragma once
#include <stdint.h>
#include "hal/adc.h"

struct ADCConfig {
    uint32_t adcParentClockSpeed;
    uint32_t adcClkMaxFreq;
};

struct ADCDriver {
    uint32_t commonCCR;
};
