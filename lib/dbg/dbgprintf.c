#include "dbgprintf.h"
#include <stdint.h>
#include <stddef.h>
#include "FreeRTOS.h"

#define BUFFER_SIZE 256

static char print_buffer[BUFFER_SIZE] = {0};
static size_t head = 0, tail = 0;

void dbgPutc(const char c)
{
    portENTER_CRITICAL();
    if ((tail + 1) % BUFFER_SIZE != head) {
        print_buffer[tail] = c;
        __asm__("dsb");
        tail = (tail + 1) % BUFFER_SIZE;
    }
    portEXIT_CRITICAL();
}

void dbgPuts(const char *c)
{
    portENTER_CRITICAL();
    while((*c) != '\0' && (tail + 1) % BUFFER_SIZE != head) {
        print_buffer[tail] = *c++;
        __asm__("dsb");
        tail = (tail + 1) % BUFFER_SIZE;
    }
    portEXIT_CRITICAL();
}

int dbgGetc(void)
{
    if (head != tail) {
        char c = print_buffer[head];
        __asm__("dsb");
        head = (head + 1) % BUFFER_SIZE;
        return c;
    }
    return -1;
}
