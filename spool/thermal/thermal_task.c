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
    .kp = F16(4),
    .ki = F16(0.01),
    .kd = F16(60),

    .m = F16(1.0 / 7),
    .b = F16(-30),

    .outputMin = F16(0),
    .outputMax = F16(100),
    .effectiveRange = F16(20),
};

pid_t bedPid = {
    .setPoint = F16(0),
    .kp = F16(40),
    .ki = F16(0.1),
    .kd = F16(40),

    .m = F16(1.0 / 0.837),
    .b = F16(-11.6),

    .outputMin = F16(0),
    .outputMax = F16(100),
    .effectiveRange = F16(10),
};

static void thermalCallback(TimerHandle_t timerHandle)
{
    fix16_t tempC_f = 0;
    fix16_t tempCBed_f = 0;
    int targetCBed, targetCE;

    for (int i = 0; i < 6; i++) {
        const fix16_t reading = platformReadTemp(0);
        tempC_f = fix16_add(tempC_f, reading);
    }
    tempC_f = fix16_div(tempC_f, F16(6));
    int tempCExtruder = fix16_to_int(tempC_f);

    for (int i = 0; i < 6; i++) {
        const fix16_t reading = platformReadTemp(-1);
        tempCBed_f = fix16_add(tempCBed_f, reading);
    }
    tempCBed_f = fix16_div(tempCBed_f, F16(6));
    int tempCBed = fix16_to_int(tempCBed_f);

    int eControl, bControl;

    if (xSemaphoreTake(pidMutex, pdMS_TO_TICKS(50)) == pdTRUE) {
        eControl = fix16_to_int(pidUpdateLoop(&e0Pid, tempC_f));
        targetCE = fix16_to_int(e0Pid.setPoint);

        bControl = fix16_to_int(pidUpdateLoop(&bedPid, tempCBed_f));
        targetCBed = fix16_to_int(bedPid.setPoint);
        xSemaphoreGive(pidMutex);
    } else {
        platformSetHeater(0, 0);
        platformSetHeater(-1, 0);
        panic();
    }

    // Apply Control
    platformSetHeater(0, eControl);
    platformSetHeater(-1, bControl);

    // Extruder Fan
    if (tempCExtruder > 50) {
        platformSetFan(-1, 100);
    } else if (tempCExtruder < 40) {
        platformSetFan(-1, 0);
    }

    // Logging
    static uint8_t log = 0;
    if (log == 16) {
        log = 0;
        dbgPrintf("s=%d c=%d o=%d; B:s=%d c=%d o=%d\n", targetCE, tempCExtruder,
                 eControl, targetCBed, tempCBed, bControl);
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

static void sSetBedTemperature(fix16_t newTemp, bool wait)
{
    xSemaphoreTake(pidMutex, portMAX_DELAY);
    if (newTemp < F16(80)) {
        bedPid.setPoint = newTemp;
    }
    xSemaphoreGive(pidMutex);

    if (wait) {
        for (;;) {
            vTaskDelay(pdMS_TO_TICKS(1000));
            if (pidStable(&bedPid))
                break;
        }
    }
}

void thermalGetTempReport(struct TemperatureReport *pReport)
{
    fix16_t e0TempF16 = -1;
    fix16_t bedTempF16 = -1;

    xSemaphoreTake(pidMutex, portMAX_DELAY);
    e0TempF16 = e0Pid.inputHistory[e0Pid.head];
    bedTempF16 = bedPid.inputHistory[bedPid.head];
    xSemaphoreGive(pidMutex);
    pReport->extrudersTarget[0] = fix16_to_int(e0Pid.setPoint);
    pReport->extruders[0] = fix16_to_int(e0TempF16);
    pReport->bedTarget = fix16_to_int(bedPid.setPoint);
    pReport->bed = fix16_to_int(bedTempF16);
}

portTASK_FUNCTION(ThermalTask, args)
{
    (void)args;
    struct GcodeCommand cmd;
    struct GcodeResponse resp;
    for (;;) {
        xQueueReceive(ThermalTaskQueue, &cmd, portMAX_DELAY);
        memset(&resp, 0, sizeof(resp));

        // Default responds ResponseOK
        resp.respKind = ResponseOK;
        switch (cmd.kind) {
        case GcodeM107:
            cmd.fan.speed = 0;
            // fall-through
        case GcodeM106:
            platformSetFan(0, fix16_to_int(cmd.fan.speed));
            break;
        case GcodeM108:
            sSetHotendTemperature(F16(0));
            break;
        case GcodeM140:
            sSetBedTemperature(cmd.temperature.sTemp, false);
            break;
        case GcodeM190:
            sSetBedTemperature(cmd.temperature.sTemp, true);
            break;
        case GcodeM104:
        case GcodeM109: // Set Temp / Set Temp and Wait
            sSetHotendTemperature(cmd.temperature.sTemp);
            if (cmd.kind == GcodeM109) {
                for (;;) {
                    bool tempGood = false;
                    xSemaphoreTake(pidMutex, portMAX_DELAY);
                    tempGood = pidStable(&e0Pid);
                    xSemaphoreGive(pidMutex);
                    if (tempGood)
                        break;
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }
            break;
        case GcodeM105: // Get Temp
        {
            resp.respKind = ResponseTemp;
            thermalGetTempReport(&resp.tempReport);
        } break;
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
