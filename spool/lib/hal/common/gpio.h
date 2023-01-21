#pragma once
#include <stdint.h>

struct IOLine {
    uint32_t group;
    uint32_t pin;
};
typedef uint32_t GPIOMode;

struct IOLine halGpioLineConstruct(uint32_t group, uint32_t pin);
void halGpioSetMode(struct IOLine line, GPIOMode mode);
void halGpioSet(struct IOLine line);
void halGpioClear(struct IOLine line);
