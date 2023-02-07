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

#include "../thermal/thermistor/thermistors.h"

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
    const struct ThermistorTable *pTable = &t100k_4k7_table;
    (void)pvParameters;
    for (;;) {
        vTaskDelay(100);
        uint32_t rawTemp = platformReadTemp(0);
        int tempC = -99;

        uint16_t scaledTemp = (uint16_t)((rawTemp * pTable->rawMax) / 4096);
        int lowerBound = 0;

        for (int i = 0; i < pTable->numEntries - 1; i++) {
            if (pTable->pTable[i].rawValue > scaledTemp) {
                break;
            }
            lowerBound = i;
        }

        const int diffDegC = pTable->pTable[lowerBound + 1].degC -
                             pTable->pTable[lowerBound].degC;
        const uint16_t diffRaw = pTable->pTable[lowerBound + 1].rawValue -
                                 pTable->pTable[lowerBound].rawValue;

        int measuredOffset = scaledTemp - pTable->pTable[lowerBound].rawValue;

        tempC = pTable->pTable[lowerBound].degC +
                (int)(measuredOffset * diffDegC / diffRaw);

        dbgPrintf("temp(%d) = %d -> %d\n", lowerBound, rawTemp, tempC);
        if (tempC < 0) {
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
