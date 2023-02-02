#include "error.h"
#include "dbgprintf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"
_Noreturn void __panic(const char *file, int line) 
{
    dbgPrintf("PANIC %s:%d\n", file, line);
    dbgEmptyBuffer();
    for (volatile int i = line;; i = line);
}
#pragma GCC diagnostic pop