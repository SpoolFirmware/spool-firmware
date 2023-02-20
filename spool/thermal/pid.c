#include "misc.h"
#include "string.h"
#include "thermal/pid.h"

void pidInit(pid_t *pPid)
{
    if (pPid->outputMax == 0)
        panic();

    if (pPid->effectiveRange == 0)
        pPid->effectiveRange = F16(-1);

    if (pPid->ki != 0) {
        pPid->maxOverKi =
            fix16_sub(fix16_div(pPid->outputMax, pPid->ki), pPid->outputMin);
    }
    memset(&pPid->inputHistory, 0, sizeof(pPid->inputHistory));
}

void pidReset(pid_t *pPid)
{
    pPid->iState = 0;
}

bool pidStable(pid_t *pPid)
{
    const fix16_t setPoint = pPid->setPoint;
    for (int i = 0; i < ARRAY_LENGTH(pPid->inputHistory); i++) {
        if (fix16_abs(fix16_sub(pPid->inputHistory[i], setPoint)) > F16(0.5)) {
            return false;
        }
    }
    return true;
}

fix16_t pidUpdateLoop(pid_t *pPid, fix16_t input)
{
    // Make sure we never heat if the target is 0.
    if (!pPid->setPoint) {
        return 0;
    }

    fix16_t error = fix16_sub(pPid->setPoint, input);
    fix16_t dInput;
    fix16_t output;

    fix16_t oldInput = pPid->inputHistory[pPid->head];
    pPid->inputHistory[pPid->head] = input;
    pPid->head = (pPid->head + 1) % ARRAY_LENGTH(pPid->inputHistory);
    dInput = fix16_sub(input, oldInput);

    if (fix16_abs(error) > pPid->effectiveRange) {
        pidReset(pPid);
        if (error > F16(0)) {
            return pPid->outputMax;
        } else {
            return pPid->outputMin;
        }
    }

    // Cap OutputSum
    pPid->iState = fix16_clamp(fix16_add(pPid->iState, error), -pPid->maxOverKi,
                               pPid->maxOverKi);

    const fix16_t dVal = fix16_mul(pPid->kd, dInput);
    output = fix16_mul(fix16_add(pPid->setPoint, pPid->b), pPid->m); // Offset
    output = fix16_add(output, fix16_mul(pPid->kp, error)); // P
    output = fix16_add(output, fix16_mul(pPid->ki, pPid->iState)); // I
    output = fix16_sub(output, dVal); // D

    output = fix16_clamp(output, pPid->outputMin, pPid->outputMax);

    return output;
}