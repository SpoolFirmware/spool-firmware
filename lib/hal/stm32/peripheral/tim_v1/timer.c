#include "misc.h"
#include "manual/mcu.h"

#include "timer.h"

void halTimerConstruct(struct TimerDriver *pDriver, size_t deviceBase)
{
    pDriver->base = deviceBase;
}

void halTimerStart(struct TimerDriver *pDriver,
                   const struct TimerConfig *pConfig)
{
    uint32_t preScaler =
        pConfig->clkDomainFrequency / pConfig->timerTargetFrequency;
    if (preScaler > 1) {
        preScaler -= 1;
    }

    REG_WR32(pDriver->base + DRF_TIM1_PSC, preScaler);
    REG_WR32(pDriver->base + DRF_TIM1_CR1, 0);
    REG_WR32(pDriver->base + DRF_TIM1_CNT, 0);
    if (pConfig->interruptEnable) {
        halTimerIrqClear(pDriver);
        REG_WR32(pDriver->base + DRF_TIM1_DIER,
                 DRF_DEF(_TIM1, _DIER, _UIE, _ENABLED));
    }
}

void halTimerStop(struct TimerDriver *pDriver)
{
    REG_WR32(pDriver->base + DRF_TIM1_CR1, 0);
}

bool halTimerPending(const struct TimerDriver *pDriver)
{
    return FLD_TEST_DRF(_TIM1, _SR, _UIF, _UPDATE_PENDING,
                        REG_RD32(pDriver->base + DRF_TIM1_SR));
}

void halTimerIrqClear(const struct TimerDriver *pDriver)
{
    REG_WR32(pDriver->base + DRF_TIM1_SR,
             ~DRF_DEF(_TIM1, _SR, _UIF, _UPDATE_PENDING));
}

void halTimerStartContinous(struct TimerDriver *pDriver, uint32_t reloadValue)
{
    if (reloadValue == 0)
        panic();

    REG_WR32(pDriver->base + DRF_TIM1_ARR, reloadValue);
    uint32_t cr1 = 0;
    cr1 |= FLD_SET_DRF(_TIM1, _CR1, _CEN, _ENABLED, cr1);
    REG_WR32(pDriver->base + DRF_TIM1_CR1, cr1);
}

void halTimerChangeReloadValue(struct TimerDriver *pDriver,
                               uint32_t reloadValue)
{
    if (reloadValue == 0)
        panic();

    REG_WR32(pDriver->base + DRF_TIM1_ARR, reloadValue);
}

void halTimerStartOneShot(struct TimerDriver *pDriver, uint32_t reloadValue)
{
    if (reloadValue == 0)
        panic();

    REG_WR32(pDriver->base + DRF_TIM1_ARR, reloadValue);
    uint32_t cr1 = 0;
    cr1 |= FLD_SET_DRF(_TIM1, _CR1, _CEN, _ENABLED, cr1);
    cr1 |= FLD_SET_DRF(_TIM1, _CR1, _OPM, _ENABLED, cr1);
    REG_WR32(pDriver->base + DRF_TIM1_CR1, cr1);
}
