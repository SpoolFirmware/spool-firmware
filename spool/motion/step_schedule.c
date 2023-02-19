#include "step_schedule.h"
#include "lib/fxp/src/fix16.h"
#include "step_plan_ng.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "error.h"
#include "platform/platform.h"
#include "bitops.h"
#include "dbgprintf.h"
#include "gcode/gcode.h"
#include "number.h"
#include "compiler.h"
#include "core/spool.h"

/* Records the state of the printer ends up in after the stepper queue
 * finishes. We could have the state be attached to each stepper job.
 * However, if we do not care about displaying the state of the print
 * head, then this is sufficient.
 */
/* only touched in scheduleMoveTo */
static struct PrinterState currentState;
static struct {
    int32_t x;
    int32_t y;
    int32_t current;
} bedLevelingFactors = { 0 };

static uint32_t s_scheduleHomeMove(enum JobType k,
                                   const int32_t plan[NR_STEPPERS],
                                   const fix16_t unitVec[X_AND_Y],
                                   const uint32_t maxV[NR_STEPPERS],
                                   const uint32_t acc[NR_STEPPERS],
                                   fix16_t len);

TaskHandle_t stepScheduleTaskHandle;

void motionPlannerTaskInit(void)
{
    MotionPlannerTaskQueue = xQueueCreate(MOTION_PLANNER_TASK_QUEUE_SIZE,
                                          sizeof(struct GcodeCommand));
    if (MotionPlannerTaskQueue == NULL) {
        panic();
    }
    if (xTaskCreate(stepScheduleTask, "stepSchedule", 1024, NULL,
                    tskIDLE_PRIORITY + 1, &stepScheduleTaskHandle) != pdTRUE) {
        panic();
    }
}

#define SECONDS_PER_MIN 60

static void scheduleMoveTo(const struct PrinterMove state)
{
    BUG_ON(state.x < 0);
    BUG_ON(state.y < 0);
    // BUG_ON(state.z < 0);

    BUG_ON(state.x > (MAX_X * STEPS_PER_MM));
    BUG_ON(state.y > (MAX_Y * STEPS_PER_MM));
    BUG_ON(state.z > (MAX_Z * STEPS_PER_MM_Z));

    int32_t dx = currentState.homedX ? (state.x - currentState.x) : 0;
    int32_t dy = currentState.homedY ? (state.y - currentState.y) : 0;
    int32_t dz = currentState.homedZ ? (state.z - currentState.z) : 0;
    int32_t de = state.e - currentState.e;
    int32_t zLevelingTarget = 0;
    if (bedLevelingFactors.x)
        zLevelingTarget += (currentState.homedX ? state.x : currentState.x) / bedLevelingFactors.x;
    if (bedLevelingFactors.y)
        zLevelingTarget += (currentState.homedY ? state.y : currentState.y) / bedLevelingFactors.y;
    const int32_t dzLeveling = zLevelingTarget - bedLevelingFactors.current;

    /* MOVE CONSTANTS */
    const uint32_t move_max_v[] = {
        min(currentState.feedrate, VEL) * STEPS_PER_MM,
        min(currentState.feedrate, VEL) * STEPS_PER_MM,
        min(currentState.feedrate, VEL_Z) * STEPS_PER_MM_Z,
        min(currentState.feedrate, VEL_E) * STEPS_PER_MM_E,
    };
    STATIC_ASSERT(ARRAY_SIZE(move_max_v) == NR_STEPPERS);

    int32_t plan[NR_STEPPERS];
    /* TODO fix fixed z inversion */
    const int32_t dir[NR_AXES] = { dx, dy, -(dz + dzLeveling), -de };
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planCoreXy(dir, plan, unitVec, &len);
    __enqueuePlan(StepperJobRun, plan, unitVec, move_max_v, STEPPER_ACC, len,
                  !currentState.continuousMode);

    currentState.x = currentState.homedX ? state.x : currentState.x;
    currentState.y = currentState.homedY ? state.y : currentState.y;
    currentState.z = currentState.homedZ ? state.z : currentState.z;
    currentState.e = state.e;
    bedLevelingFactors.current = zLevelingTarget;
}

const static uint32_t home_max_v[] = {
    HOMING_VEL * STEPS_PER_MM,
    HOMING_VEL *STEPS_PER_MM,
    HOMING_VEL_Z *STEPS_PER_MM_Z,
    0,
};
STATIC_ASSERT(ARRAY_SIZE(home_max_v) == NR_STEPPERS);

