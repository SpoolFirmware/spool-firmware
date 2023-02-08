#pragma once
#include <stdbool.h>
#include <stdint.h>

#define min(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b;      \
    })

#define max(a, b)               \
    ({                          \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b;      \
    })

#define abs(a)                  \
    ({                          \
        __typeof__(a) _a = (a); \
        _a >= 0 ? _a : -_a;     \
    })

static inline bool int32_less_abs(int32_t a, int32_t b)
{
    return abs(a) < abs(b);
}

static inline int32_t int32_copy_sign(int32_t dst, int32_t src)
{
    if (src >= 0) {
        if (dst >= 0) {
            return dst;
        } else {
            return -dst;
        }
    } else {
        if (dst < 0) {
            return dst;
        } else {
            return -dst;
        }
    }
}

static inline int int32_same_sign(int32_t a, int32_t b)
{
    if (a >= 0)
        return b >= 0;
    else
        return b < 0;
}