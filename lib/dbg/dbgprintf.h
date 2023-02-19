#pragma once

#include <stdarg.h>

void dbgPutc(const char);
void dbgPuts(const char *c);
int dbgGetc(void);

void dbgEmptyBuffer(void);

#define dbgPrintf printf_
int printf_(const char* format, ...);

//#define dbgPrintf(LEVEL, fmt...)
