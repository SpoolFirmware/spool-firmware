#include "step_plan.h"
#include "bitops.h"
#include <math.h>
#include "magic_config.h"

#pragma GCC diagnostic push
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Warith-conversion"

static inline fix16_t fix16_mul3(fix16_t a, fix16_t b, fix16_t c)
{
    return fix16_mul(fix16_mul(a, b), c);
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

static void __setPlan(fix16_t x, fix16_t vel, fix16_t accX2,

                      struct StepperPlan *plan)
{
    uint32_t xSteps = fix16_mul_abs(x, STEPS_PER_MM);
    uint32_t accX2Steps = fix16_mul_abs(accX2, STEPS_PER_MM);

    if (xSteps >= accX2Steps) {
        plan->cruiseSteps = xSteps - accX2Steps;
        plan->accelerationSteps = accX2Steps / 2;
    } else {
        plan->cruiseSteps = 0;
        plan->accelerationSteps = xSteps / 2;
    }

    plan->totalSteps = xSteps;
    plan->cruiseVel_steps_s = fix16_mul_abs(vel, STEPS_PER_MM);
}

static void __planMove(fix16_t maxVel, fix16_t dx, fix16_t dy,
                       struct StepperPlan *planA, struct StepperPlan *planB,
                       uint8_t *dirMask)
{
    fix16_t aX = fix16_add(dx, dy);
    fix16_t bX = fix16_sub(dx, dy);
    fix16_t aVel, bVel, a2AccX, b2AccX;

    __planVelocity(maxVel, aX, bX, &aVel, &bVel, &a2AccX, &b2AccX);

    __setPlan(aX, aVel, a2AccX, planA);
    __setPlan(bX, bVel, b2AccX, planB);

    *dirMask = (uint8_t)((aX >= 0) ? STEPPER_A : 0) |
               ((bX >= 0) ? STEPPER_B : 0);
}

void planMove(fix16_t dx, fix16_t dy, fix16_t feedrate,
              struct StepperPlan *planA, struct StepperPlan *planB,
              uint8_t *dirMask)
{
    __planMove(fix16_min(feedrate / SECONDS_IN_MIN, VEL_FIX), dx, dy, planA,
               planB, dirMask);
}

void planHomeX(struct StepperPlan *planA, struct StepperPlan *planB,
               uint8_t *dirMask)
{
    __planMove(HOMING_VEL_FIX, -F16(MAX_X), 0, planA, planB, dirMask);
}

void planHomeY(struct StepperPlan *planA, struct StepperPlan *planB,
               uint8_t *dirMask)
{
    __planMove(HOMING_VEL_FIX, 0, -F16(MAX_Y), planA, planB, dirMask);
}

/* assuming both axes have the same max velocity and acceleration */
void __planVelocity(fix16_t maxVel, fix16_t aX, fix16_t bX, fix16_t *aVel,
                    fix16_t *bVel, fix16_t *a2AccX, fix16_t *b2AccX)
{
    if (fix_abs(aX) > fix_abs(bX)) {
        __planVelocity(maxVel, bX, aX, bVel, aVel, b2AccX, a2AccX);
        return;
    }
    /* rest of calculation assumes bX > aX */
    if (bX == 0) {
        *aVel = 0;
        *bVel = 0;
        *a2AccX = 0;
        *b2AccX = 0;
        return;
    }

    fix16_t bVel_ = fix16_copy_sign(maxVel, bX);
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

    float t3_f = fix16_to_float(t3);
    float aAcc_f = fix16_to_float(aAcc);
    float m_f = fix16_to_float(m);
    float det = sqrtf(t3_f * t3_f * aAcc_f * aAcc_f - 4 * aAcc_f * m_f);
    // fix16_t det = fix16_sqrt(
    //     fix16_sub(fix16_mul4(t3, t3, aAcc, aAcc), fix16_mul3(F16(4), aAcc, m)));

    fix16_t det_divd = fix16_from_float(det / (-2 * aAcc_f));

    fix16_t t1_t_const = fix16_div(t3, F16(2));
    fix16_t t1;
    fix16_t t1_1 = t1_t_const + det_divd;
    fix16_t t1_2 = t1_t_const - det_divd;
    if ((t1_1 >= 0 && t1_1 <= t1_2) || t1_2 < 0) {
        t1 = t1_1;
    } else {
        t1 = t1_2;
    }

    fix16_t aVel_ = fix16_mul(t1, aAcc);

    fix16_t double_x1_a = fix16_mul3(aAcc, t1, t1);

    *aVel = aVel_;
    *bVel = bVel_;
    *a2AccX = double_x1_a;
    *b2AccX = double_x1_b;
}

#pragma GCC diagnostic pop