static void scheduleHomeX(void)
{
    const static int32_t home_x[] = { -MAX_X * STEPS_PER_MM, 0, 0, 0 };
    STATIC_ASSERT(ARRAY_SIZE(home_x) == NR_AXES);
    int32_t plan[NR_STEPPERS];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planCoreXy(home_x, plan, unitVec, &len);
    s_scheduleHomeMove(StepperJobHomeX, plan, unitVec, home_max_v, STEPPER_ACC,
                       len);
    currentState.x = 0;
    currentState.homedX = true;
}

static void scheduleHomeY(void)
{
    const static int32_t home_y[] = { 0, -MAX_Y * STEPS_PER_MM, 0, 0 };
    STATIC_ASSERT(ARRAY_SIZE(home_y) == NR_AXES);
    int32_t plan[NR_STEPPERS];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planCoreXy(home_y, plan, unitVec, &len);
    s_scheduleHomeMove(StepperJobHomeY, plan, unitVec, home_max_v, STEPPER_ACC,
                       len);
    currentState.y = 0;
    currentState.homedY = true;
}

static uint32_t s_scheduleZMeasure(uint32_t velSteps)
{
    static int32_t home_z[] = { 0, 0, MAX_Z * STEPS_PER_MM_Z, 0 };
    uint32_t home_z_max_v[] = {
        HOMING_VEL * STEPS_PER_MM,
        HOMING_VEL * STEPS_PER_MM,
        velSteps,
        0,
    };
    STATIC_ASSERT(ARRAY_SIZE(home_z) == NR_AXES);

    int32_t plan[NR_STEPPERS];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planCoreXy(home_z, plan, unitVec, &len);
    return s_scheduleHomeMove(StepperJobHomeZ, plan, unitVec, home_z_max_v,
                              STEPPER_ACC, len);
}

static void scheduleHomeZ(void)
{
    if (!(currentState.homedX && currentState.homedY))
        return;
    bedLevelingFactors.current = 0;
    bedLevelingFactors.x = 0;
    bedLevelingFactors.y = 0;
    /* TODO fix z inversion */
    int32_t savedFR, savedX = 0, savedY = 0;
    struct PrinterMove home_z_move = { 0 };
    home_z_move.x = Z_HOME_X * STEPPER_STEPS_PER_MM[X_AXIS];
    home_z_move.y = Z_HOME_Y * STEPPER_STEPS_PER_MM[Y_AXIS];

    /* move x and y to the right spot to home z */
    if (home_z_move.x || home_z_move.y) {
        savedX = currentState.x;
        savedY = currentState.y;
        scheduleMoveTo(home_z_move);
    }

    s_scheduleZMeasure(HOMING_VEL_Z * STEPPER_STEPS_PER_MM[Z_AXIS]);
    currentState.homedZ = true;
    currentState.z = 0;

    home_z_move.z = 3 * STEPPER_STEPS_PER_MM[Z_AXIS];
    savedFR = currentState.feedrate;
    currentState.feedrate =
        HOMING_VEL_Z / 4;
    scheduleMoveTo(home_z_move);
    currentState.feedrate = savedFR;

    s_scheduleZMeasure(HOMING_VEL_Z * STEPPER_STEPS_PER_MM[Z_AXIS] / 4);

    currentState.z = fix16_mul_int32(Z_OFFSET, STEPPER_STEPS_PER_MM[Z_AXIS]);

    if (Z_OFFSET) {
        home_z_move.x = savedX;
        home_z_move.y = savedY;
        home_z_move.z = 0;
        scheduleMoveTo(home_z_move);
    }
}

