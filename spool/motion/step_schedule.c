#include "step_schedule.h"
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

static void scheduleMoveTo(const struct PrinterState state)
{
    BUG_ON(state.x < 0);
    BUG_ON(state.y < 0);
    BUG_ON(state.z < 0);

    BUG_ON(state.x > F16(MAX_X));
    BUG_ON(state.y > F16(MAX_Y));
    BUG_ON(state.z > F16(MAX_Z));

    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);
    fix16_t dz = fix16_sub(state.z, currentState.z);
    fix16_t de = fix16_sub(state.e, currentState.e);

    /* MOVE CONSTANTS */
    const int32_t move_max_v[] = {
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL) * STEPS_PER_MM,
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL) * STEPS_PER_MM,
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL_Z) *
            STEPS_PER_MM_Z,
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL_E) *
            STEPS_PER_MM_E,
    };
    STATIC_ASSERT(ARRAY_SIZE(move_max_v) == NR_STEPPERS);

    int32_t plan[NR_STEPPERS];
    /* TODO fix fixed z inversion */
    const fix16_t dir[NR_AXES] = { dx, dy, -dz, -de };
    fix16_t unitVec[NR_AXES];
    fix16_t len;

    planCoreXy(dir, plan, unitVec, &len);
    __enqueuePlan(StepperJobRun, plan, unitVec, move_max_v, STEPPER_ACC, len, state.continuousMode);

    currentState = state;
}

static void scheduleHome(void)
{
    /* HOMING CONSTANTS */
    const static fix16_t home_x[] = { -F16(MAX_X), 0, 0, 0};
    STATIC_ASSERT(ARRAY_SIZE(home_x) == NR_AXES);

    const static fix16_t home_y[] = { 0, -F16(MAX_Y), 0, 0};
    STATIC_ASSERT(ARRAY_SIZE(home_y) == NR_AXES);

    /* TODO fix z inversion */
    const static fix16_t home_z[] = { 0, 0, F16(MAX_Z), 0};
    STATIC_ASSERT(ARRAY_SIZE(home_z) == NR_AXES);
    const static fix16_t home_z_bounce[] = { 0, 0, -F16(5), 0};
    STATIC_ASSERT(ARRAY_SIZE(home_z_bounce) == NR_AXES);

    const static int32_t home_max_v[] = {
        HOMING_VEL * STEPS_PER_MM,
        HOMING_VEL * STEPS_PER_MM,
        HOMING_VEL_Z * STEPS_PER_MM_Z,
        0,
    };
    STATIC_ASSERT(ARRAY_SIZE(home_max_v) == NR_STEPPERS);

    const static int32_t home_bounce_max_v[] = {
        HOMING_VEL * STEPS_PER_MM,
        HOMING_VEL * STEPS_PER_MM,
        HOMING_VEL_Z * STEPS_PER_MM_Z / 4,
        0,
    };
    STATIC_ASSERT(ARRAY_SIZE(home_bounce_max_v) == NR_STEPPERS);
    int32_t plan[NR_STEPPERS];
    fix16_t unitVec[NR_AXES];
    fix16_t len;

    planCoreXy(home_x, plan, unitVec, &len);
    __enqueuePlan(StepperJobHomeX, plan, unitVec, home_max_v, STEPPER_ACC, len, false);

    planCoreXy(home_y, plan, unitVec, &len);
    __enqueuePlan(StepperJobHomeY, plan, unitVec, home_max_v, STEPPER_ACC, len, false);

    planCoreXy(home_z, plan, unitVec, &len);
    __enqueuePlan(StepperJobHomeZ, plan, unitVec, home_max_v, STEPPER_ACC, len, false);

    planCoreXy(home_z_bounce, plan, unitVec, &len);
    __enqueuePlan(StepperJobRun, plan, unitVec, home_bounce_max_v, STEPPER_ACC, len, false);

    planCoreXy(home_z, plan, unitVec, &len);
    __enqueuePlan(StepperJobHomeZ, plan, unitVec, home_bounce_max_v, STEPPER_ACC, len, false);

    currentState.x = 0;
    currentState.y = 0;
    currentState.z = 0;
    currentState.e = 0;
    currentState.feedrate = F16(3000);
    currentState.continuousMode = false;
}

uint32_t getRequiredSpace(enum GcodeKind kind)
{
    switch (kind) {
    case GcodeG0:
    case GcodeG1:
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
    static struct PrinterState nextState = { 0 };

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
            nextState.x = cmd.xyzef.xSet ? cmd.xyzef.x : currentState.x;
            nextState.y = cmd.xyzef.ySet ? cmd.xyzef.y : currentState.y;
            nextState.z = cmd.xyzef.zSet ? cmd.xyzef.z : currentState.z;
            nextState.e = cmd.xyzef.eSet ? cmd.xyzef.e : currentState.e;

            WARN_ON(cmd.xyzef.f < 0);
            nextState.feedrate = (cmd.xyzef.fSet && cmd.xyzef.f != 0) ? fix16_abs(cmd.xyzef.f) :
                                                  currentState.feedrate;
            nextState.continuousMode = continuousMode;
            scheduleMoveTo(nextState);
            break;
        case GcodeG28:
            /* TODO, make this a stepper job */
            platformEnableStepper(0xFF);
            scheduleHome();
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
        mb->totalSteps = (uint32_t)abs(pb->x);
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
        while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
            ;
        break;
    case StepperJobHomeX:
        while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
            ;
        WARN_ON(ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY) == 0);
        break;
    case StepperJobHomeY:
        while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
            ;
        WARN_ON(ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY) == 0);
        break;
    case StepperJobHomeZ:
        while (xQueueSend(queue, &job, portMAX_DELAY) != pdTRUE)
            ;
        WARN_ON(ulTaskNotifyTakeIndexed(1, pdTRUE, portMAX_DELAY) == 0);
        break;
    case StepperJobUndef:
    default:
        WARN();
    }
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    struct PlannerJob j;
    for (;;) {
        enqueueAvailableGcode();
        if (plannerSize() > 0) {
            __dequeuePlan(&j);
            sendStepperJob(&j);
        }
    }
}

void notifyHomeXISR(void)
{
    vTaskNotifyGiveIndexedFromISR(stepScheduleTaskHandle, 1, NULL);
}

void notifyHomeYISR(void)
{
    vTaskNotifyGiveIndexedFromISR(stepScheduleTaskHandle, 1, NULL);
}

void notifyHomeZISR(void)
{
    vTaskNotifyGiveIndexedFromISR(stepScheduleTaskHandle, 1, NULL);
}
