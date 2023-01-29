#pragma once
#include "fix16.h"
#include "magic_config.h"

static const fix16_t VEL_FIX = F16(VEL);
static const fix16_t HOMING_VEL_FIX = F16(HOMING_VEL);
static const fix16_t ACC_FIX = F16(ACC);

struct StepperPlan {
    uint32_t totalSteps;
    uint32_t accelerationSteps;
    uint32_t cruiseSteps;
    uint32_t cruiseVel_steps_s;
};

void planMove(fix16_t dx, fix16_t dy, struct StepperPlan *planA,
              struct StepperPlan *planB, uint8_t *dirMask);

void planHomeX(struct StepperPlan *planA, struct StepperPlan *planB,
               uint8_t *dirMask);

void planHomeY(struct StepperPlan *planA, struct StepperPlan *planB,
               uint8_t *dirMask);

void __planVelocity(fix16_t maxVel, fix16_t aX, fix16_t bX, fix16_t *aVel,
                    fix16_t *bVel, fix16_t *aAccX, fix16_t *bAccX);
