//
// Created by codetector on 1/14/23.
//
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "step_schedule.h"
#include "step_execute.h"
#include "platform/platform.h"
#include "hal/stm32/hal.h"
#include "spool_config.h"

static portTASK_FUNCTION_PROTO(vLEDFlashTask1, pvParameters);
static portTASK_FUNCTION(vLEDFlashTask1, pvParameters)
{
    (void)pvParameters;
    struct IOLine led1 = platformGetStatusLED();

    for (;;) {
        // halGpioToggle(led1);
        // vTaskDelay(25);
    }
}

void main()
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    QueueHandle_t queue = stepTaskInit();
    stepExecuteSetQueue(queue);
    platformPostInit();

    xTaskCreate(vLEDFlashTask1, "LED", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY + 1, (TaskHandle_t *)NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
