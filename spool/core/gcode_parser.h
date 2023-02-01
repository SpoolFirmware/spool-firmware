#pragma once
#include <stdbool.h>
#include "fix16.h"
#include "error.h"

/* sign, 5 char decimal, decimal point, 5 char fraction, NULL */
#define MAX_NUM_LEN 13

enum GcodeKind {
    GcodeG0,
    GcodeG1,
    GcodeG28,
};

struct GcodeXYZEF {
    fix16_t x, y, z, e, f;
};

struct GcodeCommand {
    enum GcodeKind kind;
    union {
        struct GcodeXYZEF xyzef;
    };
};

enum TokenKind {
    TokenUndef,
    TokenG,
    TokenX,
    TokenY,
    TokenZ,
    TokenE,
    TokenF,
    TokenFix16,
};

struct Token {
    enum TokenKind kind;
    union {
        fix16_t fix16;
    };
};

struct Tokenizer {
    char currChar;
    bool hasCurrChar;
    char scratchBuf[MAX_NUM_LEN];
};

struct GcodeParser {
    struct Token currToken;
    bool hasCurrToken;
    struct Tokenizer tokenizer;
};

status_t parseGcode(struct GcodeParser *s, struct GcodeCommand *cmd);
/* on EOF, return StatusGcodeEof, 
 * ensure !(return == StatusOk && *c == '\0')
 */
status_t receiveChar(char *c);

/* required before parsing */
status_t initParser(struct GcodeParser *s);
