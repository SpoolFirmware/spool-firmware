#pragma once

#include <stdint.h>
#include "gpio.h"
#include "error.h"

#define HAL_EXTI_NR_GROUPS 16

struct ExtiDriver;

struct ExtiConfig {
	uint32_t groupNum;
	void (*handler)(struct ExtiDriver *);
};

uint32_t halExtiGroupNumFromGpio(struct IOLine line);
void halExtiInit(struct ExtiDriver *drv, const struct ExtiConfig *config);

status_t halExtiEnable(struct ExtiDriver *drv);
void halExtiDisable(struct ExtiDriver *drv);
