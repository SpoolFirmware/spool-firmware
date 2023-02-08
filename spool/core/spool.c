//
// Created by codetector on 1/14/23.
//
#include <stdint.h>
#include "fix16.h"

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
        fix16_t tempC_f = platformReadTemp(0);
        int tempC = fix16_to_int(tempC_f);

        dbgPrintf("temp = %d\n", tempC);
        if (tempC_f < F16(40)) {
            platformSetHeater(0, 100);
        } else {
            platformSetHeater(0, 0);
        }
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
    xTaskCreate(TestTask, "testTask", 512, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();
    for (;;) {
    }
}
