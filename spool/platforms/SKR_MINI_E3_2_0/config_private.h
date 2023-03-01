#pragma once

#include <stddef.h>
#include <stdint.h>

#include "misc.h"
#include "dbgprintf.h"

#include "platform/platform.h"

#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"

#include "manual/mcu.h"
#include "manual/irq.h"

/* --------------- Global Variable Declarations ------------------ */
extern const struct HalClockConfig halClockConfig;

/* --------------- Function Prototypes --------------------------- */
void privCommInit(void);
void privCommPostInit(void);
void privStepperInit(void);
void privStepperPostInit(void);
void privThermalInit(void);
void privThermalPostInit(void);

void privTestInit(void);
void privTestPostInit(void);
