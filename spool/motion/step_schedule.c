#include "step_schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>
#include "error.h"
#include "platform/platform.h"
#include "bitops.h"
#include "dbgprintf.h"
#include "gcode/gcode.h"
#include "number.h"
#include "compiler.h"
#include "misc.h"
#include "core/spool.h"
#include "motion/kinematic.h"
#include "motion/motion.h"
#include "thermal/thermal.h"
#include "configuration.h"
#include "step_plan_ng.h"
#include "fix16.h"

/* Records the state of the printer ends up in after the stepper queue
 * finishes. We could have the state be attached to each stepper job.
 * However, if we do not care about displaying the state of the print
 * head, then this is sufficient.
 */
/* mainly used by scheduleMoveTo */
static struct PrinterState currentState;

#define lvlX1                             \
    (platformBedLevelingCorners[0].x_mm * \
     PLATFORM_MOTION_STEPS_PER_MM_AXIS(X_AXIS))
#define lvlX2 (platformBedLevelingCorners[1].x_mm * PLATFORM_MOTION_STEPS_PER_MM_AXIS(X_AXIS))
#define lvlY1 (platformBedLevelingCorners[0].y_mm * PLATFORM_MOTION_STEPS_PER_MM_AXIS(Y_AXIS))
#define lvlY2 (platformBedLevelingCorners[1].y_mm * PLATFORM_MOTION_STEPS_PER_MM(Y_AXIS))

static struct {
    /*
        12 ----- 22
        |         |
        |         |
        11 ----- 21
    */
    // {11, 21, 22, 12}
    int32_t points[4];
    int32_t current;
} bedLevelingFactors = { 0 };

static inline int32_t s_lerpi32(int32_t x1, int32_t v1, int32_t x2, int32_t v2,
                                int32_t x)
{
    return ((x2 - x) * v1 / (x2 - x1)) + ((x - x1) * v2 / (x2 - x1));
}

static inline int32_t s_evaulateBedLevelingForCurrentPosition(int32_t x,
                                                              int32_t y)
{
    if (!PLATFORM_FEATURE_ENABLED(BedLeveling)) {
        return 0;
    }

    int32_t xy1 = s_lerpi32(lvlX1, bedLevelingFactors.points[0], lvlX2,
                            bedLevelingFactors.points[1], x);
    int32_t xy2 = s_lerpi32(lvlX1, bedLevelingFactors.points[3], lvlX2,
                            bedLevelingFactors.points[2], x);
    return -s_lerpi32(lvlY1, xy1, lvlY2, xy2, y);
}

static inline void s_bedLevelingReset(void)
{
    memset(&bedLevelingFactors, 0, sizeof(bedLevelingFactors));
}

static uint32_t s_scheduleHomeMove(enum JobType k,
                                   const int32_t plan[NR_STEPPER],
                                   const fix16_t unitVec[X_AND_Y],
                                   const uint32_t maxV[NR_STEPPER],
                                   const uint32_t acc[NR_STEPPER], fix16_t len);

TaskHandle_t stepScheduleTaskHandle;

static uint32_t moveAcceleration[NR_STEPPER];
static uint32_t homeVelocity[NR_STEPPER];
static uint32_t homeAcceleration[NR_STEPPER];

static bool extrudersEnabled = true;
static bool steppersEnabled = false;

static bool inRelativeMode = false;
static bool eRelativeMode = false;

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
    for_each_stepper(i)
        moveAcceleration[i] = motionGetDefaultAcceleration(i);
    for_each_stepper(i)
        homeVelocity[i] = motionGetHomingVelocity(i);
    for_each_stepper(i)
        homeAcceleration[i] = motionGetHomingAcceleration(i);
}

