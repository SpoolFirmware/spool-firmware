#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "gcode_parser.h"
#include "error.h"

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

static void printTemperature(struct GcodeTemp *temp)
{
    char buf[MAX_NUM_LEN];
    fix16_to_str(temp->rTemp, buf, 10);
    printf("R%s ", buf);
    fix16_to_str(temp->sTemp, buf, 10);
    printf("S%s\n", buf);
}

static void printGcode(struct GcodeCommand *cmd)
{
    char buf[MAX_NUM_LEN];

    switch (cmd->kind) {
    case GcodeG0:
        printf("G0 ");
        goto printXYZEF;
    case GcodeG1:
        printf("G1 ");
        goto printXYZEF;
    case GcodeG28:
        printf("G28\n");
        return;
    case GcodeM84:
        printf("M84\n");
        return;
    case GcodeM104:
        printf("M104 "); printTemperature(&(cmd->temperature)); return;
    case GcodeM105:
        printf("M105\n"); return;
    default:
        printf("Update printing function: kind=%d\n", cmd->kind);
        return;
    }
printXYZEF:
    if (cmd->xyzef.x != 0) {
        fix16_to_str(cmd->xyzef.x, buf, 10);
        printf("X%s ", buf);
    }
    if (cmd->xyzef.y != 0) {
        fix16_to_str(cmd->xyzef.y, buf, 10);
        printf("Y%s ", buf);
    }
    if (cmd->xyzef.z != 0) {
        fix16_to_str(cmd->xyzef.z, buf, 10);
        printf("Z%s ", buf);
    }
    if (cmd->xyzef.e != 0) {
        fix16_to_str(cmd->xyzef.e, buf, 10);
        printf("E%s ", buf);
    }
    if (cmd->xyzef.f != 0) {
        fix16_to_str(cmd->xyzef.f, buf, 10);
        printf("F%s ", buf);
    }
    printf("\n");
    return;
}

static status_t parseFile(void)
{
    status_t err;
    struct GcodeParser p;
    struct GcodeCommand cmd;

    err = initParser(&p);
    while (err == StatusOk) {
        err = parseGcode(&p, &cmd);
        if (err == StatusGcodeEof) {
            return StatusOk;
        }
        if (STATUS_OK(err)) {
            printGcode(&cmd);
        }
    }
    return err;
}

static status_t parseFileWithError(void)
{
    status_t err;
    struct GcodeParser p;
    struct GcodeCommand cmd;

    err = initParser(&p);
    while (err == StatusOk) {
        err = parseGcode(&p, &cmd);
        if (err == StatusGcodeEof) {
            break;
        }
        if (STATUS_OK(err))
            printGcode(&cmd);

        WARN_ON_ERR(err);
        if (STATUS_ERR(err)) {
            err = initParser(&p);
            WARN_ON_ERR(err);
            if (STATUS_ERR(err))
                return err;
        }
    }
    return StatusOk;
}

static void runTest(const char *fileName, status_t t(void))
{
    f = fopen(fileName, "r");
    if (!f) {
        perror("open file failed\n");
        exit(EXIT_FAILURE);
    }

    status_t err = t();
    fclose(f);

    if (STATUS_ERR(err)) {
        printf("%d\n", err);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: gcode_filename [strict]\n");
        return 1;
    }

    char *fname = argv[1];

    printf("=== Testing parseFile ===\n");
    runTest(fname, parseFile);
    printf("=== Testing parseFileWithError ===\n");
    runTest(fname, parseFileWithError);
    return 0;
}
