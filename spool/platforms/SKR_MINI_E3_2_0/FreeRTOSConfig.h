#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Here is a good place to include header files that are required across
your application. */
#define configCPU_CLOCK_HZ                      72000000
#define configSYSTICK_CLOCK_HZ                  (configCPU_CLOCK_HZ / 8)
// #define configTICK_RATE_HZ                      100

#include "FreeRTOSCoreConfig.h"

#endif /* FREERTOS_CONFIG_H */
