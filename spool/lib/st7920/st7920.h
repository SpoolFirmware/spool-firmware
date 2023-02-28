#pragma once
#include "hal/hal.h"

enum St7920State {
    Undef,
    Initialized,
};

enum St7920RS {
    InstructionWrite = 0,
    DataWrite,
};

struct St7920 {
    struct IOLine D4;
    struct IOLine D5;
    struct IOLine D6;
    struct IOLine D7;

    struct IOLine RS;
    struct IOLine E;

    enum St7920State state;
};

void st7920Init(struct St7920 *drv);