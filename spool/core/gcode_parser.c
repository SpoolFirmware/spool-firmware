#include <stdint.h>
#include "string.h"
#include "gcode_parser.h"

static status_t peekCurrChar(struct Tokenizer *s, char *out)
{
    if (s->hasCurrChar) {
        *out = s->currChar;
        return StatusOk;
    }
    char c;
    ASSERT_OR_RETURN(receiveChar(&c));
    s->currChar = c;
    s->hasCurrChar = true;
    *out = c;
    return StatusOk;
}

static status_t eatCurrChar(struct Tokenizer *s)
{
    if (!s->hasCurrChar) {
        WARN();
        return StatusOk;
    }
    s->hasCurrChar = false;
    s->currChar = '\0';
    return StatusOk;
}

/* clear current character, and receive a new one */
static status_t nextChar(struct Tokenizer *s, char *out)
{
    char c;
    if (!s->hasCurrChar) {
        WARN();
        return StatusOk;
    }

    s->hasCurrChar = false;
    s->currChar = '\0';
    ASSERT_OR_RETURN(receiveChar(&c));
    s->currChar = c;
    s->hasCurrChar = true;
    *out = c;
    return StatusOk;
}

/* Tokenizer */

static bool isWhitespace(char c)
{
    return c == ' ' || c == '\t' || c == '\v' || c == '\f';
}

static bool isNewline(char c)
{
    return c == '\r' || c == '\n';
}

static bool isFix16Start(char c)
{
    return ('0' <= c && c <= '9') || c == '-';
}

static bool isLetter(char c)
{
    return ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
}

static char lowerCase(char c)
{
    if ('A' <= c && c <= 'Z') {
        c |= 0x60;
    }
    return c;
}

static char isComment(char c)
{
    return c == ';';
}

/*
 * on Eof, return StatusGcodeEof
 */
static status_t skipWhitespaces(struct Tokenizer *s)
{
    char c;

    ASSERT_OR_RETURN(peekCurrChar(s, &c));
    while (isWhitespace(c)) {
        ASSERT_OR_RETURN(nextChar(s, &c));
    }

    return StatusOk;
}

/*
 * on Eof, return StatusGcodeEof
 */
static status_t skipUntilNewline(struct Tokenizer *s)
{
    char c;

    ASSERT_OR_RETURN(peekCurrChar(s, &c));
    while (!isNewline(c)) {
        ASSERT_OR_RETURN(nextChar(s, &c));
    }

    return StatusOk;
}

/*
 * on Eof, parses token, return StatusOk
 * if the content flows beyond MAX_NUM_LEN - 1,
 * take MAX_NUM_LEN - 1 characters and discard the rest
 */
static status_t takeUntilSeparator(struct Tokenizer *s)
{
    status_t err;
    char c;

    ASSIGN_OR_RETURN(err, peekCurrChar(s, &c));
    unsigned int i;
    for (i = 0;; ++i) {
        if (err == StatusGcodeEof) {
            break;
        }
        if (STATUS_ERR(err))
            return err;
        if (isWhitespace(c) || isNewline(c))
            break;
        if (i < MAX_NUM_LEN - 1)
            s->scratchBuf[i] = c;
        if (i == MAX_NUM_LEN - 1)
            s->scratchBuf[i] = '\0';
        err = nextChar(s, &c);
    }
    if (i < MAX_NUM_LEN)
        s->scratchBuf[i] = '\0';
    return StatusOk;
}

struct TokenizerState;
typedef status_t tok_fun_t(struct Tokenizer *, struct Token *t,
                           struct TokenizerState *);
struct TokenizerState {
    tok_fun_t *f;
};
static tok_fun_t tokenize, dec, letter, comment, newline;

static status_t dec(struct Tokenizer *s, struct Token *t,
                    struct TokenizerState *next)
{
    ASSERT_OR_RETURN(takeUntilSeparator(s));
    fix16_t res = fix16_from_str(s->scratchBuf);

