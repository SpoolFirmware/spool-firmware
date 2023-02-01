#include "clock.h"
#include "hal/clock.h"
#include "drf.h"
#include "manual/mcu.h"
#include "error.h"
#include <stdint.h>

static void configureHse(void)
{
    uint32_t cr = REG_RD32(DRF_REG(_RCC, _CR));
    cr = FLD_SET_DRF(_RCC, _CR, _HSEON, _ON, cr);
    REG_WR32(DRF_REG(_RCC, _CR), cr);
    while (FLD_TEST_DRF(_RCC, _CR, _HSERDY, _NOT_READY,
                        REG_RD32(DRF_REG(_RCC, _CR))))
        ;
}

static void configurePrescalers(const struct HalClockConfig *config)
{
    uint32_t cfgr = REG_RD32(DRF_REG(_RCC, _CFGR));

    if (config->pllXtPre == 2U) {
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PLLXTPRE, _DIV2, cfgr);
    } else {
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _PLLXTPRE, _DIV1, cfgr);
    }

    if (config->usbPre1_5X == 0U) {
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _USBPRE, _DIV1, cfgr);
    } else {
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _USBPRE, _DIV1_5, cfgr);
    }

    if (config->pllMul >= 2 && config->pllMul <= 16) {
        cfgr = FLD_SET_DRF_NUM(_RCC, _CFGR, _PLLMUL, (config->pllMul - 2), cfgr);
    } else {
        panic();
    }

    // TODO: support HSI PLL
    // PLL use HSE 
    cfgr = FLD_SET_DRF(_RCC, _CFGR, _PLLSRC, _HSE__DIV_PREDIV, cfgr);

    switch (config->apb2Prescaler) {
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
    switch (config->apb1Prescaler) {
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

    switch (config->ahbPrescaler) {
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

    switch(config->adcPrescaler) {
    case 2:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _ADCPRE, _DIV2, cfgr);
        break;
    case 4:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _ADCPRE, _DIV4, cfgr);
        break;
    case 6:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _ADCPRE, _DIV6, cfgr);
        break;
    case 8:
        cfgr = FLD_SET_DRF(_RCC, _CFGR, _ADCPRE, _DIV8, cfgr);
        break;
    default:
        panic();
    }

    REG_WR32(DRF_REG(_RCC, _CFGR), cfgr);
}

static void switchClockSource(void)
{
    REG_FLD_SET_DRF(_RCC, _CFGR, _SW, _PLL);
    while(!REG_FLD_TEST_DRF(_RCC, _CFGR, _SWS, _PLL))
        ;
}

static void enablePLL(void)
{
    REG_FLD_SET_DRF(_RCC, _CR, _PLLON, _ON);
    while (FLD_TEST_DRF(_RCC, _CR, _PLLRDY, _NOT_READY,
                        REG_RD32(DRF_REG(_RCC, _CR))))
        ;
}

static void configureFlash(void)
{
    uint32_t flash_acr = REG_RD32(DRF_REG(_FLASH, _ACR));
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _PRFTBE, _SET, flash_acr);
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _HLFCYA, _CLR, flash_acr);
    flash_acr = FLD_SET_DRF(_FLASH, _ACR, _LATENCY, _WS2, flash_acr);
    REG_WR32(DRF_REG(_FLASH, _ACR), flash_acr);

    while (!FLD_TEST_DRF(_FLASH, _ACR, _LATENCY, _WS2,
                         REG_RD32(DRF_REG(_FLASH, _ACR))))
        ;
}

void halClockInit(const struct HalClockConfig *config)
{
    configureHse();

    // Switch to HSI
    REG_WR32(DRF_REG(_RCC, _CR), FLD_SET_DRF(_RCC, _CR, _HSION, _ON, REG_RD32(DRF_REG(_RCC, _CR))));
    while(FLD_TEST_DRF(_RCC, _CR, _HSIRDY, _NOT_READY, REG_RD32(DRF_REG(_RCC, _CR))))
        ;
    REG_WR32(DRF_REG(_RCC, _CR), 0);
    while(!FLD_TEST_DRF(_RCC, _CFGR, _SWS, _HSI, REG_RD32(DRF_REG(_RCC, _CFGR))))
        ;
    
    // Enable HSE
    REG_FLD_SET_DRF(_RCC, _CR, _HSEON, _ON);
    while(REG_FLD_TEST_DRF(_RCC, _CR, _HSERDY, _NOT_READY))
        ;
        
    // Configure scalers and muxes
    configurePrescalers(config);
    // Enable PLL
    enablePLL();

    configureFlash();
    // Moment of truth.

    switchClockSource();
}

uint32_t halClockSysclkFreqGet(const struct HalClockConfig *cfg)
{
    if (cfg->pllXtPre == 2) {
        return (cfg->hseFreqHz / cfg->ahbPrescaler / 2) * cfg->pllMul;
    }
    return (cfg->hseFreqHz / cfg->ahbPrescaler) * cfg->pllMul;
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
    const uint32_t apb1Freq = halClockApb1FreqGet(cfg);
    if (cfg->apb1Prescaler == 1) {
        return apb1Freq;
    }
    else
    {
        return apb1Freq * 2;
    }
}

uint32_t halClockApb2FreqGet(const struct HalClockConfig *cfg)
{
    return halClockAhbFreqGet(cfg) / cfg->apb2Prescaler;
}

uint32_t halClockApb2TimerFreqGet(const struct HalClockConfig *cfg)
{
    const uint32_t apb2Freq = halClockApb2FreqGet(cfg);
    if (cfg->apb2Prescaler == 1) {
        return apb2Freq;
    }
    else
    {
        return apb2Freq * 2;
    }
}
