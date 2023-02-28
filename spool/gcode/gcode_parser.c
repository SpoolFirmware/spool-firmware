#include "lib/common/error.h"
#include <stdint.h>
#include "string.h"
#include "dbgprintf.h"
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

static bool isDigit(char c)
{
    return '0' <= c && c <= '9';
}

static bool isFix16Start(char c)
{
    return isDigit(c) || c == '-';
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
    return c == ';' || c == '*';
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

static status_t fix16OrInt32FromStr(const char *buf, int32_t *out,
                                    bool *isInt32)
{
    while (isWhitespace((unsigned char)*buf))
        buf++;

    /* Decode the sign */
    bool negative = (*buf == '-');
    if (*buf == '+' || *buf == '-')
        buf++;

    /* Decode the integer part */
    uint32_t intpart = 0;
    int count = 0;

    while (isDigit((unsigned char)*buf)) {
        intpart *= 10;
        intpart += *buf++ - '0';
        count++;
    }

    if (!(*buf == '.' || *buf == ',')) {
        if (count == 0) {
            PR_WARN("parsed no digits\n");
            return StatusGcodeBadNumber;
        }
        *out = negative ? -(int32_t)intpart : (int32_t)intpart;
        *isInt32 = true;
        return StatusOk;
    }

    /* has . or , must be fix16 */
    if (count == 0 || count > 5 || intpart > 32768 ||
        (!negative && intpart > 32767)) {
        PR_WARN("fix16 overflow/no digits %d, count=%d\n", intpart, count);
        return StatusGcodeBadNumber;
    }

    fix16_t value = intpart << 16;

    /* Decode the decimal part */
    buf++;

    uint32_t fracpart = 0;
    uint32_t scale = 1;
    while (isDigit((unsigned char)*buf) && scale < 100000) {
        scale *= 10;
        fracpart *= 10;
        fracpart += *buf++ - '0';
    }
    value += fix16_div(fracpart, scale);

    /* Verify that there is no garbage left over */
    while (*buf != '\0') {
        if (!isDigit((unsigned char)*buf) &&
            !isWhitespace((unsigned char)*buf)) {
            PR_WARN("fix16 leftover garbage num=%d\n", value);
            return StatusGcodeBadNumber;
        }

        buf++;
    }

    *out = negative ? -value : value;
    *isInt32 = false;
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
    int32_t res;
    bool isInt32;
    ASSERT_OR_RETURN(fix16OrInt32FromStr(s->scratchBuf, &res, &isInt32));

    memset(s->scratchBuf, 0, sizeof(s->scratchBuf));

    if (isInt32) {
        t->kind = TokenInt32;
        t->int32 = res;
    } else {
        t->kind = TokenFix16;
        t->fix16 = res;
    }
    return StatusOk;
}

static status_t letter(struct Tokenizer *s, struct Token *t,
                       struct TokenizerState *next)
{
    char c;
    ASSERT_OR_RETURN(peekCurrChar(s, &c));

    switch (lowerCase(c)) {
    case 'e':
        t->kind = TokenE;
        break;
    case 'f':
        t->kind = TokenF;
        break;
    case 'g':
        t->kind = TokenG;
        break;
    case 'm':
        t->kind = TokenM;
        break;
    case 'n':
        t->kind = TokenN;
        break;
    case 'r':
        t->kind = TokenR;
        break;
    case 's':
        t->kind = TokenS;
        break;
    case 't':
        t->kind = TokenT;
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
    default:
        dbgPrintf("Unknown token %c\n", lowerCase(c));
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
    dbgPrintf("tokenize: %c\n", lowerCase(c));
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

static status_t assertAndEatNumber(struct GcodeParser *s, struct Token *out)
{
    struct Token curr;

    ASSERT_OR_RETURN(peekToken(s, &curr));
    if (!(curr.kind == TokenFix16 || curr.kind == TokenInt32)) {
        PR_WARN("ate token %d instead of number\n", curr.kind);
        return StatusInvalidGcodeCommand;
    }
    ASSERT_OR_RETURN(eatToken(s));
    if (out) {
        *out = curr;
    }
    return StatusOk;
}

/* coerse a token to fix16_t, return error if it can't */
static status_t assertGetFix16(const struct Token *token, fix16_t *out)
{
    if (token->kind == TokenFix16) {
        *out = token->fix16;
        return StatusOk;
    }
    if (token->kind == TokenInt32) {
        if (token->int32 <= 32767 && token->int32 > -32768) {
            *out = fix16_from_int(token->int32);
            return StatusOk;
        } else {
            PR_WARN("trying to coerse %d to fix16\n", token->int32);
            return StatusGcodeBadNumber;
        }
    }
    PR_WARN("trying to coerse token %d to fix16\n", token->kind);
    return StatusInvalidGcodeToken;
}

struct ParserState;
typedef status_t parse_fun_t(struct GcodeParser *, struct GcodeCommand *t,
                             struct ParserState *);
struct ParserState {
    parse_fun_t *f;
};
static parse_fun_t parseXYZEF, parseTemperature, parseFan, parseCmdG, parseCmdM,
    _parseGcode;

/**
 * @brief skip until a newline is found.
 * 
 * @param s the parser
 * @return status_t 
 */
static status_t s_skipUntilNewline(struct GcodeParser *s)
{   
    struct Token t = { .kind=TokenUndef };
    do {
        if (t.kind != TokenUndef) {
            eatToken(s);
        }
        peekToken(s, &t);
    } while(t.kind != TokenNewline);

    return StatusOk;
}

static status_t parseFan(struct GcodeParser *s, struct GcodeCommand *cmd,
                         struct ParserState *next)
{
    struct Token t;
    status_t err;

    err = peekToken(s, &t);
    if (err == StatusGcodeEof) {
        return StatusOk;
    }
    ASSERT_OR_RETURN(err);

    fix16_t *target;

    switch (t.kind) {
    case TokenS:
        target = &cmd->fan.speed;
        break;
    default:
        return StatusOk;
    }

    ASSERT_OR_RETURN(eatToken(s));
    ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
    ASSERT_OR_RETURN(assertGetFix16(&t, target));

    next->f = parseFan;
    return StatusAgain;
}

static status_t parseTemperature(struct GcodeParser *s,
                                 struct GcodeCommand *cmd,
                                 struct ParserState *next)
{
    struct Token t;
    status_t err;

    err = peekToken(s, &t);
    if (err == StatusGcodeEof) {
        return StatusOk;
    }
    ASSERT_OR_RETURN(err);

    fix16_t *target;

    switch (t.kind) {
    case TokenT:
        target = NULL;
        break;
    case TokenS:
        target = &cmd->temperature.sTemp;
        break;
    case TokenR:
        target = &cmd->temperature.rTemp;
        break;
    default:
        return StatusOk;
    }

    ASSERT_OR_RETURN(eatToken(s));
    if (target) {
        ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
        ASSERT_OR_RETURN(assertGetFix16(&t, target));
    } else {
        peekToken(s, &t);
        eatToken(s);
    }

    next->f = parseTemperature;
    return StatusAgain;
}

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
    ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
    ASSERT_OR_RETURN(assertGetFix16(&t, target));
    next->f = parseXYZEF;
    return StatusAgain;
}

static status_t parseCmdG(struct GcodeParser *s, struct GcodeCommand *cmd,
                          struct ParserState *next)
{
    struct Token t;

    ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
    if (t.kind != TokenInt32) {
        PR_WARN("G called with fix16 %d\n", t.fix16);
        return StatusInvalidGcodeCommand;
    }
    memset(cmd, 0, sizeof(*cmd));
    switch (t.int32) {
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
        next->f = parseXYZEF;
        return StatusAgain;
    case 29:
        cmd->kind = GcodeG29;
        return StatusOk;
    case 90:
        cmd->kind = GcodeG90;
        return StatusOk;
    case 91:
        cmd->kind = GcodeG91;
        return StatusOk;
    case 92:
        cmd->kind = GcodeG92;
        next->f = parseXYZEF;
        return StatusAgain;
    default:
        PR_WARN("UNIMPL G%d\n", t.int32);
        break;
    }
    return StatusUnimplementedGcodeCommand;
}

static status_t parseCmdM(struct GcodeParser *s, struct GcodeCommand *cmd,
                          struct ParserState *next)
{
    struct Token t;

    ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
    if (t.kind != TokenInt32) {
        PR_WARN("G called with fix16 %d\n", t.fix16);
        return StatusInvalidGcodeCommand;
    }
    memset(cmd, 0, sizeof(*cmd));
    switch (t.int32) {
    case 82:
        cmd->kind = GcodeM82;
        return StatusOk;
    case 83:
        cmd->kind = GcodeM83;
        return StatusOk;
    case 84:
        cmd->kind = GcodeM84;
        return s_skipUntilNewline(s);
    case 101:
        cmd->kind = GcodeM101;
        return StatusOk;
    case 103:
        cmd->kind = GcodeM103;
        return StatusOk;
    case 104:
        cmd->kind = GcodeM104;
        next->f = parseTemperature;
        return StatusAgain;
    case 105:
        cmd->kind = GcodeM105;
        return StatusOk;
    case 106:
        cmd->kind = GcodeM106;
        next->f = parseFan;
        return StatusAgain;
    case 107:
        cmd->kind = GcodeM107;
        return StatusOk;
    case 108:
        cmd->kind = GcodeM108;
        return StatusOk;
    case 109:
        cmd->kind = GcodeM109;
        next->f = parseTemperature;
        return StatusAgain;
    case 140:
        cmd->kind = GcodeM140;
        next->f = parseTemperature;
        return StatusAgain;
    case 190:
        cmd->kind = GcodeM190;
        next->f = parseTemperature;
        return StatusAgain;
    // IDGAF
    case 29: /* STOP WRITING SD CARD */
    case 110:
    case 111:
    case 115:
        do {
            ASSERT_OR_RETURN(peekToken(s, &t));
            eatToken(s);
        } while (t.kind != TokenNewline);
        cmd->kind = GcodeM_IDGAF;
        return StatusOk;
    default:
        PR_WARN("UNIMPL M%d\n", t.int32);
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
    case TokenN:
        ASSERT_OR_RETURN(eatToken(s));
        ASSERT_OR_RETURN(assertAndEatNumber(s, &t));
        next->f = _parseGcode;
        return StatusAgain;
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
        PR_WARN("unknown token %d\n", t.kind);
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
