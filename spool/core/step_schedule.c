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

static struct StepperJob queueBuf[STEP_QUEUE_SIZE];
static StaticQueue_t stepQueue;

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

QueueHandle_t stepTaskInit(QueueHandle_t gcodeCommandQueue_, TaskHandle_t *out)
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
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);
    const int32_t max_v[NR_STEPPERS] = {
        fix16_to_int(state.feedrate) / SECONDS_PER_MIN * STEPS_PER_MM,
        fix16_to_int(state.feedrate) / SECONDS_PER_MIN * STEPS_PER_MM
    };

    int32_t plan[NR_STEPPERS];
    const fix16_t dir[NR_AXES] = { dx, dy };

    planCoreXy(dir, plan);
    __enqueuePlan(StepperJobRun, plan, max_v, state.continuousMode);

    /* TODO, if we want to interrupt the print, we might have to
     * set the timeout to some other reasonable value, or have
     * to drain the queue */
    currentState = state;
}

static void scheduleHome(void)
{
    int32_t plan[NR_STEPPERS];
    const static fix16_t home_x[NR_AXES] = { -F16(MAX_X), 0 };
    const static fix16_t home_y[NR_AXES] = { 0, -F16(MAX_Y) };
    const static int32_t max_v[NR_STEPPERS] = { HOMING_VEL * STEPS_PER_MM,
                                                HOMING_VEL * STEPS_PER_MM };

    planCoreXy(home_x, plan);
    __enqueuePlan(StepperJobHomeX, plan, max_v, false);

    planCoreXy(home_y, plan);
    __enqueuePlan(StepperJobHomeY, plan, max_v, false);

    currentState.x = 0;
    currentState.y = 0;
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
        return 2;
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
            WARN_ON(cmd.xyzef.f < 0);
            nextState.feedrate = cmd.xyzef.f == 0 ? currentState.feedrate :
                                                    fix16_abs(cmd.xyzef.f);
            nextState.continuousMode = continuousMode;
            scheduleMoveTo(nextState);
            break;
        case GcodeG28:
            /* TODO, make this a stepper job */
            enableStepper(STEPPER_A | STEPPER_B);
            scheduleHome();
            break;
        case GcodeM84:
            /* TODO, make this a stepper job */
            enableStepper(0);
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
            job.stepDirs |= BIT(i);
        }
        // dbgPrintf("stepper%d total%u cruise%u entry%u vel%u exit%u\r\n", i,
        //           mb->totalSteps, mb->cruiseSteps, mb->entryVel_steps_s,
        //           mb->cruiseVel_steps_s, mb->exitVel_steps_s);
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
