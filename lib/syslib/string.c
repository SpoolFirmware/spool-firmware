#include "string.h"

#pragma GCC push_options
#pragma GCC optimize("O1")
void *memset(void *m, int c, size_t n)
{
    char *s = (char *)m;
    while (n--)
        *s++ = (char)c;
    return m;
}

void *memcpy(void *dst, const void *src, size_t n)
{
    char *dst1 = dst;
    const char *src1 = src;
    while (n--)
        *dst1++ = *src1++;
    return dst;
}

size_t strlen(const char *s)
{
	const char *ss = s;
	while (*ss)
		ss++;
	return ss - s;
}

#pragma GCC pop_options
