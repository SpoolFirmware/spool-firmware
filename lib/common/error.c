#include "error.h"
#include "dbgprintf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

_Noreturn void __panic(const char *file, int line, const char* err) 
{
    __asm volatile("cpsid i");
    dbgPrintf("PANIC %s at %s:%d\n", err, file, line);
    dbgEmptyBuffer();
    for (volatile int i = line;; i = line);
}

#pragma GCC diagnostic pop
