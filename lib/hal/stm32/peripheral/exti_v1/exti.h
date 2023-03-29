#pragma once

#include "hal/exti.h"
#include <stdbool.h>

struct ExtiDriver {
	struct ExtiConfig config;
	bool enabled;
};
