#include "error.h"
#include "dbgprintf.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-but-set-variable"

extern void safeShutdown(void);

_Noreturn void __panic(const char *file, int line, const char* err) 
{
    __asm volatile("cpsid i");
    safeShutdown();
    dbgPrintf("PANIC");
	if (err)
		dbgPrintf(" %s", err);
    if (file)
        dbgPrintf(" at %s:%d\n", file, line);
    dbgEmptyBuffer();
    for (volatile int i = line;; i = line);
}

void __warn(const char *file, int line, const char *err)
{
    dbgPrintf("WARN %s in %s:%d\n", err, file, line);
}

void __warn_on_err(const char *file, int line, status_t err)
{
    dbgPrintf("WARN ERR %d in %s:%d\n", err, file, line);
}

#pragma GCC diagnostic pop
