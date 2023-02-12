#pragma once
#include "bitops.h"
#include "drf.h"
#include "error.h"

#define ARRAY_LENGTH(array) (sizeof((array)) / sizeof((array)[0]))

#define ROUND_UP(N, S)   ((((N) + (S)-1) / (S)) * (S))
#define ROUND_DOWN(N, S) ((N / S) * S)
