#pragma once
#include "hal/gpio.h"


/* please don't access state */
struct BLTouch {
    struct IOLine pwm;
    struct IOLine input;
};

void bltouchInit(struct BLTouch *pDev, struct IOLine pwm, struct IOLine input);