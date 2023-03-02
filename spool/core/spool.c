//
// Created by codetector on 1/14/23.
//
#include "spool.h"
#include <stdint.h>
#include "fix16.h"

#include "FreeRTOS.h"
#include "task.h"
#include "dbgprintf.h"
#include "misc.h"

#include "platform/platform.h"

#include "thermal/thermal.h"

#include "motion/motion.h"
#include "gcode/gcode_serial.h"
#include "ui/ui.h"

/* ----------- Global Task Input Queues --------------- */
QueueHandle_t ThermalTaskQueue;
QueueHandle_t ResponseQueue;
QueueHandle_t MotionPlannerTaskQueue;
QueueHandle_t StepperExecutionQueue;

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

void vApplicationMallocFailedHook(void)
{
    dbgPrintf("NOMEM, %d free\n", xPortGetFreeHeapSize());
}

void vApplicationDaemonTaskStartupHook(void) {
    // Create Tasks & Setup Queues
    gcodeSerialTaskInit();
    thermalTaskInit();
    motionInit();
    // Inform platform that execution is about to begin
    platformPostInit();

    if (PLATFORM_FEATURE_ENABLED(Display)) {
        uiInit();
    }

    // Disable Allocation at this point
    vPortDisableHeapAllocation();
}

void main(void)
{
    // Platform Init
    struct PlatformConfig platformConfig = { 0 };
    platformInit(&platformConfig);

    platformDisableStepper(0xFF);
    dbgPrintf("initSpoolApp\n");

    // Create the task that should handle prints
    configASSERT(xTaskCreate(DebugPrintTask, "dbgPrintf",
                             configMINIMAL_STACK_SIZE, NULL,
                             configMAX_PRIORITIES - 1, &dbgPrintTaskHandle));

    vTaskStartScheduler();

    // NO RETURN
    for (;;)
        ;
}
