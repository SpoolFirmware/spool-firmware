//
// Created by codetector on 1/14/23.
//
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "dbgprintf.h"

#include "step_schedule.h"
#include "step_execute.h"
#include "platform/platform.h"
#include "hal/stm32/hal.h"
#include "spool_config.h"

extern struct UARTDriver printUart;

static portTASK_FUNCTION_PROTO(DebugPrintTask, pvParameters);
static portTASK_FUNCTION(DebugPrintTask, pvParameters)
{
    (void)pvParameters;

    int c;
    for (;;) {
        if ((c = dbgGetc()) > 0) {
            halUartSendByte(&printUart, (uint8_t)(char)c);
        } else {
            vTaskDelay(0);
        }
    }
}

void main(void)
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    QueueHandle_t queue = stepTaskInit();
    stepExecuteSetQueue(queue);
    platformPostInit();

    xTaskCreate(DebugPrintTask, "dbgPrintf", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY + 1, (TaskHandle_t *)NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
