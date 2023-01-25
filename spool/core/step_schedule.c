#include "step_schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "bitops.h"

struct StepperJob queueBuf[STEP_QUEUE_SIZE];

const fix16_t STEPS_PER_MM = F16(160);

StaticQueue_t stepQueue;

// static const struct StepperJob jobs[2] = {
//     {
//         .interval = { 10, 10 },
//         .steps = { 10000, 10000 },
//         .stepperDirs = BIT(0) | BIT(1),
//     },
//     {
//         .interval = { 10, 10 },
//         .steps = { 10000, 10000 },
//         .stepperDirs = 0,
//     },
// };

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

QueueHandle_t stepTaskInit(void)
{
    QueueHandle_t handle = xQueueCreateStatic(STEP_QUEUE_SIZE,
                                              sizeof(struct StepperJob),
                                              (uint8_t *)queueBuf, &stepQueue);
    xTaskCreate(stepScheduleTask, "stepSchedule", 1024, (void *)handle,
                tskIDLE_PRIORITY + 2, (TaskHandle_t *)NULL);
    return handle;
}

inline uint16_t abs(int16_t x)
{
    return (x >= 0) ? x : -x;
}

const uint16_t STARTUP_STEPS = 400;
const uint16_t STARTUP_STEP = 10;

static void scheduleMoveTo(QueueHandle_t handle,
                           const struct PrinterState state)
{
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);
    /* we may lose steps here, should we accumulate error? */
    int16_t aStepsDir = fix16_to_int(fix16_mul(fix16_add(dx, dy), STEPS_PER_MM));
    int16_t bStepsDir = fix16_to_int(fix16_mul(fix16_sub(dx, dy), STEPS_PER_MM));
    /* sqrt(vf / JERK) */

    uint8_t dirMask = ((aStepsDir >= 0) ? STEPPER_A : 0) |
                      ((bStepsDir >= 0) ? STEPPER_B : 0);

    uint16_t stepsTotal[NR_STEPPERS] = { abs(aStepsDir), abs(bStepsDir) };
    uint16_t stepsUp[NR_STEPPERS];
    uint16_t stepsDown[NR_STEPPERS];
    uint16_t stepsRun[NR_STEPPERS] = { 0 };
    for (uint8_t i = 0; i < NR_STEPPERS; ++i) {
        stepsUp[i] = (stepsTotal[i] + 1) / 2;
        if (stepsUp[i] > STARTUP_STEPS) {
            stepsRun[i] += stepsUp[i] - STARTUP_STEPS;
            stepsUp[i] = STARTUP_STEPS;
        }
        stepsDown[i] = stepsTotal[i] / 2;
        if (stepsDown[i] > STARTUP_STEPS) {
            stepsRun[i] += stepsDown[i] - STARTUP_STEPS;
            stepsDown[i] = STARTUP_STEPS;
        }
    }

    uint16_t steps[NR_STEPPERS];
    for (int8_t i = 40; i >= 1; --i) {
        for (uint8_t j = 0; j < NR_STEPPERS; ++j) {
            if (stepsUp[j] > STARTUP_STEP && i > 1) {
                steps[j] = STARTUP_STEP;
                stepsUp[j] -= STARTUP_STEP;
            } else {
                steps[j] = stepsUp[j];
                stepsUp[j] = 0;
            }
        }
        struct StepperJob job = {
            .stepperDirs = dirMask,
            .steps = { steps[0], steps[1] },
            .interval = { i, i },
        };
        xQueueSend(handle, &job, -1);
    }

    struct StepperJob runJob = {
        .stepperDirs = dirMask,
        .steps = { stepsRun[0], stepsRun[1] },
        .interval = { 0 , 0 },
    };
    xQueueSend(handle, &runJob, -1);

    for (int8_t i = 1; i <= 40; ++i) {
        for (uint8_t j = 0; j < NR_STEPPERS; ++j) {
            if (stepsDown[j] > STARTUP_STEP && i < 40) {
                steps[j] = STARTUP_STEP;
                stepsDown[j] -= STARTUP_STEP;
            } else {
                steps[j] = stepsDown[j];
                stepsDown[j] = 0;
            }
        }
        struct StepperJob job = {
            .stepperDirs = dirMask,
            .steps = { steps[0], steps[1] },
            .interval = { i, i },
        };
        xQueueSend(handle, &job, -1);
    }

    /* TODO, if we want to interrupt the print, we might have to
     * set the timeout to some other reasonable value, or have
     * to drain the queue */
    currentState = state;
}

portTASK_FUNCTION(stepScheduleTask, pvParameters)
{
    QueueHandle_t queue = (QueueHandle_t)pvParameters;
    int x = 0;

    enableStepper(STEPPER_A | STEPPER_B);
    for (;; ++x, x = x % MAGIC_PRINTER_STATES) {
        scheduleMoveTo(queue, states[x]);
    }
}