static void s_performBedLeveling(void)
{
    if (!(currentState.homedX && currentState.homedY && currentState.homedZ)) {
        return;
    }

    // Reset leveling factor
    bedLevelingFactors.current = 0;
    bedLevelingFactors.x = 0;
    bedLevelingFactors.y = 0;

    currentState.feedrate = 60;
    // Probe (5,10), (120, 10), (5, 155)
    static struct PrinterMove move_position = {
        .z = 5 * STEPPER_STEPS_PER_MM[Z_AXIS],
    };
    int32_t travels[3] = {0};
    struct {
        int32_t x;
        int32_t y;
    } positions[] = { { 5, 10 }, { 120, 10 }, { 5, 155 } };
    for (int i = 0; i < ARRAY_LENGTH(positions); i++) {
        move_position.x = positions[i].x * STEPPER_STEPS_PER_MM[X_AXIS];
        move_position.y = positions[i].y * STEPPER_STEPS_PER_MM[X_AXIS];
        scheduleMoveTo(move_position);
        int32_t actualTravel = (int32_t)s_scheduleZMeasure(
            HOMING_VEL_Z * STEPPER_STEPS_PER_MM[Z_AXIS] / 4);
        travels[i] = actualTravel;
        currentState.z -= actualTravel;
        scheduleMoveTo(move_position);
    }
    move_position.x = Z_HOME_X * STEPPER_STEPS_PER_MM[X_AXIS];
    move_position.y = Z_HOME_Y * STEPPER_STEPS_PER_MM[Y_AXIS];
    scheduleMoveTo(move_position);

    if (travels[1] != travels[0]) {
        bedLevelingFactors.x =
            -(((positions[1].x - positions[0].x) * STEPPER_STEPS_PER_MM[X_AXIS]) /
            (travels[1] - travels[0]));
    }
    if (travels[2] != travels[1]) {
        bedLevelingFactors.y =
            -(((positions[2].y - positions[0].y) * STEPPER_STEPS_PER_MM[X_AXIS]) /
            (travels[2] - travels[0]));
    }
    bedLevelingFactors.current = move_position.x / bedLevelingFactors.x;
    bedLevelingFactors.current += move_position.y / bedLevelingFactors.y;
    dbgPrintf("BLVL: z/x=%d, z/y=%d\n", bedLevelingFactors.x, bedLevelingFactors.y);
}

uint32_t getRequiredSpace(enum GcodeKind kind)
{
    switch (kind) {
    case GcodeG90:
    case GcodeG91:
    case GcodeG92:
    case GcodeM82:
    case GcodeM83:
        return 0;
    case GcodeG0:
    case GcodeG1:
    case GcodeG29: // Correct? Do we even need this?
    case GcodeM84:
        return 1;
    case GcodeG28:
        return 3;
    default:
        panic();
    }
    return 0;
}

#define CONTINUOUS_THRESHOLD 5

static void enqueueAvailableGcode()
{
    static bool commandAvailable = false;
    bool continuousMode = false;
    static struct GcodeCommand cmd;
    static struct PrinterMove nextState = { 0 };

    static bool inRelativeMode = false;
    static bool eRelativeMode = false;

    while (plannerAvailableSpace() > 0) {
        continuousMode = uxQueueSpacesAvailable(MotionPlannerTaskQueue) <
                         CONTINUOUS_THRESHOLD;
        if (!commandAvailable) {
            if (xQueueReceive(MotionPlannerTaskQueue, &cmd,
                              plannerSize() == 0 ? portMAX_DELAY : 0) !=
                pdTRUE) {
                return;
            }
            commandAvailable = true;
        }
        if (plannerAvailableSpace() < getRequiredSpace(cmd.kind)) {
            return;
        }
        switch (cmd.kind) {
        case GcodeG0:
        case GcodeG1:
            nextState.x = cmd.xyzef.xSet ?
                              ((inRelativeMode ? currentState.x : 0) +
                               fix16_mul_int32(cmd.xyzef.x,
                                               STEPPER_STEPS_PER_MM[X_AXIS])) :
                              currentState.x;
            nextState.y = cmd.xyzef.ySet ?
                              ((inRelativeMode ? currentState.y : 0) +
                               fix16_mul_int32(cmd.xyzef.y,
                                               STEPPER_STEPS_PER_MM[Y_AXIS])) :
                              currentState.y;
            nextState.z = cmd.xyzef.zSet ?
                              ((inRelativeMode ? currentState.z : 0) +
                               fix16_mul_int32(cmd.xyzef.z,
                                               STEPPER_STEPS_PER_MM[Z_AXIS])) :
                              currentState.z;
            nextState.e = cmd.xyzef.eSet ?
                              ((eRelativeMode ? currentState.e : 0) +
                               fix16_mul_int32(cmd.xyzef.e,
                                               STEPPER_STEPS_PER_MM[E_AXIS])) :
                              currentState.e;

            WARN_ON(cmd.xyzef.f < 0);
            currentState.feedrate =
                (cmd.xyzef.fSet && cmd.xyzef.f != 0) ?
                    fix16_to_int(cmd.xyzef.f) / SECONDS_PER_MIN :
                    currentState.feedrate;
            currentState.continuousMode = continuousMode;
            scheduleMoveTo(nextState);
            break;
        case GcodeG28:
            /* TODO, make this a stepper job */
            platformEnableStepper(0xFF);
            currentState.feedrate = VEL;
            currentState.continuousMode = false;
            if (!(cmd.xyzef.xSet || cmd.xyzef.ySet || cmd.xyzef.zSet)) {
                scheduleHomeX();
                scheduleHomeY();
                scheduleHomeZ();
            } else {
                if (cmd.xyzef.xSet) {
                    scheduleHomeX();
                }
                if (cmd.xyzef.ySet) {
                    scheduleHomeY();
                }
                if (cmd.xyzef.zSet) {
                    scheduleHomeZ();
                }
            }
            WARN_ON(cmd.xyzef.eSet);
            WARN_ON(cmd.xyzef.fSet);
            break;
        case GcodeG29:
            s_performBedLeveling();
            break;
        case GcodeG90:
            inRelativeMode = false;
            break;
        case GcodeG91:
            inRelativeMode = true;
            break;
        case GcodeG92:
            if (cmd.xyzef.xSet)
                currentState.x =
                    fix16_mul_int32(cmd.xyzef.x, STEPPER_STEPS_PER_MM[X_AXIS]);
            if (cmd.xyzef.ySet)
                currentState.y =
                    fix16_mul_int32(cmd.xyzef.y, STEPPER_STEPS_PER_MM[Y_AXIS]);
            if (cmd.xyzef.zSet)
                currentState.z =
                    fix16_mul_int32(cmd.xyzef.z, STEPPER_STEPS_PER_MM[Z_AXIS]);
            if (cmd.xyzef.eSet)
                currentState.e =
                    fix16_mul_int32(cmd.xyzef.e, STEPPER_STEPS_PER_MM[E_AXIS]);
            break;
        case GcodeM82:
            eRelativeMode = false;
            break;
        case GcodeM83:
            eRelativeMode = true;
            break;
        case GcodeM84:
            currentState.homedX = false;
            currentState.homedY = false;
            currentState.homedZ = false;
            platformDisableStepper(0xFF);
            break;
        default:
            panic();
            break;
        }
        commandAvailable = false;
    }
}

