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

#include "thermal/thermal.h"
#include "core/spool.h"
#include "gcode/gcode.h"

typedef struct {
    // Target State
    fix16_t setPoint;
    // Configuration State
    fix16_t kp;
    fix16_t ki;
    fix16_t kd;

    fix16_t outputMin;
    fix16_t outputMax;
    fix16_t effectiveRange;

    // Internal State
    fix16_t maxOverKi;
    fix16_t iState;
    fix16_t lastInput;

    fix16_t inputHistory[20];
    uint8_t head;
} pid_t;

static void thermalCallback(TimerHandle_t timerHandle);

void pidInit(pid_t *pPid)
{
    if (pPid->outputMax == 0)
        panic();

    if (pPid->effectiveRange == 0)
        pPid->effectiveRange = F16(-1);

    if (pPid->ki != 0) {
        pPid->maxOverKi =
            fix16_sub(fix16_div(pPid->outputMax, pPid->ki), pPid->outputMin);
    }
    memset(&pPid->inputHistory, 0, sizeof(pPid->inputHistory));
}

void pidReset(pid_t *pPid)
{
    pPid->iState = 0;
}

bool pidStable(pid_t *pPid)
{
    const fix16_t setPoint = pPid->setPoint;
    for (int i = 0; i < ARRAY_LENGTH(pPid->inputHistory); i++) {
        if (fix16_abs(fix16_sub(pPid->inputHistory[i], setPoint)) > F16(0.5)) {
            return false;
        }
    }
    return true;
}

fix16_t pidUpdateLoop(pid_t *pPid, fix16_t input)
{
    fix16_t error = fix16_sub(pPid->setPoint, input);
    fix16_t dInput;
    fix16_t output;

    if (fix16_abs(error) > pPid->effectiveRange) {
        pidReset(pPid);
        pPid->lastInput = input;
        if (error > F16(0)) {
            return pPid->outputMax;
        } else {
            return pPid->outputMin;
        }
    }

    fix16_t oldInput = pPid->inputHistory[pPid->head];
    pPid->inputHistory[pPid->head] = input;
    pPid->head = (pPid->head + 1) % ARRAY_LENGTH(pPid->inputHistory);
    dInput = fix16_sub(input, oldInput);

    // Cap OutputSum
    pPid->iState = fix16_clamp(fix16_add(pPid->iState, error), -pPid->maxOverKi,
                               pPid->maxOverKi);

    const fix16_t dVal = fix16_mul(pPid->kd, dInput);
    output = fix16_div(fix16_sub(pPid->setPoint, F16(30)), F16(7));
    output = fix16_add(output, fix16_mul(pPid->kp, error)); // P
    output = fix16_add(output, fix16_mul(pPid->ki, pPid->iState)); // I
    output = fix16_sub(output, dVal); // D

    output = fix16_clamp(output, pPid->outputMin, pPid->outputMax);

    // Kd? lol not happening rn
    pPid->lastInput = input;
    return output;
}

// 24V
SemaphoreHandle_t pidMutex;
pid_t myPid = {
    .setPoint = F16(0),
    .kp = F16(6),
    .ki = F16(0.02),
    .kd = F16(10),

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
    int control = fix16_to_int(pidUpdateLoop(&myPid, tempC_f));
    targetC = fix16_to_int(myPid.setPoint);
    xSemaphoreGive(pidMutex);

    platformSetHeater(0, control);
    if (tempC > 40) {
        platformSetFan(0, 100);
    } else {
        platformSetFan(0, 0);
    }

    // Logging
    static uint8_t log = 0;
    if (log % 16 == 0) {
        dbgPrintf("set=%d, cur=%d out:%d\n", targetC, tempC, control);
    }
    log++;
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
        case GcodeM104:
        case GcodeM109: {
            xSemaphoreTake(pidMutex, portMAX_DELAY);
            if (cmd.temperature.sTemp < F16(230)) {
                myPid.setPoint = cmd.temperature.sTemp;
            }
            xSemaphoreGive(pidMutex);

            if (cmd.kind == GcodeM109) {
                for(;;) {
                    bool tempGood = false;
                    xSemaphoreTake(pidMutex, portMAX_DELAY);
                    tempGood = pidStable(&myPid);
                    xSemaphoreGive(pidMutex);
                    if (tempGood) break;
                    vTaskDelay(pdMS_TO_TICKS(500));
                }
            }

            resp.respKind = ResponseOK;
            xQueueSend(ResponseQueue, &resp, portMAX_DELAY);
        } break;
        default:
            dbgPrintf("EPERM: Thermal\n");
            panic();
            break;
        }
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
    pidInit(&myPid);

    xTimerStart(thermalTimer, 0);

    if (xTaskCreate(ThermalTask, "ThermalTask", 512, NULL, tskIDLE_PRIORITY + 1,
                    &thermalTaskHandle) != pdTRUE) {
        panic();
    }
}
