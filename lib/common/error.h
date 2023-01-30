#pragma once

typedef enum {
    StatusOk,
    StatusInvalidClockConfig
} status_t;

#define STATUS_OK(x) ((x) == StatusOk)
#define STATUS_ERR(x) ((x) != StatusOk)

#define _ASSIGN_OR_STATEMENT(x, expr, stmt)                                    \
  if ((x = (expr)) != StatusOk) {                                              \
    stmt;                                                                      \
  } else {                                                                     \
  }

#define _DEFINE_OR_STATEMENT(x, expr, stmt)                                    \
  status_t x = (expr);                                                         \
  if (x != StatusOk) {                                                         \
    stmt;                                                                      \
  } else {                                                                     \
  }

_Noreturn void __panic(const char *file, int line);

#define ASSIGN_OR_GOTO(x, expr, l) _ASSIGN_OR_STATEMENT(x, expr, goto l)

#define DEFINE_OR_GOTO(x, expr, l) _DEFINE_OR_STATEMENT(x, expr, goto l)

#define ASSIGN_OR_RETURN(x, expr) _ASSIGN_OR_STATEMENT(x, expr, return x)

#define DEFINE_OR_RETURN(x, expr) _DEFINE_OR_STATEMENT(x, expr, return x)

#define panic() __panic(__FILE__, __LINE__)

