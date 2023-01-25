#include "clock.h"
#include "hal/clock.h"
#include "drf.h"
#include "manual/mcu.h"
#include "error.h"
#include <stdint.h>

static void configurePll(uint32_t q, uint32_t p, uint32_t n, uint32_t m)
{
    if (!(2 <= q && q <= 15)) {
        panic();
    }
    if (!(192 <= n && n <= 432)) {
        panic();
    }
    if (!(2 <= m && m <= 63)) {
        panic();
    }
    /* PLL values M25 N336 P4 Q7 */
    uint32_t pll_cfgr = REG_RD32(DRF_REG(_RCC, _PLLCFGR));

    switch (p) {
    case 2:
        pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP, _DIV2, pll_cfgr);
        break;
    case 4:
        pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP, _DIV4, pll_cfgr);
        break;
    case 6:
        pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP, _DIV6, pll_cfgr);
        break;
    case 8:
        pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLP, _DIV8, pll_cfgr);
        break;
    default:
        panic();
    }

    pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLQ, q, pll_cfgr);
    pll_cfgr = FLD_SET_DRF(_RCC, _PLLCFGR, _PLLSRC, _HSE, pll_cfgr);
    pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLN, n, pll_cfgr);
    pll_cfgr = FLD_SET_DRF_NUM(_RCC, _PLLCFGR, _PLLM, m, pll_cfgr);

    REG_WR32(DRF_REG(_RCC, _PLLCFGR), pll_cfgr);

    uint32_t cr = REG_RD32(DRF_REG(_RCC, _CR));
    cr = FLD_SET_DRF(_RCC, _CR, _PLLON, _ON, cr);
    REG_WR32(DRF_REG(_RCC, _CR), cr);
}

static void configureHse(void)
{
    uint32_t cr = REG_RD32(DRF_REG(_RCC, _CR));
    cr = FLD_SET_DRF(_RCC, _CR, _HSEON, _ON, cr);
    REG_WR32(DRF_REG(_RCC, _CR), cr);
    while (FLD_TEST_DRF(_RCC, _CR, _HSERDY, _NOT_READY,
                        REG_RD32(DRF_REG(_RCC, _CR))))
        ;
}

static void configurePrescalers(uint32_t hseFreqHz, uint32_t apb2Prescaler,
                                uint32_t apb1Prescaler, uint32_t ahbPrescaler)
{
    if (!(2000000 <= hseFreqHz && hseFreqHz <= 31000000)) {
        panic();
    }

    uint32_t cfgr = REG_RD32(DRF_REG(_RCC, _CFGR));

    cfgr = FLD_SET_DRF_NUM(_RCC, _CFGR, _RTCPRE, (hseFreqHz / 1000000), cfgr);

    switch (apb2Prescaler) {
    case 1:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE2, _DIV1, cfgr);
        break;
    case 2:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE2, _DIV2, cfgr);
        break;
    case 4:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE2, _DIV4, cfgr);
        break;
    case 8:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE2, _DIV8, cfgr);
        break;
    case 16:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE2, _DIV16, cfgr);
        break;
    default:
        panic();
    }
    switch (apb1Prescaler) {
    case 1:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV1, cfgr);
        break;
    case 2:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV2, cfgr);
        break;
    case 4:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV4, cfgr);
        break;
    case 8:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV8, cfgr);
        break;
    case 16:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PPRE1, _DIV16, cfgr);
        break;
    default:
        panic();
    }

    switch (ahbPrescaler) {
    case 1:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV1, cfgr);
        break;
    case 2:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV2, cfgr);
        break;
    case 4:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV4, cfgr);
        break;
    case 8:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV8, cfgr);
        break;
    case 16:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV16, cfgr);
        break;
    case 64:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV64, cfgr);
        break;
    case 128:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV128, cfgr);
        break;
    case 256:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV256, cfgr);
        break;
    case 512:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _HPRE, _DIV512, cfgr);
        break;
    default:
        panic();
    }

    REG_WR32(DRF_REG(_RCC, _CFGR), cfgr);
}

