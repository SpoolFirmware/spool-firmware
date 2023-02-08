//
// Created by codetector on 1/14/23.
//
#include <stdint.h>
#include "fix16.h"

#include "FreeRTOS.h"
#include "task.h"
#include "dbgprintf.h"
#include "misc.h"

#include "platform/platform.h"

#include "thermal/thermal.h"

#include "step_schedule.h"
#include "step_plan_ng.h"
#include "step_execute.h"
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

/* Timer Task Support */
static StaticTask_t TimerTaskTCBBuffer;
static StackType_t TimerTaskStack[configTIMER_TASK_STACK_DEPTH];
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer, uint32_t *pulTimerTaskStackSize)
{
    *ppxTimerTaskTCBBuffer = &TimerTaskTCBBuffer;
    *ppxTimerTaskStackBuffer = TimerTaskStack;
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void main(void)
{
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);
    platformDisableStepper(0xFF);
    /* unused */
    TaskHandle_t _gcodeSerial, _stepSchedule;
    QueueHandle_t gcodeCommandQueue = gcodeSerialInit(&_gcodeSerial);
    QueueHandle_t stepperJobQueue =
        stepTaskInit(gcodeCommandQueue, &_stepSchedule);
    initPlanner();
    stepExecuteSetQueue(stepperJobQueue);
    dbgPrintf("initSpoolApp\n");
    platformPostInit();

    // Create the highest priority "printf" task
    configASSERT(xTaskCreate(DebugPrintTask, "dbgPrintf",
                             configMINIMAL_STACK_SIZE, NULL,
                             configMAX_PRIORITIES - 1, &dbgPrintTaskHandle));
    thermalManagerStart();

    vTaskStartScheduler();
    for (;;) {
    }
}
