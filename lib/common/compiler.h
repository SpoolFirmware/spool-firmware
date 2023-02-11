#pragma once

#define likely(x)   __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

#define barrier() __asm__ __volatile__("" : : : "memory")

#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
