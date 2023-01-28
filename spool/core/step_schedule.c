#include "step_schedule.h"
#include <stdint.h>
#include <stdbool.h>
#include "platform/platform.h"
#include "bitops.h"

struct StepperJob queueBuf[STEP_QUEUE_SIZE];
StaticQueue_t stepQueue;

const fix16_t STEPS_PER_MM_FIX = F16(160);
const fix16_t VEL_FIX = F16(VEL);
const fix16_t ACC_FIX = F16(ACC);

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

static inline fix16_t fix16_mul3(fix16_t a, fix16_t b, fix16_t c)
{
    return fix16_mul(fix16_mul(a, b), c);
}

static inline fix16_t fix16_mul4(fix16_t a, fix16_t b, fix16_t c, fix16_t d)
{
    return fix16_mul(fix16_mul(a, b), fix16_mul(c, d));
}

/* assuming both axes have the same max velocity and acceleration */
static void planVelocity(fix16_t aX, fix16_t bX, fix16_t *aVel, fix16_t *bVel,
                         fix16_t *aAccX, fix16_t *bAccX)
{
    if (fix_abs(aX) > fix_abs(bX)) {
        planVelocity(bX, aX, bVel, aVel, bAccX, aAccX);
        return;
    }
    /* rest of calculation assumes bX > aX */
    if (bX == 0) {
        *aVel = 0;
        *bVel = 0;
        return;
    }

    fix16_t bVel_ = fix16_copy_sign(VEL_FIX, bX);
    fix16_t bAcc = fix16_copy_sign(ACC_FIX, bX);

    fix16_t t1_b = fix16_div(bVel_, bAcc);
    fix16_t double_x1_b = fix16_mul3(bAcc, t1_b, t1_b);
    if (fix_abs(bX) < fix_abs(double_x1_b)) {
        t1_b = fix16_sqrt(fix16_div(bX, bAcc));
        double_x1_b = bX;
        bVel_ = fix16_mul(t1_b, bAcc);
    }

    fix16_t t3 = fix16_add(fix16_mul(F16(2), t1_b),
                           fix16_div(fix16_sub(bX, double_x1_b), bVel_));

    fix16_t m = fix16_add(fix16_sub(aX, bX), fix16_sub(fix16_mul(bVel_, t3),
                                                       fix16_mul(bVel_, t1_b)));

    fix16_t aAcc = fix16_copy_sign(ACC_FIX, aX);

    fix16_t det = fix16_sqrt(
        fix16_sub(fix16_mul4(t3, t3, aAcc, aAcc), fix16_mul3(F16(4), aAcc, m)));

    fix16_t t1_t_denom = fix16_mul(F16(-2), aAcc);
    fix16_t t1_t_const = fix16_mul(-t3, aAcc);
    fix16_t t1 = fix16_div(fix16_add(t1_t_const, det), t1_t_denom);
    if (t1 < 0) {
        t1 = fix16_div(fix16_sub(t1_t_const, det), t1_t_denom);
    }

    fix16_t aVel_ = fix16_mul(t1, aAcc);

    fix16_t double_x1_a = fix16_mul3(aAcc, t1, t1);

    *aVel = aVel_;
    *bVel = bVel_;
    *aAccX = fix16_div(double_x1_a, F16(2));
    *bAccX = fix16_div(double_x1_b, F16(2));
}

static uint32_t fix16_mul_abs(fix16_t a, int32_t b)
{
    int64_t product = (int64_t)a * b;
    fix16_t result = product >> 16;
    result += (product & 0x8000) >> 15;
    return result >= 0 ? result : -result;
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

    uint32_t aXSteps = fix16_mul_abs(aX, STEPS_PER_MM_FIX);
    uint32_t bXSteps = fix16_mul_abs(bX, STEPS_PER_MM_FIX);
    uint32_t aVelSteps = fix16_mul_abs(aVel, STEPS_PER_MM_FIX);
    uint32_t bVelSteps = fix16_mul_abs(bVel, STEPS_PER_MM_FIX);
    uint32_t aAccXSteps = fix16_mul_abs(aAccX, STEPS_PER_MM_FIX);
    uint32_t bAccXSteps = fix16_mul_abs(bAccX, STEPS_PER_MM_FIX);

    job_t job = {
        .blocks = { 
{
            .totalSteps = aXSteps,
            .accelerationSteps = aAccXSteps,
            .cruiseSteps = aXSteps - 2 * aAccXSteps,
            .decelerationSteps = aAccXSteps,

            .entryVel_steps_s = 0,
            .cruiseVel_steps_s = aVelSteps,
            .exitVel_steps_s = 0,
            .blockState = BlockStateAccelerating,
                    },
                    {
            .totalSteps = bXSteps,
            .accelerationSteps = bAccXSteps,
            .cruiseSteps = bXSteps - 2 * bAccXSteps,
            .decelerationSteps = bAccXSteps,

            .entryVel_steps_s = 0,
            .cruiseVel_steps_s = bVelSteps,
            .exitVel_steps_s = 0,
            .blockState = BlockStateAccelerating,
},
        },
        .stepDirs = dirMask,
    };

    while (xQueueSend(handle, &job, -1) != pdTRUE)
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