static void scheduleMoveTo(const struct PrinterMove state,
                           uint32_t maxVel[NR_STEPPER])
{
    BUG_ON(state.x < 0);
    BUG_ON(state.y < 0);

    BUG_ON(state.x > (platformMotionLimits[X_AXIS] *
                      platformMotionStepsPerMMAxis[X_AXIS]));
    BUG_ON(state.y > (platformMotionLimits[Y_AXIS] *
                      platformMotionStepsPerMMAxis[Y_AXIS]));
    BUG_ON(state.z > (platformMotionLimits[Z_AXIS] *
                      platformMotionStepsPerMMAxis[Z_AXIS]));

    int32_t dzLeveling = 0, zLevelingTarget;

    int32_t dx = currentState.homedX ? (state.x - currentState.x) : 0;
    int32_t dy = currentState.homedY ? (state.y - currentState.y) : 0;
    int32_t dz = currentState.homedZ ? (state.z - currentState.z) : 0;
    int32_t de = state.e - currentState.e;

    if (PLATFORM_FEATURE_ENABLED(BedLeveling)) {
        zLevelingTarget =
            s_evaulateBedLevelingForCurrentPosition(state.x, state.y);
        dzLeveling = zLevelingTarget - bedLevelingFactors.current;
    }

    /* MOVE CONSTANTS */
    if (!maxVel) {
        static uint32_t moveMaxV[NR_STEPPER];
        for_each_stepper(i)
            moveMaxV[i] = motionGetMaxVelocity(i);
        maxVel = moveMaxV;
    }

    int32_t plan[NR_STEPPER];
    /* TODO fix fixed z inversion */
    const int32_t dir[NR_AXIS] = { dx, dy, dz + dzLeveling, de };
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planMove(dir, plan, unitVec, &len);
    plannerEnqueueMove(StepperJobRun, plan, unitVec, maxVel, moveAcceleration,
                       len, !currentState.continuousMode);

    currentState.x = currentState.homedX ? state.x : currentState.x;
    currentState.y = currentState.homedY ? state.y : currentState.y;
    currentState.z = currentState.homedZ ? state.z : currentState.z;
    currentState.e = state.e;

    if (PLATFORM_FEATURE_ENABLED(BedLeveling)) {
        bedLevelingFactors.current = zLevelingTarget;
    }
}

static void scheduleHomeX(void)
{
    const static int32_t home_x[NR_AXIS] = {
        -PLATFORM_MOTION_LIMIT(X_AXIS) *
            PLATFORM_MOTION_STEPS_PER_MM_AXIS(X_AXIS),
        0,
        0,
    };
    int32_t plan[NR_STEPPER];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planMove(home_x, plan, unitVec, &len);
    s_scheduleHomeMove(StepperJobHomeX, plan, unitVec, homeVelocity,
                       homeAcceleration, len);
    currentState.x = 0;
    currentState.homedX = true;
}

static void scheduleHomeY(void)
{
    const static int32_t home_y[NR_AXIS] = {
        0,
        -PLATFORM_MOTION_LIMIT(Y_AXIS) *
            PLATFORM_MOTION_STEPS_PER_MM_AXIS(Y_AXIS),
        0,
    };
    int32_t plan[NR_STEPPER];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planMove(home_y, plan, unitVec, &len);
    s_scheduleHomeMove(StepperJobHomeY, plan, unitVec, homeVelocity,
                       homeAcceleration, len);
    currentState.y = 0;
    currentState.homedY = true;
}

static uint32_t s_scheduleZMeasure(uint32_t velSteps)
{
    const static int32_t home_z[NR_AXIS] = {
        0,
        0,
        -PLATFORM_MOTION_LIMIT(Z_AXIS) *
            PLATFORM_MOTION_STEPS_PER_MM_AXIS(Z_AXIS),
    };
    static uint32_t rehomeZVelocity[NR_STEPPER];
    for_each_stepper(i)
        rehomeZVelocity[i] = motionGetHomingVelocity(i);
    rehomeZVelocity[Z_AXIS] = velSteps;

    int32_t plan[NR_STEPPER];
    fix16_t unitVec[X_AND_Y];
    fix16_t len;

    planMove(home_z, plan, unitVec, &len);
    return s_scheduleHomeMove(StepperJobHomeZ, plan, unitVec, rehomeZVelocity,
                              homeAcceleration, len);
}

static void scheduleHomeZ(void)
{
    if (!(currentState.homedX && currentState.homedY))
        return;
    s_bedLevelingReset();
    /* TODO fix z inversion */
    int32_t savedX = 0, savedY = 0;
    struct PrinterMove home_z_move = { 0 };
    home_z_move.e = currentState.e;
    home_z_move.x =
        platformFeatureZHomingPos.x_mm * platformMotionStepsPerMMAxis[X_AXIS];
    home_z_move.y =
        platformFeatureZHomingPos.y_mm * platformMotionStepsPerMMAxis[Y_AXIS];

    /* move x and y to the right spot to home z */
    if (home_z_move.x || home_z_move.y) {
        savedX = currentState.x;
        savedY = currentState.y;
        scheduleMoveTo(home_z_move, homeVelocity);
    }
    while (s_scheduleZMeasure(motionGetHomingVelocity(Z_AXIS)) == 0) {
        currentState.homedZ = true;
        currentState.z = 0;

        home_z_move.z = 5 * platformMotionStepsPerMMAxis[Z_AXIS];
        scheduleMoveTo(home_z_move, homeVelocity);
    }
    
    currentState.homedZ = true;
    currentState.z = 0;
    home_z_move.z = 5 * platformMotionStepsPerMMAxis[Z_AXIS];
    scheduleMoveTo(home_z_move, homeVelocity);

    s_scheduleZMeasure(motionGetHomingVelocity(STEPPER_C) / 4);
    currentState.z = 0;

    if (platformFeatureZOffset != 0) {
        currentState.z = (int32_t)(platformFeatureZOffset *
                                   (float)platformMotionStepsPerMMAxis[Z_AXIS]);

        home_z_move.x = savedX;
        home_z_move.y = savedY;
        home_z_move.z = 0;
        scheduleMoveTo(home_z_move, homeVelocity);
    }
}

