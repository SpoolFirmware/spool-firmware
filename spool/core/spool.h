#pragma once

#include "misc.h"
#include "FreeRTOS.h"
#include "task.h"

/**
 * @brief return the numver of millis seconds since boot.
 * 
 * @note This is limited by the tick resolution.
 * 
 * @return uint32_t ms since boot.
 */
static uint32_t taskGetMillis(void)
{
    const uint32_t ticks = xTaskGetTickCount();

    #if configTICK_RATE_HZ > 1000
        return ticks / (configTICK_RATE_HZ / 1000);
    #else
        return ticks * (1000 / configTICK_RATE_HZ);
    #endif
}

#define MS2T(ms)    ((int)((configTICK_RATE_HZ / 1000.0) * (ms)))
