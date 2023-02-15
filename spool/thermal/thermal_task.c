#include "string.h"
#include "misc.h"
#include "dbgprintf.h"
#include "platform/platform.h"

#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "fix16.h"

#include "semphr.h"

#include "thermal/pid.h"
#include "thermal/thermal.h"
#include "core/spool.h"
#include "gcode/gcode.h"

static void thermalCallback(TimerHandle_t timerHandle);

SemaphoreHandle_t pidMutex;
pid_t e0Pid = {
    .setPoint = F16(0),
    .kp = F16(6),
    .ki = F16(0.02),
    .kd = F16(30),

    .m = F16(1.0 / 7),
    .b = F16(-30),

    .outputMin = F16(0),
    .outputMax = F16(100),
    .effectiveRange = F16(20),
};

static void thermalCallback(TimerHandle_t timerHandle)
{
    fix16_t tempC_f = 0;
    for (int i = 0; i < 6; i++) {
        const fix16_t reading = platformReadTemp(0);
        tempC_f = fix16_add(tempC_f, reading);
    }
    tempC_f = fix16_div(tempC_f, F16(6));
    int tempC = fix16_to_int(tempC_f);
    int targetC;

    xSemaphoreTake(pidMutex, pdMS_TO_TICKS(50));
    int control = fix16_to_int(pidUpdateLoop(&e0Pid, tempC_f));
    targetC = fix16_to_int(e0Pid.setPoint);
    xSemaphoreGive(pidMutex);

    platformSetHeater(0, control);
    if (tempC > 50) {
        platformSetFan(-1, 100);
    } else if (tempC < 40) {
        platformSetFan(-1, 0);
    }

    // Logging
    static uint8_t log = 0;
    if (log == 16) {
        log = 0;
        dbgPrintf("set=%d, cur=%d out:%d\n", targetC, tempC, control);
    }
    log++;
}

static void sSetHotendTemperature(fix16_t newTemp) 
{
    xSemaphoreTake(pidMutex, portMAX_DELAY);
    if (newTemp < F16(230)) {
        e0Pid.setPoint = newTemp;
    }
    xSemaphoreGive(pidMutex);
}

portTASK_FUNCTION(ThermalTask, args)
{
    (void)args;
    struct GcodeCommand cmd;
    struct GcodeResponse resp;
    for (;;) {
        xQueueReceive(ThermalTaskQueue, &cmd, portMAX_DELAY);
        memset(&resp, 0, sizeof(resp));

        switch (cmd.kind) {
        case GcodeM107:
            cmd.fan.speed = 0;
            // fall-through
        case GcodeM106:
            platformSetFan(0, fix16_to_int(cmd.fan.speed));
            resp.respKind = ResponseOK;
            break;
        case GcodeM108:
            sSetHotendTemperature(F16(0));
            resp.respKind = ResponseOK;
            break;
        case GcodeM104:
        case GcodeM109: // Set Temp / Set Temp and Wait
            sSetHotendTemperature(cmd.temperature.sTemp);
            if (cmd.kind == GcodeM109) {
                for(;;) {
                    bool tempGood = false;
                    xSemaphoreTake(pidMutex, portMAX_DELAY);
                    tempGood = pidStable(&e0Pid);
                    xSemaphoreGive(pidMutex);
                    if (tempGood) break;
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }

            resp.respKind = ResponseOK;
            break;
        case GcodeM105: // Get Temp
            fix16_t e0TempF16 = -1;
            xSemaphoreTake(pidMutex, portMAX_DELAY);
            e0TempF16 = e0Pid.inputHistory[e0Pid.head];
            xSemaphoreGive(pidMutex);
            resp.respKind = ResponseTemp;
            resp.tempReport.extruders[0] = fix16_to_int(e0TempF16);
            resp.tempReport.bed = fix16_to_int(platformReadTemp(-1));
            break;
        default:
            dbgPrintf("EPERM: Thermal\n");
            panic();
            break;
        }

        xQueueSend(ResponseQueue, &resp, portMAX_DELAY);
    }
}

void thermalTaskInit(void)
{
    TimerHandle_t thermalTimer;
    TaskHandle_t thermalTaskHandle;
    if ((thermalTimer = xTimerCreate("thermalManager", pdMS_TO_TICKS(50),
                                     pdTRUE, NULL, thermalCallback)) == NULL) {
        panic();
    }
    ThermalTaskQueue =
        xQueueCreate(THERMAL_TASK_QUEUE_SIZE, sizeof(struct GcodeCommand));
    if (ThermalTaskQueue == NULL) {
        panic();
    }

    pidMutex = xSemaphoreCreateMutex();
    if (pidMutex == NULL) {
        panic();
    }
    pidInit(&e0Pid);

    xTimerStart(thermalTimer, 0);

    if (xTaskCreate(ThermalTask, "ThermalTask", 512, NULL, tskIDLE_PRIORITY + 1,
                    &thermalTaskHandle) != pdTRUE) {
        panic();
    }
}