static void s_performBedLeveling(void)
{
    if (!(currentState.homedX && currentState.homedY && currentState.homedZ)) {
        return;
    }

    // Reset leveling factor
    memset(&bedLevelingFactors, 0, sizeof(bedLevelingFactors));

    if (!PLATFORM_FEATURE_ENABLED(BedLeveling))
        return;

    const struct {
        int32_t x;
        int32_t y;
    } BedLevelingPositions[4] = {
        { lvlX1, lvlY1 },
        { lvlX2, lvlY1 },
        { lvlX2, lvlY2 },
        { lvlX1, lvlY2 },
    };

    struct PrinterMove saved;
    saved.x = currentState.x;
    saved.y = currentState.y;
    saved.z = currentState.z;
    saved.e = currentState.e;

    // Probe (5,10), (120, 10), (5, 155)
    static struct PrinterMove move_position = {
        .z = 10 * PLATFORM_MOTION_STEPS_PER_MM_AXIS(Z_AXIS),
    };
    int32_t travels[ARRAY_LENGTH(BedLevelingPositions)];
    for (int i = 0; i < ARRAY_LENGTH(BedLevelingPositions); i++) {
        move_position.x = BedLevelingPositions[i].x;
        move_position.y = BedLevelingPositions[i].y;
        scheduleMoveTo(move_position, homeVelocity);
        int32_t actualTravel =
            (int32_t)s_scheduleZMeasure(motionGetHomingVelocity(STEPPER_C) / 4);
        currentState.z -= actualTravel;
        travels[i] = actualTravel;
        scheduleMoveTo(move_position, homeVelocity);
    }
    move_position.x =
        platformFeatureZHomingPos.x_mm * platformMotionStepsPerMMAxis[X_AXIS];
    move_position.y =
        platformFeatureZHomingPos.y_mm * platformMotionStepsPerMMAxis[Y_AXIS];
    scheduleMoveTo(move_position, homeVelocity);
    memcpy(bedLevelingFactors.points, travels, sizeof(travels));
    bedLevelingFactors.current =
        s_evaulateBedLevelingForCurrentPosition(currentState.x, currentState.y);

    scheduleMoveTo(saved, homeVelocity);
}

uint32_t getRequiredSpace(enum GcodeKind kind)
{
    switch (kind) {
    case GcodeG90:
    case GcodeG91:
    case GcodeG92:
    case GcodeM82:
    case GcodeM83:
    case GcodeM101:
    case GcodeM103:
        return 0;
    case GcodeG0:
    case GcodeG1:
    case GcodeM84:
    case GcodeISRSync:
        return 1;
    /* require planner queue to be empty, G28/29 routinely flushes planner */
    case GcodeG29:
    case GcodeG28:
        return plannerCapacity();
    default:
        panic();
    }
    return 0;
}

#define CONTINUOUS_THRESHOLD 5

static void s_enqueueG0G1(struct GcodeCommand *cmd, bool continuousMode)
{
    static struct PrinterMove nextState = { 0 };
    nextState.x = cmd->xyzef.xSet ?
                      ((inRelativeMode ? currentState.x : 0) +
                       fix16_mul_int32(cmd->xyzef.x,
                                       platformMotionStepsPerMMAxis[X_AXIS])) :
                      currentState.x;
    nextState.y = cmd->xyzef.ySet ?
                      ((inRelativeMode ? currentState.y : 0) +
                       fix16_mul_int32(cmd->xyzef.y,
                                       platformMotionStepsPerMMAxis[Y_AXIS])) :
                      currentState.y;
    nextState.z = cmd->xyzef.zSet ?
                      ((inRelativeMode ? currentState.z : 0) +
                       fix16_mul_int32(cmd->xyzef.z,
                                       platformMotionStepsPerMMAxis[Z_AXIS])) :
                      currentState.z;
    if (extrudersEnabled) {
        nextState.e =
            cmd->xyzef.eSet ?
                (((eRelativeMode || inRelativeMode) ? currentState.e : 0) +
                 fix16_mul_int32(cmd->xyzef.e,
                                 platformMotionStepsPerMMAxis[E_AXIS])) :
                currentState.e;
    } else {
        nextState.e = currentState.e;
    }

    WARN_ON(cmd->xyzef.f < 0);
    if (cmd->xyzef.fSet && cmd->xyzef.f != 0) {
        for_each_stepper(i) {
            const int32_t clampedVelMM =
                min(abs(fix16_to_int(cmd->xyzef.f)) / SECONDS_PER_MIN,
                    platformMotionDefaultMaxVel[i]);
            motionSetMaxVelocity(i, platformMotionStepsPerMM[i] * clampedVelMM);
        }
    }
    currentState.continuousMode = continuousMode;

    bool motionOutOfRange =
        (platformMotionLimits[X_AXIS] &&
         OUT_OF_RANGE(nextState.x, 0,
                      platformMotionLimits[X_AXIS] *
                          platformMotionStepsPerMMAxis[X_AXIS])) ||
        (platformMotionLimits[Y_AXIS] &&
         OUT_OF_RANGE(nextState.y, 0,
                      platformMotionLimits[Y_AXIS] *
                          platformMotionStepsPerMMAxis[Y_AXIS])) ||
        (platformMotionLimits[Z_AXIS] &&
         OUT_OF_RANGE(nextState.z, 0,
                      platformMotionLimits[Z_AXIS] *
                          platformMotionStepsPerMMAxis[Z_AXIS]));
    if (!motionOutOfRange)
        scheduleMoveTo(nextState, NULL);
    else
        WARN();
}

