#include "step_schedule.h"
#include "step_plan.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "error.h"
#include "platform/platform.h"
#include "bitops.h"
#include "dbgprintf.h"

struct StepperJob queueBuf[STEP_QUEUE_SIZE];
StaticQueue_t stepQueue;

const fix16_t STEPS_PER_MM_FIX = F16(160);

static const struct PrinterState startState = {
    .x = F16(50),
    .y = F16(50),
};

#define MAGIC_PRINTER_STATES 4
static const struct PrinterState states[MAGIC_PRINTER_STATES] = {
    { .x = F16(50), .y = F16(0) },
    { .x = F16(0), .y = F16(50) },
    { .x = F16(50), .y = F16(50) },
    { .x = F16(0), .y = F16(0) },
};

/* Records the state of the printer ends up in after the stepper queue
 * finishes. We could have the state be attached to each stepper job.
 * However, if we do not care about displaying the state of the print
 * head, then this is sufficient.
 */
/* only touched in scheduleMoveTo */
static struct PrinterState currentState;

#define STACK_SIZE 1024
static StaticTask_t stepScheduleTaskBuf;
static StackType_t stepScheduleStack[STACK_SIZE];
static TaskHandle_t stepScheduleTaskHandle;

QueueHandle_t stepTaskInit(void)
{
    QueueHandle_t handle = xQueueCreateStatic(STEP_QUEUE_SIZE,
                                              sizeof(struct StepperJob),
                                              (uint8_t *)queueBuf, &stepQueue);
    stepScheduleTaskHandle = xTaskCreateStatic(
        stepScheduleTask, "stepSchedule", STACK_SIZE, (void *)handle,
        tskIDLE_PRIORITY + 2, stepScheduleStack, &stepScheduleTaskBuf);
    return handle;
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
                           const struct PrinterState state, bool start)
{
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);

    job_t job = { 0 };
    struct StepperPlan plan[NR_STEPPERS];
    planMove(dx, dy, &plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &job.stepDirs);

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        __fillJob(&job.blocks[i], &plan[i]);
    }

    job.type = start ? StepperJobStart : StepperJobRun;

    while (xQueueSend(handle, &job, (unsigned int)-1) != pdTRUE)
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

    while (xQueueSend(handle, &job, (unsigned int)-1) != pdTRUE)
        ;

    if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == 0) {
        /* homing X failed */
        panic();
    }

    planHomeY(&plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &job.stepDirs);

    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        __fillJob(&job.blocks[i], &plan[i]);
    }
    job.type = StepperJobHomeY;

    while (xQueueSend(handle, &job, (unsigned int)-1) != pdTRUE)
        ;

    if (ulTaskNotifyTake(pdFALSE, portMAX_DELAY) == 0) {
        /* homing Y failed */
        panic();
    }

    dbgPrintf("homing completed");
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    int x = 0;

    enableStepper(STEPPER_A | STEPPER_B);
    scheduleHome(queue);

    // scheduleMoveTo(queue, startState, true);
    // for (;; ++x, x = x % MAGIC_PRINTER_STATES) {
    //     scheduleMoveTo(queue, states[x], false);
    // }
    for(;;);
}

void notifyHomeXISR(void)
{
    vTaskNotifyGiveFromISR(stepScheduleTaskHandle, NULL);
}

void notifyHomeYISR(void)
{
    vTaskNotifyGiveFromISR(stepScheduleTaskHandle, NULL);
}
