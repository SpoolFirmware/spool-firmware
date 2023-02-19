#pragma once

#include "manual/mcu.h"
#include "manual/irq.h"

#if defined(DRF_ADC3)
#define NUM_ADC 3
#elif defined(DRF_ADC2)
#define NUM_ADC 2
#elif defined(DRF_ADC1)
#define NUM_ADC 1
#else
#error ADC object not found in manual
#endif
