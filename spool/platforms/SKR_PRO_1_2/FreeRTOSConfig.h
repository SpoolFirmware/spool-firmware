#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

/* Here is a good place to include header files that are required across
your application. */

#define configCPU_CLOCK_HZ                      168000000
#define configSYSTICK_CLOCK_HZ                  (configCPU_CLOCK_HZ / 8)

/* A header file that defines trace macro can be included here. */
/* Definitions that map the FreeRTOS port interrupt handlers to their CMSIS
standard names. */
#define vPortSVCHandler SVC_Handler
#define xPortPendSVHandler PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#include "FreeRTOSCoreConfig.h"

#endif /* FREERTOS_CONFIG_H */
