#pragma once

#include <stdint.h>

typedef uint32_t uint32_mhz_t;

struct ChipHalClockConfig;

void chipHalClockInit(struct ChipHalClockConfig *config);
