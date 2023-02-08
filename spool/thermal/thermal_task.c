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
} pid_t;

portTASK_FUNCTION(ThermalTask, pvParameters)
{
    (void)pvParameters;
    for (;;) {
        vTaskDelay(MS2T(100));
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
