#include "step_schedule.h"
#include "step_plan.h"
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include "platform/platform.h"
#include "bitops.h"

struct StepperJob queueBuf[STEP_QUEUE_SIZE];
StaticQueue_t stepQueue;

const fix16_t STEPS_PER_MM_FIX = F16(160);

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

static uint32_t fix16_mul_abs(fix16_t a, int32_t b)
{
    int64_t product = (int64_t)a * b;
    fix16_t result = (fix16_t)(product >> 16);
    result += (fix16_t)((product & 0x8000) >> 15);
    if (result == fix16_minimum) {
        // minimum negative number has same representation as
        // its absolute value in unsigned
        return 0x80000000;
    } else {
        return result >= 0 ? (uint32_t)result : (uint32_t)-result;
    }
}

static void scheduleMoveTo(QueueHandle_t handle,
                           const struct PrinterState state)
{
    fix16_t dx = fix16_sub(state.x, currentState.x);
    fix16_t dy = fix16_sub(state.y, currentState.y);
    /* we may lose steps here, should we accumulate error? */
    fix16_t aX = fix16_add(dx, dy);
    fix16_t bX = fix16_sub(dx, dy);
    fix16_t aVel, bVel, aAccX, bAccX;
    /* sqrt(vf / JERK) */

    uint8_t dirMask = ((aX >= 0) ? STEPPER_A : 0) | ((bX >= 0) ? STEPPER_B : 0);

    planVelocity(aX, bX, &aVel, &bVel, &aAccX, &bAccX);

    uint32_t aXSteps = fix16_mul_abs(aX, STEPS_PER_MM);
    uint32_t bXSteps = fix16_mul_abs(bX, STEPS_PER_MM);
    uint32_t aVelSteps = fix16_mul_abs(aVel, STEPS_PER_MM);
    uint32_t bVelSteps = fix16_mul_abs(bVel, STEPS_PER_MM);
    uint32_t aAccXSteps = fix16_mul_abs(aAccX, STEPS_PER_MM);
    uint32_t bAccXSteps = fix16_mul_abs(bAccX, STEPS_PER_MM);

    uint32_t aCruise = aXSteps > 2 * aAccXSteps ? aXSteps - 2 * aAccXSteps : 0;
    uint32_t bCruise = bXSteps > 2 * bAccXSteps ? bXSteps - 2 * bAccXSteps : 0;

    job_t job = {
        .blocks = { 
{
            .totalSteps = aXSteps,
            .accelerationSteps = aAccXSteps,
            .cruiseSteps = aCruise,
            .decelerationSteps = aAccXSteps,

            .entryVel_steps_s = 0,
            .cruiseVel_steps_s = aVelSteps,
            .exitVel_steps_s = 0,
            .blockState = BlockStateAccelerating,
                    },
                    {
            .totalSteps = bXSteps,
            .accelerationSteps = bAccXSteps,
            .cruiseSteps = bCruise,
            .decelerationSteps = bAccXSteps,

            .entryVel_steps_s = 0,
            .cruiseVel_steps_s = bVelSteps,
            .exitVel_steps_s = 0,
            .blockState = BlockStateAccelerating,
},
        },
        .stepDirs = dirMask,
    };

    while (xQueueSend(handle, &job, (unsigned int)-1) != pdTRUE)
        ;

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
