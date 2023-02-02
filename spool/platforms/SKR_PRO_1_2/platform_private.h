#pragma once

#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"
#include "manual/mcu.h"
#include "manual/irq.h"

extern const struct HalClockConfig halClockConfig;

void communicationInit(void);
void communicationPostInit(void);