static void switchClockSource(void)
{
    uint32_t cfgr = REG_RD32(DRF_REG(_RCC, _CFGR));
    cfgr = FLD_SET_DRF(_RCC, _CFGR, _SW, _PLL, cfgr);
    REG_WR32(DRF_REG(_RCC, _CFGR), cfgr);
}

static void configureVoltage(void)
{
    uint32_t rcc_apb1enr = REG_RD32(DRF_REG(_RCC, _APB1ENR));
    rcc_apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _PWREN, _ENABLED, rcc_apb1enr);
    REG_WR32(DRF_REG(_RCC, _APB1ENR), rcc_apb1enr);

    uint32_t pwr_cr = REG_RD32(DRF_REG(_PWR, _CR));
    /* scale 2 mode */
    pwr_cr = FLD_SET_DRF_NUM(_PWR, _CR, _VOS, 2, pwr_cr);
    REG_WR32(DRF_REG(_PWR, _CR), pwr_cr);
}

static void waitForVosReady(void)
{
    // wait for power ready
    while (
        FLD_TEST_DRF(_PWR, _CSR, _VOSRDY, _CLR, REG_RD32(DRF_REG(_PWR, _CSR))))
        ;
}

static void waitForPllReady(void)
{
    while (FLD_TEST_DRF(_RCC, _CR, _PLLRDY, _NOT_READY,
                        REG_RD32(DRF_REG(_RCC, _CR))))
        ;
}

static void configureFlash(void)
{
    uint32_t flash_acr = REG_RD32(DRF_REG(_FLASH, _ACR));
    /* stm32f401 wait state 2 */
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _LATENCY, _WS2, flash_acr);
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _PRFTEN, _ENABLED, flash_acr);
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _ICEN, _ENABLED, flash_acr);
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _DCEN, _ENABLED, flash_acr);
    REG_WR32(DRF_REG(_FLASH, _ACR), flash_acr);

    while (!FLD_TEST_DRF(_FLASH, _ACR, _LATENCY, _WS2,
                         REG_RD32(DRF_REG(_FLASH, _ACR))))
        ;
}

void halClockInit(const struct HalClockConfig *config)
{
    configureVoltage();
    configureHse();
    configurePll(config->q, config->p, config->n, config->m);
    waitForVosReady();
    waitForPllReady();
    configurePrescalers(config->hseFreqHz, config->apb2Prescaler,
                        config->apb1Prescaler, config->ahbPrescaler);
    configureFlash();
    switchClockSource();
}

uint32_t halClockSysclkFreqGet(const struct HalClockConfig *cfg)
{
    return ((cfg->hseFreqHz * (cfg->n / cfg->m)) / cfg->p);
}

uint32_t halClockAhbFreqGet(const struct HalClockConfig *cfg)
{
    return halClockSysclkFreqGet(cfg) / cfg->ahbPrescaler;
}

uint32_t halClockApb1FreqGet(const struct HalClockConfig *cfg)
{
    return halClockAhbFreqGet(cfg) / cfg->apb1Prescaler;
}

uint32_t halClockApb1TimerFreqGet(const struct HalClockConfig *cfg)
{
    if (cfg->apb1Prescaler == 1) {
        return halClockAhbFreqGet(cfg);
    }
    else 
    {
        return halClockAhbFreqGet(cfg) * 2;
    }
}

uint32_t halClockApb2FreqGet(const struct HalClockConfig *cfg)
{
    return halClockAhbFreqGet(cfg) / cfg->apb2Prescaler;
}

uint32_t halClockApb2TimerFreqGet(const struct HalClockConfig *cfg)
{
    const uint32_t apb2Freq = halClockAhbFreqGet(cfg);
    if (cfg->apb2Prescaler == 1) {
        return apb2Freq;
    }
    else 
    {
        return apb2Freq * 2;
    }
}
