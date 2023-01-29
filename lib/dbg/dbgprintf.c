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
        print_buffer[tail++] = c;
    }
    portEXIT_CRITICAL();
}

void dbgPuts(const char *c)
{
    portENTER_CRITICAL();
    while((*c) != '\0' && (tail + 1) % BUFFER_SIZE != head) {
        print_buffer[tail++] = *c++;
    }
    portEXIT_CRITICAL();
}

int dbgGetc(void)
{
    if (head != tail) {
        char c = print_buffer[head];
        head++;
        return c;
    }
    return -1;
}
