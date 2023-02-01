#include <stddef.h>
#include "drf.h"
#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"
#include "manual/irq.h"
#include "manual/mcu.h"
#include "stream_buffer.h"

#include "FreeRTOS.h"

// const static struct HalClockConfig halClockConfig = {
//     .hseFreqHz = 25000000,
//     .q = 7,
//     .p = 4,
//     .n = 336,
//     .m = 25,

//     .apb2Prescaler = 1,
//     .apb1Prescaler = 2,
//     .ahbPrescaler = 1,
// };

const static struct IOLine statusLED = { .group = DRF_BASE(DRF_GPIOA), .pin = 13 };

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return 0;
}


void platformPostInit(void)
{
}

void enableStepper(uint8_t stepperMask)
{
}

void platformInit(struct PlatformConfig *config)
{
    // halClockInit(&halClockConfig);

}

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

_Noreturn void __panic(const char *file, int line) {
    for (volatile int i = line;; i = line);
}

void platformDbgPutc(char c)
{
    (void)c;
}
