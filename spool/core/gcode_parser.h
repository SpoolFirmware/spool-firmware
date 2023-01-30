#pragma once
#include "fix16.h"
#include "error.h"

/* sign, 5 char decimal, decimal point, 5 char fraction, NULL */
#define MAX_NUM_LEN 13

enum GcodeKind {
    GcodeG0,
    GcodeG1,
};

struct GcodeXY {
    /* Only support x and y */
    fix16_t x, y;
};

struct GcodeCommand {
    enum GcodeKind kind;
    union {
        struct GcodeXY xy;
    };
};

status_t parseGcode(struct GcodeCommand *cmd);
status_t receiveChar(char *c);

/* resets parser state, need to call initParser again */
void resetParser(void);
/* required before parsing, blocks if there are no characters */
status_t initParser(void);
