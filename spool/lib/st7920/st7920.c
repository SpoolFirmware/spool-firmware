#include "st7920.h"
#include "misc.h"
#include "dbgprintf.h"
#include "FreeRTOSConfig.h"

#define MICROSECOND 1000000

static inline void waitCycles(uint32_t us)
{
    uint32_t expiration = (configSYSTICK_CLOCK_HZ / MICROSECOND * us);

}

static void st7920SendNibble(const struct St7920 *drv, uint8_t nib)
{
    if (nib & 0b1) {
        halGpioSet(drv->D4);
    } else {
        halGpioClear(drv->D4);
    }

    if (nib & 0b10) {
        halGpioSet(drv->D5);
    } else {
        halGpioClear(drv->D5);
    }

    if (nib & 0b100) {
        halGpioSet(drv->D6);
    } else {
        halGpioClear(drv->D6);
    }

    if (nib & 0b1000) {
        halGpioSet(drv->D7);
    } else {
        halGpioClear(drv->D7);
    }

    asm("nop");
    halGpioSet(drv->E);

    halGpioClear(drv->E);
}

void st7920Init(struct St7920 *drv)
{
    if (drv->state == Initialized) {
        PR_WARN("st7920 already initialized\n");
    }


}