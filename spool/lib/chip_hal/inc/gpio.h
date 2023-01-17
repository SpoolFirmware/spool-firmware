#pragma once
#include <stdint.h>

struct IOLine {
    uint32_t group;
    uint32_t pin;
};

inline struct IOLine chipHalGpioLineConstruct(uint32_t group, uint32_t pin);
inline void chipHalGpioSet(struct IOLine line);
inline void chipHalGpioClear(struct IOLine line);
