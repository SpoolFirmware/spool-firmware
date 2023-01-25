#include "error.h"

_Noreturn __attribute__((noinline)) void __panic(int line) {
  for (volatile int i = line;; i = line)
    (void)i;
}
