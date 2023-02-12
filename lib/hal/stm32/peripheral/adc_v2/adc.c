#include <stdint.h>

#include "misc.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "adc.h"
#include "adc_private.h"


static StaticSemaphore_t adcSemaphoresObjs[NUM_ADC];
static SemaphoreHandle_t adcSemaphores[NUM_ADC] = {0};

static const uint32_t ADCList[] = {
    DRF_BASE(DRF_ADC1),

    #if NUM_ADC > 1
    DRF_BASE(DRF_ADC2),
    #endif

    #if NUM_ADC > 2
    DRF_BASE(DRF_ADC3)
    #endif
};

void halADCInit(struct ADCDriver *pD, struct ADCConfig *pCfg)
{
    if (pD == NULL || pCfg == NULL) {
        panic();
    }
    if (adcSemaphores[0] != NULL) {
        panic();
    }

    for (int i = 0; i < NUM_ADC; i++) {
        adcSemaphores[i] = xSemaphoreCreateBinaryStatic(&adcSemaphoresObjs[i]);
        if (adcSemaphores[i] == NULL) {
            panic();
        }
    }

    uint32_t commonCCR = 0;

    // Set COMMON_CCR_ADCPRE
    const uint8_t divFactor = ROUND_UP(pCfg->adcParentClockSpeed, pCfg->adcClkMaxFreq);
    if (divFactor >= 8) {
        commonCCR = FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV8, commonCCR);
    } else if (divFactor >= 6) {
        commonCCR = FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV6, commonCCR);
    } else if (divFactor >= 4) {
        commonCCR = FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV4, commonCCR);
    } else {
        commonCCR = FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV2, commonCCR);
    }
}

void halADCStart(struct ADCDriver *pD)
{
    if (pD == NULL) {
        panic();
    }
    
    REG_WR32(DRF_REG(_ADC_COMMON, _CCR), pD->commonCCR);

    for (int i = 0; i < ARRAY_LENGTH(ADCList); i++) {
        REG_WR32(ADCList[i] + DRF_ADC1_CR1, DRF_DEF(_ADC1, _CR1, _EOCIE, _ENABLED));
        REG_WR32(ADCList[i] + DRF_ADC1_CR2, DRF_DEF(_ADC1, _CR2, _ADON, _ENABLED) |
                 DRF_DEF(_ADC1, _CR2, _EOCS, _EACH_CONVERSION));
    }
}

uint8_t halADCGetResolutionBits(struct ADCDriver *pD)
{

}

uint16_t halADCSampleStream(struct ADCDriver *pD, uint32_t streamId)
{

}