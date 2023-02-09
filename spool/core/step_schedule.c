#include "step_schedule.h"
#include "step_plan_ng.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "error.h"
#include "platform/platform.h"
#include "bitops.h"
#include "dbgprintf.h"
#include "gcode_parser.h"
#include "number.h"
#include "compiler.h"

static struct StepperJob queueBuf[STEP_QUEUE_SIZE];
static StaticQueue_t stepQueue;

/* HOMING CONSTANTS */
const static fix16_t home_x[] = { -F16(MAX_X), 0, 0 };
STATIC_ASSERT(ARRAY_SIZE(home_x) == NR_AXES);
const static fix16_t home_y[] = { 0, -F16(MAX_Y), 0 };
STATIC_ASSERT(ARRAY_SIZE(home_y) == NR_AXES);
/* TODO fix z inversion */
const static fix16_t home_z[] = { 0, 0, F16(MAX_Z) };
STATIC_ASSERT(ARRAY_SIZE(home_z) == NR_AXES);
const static int32_t home_max_v[] = {
    HOMING_VEL * STEPS_PER_MM,
    HOMING_VEL *STEPS_PER_MM,
    HOMING_VEL *STEPS_PER_MM_Z,
};
STATIC_ASSERT(ARRAY_SIZE(home_max_v) == NR_STEPPERS);

/* Records the state of the printer ends up in after the stepper queue
 * finishes. We could have the state be attached to each stepper job.
 * However, if we do not care about displaying the state of the print
 * head, then this is sufficient.
 */
/* only touched in scheduleMoveTo */
static struct PrinterState currentState;
static QueueHandle_t gcodeCommandQueue;

#define STACK_SIZE 1024
static StaticTask_t stepScheduleTaskBuf;
static StackType_t stepScheduleStack[STACK_SIZE];
static TaskHandle_t stepScheduleTaskHandle;

QueueHandle_t stepTaskInit(QueueHandle_t gcodeCommandQueue_)
{
    QueueHandle_t handle = xQueueCreateStatic(STEP_QUEUE_SIZE,
                                              sizeof(struct StepperJob),
                                              (uint8_t *)queueBuf, &stepQueue);
    configASSERT(handle);
    configASSERT(gcodeCommandQueue_);
    gcodeCommandQueue = gcodeCommandQueue_;
    stepScheduleTaskHandle = xTaskCreateStatic(
        stepScheduleTask, "stepSchedule", STACK_SIZE, (void *)handle,
        tskIDLE_PRIORITY + 2, stepScheduleStack, &stepScheduleTaskBuf);
    return handle;
}

#define SECONDS_PER_MIN 60

static void scheduleMoveTo(const struct PrinterState state)
{
    BUG_ON(state.x < 0);
    BUG_ON(state.y < 0);
    BUG_ON(state.z < 0);
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);
    fix16_t dz = fix16_sub(state.z, currentState.z);

    /* MOVE CONSTANTS */
    const int32_t move_max_v[] = {
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL) * STEPS_PER_MM,
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL) * STEPS_PER_MM,
        min(fix16_to_int(state.feedrate) / SECONDS_PER_MIN, VEL_Z) *
            STEPS_PER_MM_Z,
    };
    STATIC_ASSERT(ARRAY_SIZE(move_max_v) == NR_STEPPERS);

    int32_t plan[NR_STEPPERS];
    /* TODO fix fixed z inversion */
    const fix16_t dir[NR_AXES] = { dx, dy, -dz };

    planCoreXy(dir, plan);
    __enqueuePlan(StepperJobRun, plan, move_max_v, state.continuousMode);

    currentState = state;
}

static void scheduleHome(void)
{
    int32_t plan[NR_STEPPERS];

    planCoreXy(home_x, plan);
    __enqueuePlan(StepperJobHomeX, plan, home_max_v, false);

    planCoreXy(home_y, plan);
    __enqueuePlan(StepperJobHomeY, plan, home_max_v, false);

    planCoreXy(home_z, plan);
    __enqueuePlan(StepperJobHomeZ, plan, home_max_v, false);

    currentState.x = 0;
    currentState.y = 0;
    currentState.z = 0;
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
    }
    WARN();
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
        continuousMode = uxQueueSpacesAvailable(gcodeCommandQueue) <
                         CONTINUOUS_THRESHOLD;
        if (!commandAvailable) {
            if (xQueueReceive(gcodeCommandQueue, &cmd,
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
            nextState.x = cmd.xyzef.x;
            nextState.y = cmd.xyzef.y;
            nextState.z = cmd.xyzef.z;
            WARN_ON(cmd.xyzef.f < 0);
            nextState.feedrate = cmd.xyzef.f == 0 ? currentState.feedrate :
                                                    fix16_abs(cmd.xyzef.f);
            nextState.continuousMode = continuousMode;
            scheduleMoveTo(nextState);
            break;
        case GcodeG28:
            /* TODO, make this a stepper job */
            platformEnableStepper(STEPPER_A | STEPPER_B | STEPPER_C);
            scheduleHome();
            break;
        case GcodeM84:
            /* TODO, make this a stepper job */
            platformEnableStepper(0);
            break;
        }
        commandAvailable = false;
    }
}

static void sendStepperJob(QueueHandle_t queue, const struct PlannerJob *j)
{
    job_t job = { 0 };
    for_each_stepper(i) {
        motion_block_t *mb = &job.blocks[i];
        const struct PlannerBlock *pb = &j->steppers[i];
        mb->totalSteps = (uint32_t)abs(pb->x);
        mb->accelerationSteps = (uint32_t)abs(pb->accelerationX);
        mb->decelerationSteps = (uint32_t)abs(pb->decelerationX);
        mb->cruiseSteps =
            mb->totalSteps - mb->accelerationSteps - mb->decelerationSteps;
        mb->entryVel_steps_s = (uint32_t)sqrtf((float)pb->viSq);
        mb->cruiseVel_steps_s = (uint32_t)sqrtf((float)pb->vSq);
        mb->exitVel_steps_s = (uint32_t)sqrtf((float)pb->vfSq);
        if (pb->x >= 0) {
            job.stepDirs |= (uint8_t)BIT(i);
        }
    }
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
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    struct PlannerJob j;

    for (;;) {
        enqueueAvailableGcode();
        if (plannerSize() > 0) {
            __dequeuePlan(&j);
            sendStepperJob(queue, &j);
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
