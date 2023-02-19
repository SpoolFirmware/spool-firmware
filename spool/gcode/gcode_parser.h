#pragma once
#include <stdbool.h>
#include "fix16.h"
#include "error.h"
#include "gcode/gcode.h"

/* sign, 5 char decimal, decimal point, 5 char fraction, NULL */
#define MAX_NUM_LEN 13

enum TokenKind {
    TokenUndef = 0,
    TokenE,
    TokenF,
    TokenG,
    TokenM,
    TokenN,
    TokenR,
    TokenS,
    TokenT,
    TokenX,
    TokenY,
    TokenZ,
    TokenFix16,
    TokenNewline,
    TokenInt32,
};

struct Token {
    enum TokenKind kind;
    union {
        fix16_t fix16;
        int32_t int32;
    };
};

struct Tokenizer {
    char currChar;
    bool hasCurrChar;
    char scratchBuf[MAX_NUM_LEN];
};

struct GcodeParser {
    struct Token currToken;
    struct Tokenizer tokenizer;
};

status_t parseGcode(struct GcodeParser *s, struct GcodeCommand *cmd);
/* on EOF, return StatusGcodeEof, 
 * ensure !(return == StatusOk && *c == '\0')
 */
status_t receiveChar(char *c);

/* required before parsing */
status_t initParser(struct GcodeParser *s);
