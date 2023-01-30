#include <stdint.h>
#include <stdbool.h>
#include "string.h"
#include "gcode_parser.h"

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

static char currChar;
static char scratchBuf[MAX_NUM_LEN];
static struct Token currToken;

/* Tokenizer */

static bool isWhitespace()
{
    return currChar == ' ' || currChar == '\r' || currChar == '\n' ||
           currChar == '\t' || currChar == '\v' || currChar == '\f';
}

static bool isEof()
{
    return currChar == '\0';
}

static status_t skipWhitespaces(void)
{
    status_t err;

    while (isWhitespace()) {
        err = receiveChar(&currChar);
        if (err == StatusGcodeEof) {
            return StatusOk;
        }
        if (STATUS_ERR(err)) {
            return err;
        }
    }

    return StatusOk;
}

static status_t takeUntilWhitespace(void)
{
    status_t err;

    for (unsigned int i = 0; i < MAX_NUM_LEN; ++i) {
        if (isWhitespace() || isEof()) {
            scratchBuf[i] = '\0';
            return StatusOk;
        }
        scratchBuf[i] = currChar;
        err = receiveChar(&currChar);
        if (STATUS_ERR(err) && err != StatusGcodeEof) {
            return err;
        }
    }
    return StatusGcodeTooLong;
}

static status_t dec(struct Token *t)
{
    status_t err;

    err = takeUntilWhitespace();
    if (STATUS_ERR(err)) {
        return err;
    }

    t->kind = TokenFix16;
    t->fix16 = fix16_from_str(scratchBuf);
    return StatusOk;
}

static bool isLetter(void)
{
    return ('a' <= currChar && currChar <= 'z') ||
           ('A' <= currChar && currChar <= 'Z');
}

static void lowerCase(void)
{
    if ('A' <= currChar && currChar <= 'Z') {
        currChar |= 0x60;
    }
}

static status_t letter(struct Token *t)
{
    if (!isLetter()) {
        return StatusInvalidGcodeToken;
    }

    lowerCase();
    switch (currChar) {
    case 'g':
        t->kind = TokenG;
        break;
    case 'x':
        t->kind = TokenX;
        break;
    case 'y':
        t->kind = TokenY;
        break;
    case 'z':
        t->kind = TokenZ;
        break;
    case 'e':
        t->kind = TokenE;
        break;
    case 'f':
        t->kind = TokenF;
        break;
    default:
        return StatusInvalidGcodeToken;
    }
    return receiveChar(&currChar);
}

static status_t tokenize(struct Token *t)
{
    status_t err;

    if (isEof()) {
        return StatusGcodeEof;
    }

    if (isWhitespace()) {
        err = skipWhitespaces();
        if (STATUS_ERR(err)) {
            return err;
        }
    }
    if (('0' <= currChar && currChar <= '9') || currChar == '-') {
        return dec(t);
    }
    return letter(t);
}

static status_t initTokenizer()
{
    return receiveChar(&currChar);
}

/* Parser */

static status_t getCurrToken(void)
{
    if (currToken.kind == TokenUndef) {
        status_t err = tokenize(&currToken);
        return err;
    }
    return StatusOk;
}

static void eatToken(void)
{
    memset(&currToken, 0, sizeof(currToken));
}

static status_t eatFix16(fix16_t *n)
{
    if (currToken.kind != TokenFix16) {
        return StatusInvalidGcodeCommand;
    }
    *n = currToken.fix16;
    memset(&currToken, 0, sizeof(currToken));
    return StatusOk;
}

/* refactor to be less clunky, note this should not get more
 * tokens than what is required. 
 */
static status_t parseXY(struct GcodeCommand *cmd)
{
    status_t err;
    bool xSet = false;
    bool ySet = false;

    while (true) {
        err = getCurrToken();
        if (err == StatusGcodeEof) {
            if (xSet || ySet) {
                return StatusOk;
            } else {
                return StatusInvalidGcodeDuplicateAxis;
            }
        }
        if (STATUS_ERR(err)) {
            return err;
        }

        switch (currToken.kind) {
        case TokenX:
            if (xSet) {
                return StatusInvalidGcodeDuplicateAxis;
            }
            eatToken();
            xSet = true;
            err = getCurrToken();
            if (STATUS_ERR(err)) {
                return StatusInvalidGcodeMissingToken;
            }
            err = eatFix16(&cmd->xy.x);
            break;
        case TokenY:
            if (ySet) {
                return StatusInvalidGcodeDuplicateAxis;
            }
            eatToken();
            ySet = true;
            err = getCurrToken();
            if (STATUS_ERR(err)) {
                return StatusInvalidGcodeMissingToken;
            }
            err = eatFix16(&cmd->xy.y);
            break;
        default:
            if (xSet || ySet) {
                return StatusOk;
            }
            return StatusInvalidGcodeMissingToken;
        }
        if (STATUS_ERR(err)) {
            return err;
        }
        if (xSet && ySet) {
            return StatusOk;
        }
    }
}

static status_t parseCmdG(struct GcodeCommand *cmd)
{
    status_t err;

    memset(cmd, 0, sizeof(*cmd));
    err = getCurrToken();
    if (STATUS_ERR(err)) {
        return StatusInvalidGcodeCommand;
    }
    switch (currToken.kind) {
    case TokenFix16:
        if (!fix16_is_uint(currToken.fix16)) {
            return StatusInvalidGcodeCommand;
        }
        unsigned int cmdNum = (unsigned int)fix16_to_int(currToken.fix16);
        switch (cmdNum) {
        case 0:
            eatToken();
            cmd->kind = GcodeG0;
            return parseXY(cmd);
            break;
        case 1:
            eatToken();
            cmd->kind = GcodeG1;
            return parseXY(cmd);
            break;
        default:
            return StatusInvalidGcodeCommand;
        }
        break;
    default:
        return StatusInvalidGcodeCommand;
    }
}

status_t parseGcode(struct GcodeCommand *cmd)
{
    status_t err = getCurrToken();
    if (STATUS_ERR(err)) {
        return err;
    }

    switch (currToken.kind) {
    case TokenG:
        eatToken();
        return parseCmdG(cmd);
    default:
        return StatusInvalidGcodeCommand;
    }
}

void resetParser(void)
{
    currChar = '\0';
    memset(scratchBuf, 0, MAX_NUM_LEN);
    memset(&currToken, 0, sizeof(currToken));
}

status_t initParser(void)
{
    return initTokenizer();
}
