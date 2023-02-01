#pragma once

#include <stdint.h>

struct HalClockConfig;

void halClockInit(const struct HalClockConfig *config);
