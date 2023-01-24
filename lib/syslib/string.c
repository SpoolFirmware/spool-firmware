#include "string.h"

void* memset(void * m, int c, size_t n)
{
    char *s = (char *) m;
    while (n--)
        *s++ = (char) c;
    return m;
}

void *memcpy(void *dst, const void *src, size_t n) {
    char *dst1 = dst;
    const char *src1 = src;
    while (n--)
        *dst1++ = *src1++;
    return dst;
}