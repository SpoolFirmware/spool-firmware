#include "error.h"
#include "dbgprintf.h"

void empty_buffer(void);

_Noreturn void __panic(const char *file, int line) {
    dbgPrintf("PANIC %s:%d\n", file, line);
    empty_buffer();
    for (volatile int i = line;; i = line);
}
