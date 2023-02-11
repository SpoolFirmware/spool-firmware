#pragma once

typedef enum {
    StatusOk = 0,
    StatusInvalidClockConfig,
    StatusReadErr,
    StatusGcodeTooLong,
    StatusUnsupportedChar,
    StatusInvalidGcodeToken,
    StatusInvalidGcodeMissingToken,
    StatusInvalidGcodeDuplicateAxis,
    StatusInvalidGcodeCommand,
    StatusUnimplementedGcodeCommand,
    StatusGcodeEof,
    StatusGcodeParserLogicError,
    StatusAgain,
} status_t;

#define STATUS_OK(x)  ((x) == StatusOk)
#define STATUS_ERR(x) ((x) != StatusOk)

#define _ASSIGN_OR_STATEMENT(x, expr, stmt) \
    if ((x = (expr)) != StatusOk) {         \
        stmt;                               \
    } else {                                \
    }

#define _DEFINE_OR_STATEMENT(x, expr, stmt) \
    status_t x = (expr);                    \
    if (x != StatusOk) {                    \
        stmt;                               \
    } else {                                \
    }

#define _ASSERT_OR_STATEMENT(expr, stmt) \
    do {                                 \
        status_t _err = (expr);          \
        if (_err != StatusOk)            \
            stmt;                        \
    } while (0)

_Noreturn void __panic(const char *file, int line, const char *err);

void __warn(const char *file, int line, const char *err);

void __warn_on_err(const char *file, int line, status_t err);

#define ASSERT_OR_GOTO(expr, l) _ASSERT_OR_STATEMENT(expr, goto l)

#define ASSIGN_OR_GOTO(x, expr, l) _ASSIGN_OR_STATEMENT(x, expr, goto l)

#define DEFINE_OR_GOTO(x, expr, l) _DEFINE_OR_STATEMENT(x, expr, goto l)

#define ASSERT_OR_RETURN(expr) _ASSERT_OR_STATEMENT(expr, return _err)

#define ASSERT_OR_PANIC(expr) _ASSERT_OR_STATEMENT(expr, panic())

#define ASSIGN_OR_RETURN(x, expr) _ASSIGN_OR_STATEMENT(x, expr, return x)

#define DEFINE_OR_RETURN(x, expr) _DEFINE_OR_STATEMENT(x, expr, return x)

#define panic() __panic(__FILE__, __LINE__, "")

#define UNIMPLEMENTED(x) __panic(__FILE__, __LINE__, "unimplemented " #x)

#define WARN() __warn(__FILE__, __LINE__, "")

#define WARN_ON(x)                          \
    do {                                    \
        if (x)                              \
            __warn(__FILE__, __LINE__, #x); \
    } while (0)

#define WARN_ON_ERR(x)                               \
    do {                                             \
        status_t _err = (x);                         \
        if (_err != StatusOk)                        \
            __warn_on_err(__FILE__, __LINE__, _err); \
    } while (0)
