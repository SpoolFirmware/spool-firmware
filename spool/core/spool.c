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

#include "lib/sdspi/sd_spi.h"

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

// TODO REMOVE v

uint32_t sd_spi_millis(void)
{
    return (uint32_t)(platformGetTimeUs() / 1000);
}

void sd_spi_set_cs(uint8_t high)
{
    if (high) {
        halGpioSet(sdCSPin);
    } else { 
        halGpioClear(sdCSPin);
    }
}

void sd_spi_begin_transaction(uint32_t transfer_speed_hz)
{
    // NOOP
}

void sd_spi_end_transaction(void)
{
    // NOOP?
}

void sd_spi_send_bytes(const uint8_t *bytes, uint32_t len)
{
    halSpiSend(platformGetSDSPI(), bytes, len);
}

void sd_spi_send_byte(uint8_t byte)
{
    halSpiSend(platformGetSDSPI(), &byte, 1);
}

uint8_t sd_spi_receive_byte(void)
{
    uint8_t byte = 0;
    halSpiXchg(platformGetSDSPI(), NULL, &byte, 1);
    return byte;
}

void sd_spi_receive_bytes(uint8_t *bytes, uint32_t len)
{
    halSpiXchg(platformGetSDSPI(), NULL, bytes, len);
}

void hexdump(const void *buffer, size_t size) {
    const unsigned char *p = buffer;
    size_t i, j;

    for (i = 0; i < size; i += 16) {
        dbgPrintf("%06zx: ", i);
        for (j = 0; j < 16; j++) {
            if (i + j < size) {
                dbgPrintf("%02x ", p[i + j]);
            } else {
                dbgPrintf("   ");
            }
        }
        dbgPrintf("\n");
    }
}

// TODO REMOVE ^

static uint8_t buffer[512];
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
    if (platformGetSDSPI()) {
        int8_t status;
        status = sd_spi_init();
        dbgPrintf("sd_spi_init = %d\n", status);
        if (status == SD_ERR_OK) {
            sd_spi_read(0, buffer, 512, 0);
            hexdump(buffer, 512);
        }
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
