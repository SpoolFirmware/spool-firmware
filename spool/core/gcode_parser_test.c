#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcode_parser.h"

static FILE *f;

status_t receiveChar(char *c)
{
    /* replace with stream functions */
    *c = fgetc(f);
    if (*c == EOF) {
        if (feof(f)) {
            *c = '\0';
            return StatusGcodeEof;
        }
        return StatusReadErr;
    }
    if (*c == '\0') {
        return StatusUnsupportedChar;
    }
    return StatusOk;
}

static void printGcode(struct GcodeCommand *cmd)
{
    char buf[MAX_NUM_LEN];

    switch (cmd->kind) {
    case GcodeG0:
        printf("G0 ");
        break;
    case GcodeG1:
        printf("G1 ");
        break;
    }
    if (cmd->xy.x != 0) {
        fix16_to_str(cmd->xy.x, buf, 10);
        printf("X%s ", buf);
    }
    if (cmd->xy.y != 0) {
        fix16_to_str(cmd->xy.y, buf, 10);
        printf("Y%s", buf);
    }
    printf("\n");
}

static status_t parseFile(void)
{
    status_t err;
    struct GcodeCommand cmd;

    err = initParser();
    while (err == StatusOk) {
        err = parseGcode(&cmd);
        if (err == StatusGcodeEof) {
            return StatusOk;
        }
        if (STATUS_OK(err)) {
            printGcode(&cmd);
        }
    }
    return err;
}

int main(int argc, char **argv)
{
    if (argc != 2)
        printf("usage: [gcode filename]\n");

    char *fname = argv[1];
    f = fopen(fname, "r");
    if (!f) {
        perror("open file failed\n");
        exit(EXIT_FAILURE);
    }

    status_t err = parseFile();

    if (STATUS_ERR(err)) {
        printf("%d\n", err);
        return err;
    }

    return 0;
}