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
            platformDbgPutc((char)c);
        } else {
            ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        }
    }
}

void main(void)
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    QueueHandle_t gcodeCommandQueue = gcodeSerialInit();
    QueueHandle_t stepperJobQueue = stepTaskInit(gcodeCommandQueue);
    stepExecuteSetQueue(stepperJobQueue);
    dbgPrintf("initSpoolApp\n");
    platformPostInit();

    // Create the highest priority "printf" task
    configASSERT(xTaskCreate(DebugPrintTask, "dbgPrintf",
                             configMINIMAL_STACK_SIZE, NULL,
                             configMAX_PRIORITIES - 1, &dbgPrintTaskHandle));

    vTaskStartScheduler();
    for (;;) {
    }
}
