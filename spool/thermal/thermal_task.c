#include "FreeRTOS.h"
#include "task.h"
#include "misc.h"
#include "dbgprintf.h"

#include "thermal/thermal.h"
#include "fix16.h"
#include "platform/platform.h"
#include "core/spool.h"

typedef struct {
    // Target State
    fix16_t setPoint;
    // Configuration State
    fix16_t kp;
    fix16_t ki;
    fix16_t kd;

    fix16_t outputMin;
    fix16_t outputMax;

    // Internal State
    fix16_t lastInput;
    fix16_t outputSum;
} pid_t;

fix16_t updateLoop(pid_t *pPid, fix16_t input)
{
    fix16_t error = fix16_sub(pPid->setPoint, input);
    fix16_t dInput = fix16_sub(input, pPid->lastInput);
    fix16_t output;

    pPid->outputSum = fix16_add(pPid->outputSum, fix16_mul(pPid->ki, error));
    
    // P on Observed Rate of Change
    pPid->outputSum = fix16_sub(pPid->outputSum, fix16_mul(pPid->kp, dInput));

    // Cap OutputSum
    pPid->outputSum = fix16_clamp(pPid->outputSum, pPid->outputMin, pPid->outputMax);

    output = pPid->outputSum;
    output = fix16_sub(output, fix16_mul(pPid->kd, dInput));
    // Kd? lol not happening rn
    pPid->lastInput = input;
    return output;
}

pid_t myPid = {
    .setPoint = F16(40),
    .kp = F16(15),
    .ki = F16(0),
    .kd = F16(0),

    .outputMin = F16(0),
    .outputMax = F16(100),
};

portTASK_FUNCTION(ThermalTask, pvParameters)
{
    (void)pvParameters;
    for (;;) {
        vTaskDelay(MS2T(100));
        fix16_t tempC_f = platformReadTemp(0);
        int tempC = fix16_to_int(tempC_f);

        int control = fix16_to_int(updateLoop(&myPid, tempC_f));

        dbgPrintf("temp = %d action: %d\n", tempC, control);
        platformSetHeater(0, control);
    }
}
