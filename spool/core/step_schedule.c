#include <stdint.h>
#include <stdbool.h>

#include "misc.h"
#include "dbgprintf.h"

#include "platform/platform.h"

#include "spool.h"

#include "step_schedule.h"
#include "step_plan.h"
/* #include <math.h> */
#include "gcode/gcode.h"

/* Records the state of the printer ends up in after the stepper queue
 * finishes. We could have the state be attached to each stepper job.
 * However, if we do not care about displaying the state of the print
 * head, then this is sufficient.
 */
/* only touched in scheduleMoveTo */
static struct PrinterState currentState;

static TaskHandle_t stepScheduleTaskHandle;

void motionPlannerTaskInit(void)
{
    MotionPlannerTaskQueue = xQueueCreate(MOTION_PLANNER_TASK_QUEUE_SIZE,
                                          sizeof(struct GcodeCommand));
    if (MotionPlannerTaskQueue == NULL)
        panic();

    StepperExecutionQueue =
        xQueueCreate(STEPPER_EXECUTION_QUEUE_SIZE, sizeof(struct StepperJob));
    configASSERT(StepperExecutionQueue);

    if (xTaskCreate(stepScheduleTask, "motionPlanner", 1024, NULL,
                    tskIDLE_PRIORITY + 1, &stepScheduleTaskHandle) != pdTRUE) {
        panic();
    }
}

static void __fillJob(motion_block_t *block, const struct StepperPlan *plan)
{
    block->totalSteps = plan->totalSteps;
    block->accelerationSteps = plan->accelerationSteps;
    block->cruiseSteps = plan->cruiseSteps;
    block->decelerationSteps = plan->accelerationSteps;
    block->cruiseVel_steps_s = plan->cruiseVel_steps_s;
}

static void scheduleMoveTo(QueueHandle_t handle,
                           const struct PrinterState state)
{
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);

    job_t job = { 0 };
    struct StepperPlan plan[NR_STEPPERS];
    planMove(dx, dy, state.feedrate, &plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX],
             &job.stepDirs);

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        __fillJob(&job.blocks[i], &plan[i]);
    }

    job.type = StepperJobRun;

    while (xQueueSend(handle, &job, portMAX_DELAY) != pdTRUE)
        ;

    /* TODO, if we want to interrupt the print, we might have to
     * set the timeout to some other reasonable value, or have
     * to drain the queue */
    currentState = state;
}

static void scheduleHome(QueueHandle_t handle)
{
    job_t job = { 0 };
    struct StepperPlan plan[NR_STEPPERS];
    planHomeX(&plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &job.stepDirs);

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        __fillJob(&job.blocks[i], &plan[i]);
    }
    job.type = StepperJobHomeX;

    while (xQueueSend(handle, &job, portMAX_DELAY) != pdTRUE)
        ;

    /* homing X failed */
    WARN_ON(ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == 0);

    currentState.x = 0;

    planHomeY(&plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &job.stepDirs);

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        __fillJob(&job.blocks[i], &plan[i]);
    }
    job.type = StepperJobHomeY;

    while (xQueueSend(handle, &job, portMAX_DELAY) != pdTRUE)
        ;

    /* homing Y failed */
    WARN_ON(ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == 0);

    currentState.y = 0;
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    struct GcodeCommand cmd;
    struct PrinterState nextState;

    for (;;) {
        if (xQueueReceive(MotionPlannerTaskQueue, &cmd, portMAX_DELAY) ==
            pdTRUE) {
            switch (cmd.kind) {
            case GcodeG0:
            case GcodeG1:
                nextState.x = cmd.xyzef.x;
                nextState.y = cmd.xyzef.y;
                WARN_ON(cmd.xyzef.f < 0);
                nextState.feedrate = cmd.xyzef.f == 0 ? currentState.feedrate :
                                                        fix16_abs(cmd.xyzef.f);
                scheduleMoveTo(StepperExecutionQueue, nextState);
                break;
            case GcodeG28:
                platformEnableStepper(STEPPER_A | STEPPER_B);
                scheduleHome(StepperExecutionQueue);
                break;
            case GcodeM84:
                platformDisableStepper(0xFF);
                break;
            default:
                panic();
                break;
            }
        }
    }
}

void notifyHomeXISR(void)
{
    vTaskNotifyGiveFromISR(stepScheduleTaskHandle, NULL);
}

void notifyHomeYISR(void)
{
    vTaskNotifyGiveFromISR(stepScheduleTaskHandle, NULL);
}
