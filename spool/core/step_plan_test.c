#include "fix16.h"
#include "step_plan.h"
#include "munit.h"
#include <stdio.h>

static void testPlanVelocity(fix16_t aX, fix16_t bX)
{
    fix16_t aVel, bVel, aAccX, bAccX;
    __planVelocity(VEL_FIX, aX, bX, &aVel, &bVel, &aAccX, &bAccX);
    // munit_assert_int32(4, ==, fix16_to_int(sum));
    printf("====================\n");
    printf("aX %f bX %f\n", fix16_to_float(aX), fix16_to_float(bX));
    printf("aVel %f bVel %f\n", fix16_to_float(aVel), fix16_to_float(bVel));
    printf("aAccX %f bAccX %f\n", fix16_to_float(aAccX), fix16_to_float(bAccX));
}

static void testPlanMove(fix16_t dx, fix16_t dy)
{
    struct StepperPlan plan[NR_STEPPERS];
    uint8_t dirMask;
    planMove(dx, dy, &plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &dirMask);
    printf("====================\n");
    printf("aX %u bX %u\n", plan[0].totalSteps, plan[1].totalSteps);
    printf("aVel %u bVel %u\n", plan[0].cruiseVel_steps_s,
           plan[1].cruiseVel_steps_s);
    printf("aAccX %u bAccX %u\n", plan[0].accelerationSteps,
           plan[1].accelerationSteps);
    printf("aCruise %u bCruise %u\n", plan[0].cruiseSteps, plan[1].cruiseSteps);
}

static void testPlanHomeX(void)
{
    struct StepperPlan plan[NR_STEPPERS];
    uint8_t dirMask;
    planHomeX(&plan[STEPPER_A_IDX], &plan[STEPPER_B_IDX], &dirMask);
    printf("====================\n");
    printf("aX %u bX %u\n", plan[0].totalSteps, plan[1].totalSteps);
    printf("aVel %u bVel %u\n", plan[0].cruiseVel_steps_s,
           plan[1].cruiseVel_steps_s);
    printf("aAccX %u bAccX %u\n", plan[0].accelerationSteps,
           plan[1].accelerationSteps);
    printf("aCruise %u bCruise %u\n", plan[0].cruiseSteps, plan[1].cruiseSteps);
}

int main()
{
    testPlanVelocity(F16(50), F16(0));
    testPlanVelocity(F16(50), F16(50));
    testPlanVelocity(F16(0), F16(50));
    testPlanVelocity(F16(-50), F16(0));
    testPlanVelocity(F16(50), F16(-50));
    testPlanVelocity(F16(-50), F16(50));

    testPlanMove(F16(50), F16(0));
    testPlanMove(F16(50), F16(50));
    testPlanMove(F16(0), F16(50));
    testPlanMove(F16(-50), F16(0));
    testPlanMove(F16(0), F16(-50));
    testPlanMove(F16(50), F16(-50));

    testPlanHomeX();
    return 0;
}