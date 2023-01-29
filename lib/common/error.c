#include "error.h"
#include "dbgprintf.h"

void empty_buffer(void);

_Noreturn __attribute__((noinline)) void __panic(int line) {
    dbgPrintf("PANIC %d", line);
    empty_buffer();
    for (volatile int i = line;; i = line);
}
