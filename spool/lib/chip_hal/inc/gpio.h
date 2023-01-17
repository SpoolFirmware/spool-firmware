#pragma once
#include <stdint.h>

struct IOLine {
    uint32_t group;
    uint32_t pin;
};
typedef uint32_t GPIOMode;

inline struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin) __attribute__((always_inline));
void chipHalGpioSetMode(struct IOLine line, GPIOMode mode);
inline void chipHalGpioSet(struct IOLine line) __attribute__((always_inline));
inline void chipHalGpioClear(struct IOLine line) __attribute__((always_inline));
