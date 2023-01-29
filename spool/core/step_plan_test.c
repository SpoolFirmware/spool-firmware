#include "fix16.h"
#include "step_plan.h"
#include "munit.h"
#include <stdio.h>

static void testPlanVelocity(fix16_t aX, fix16_t bX)
{
    fix16_t aVel, bVel, aAccX, bAccX;
    planVelocity(aX, bX, &aVel, &bVel, &aAccX, &bAccX);
    // munit_assert_int32(4, ==, fix16_to_int(sum));
    printf("====================\n");
    printf("aX %f bX %f\n", fix16_to_float(aX), fix16_to_float(bX));
    printf("aVel %f bVel %f\n", fix16_to_float(aVel), fix16_to_float(bVel));
    printf("aAccX %f bAccX %f\n", fix16_to_float(aAccX), fix16_to_float(bAccX));
}
int main()
{
    testPlanVelocity(F16(50), F16(0));
    testPlanVelocity(F16(50), F16(50));
    testPlanVelocity(F16(0), F16(50));
    testPlanVelocity(F16(-50), F16(0));
    testPlanVelocity(F16(50), F16(-50));
    return 0;
}