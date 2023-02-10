#include "error.h"
#include <stdio.h>

_Noreturn void __panic(const char *file, int line, const char *err)
{
    /* TODO disable interrupts */
    printf("PANIC %s in %s:%d\n", err, file, line);
    for (volatile int i = line;; i = line)
        (void)i;
}

void __warn(const char *file, int line, const char *err)
{
    printf("WARN %s in %s:%d\n", err, file, line);
}

void __warn_on_err(const char *file, int line, status_t err)
{
    printf("WARN ERR %d in %s:%d\n", err, file, line);
}

#define dbgPrintf printf