    memset(s->scratchBuf, 0, sizeof(s->scratchBuf));
    if (res == fix16_overflow)
        return StatusInvalidGcodeToken;

    t->kind = TokenFix16;
    t->fix16 = res;
    return StatusOk;
}

static status_t letter(struct Tokenizer *s, struct Token *t,
                       struct TokenizerState *next)
{
    char c;
    ASSERT_OR_RETURN(peekCurrChar(s, &c));

    switch (lowerCase(c)) {
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
    case 'm':
        t->kind = TokenM;
        break;
    default:
        return StatusInvalidGcodeToken;
    }
    ASSERT_OR_RETURN(eatCurrChar(s));
    return StatusOk;
}

static status_t newline(struct Tokenizer *s, struct Token *t,
                        struct TokenizerState *next)
{
    t->kind = TokenNewline;
    ASSERT_OR_RETURN(eatCurrChar(s));
    return StatusOk;
}

static status_t comment(struct Tokenizer *s, struct Token *t,
                        struct TokenizerState *next)
{
    ASSERT_OR_RETURN(skipUntilNewline(s));
    next->f = tokenize;
    return StatusAgain;
}

static status_t tokenize(struct Tokenizer *s, struct Token *t,
                         struct TokenizerState *next)
{
    char c;

    ASSERT_OR_RETURN(skipWhitespaces(s));
    ASSERT_OR_RETURN(peekCurrChar(s, &c));

    if (isFix16Start(c)) {
        next->f = dec;
        return StatusAgain;
    }
    if (isLetter(c)) {
        next->f = letter;
        return StatusAgain;
    }
    if (isComment(c)) {
        next->f = comment;
        return StatusAgain;
    }
    if (isNewline(c)) {
        next->f = newline;
        return StatusAgain;
    }
    return StatusInvalidGcodeToken;
}

static status_t getToken(struct Tokenizer *s, struct Token *t)
{
    struct TokenizerState next = {
        .f = tokenize,
    };
    status_t err;
    while (StatusAgain == (err = next.f(s, t, &next)))
        ;

    return err;
}

/* Parser */

static status_t peekToken(struct GcodeParser *s, struct Token *out)
{
    struct Token t;

    if (s->currToken.kind != TokenUndef) {
        *out = s->currToken;
        return StatusOk;
    }

    while (true) {
        status_t err = getToken(&s->tokenizer, &t);

        if (err == StatusGcodeEof) {
            return err;
        }
        if (STATUS_OK(err)) {
            break;
        }

        WARN_ON_ERR(err);
        err = eatCurrChar(&s->tokenizer);
        /* failed to clear current char, give up */
        WARN_ON_ERR(err);
        if (STATUS_ERR(err)) {
            return err;
        }
        /* try again */
    }
    s->currToken = t;
    *out = t;
    return StatusOk;
}

static status_t eatToken(struct GcodeParser *s)
{
    if (s->currToken.kind == TokenUndef) {
        WARN();
        return StatusGcodeParserLogicError;
    }
    memset(&s->currToken, 0, sizeof(s->currToken));
    return StatusOk;
}

static status_t assertAndEatToken(struct GcodeParser *s, enum TokenKind k,
                                  struct Token *out)
{
    struct Token curr;

    ASSERT_OR_RETURN(peekToken(s, &curr));
    if (curr.kind != k) {
        WARN();
        return StatusInvalidGcodeCommand;
    }
    ASSERT_OR_RETURN(eatToken(s));
    *out = curr;
    return StatusOk;
}

struct ParserState;
typedef status_t parse_fun_t(struct GcodeParser *, struct GcodeCommand *t,
                             struct ParserState *);
struct ParserState {
    parse_fun_t *f;
};
static parse_fun_t parseXYZEF, parseCmdG, parseCmdM, _parseGcode;

/* refactor to be less clunky, note this should not get more
 * tokens than what is required. 
 */
