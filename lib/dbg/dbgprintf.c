#include "dbgprintf.h"
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"
#include "task.h"

#define BUFFER_SIZE 256

static char print_buffer[BUFFER_SIZE] = { 0 };
static size_t head = 0, tail = 0;

extern TaskHandle_t dbgPrintTaskHandle;

void dbgPutc(const char c)
{
    uint8_t signalTask = 0;

    if (c == '\n') {
        dbgPutc('\r');
    }

    uint32_t basePri = ulPortRaiseBASEPRI();
    if (head == tail)
        signalTask = 1;
    if ((tail + 1) % BUFFER_SIZE != head) {
        print_buffer[tail] = c;
        __asm volatile("dsb");
        tail = (tail + 1) % BUFFER_SIZE;
    }
    vPortSetBASEPRI(basePri);

    if (signalTask && dbgPrintTaskHandle != NULL) {
        if (xPortIsInsideInterrupt() == pdTRUE) {
            vTaskNotifyGiveFromISR(dbgPrintTaskHandle, NULL);
        } else {
            xTaskNotifyGive(dbgPrintTaskHandle);
        }
    }
}

void dbgPuts(const char *c)
{
    while ((*c != '\0')) {
        dbgPutc(*c++);
    }
}

int dbgGetc(void)
{
    if (head != tail) {
        char c = print_buffer[head];
        __asm volatile("dsb");
        head = (head + 1) % BUFFER_SIZE;
        return c;
    }
    return -1;
}
