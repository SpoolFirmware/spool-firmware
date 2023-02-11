#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "fix16.h"

#include "misc.h"
#include "dbgprintf.h"
#include "platform/platform.h"

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
}

void pidReset(pid_t *pPid)
{
    pPid->iState = 0;
}

fix16_t pidUpdateLoop(pid_t *pPid, fix16_t input)
{
    fix16_t error = fix16_sub(pPid->setPoint, input);
    fix16_t dInput;
    fix16_t output;

    if (fix16_abs(error) > pPid->effectiveRange) {
        pidReset(pPid);
        if (error > F16(0)) {
            return pPid->outputMax;
        } else {
            return pPid->outputMin;
        }
    }

    dInput = fix16_sub(input, pPid->lastInput);

    // Cap OutputSum
    pPid->iState =
        fix16_clamp(fix16_add(pPid->iState, error), 0, pPid->maxOverKi);

    output = fix16_mul(pPid->kp, error); // P
    output = fix16_add(output, fix16_mul(pPid->ki, pPid->iState)); // I
    output = fix16_sub(output, fix16_mul(pPid->kd, dInput)); // D

    output = fix16_clamp(output, pPid->outputMin, pPid->outputMax);

    // Kd? lol not happening rn
    pPid->lastInput = input;
    return output;
}

// 24V
pid_t myPid = {
    .setPoint = F16(10),
    .kp = F16(35),
    .ki = F16(4.3),
    .kd = F16(75),

    .outputMin = F16(0),
    .outputMax = F16(100),
    .effectiveRange = F16(20),
};

static void thermalCallback(TimerHandle_t timerHandle)
{
    vTaskDelay(pdMS_TO_TICKS(50));
    fix16_t tempC_f = 0;
    for (int i = 0; i < 6; i++) {
        const fix16_t reading = platformReadTemp(0);
        tempC_f = fix16_add(tempC_f, reading);
    }
    tempC_f = fix16_div(tempC_f, F16(6));
    int tempC = fix16_to_int(tempC_f);

    int control = fix16_to_int(pidUpdateLoop(&myPid, tempC_f));

    platformSetHeater(0, control);
    if (tempC > 40) {
        platformSetFan(0, 100);
    }
    /* dbgPrintf("temp = %d action: %d\n", tempC, control); */
}

portTASK_FUNCTION(ThermalTask, args)
{
    (void)args;
    for (;;) {
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
    xQueueCreate(8, sizeof(struct GcodeCommand));
    xTimerStart(thermalTimer, 0);
    if (xTaskCreate(ThermalTask, "ThermalTask", 512, NULL,
                               tskIDLE_PRIORITY + 1, &thermalTaskHandle) != pdTRUE)
    {
        panic();
    }

}
