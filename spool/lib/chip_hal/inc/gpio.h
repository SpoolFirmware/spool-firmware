#pragma once
#include <stdint.h>

struct IOLine {
    uint32_t group;
    uint32_t pin;
};

struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin) __attribute__((always_inline));
void chipHalGpioSet(struct IOLine line) __attribute__((always_inline));
void chipHalGpioClear(struct IOLine line) __attribute__((always_inline));
