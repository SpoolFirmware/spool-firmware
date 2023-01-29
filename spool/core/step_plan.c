#include "step_plan.h"
#include <math.h>
#include "magic_config.h"

const fix16_t VEL_FIX = F16(VEL);
const fix16_t ACC_FIX = F16(ACC);

static inline fix16_t fix16_mul3(fix16_t a, fix16_t b, fix16_t c)
{
    return fix16_mul(fix16_mul(a, b), c);
}

/* assuming both axes have the same max velocity and acceleration */
void planVelocity(fix16_t aX, fix16_t bX, fix16_t *aVel, fix16_t *bVel,
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
        *aAccX = 0;
        *bAccX = 0;
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

    float t3_f = fix16_to_float(t3);
    float aAcc_f = fix16_to_float(aAcc);
    float m_f = fix16_to_float(m);
    float det = sqrtf(t3_f * t3_f * aAcc_f * aAcc_f - 4 * aAcc_f * m_f);
    // fix16_t det = fix16_sqrt(
    //     fix16_sub(fix16_mul4(t3, t3, aAcc, aAcc), fix16_mul3(F16(4), aAcc, m)));

    fix16_t t1_t_denom = fix16_mul(F16(-2), aAcc);

    fix16_t det_divd = fix16_from_float(det / (-2 * aAcc_f));

    fix16_t t1_t_const = fix16_div(fix16_mul(-t3, aAcc), t1_t_denom);
    fix16_t t1 = t1_t_const + det_divd;
    if (t1 < 0) {
        t1 = t1_t_const - det_divd;
    }

    fix16_t aVel_ = fix16_mul(t1, aAcc);

    fix16_t double_x1_a = fix16_mul3(aAcc, t1, t1);

    *aVel = aVel_;
    *bVel = bVel_;
    *aAccX = fix16_div(double_x1_a, F16(2));
    *bAccX = fix16_div(double_x1_b, F16(2));
}