static void sendStepperJob(const struct PlannerJob *j)
{
    QueueHandle_t queue = StepperExecutionQueue;
    job_t job = { 0 };
    for_each_stepper(i) {
        motion_block_t *mb = &job.blocks[i];
        const struct PlannerBlock *pb = &j->steppers[i];
        mb->totalSteps = pb->x;
    }
    job.totalStepEvents = j->x;
    job.accelerateUntil = j->accelerationX;
    job.decelerateAfter = j->x - j->decelerationX;

    job.entry_steps_s = (uint32_t)sqrtf((float)j->viSq);
    job.cruise_steps_s = (uint32_t)sqrtf((float)j->vSq);
    job.exit_steps_s = (uint32_t)sqrtf((float)j->vfSq);
    job.accel_steps_s2 = j->a;

    job.stepDirs = j->stepDirs;
    job.type = j->type;

    switch (j->type) {
    case StepperJobRun:
    case StepperJobHomeX:
    case StepperJobHomeY:
    case StepperJobHomeZ:
        while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
            ;
        break;
    case StepperJobUndef:
    default:
        WARN();
    }
}

static bool s_executePlannerJobs(void)
{
    struct PlannerJob j;
    if (plannerSize() > 0) {
        __dequeuePlan(&j);
        sendStepperJob(&j);
    }
    return plannerSize() > 0;
}

uint32_t s_scheduleHomeMove(enum JobType k, const int32_t plan[NR_STEPPERS],
                            const fix16_t unitVec[X_AND_Y],
                            const uint32_t maxV[NR_STEPPERS],
                            const uint32_t acc[NR_STEPPERS], fix16_t len)
{
    uint32_t stepsMoved = 0;
    xTaskNotifyStateClear(NULL);
    __enqueuePlan(k, plan, unitVec, maxV, acc, len, true);
    // Empty Planner Queue
    while (s_executePlannerJobs())
        ;
    xTaskNotifyWait(0, 0, &stepsMoved, portMAX_DELAY);
    return stepsMoved;
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    for (;;) {
        enqueueAvailableGcode();
        s_executePlannerJobs();
    }
}

void notifyHomeISR(uint32_t stepsMoved)
{
    xTaskNotifyFromISR(stepScheduleTaskHandle, stepsMoved,
                       eSetValueWithOverwrite, NULL);
}
