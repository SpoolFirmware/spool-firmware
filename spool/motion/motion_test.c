#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include "error.h"
#include "step_plan_ng.h"
#include "compiler.h"
#include "gcode/gcode_parser.h"
#include "number.h"

struct PrinterState {
    fix16_t x;
    fix16_t y;
    fix16_t z;
    fix16_t feedrate;
    bool continuousMode;
};

static FILE *f;

status_t receiveChar(char *c)
{
    /* replace with stream functions */
    *c = (char)fgetc(f);
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
}

static void printPlan()
{
    struct PlannerJob j;
    if (plannerSize() > 0) {
        __dequeuePlan(&j);

        printf("===\n");
        printf("x acc dec a viSq vSq v vfSq\n");
        for_each_stepper(i) {
            struct PlannerBlock *s = &j.steppers[i];
            BUG_ON(abs(s->x) - abs(s->accelerationX) - abs(s->decelerationX) < 0);
            printf("%d %d %d %d %u %u %u %u\n", s->x, s->accelerationX,
                   s->decelerationX, s->a, s->viSq, s->vSq, (uint32_t)sqrtf((float)s->vSq), s->vfSq);
        }
        printf("===\n");
    }
}

static struct PrinterState currentState;

static void testPlanContinuous(struct PrinterState nextState)
{
    int32_t plan[NR_STEPPERS];
    const int32_t max_v[] = {
        min(VEL, fix16_to_int(nextState.feedrate) / SECONDS_IN_MIN) *
            STEPPER_STEPS_PER_MM[0],
        min(VEL, fix16_to_int(nextState.feedrate) / SECONDS_IN_MIN) *
            STEPPER_STEPS_PER_MM[1],
        min(VEL_Z, fix16_to_int(nextState.feedrate) / SECONDS_IN_MIN) *
            STEPPER_STEPS_PER_MM[2],
    };
    STATIC_ASSERT(ARRAY_SIZE(max_v) == NR_STEPPERS);

    fix16_t dx = fix16_sub(nextState.x, currentState.x);
    fix16_t dy = fix16_sub(nextState.y, currentState.y);
    fix16_t dz = fix16_sub(nextState.z, currentState.z);
    const fix16_t movement[] = { dx, dy, dz };
    STATIC_ASSERT(ARRAY_SIZE(movement) == NR_AXES);

    planCoreXy(movement, plan);
    printf("(corexy) (%f, %f, %f) => (%d, %d, %d)\n",
           fix16_to_float(nextState.x), fix16_to_float(nextState.y),
           fix16_to_float(nextState.z), plan[0], plan[1], plan[2]);

    if (plannerAvailableSpace() == 0) {
        printPlan();
    }
    BUG_ON(plannerAvailableSpace() == 0);
    __enqueuePlan(StepperJobRun, plan, max_v, true);
    currentState = nextState;
}

static status_t runTest(void)
{
    status_t err;
    struct GcodeParser p;
    struct GcodeCommand cmd;
    static struct PrinterState nextState = { 0 };

    err = initParser(&p);
    initPlanner();
    while (err == StatusOk) {
        err = parseGcode(&p, &cmd);
        if (err == StatusGcodeEof) {
            break;
        }
        if (STATUS_OK(err)) {
            // printGcode(&cmd);
            switch (cmd.kind) {
            case GcodeG0:
            case GcodeG1:
                nextState.x = cmd.xyzef.xSet ? cmd.xyzef.x : currentState.x;
                nextState.y = cmd.xyzef.ySet ? cmd.xyzef.y : currentState.y;
                nextState.z = cmd.xyzef.zSet ? cmd.xyzef.z : currentState.z;

                WARN_ON(cmd.xyzef.f < 0);
                nextState.feedrate = cmd.xyzef.fSet ? fix16_abs(cmd.xyzef.f) :
                                                      currentState.feedrate;
                testPlanContinuous(nextState);
            case GcodeG28:
                break;
            case GcodeM84:
                break;
            }
        } else {
            return err;
        }
    }
    return err;
}

int main(int argc, char **argv)
{
    if (argc < 2)
        printf("usage: gcode_filename\n");

    char *fileName = argv[1];

    f = fopen(fileName, "r");
    runTest();
}