#pragma once
#include <stdint.h>

struct IOLine {
    uint32_t group;
    uint32_t pin;
};
typedef uint32_t GPIOMode;

struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin);
void chipHalGpioSetMode(struct IOLine line, GPIOMode mode);
void chipHalGpioSet(struct IOLine line);
void chipHalGpioClear(struct IOLine line);
