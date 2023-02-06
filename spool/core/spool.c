//
// Created by codetector on 1/14/23.
//
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "dbgprintf.h"
#include "error.h"

#include "step_schedule.h"
#include "step_execute.h"
#include "platform/platform.h"
#include "hal/stm32/hal.h"
#include "gcode_serial.h"

void dbgEmptyBuffer(void)
{
    int c;
    while ((c = dbgGetc()) > 0)
        platformDbgPutc((char)c);
}

TaskHandle_t dbgPrintTaskHandle = NULL;
static portTASK_FUNCTION_PROTO(DebugPrintTask, pvParameters);
static portTASK_FUNCTION(DebugPrintTask, pvParameters)
{
    (void)pvParameters;

    int c;
    for (;;) {
        if ((c = dbgGetc()) > 0) {
            if (c == '\n')
                platformDbgPutc('\r');
            platformDbgPutc((char)c);
        } else {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

static portTASK_FUNCTION_PROTO(TestTask, pvParameters);
static portTASK_FUNCTION(TestTask, pvParameters)
{
    (void)pvParameters;
    for (;;) {
        vTaskDelay(100);
        /* uint16_t temp = platformReadTemp(0); */
        /* dbgPrintf("temp = %d\n", temp); */
    }
}

void main(void)
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    platformDisableStepper(0xFF);
    QueueHandle_t gcodeCommandQueue = gcodeSerialInit();
    QueueHandle_t stepperJobQueue = stepTaskInit(gcodeCommandQueue);
    stepExecuteSetQueue(stepperJobQueue);
    dbgPrintf("initSpoolApp\n");
    platformPostInit();

    // Create the highest priority "printf" task
    configASSERT(xTaskCreate(DebugPrintTask, "dbgPrintf",
                             configMINIMAL_STACK_SIZE, NULL,
                             configMAX_PRIORITIES - 1, &dbgPrintTaskHandle));
    xTaskCreate(TestTask, "testTask", 256, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