static status_t parseXYZEF(struct GcodeParser *s, struct GcodeCommand *cmd,
                           struct ParserState *next)
{
    struct Token t;
    status_t err;

    /* arg list cannot be empty */
    fix16_t *target;

    err = peekToken(s, &t);
    if (err == StatusGcodeEof) {
        return StatusOk;
    }
    ASSERT_OR_RETURN(err);
    switch (t.kind) {
    case TokenX:
        target = &cmd->xyzef.x;
        cmd->xyzef.xSet = true;
        break;
    case TokenY:
        target = &cmd->xyzef.y;
        cmd->xyzef.ySet = true;
        break;
    case TokenZ:
        target = &cmd->xyzef.z;
        cmd->xyzef.zSet = true;
        break;
    case TokenE:
        target = &cmd->xyzef.e;
        cmd->xyzef.eSet = true;
        break;
    case TokenF:
        target = &cmd->xyzef.f;
        cmd->xyzef.fSet = true;
        break;
    case TokenNewline:
        ASSERT_OR_RETURN(eatToken(s));
        return StatusOk;
    default:
        return StatusOk;
    }

    ASSERT_OR_RETURN(eatToken(s));
    ASSERT_OR_RETURN(assertAndEatToken(s, TokenFix16, &t));
    *target = t.fix16;
    next->f = parseXYZEF;
    return StatusAgain;
}

static status_t parseCmdG(struct GcodeParser *s, struct GcodeCommand *cmd,
                          struct ParserState *next)
{
    struct Token t;

    ASSERT_OR_RETURN(assertAndEatToken(s, TokenFix16, &t));
    memset(cmd, 0, sizeof(*cmd));
    if (!fix16_is_uint(t.fix16)) {
        return StatusInvalidGcodeCommand;
    }

    switch ((unsigned int)fix16_to_int(t.fix16)) {
    case 0:
        cmd->kind = GcodeG0;
        next->f = parseXYZEF;
        return StatusAgain;
    case 1:
        cmd->kind = GcodeG1;
        next->f = parseXYZEF;
        return StatusAgain;
    case 28:
        cmd->kind = GcodeG28;
        return StatusOk;
    default:
        break;
    }
    return StatusUnimplementedGcodeCommand;
}

static status_t parseCmdM(struct GcodeParser *s, struct GcodeCommand *cmd,
                          struct ParserState *next)
{
    struct Token t;

    ASSERT_OR_RETURN(assertAndEatToken(s, TokenFix16, &t));
    memset(cmd, 0, sizeof(*cmd));
    if (!fix16_is_uint(t.fix16)) {
        return StatusInvalidGcodeCommand;
    }

    switch ((unsigned int)fix16_to_int(t.fix16)) {
    case 84:
        cmd->kind = GcodeM84;
        return StatusOk;
    default:
        break;
    }
    return StatusUnimplementedGcodeCommand;
}

static status_t _parseGcode(struct GcodeParser *s, struct GcodeCommand *cmd,
                            struct ParserState *next)
{
    struct Token t;
    ASSERT_OR_RETURN(peekToken(s, &t));

    switch (t.kind) {
    case TokenG:
        ASSERT_OR_RETURN(eatToken(s));
        next->f = parseCmdG;
        return StatusAgain;
    case TokenM:
        ASSERT_OR_RETURN(eatToken(s));
        next->f = parseCmdM;
        return StatusAgain;
    case TokenNewline:
        ASSERT_OR_RETURN(eatToken(s));
        next->f = _parseGcode;
        return StatusAgain;
    default:
        return StatusUnimplementedGcodeCommand;
    }
}

status_t parseGcode(struct GcodeParser *s, struct GcodeCommand *cmd)
{
    struct ParserState next = {
        .f = _parseGcode,
    };
    status_t err;
    while (StatusAgain == (err = next.f(s, cmd, &next)))
        ;

    return err;
}

status_t initParser(struct GcodeParser *s)
{
    memset(s, 0, sizeof(*s));
    return StatusOk;
}