static void enqueueAvailableGcode()
{
    static bool commandAvailable = false;
    static struct GcodeCommand cmd;
    bool continuousMode = false;

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

        /* only accept G28 and sync when steppers are disabled */
        if (!steppersEnabled && cmd.kind != GcodeG28 &&
            cmd.kind != GcodeISRSync) {
            commandAvailable = false;
            return;
        }

        if (plannerAvailableSpace() < getRequiredSpace(cmd.kind)) {
            return;
        }

        switch (cmd.kind) {
        case GcodeISRSync:
            plannerEnqueueNotify(StepperJobSync, thermalTaskHandle,
                                 cmd.seq.seqNumber);
            break;
        case GcodeG0:
        case GcodeG1:
            s_enqueueG0G1(&cmd, continuousMode);
            break;
        case GcodeG28:
            plannerEnqueue(StepperJobEnableSteppers);
            steppersEnabled = true;
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
            eRelativeMode = false;
            break;
        case GcodeG91:
            inRelativeMode = true;
            break;
        case GcodeG92:
            if (cmd.xyzef.xSet)
                currentState.x = fix16_mul_int32(
                    cmd.xyzef.x, platformMotionStepsPerMMAxis[X_AXIS]);
            if (cmd.xyzef.ySet)
                currentState.y = fix16_mul_int32(
                    cmd.xyzef.y, platformMotionStepsPerMMAxis[Y_AXIS]);
            if (cmd.xyzef.zSet)
                currentState.z = fix16_mul_int32(
                    cmd.xyzef.z, platformMotionStepsPerMMAxis[Z_AXIS]);
            if (cmd.xyzef.eSet)
                currentState.e = fix16_mul_int32(
                    cmd.xyzef.e, platformMotionStepsPerMMAxis[E_AXIS]);
            break;
        case GcodeM82:
            eRelativeMode = false;
            break;
        case GcodeM83:
            eRelativeMode = true;
            break;
        case GcodeM101:
            extrudersEnabled = true;
            break;
        case GcodeM103:
            extrudersEnabled = false;
            break;
        case GcodeM84:
            plannerEnqueue(StepperJobDisableSteppers);
            steppersEnabled = false;
            currentState.homedX = false;
            currentState.homedY = false;
            currentState.homedZ = false;
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
    job.type = j->type;

    switch (j->type) {
    /* move */
    case StepperJobRun:
    case StepperJobHomeX:
    case StepperJobHomeY:
    case StepperJobHomeZ:
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
        break;
    /* no move */
    case StepperJobSync:
        job.notify = j->notify;
        job.seq = j->seq;
        break;
    case StepperJobEnableSteppers:
    case StepperJobDisableSteppers:
        break;
    case StepperJobUndef:
    default:
        PR_WARN("unknown job type %d\n", j->type);
        return;
    }
    while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
        ;
}

static bool s_executePlannerJobs(void)
{
    struct PlannerJob j;
    if (plannerSize() > 0) {
        plannerDequeue(&j);
        sendStepperJob(&j);
    }
    return plannerSize() > 0;
}

uint32_t s_scheduleHomeMove(enum JobType k, const int32_t plan[NR_STEPPER],
                            const fix16_t unitVec[X_AND_Y],
                            const uint32_t maxV[NR_STEPPER],
                            const uint32_t acc[NR_STEPPER], fix16_t len)
{
    uint32_t stepsMoved = 0;
    xTaskNotifyStateClear(NULL);
    plannerEnqueueMove(k, plan, unitVec, maxV, acc, len, true);
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
