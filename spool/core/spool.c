//
// Created by codetector on 1/14/23.
//
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "step_schedule.h"
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

// static portTASK_FUNCTION_PROTO(motorTask, pvParameters);
// static portTASK_FUNCTION(motorTask, pvParameters)
// {
//     (void)pvParameters;

//     for (;;) {
//         stepX();
//         vTaskDelay(1);
//     }
// }

void main()
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    QueueHandle_t queue = stepTaskInit();
    __platformInitMotor(queue);

    xTaskCreate(vLEDFlashTask1, "LED", configMINIMAL_STACK_SIZE, NULL,
                tskIDLE_PRIORITY + 1, (TaskHandle_t *)NULL);
    xTaskCreate(stepScheduleTask, "stepSchedule", 1024, (void *)queue,
                tskIDLE_PRIORITY + 2, (TaskHandle_t *)NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
