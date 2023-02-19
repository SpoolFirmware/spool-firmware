#include <stdint.h>

#include "misc.h"

#include "FreeRTOS.h"
#include "semphr.h"

#include "adc.h"
#include "adc_private.h"

#include "hal/cortexm/hal.h"

/**
 * @brief   The adcMutex guarentees only one thread can start a sample operation
 *          at any given point.
 */
static SemaphoreHandle_t adcMutex = NULL;

/**
 * @brief   The adcSemaphore provides synchorization / notificaton mechanism for a
 *          task to sleep while waiting for adc result.
 */
static SemaphoreHandle_t adcSemaphore = NULL;

static const uint32_t ADCList[] = {
    DRF_BASE(DRF_ADC1),

#if NUM_ADC > 1
    DRF_BASE(DRF_ADC2),
#endif

#if NUM_ADC > 2
    DRF_BASE(DRF_ADC3),
#endif
};

static inline uint32_t s_getADCBaseFromStream(uint32_t streamId)
{
    const uint32_t acdIndex = DRF_VAL(_HAL_ADC, _STREAM, _ADC, streamId);
    if (acdIndex >= NUM_ADC) {
        panic();
    }
    return ADCList[acdIndex];
}

IRQ_HANDLER_ADC(void)
{
    xSemaphoreGiveFromISR(adcSemaphore, NULL);
    for (int i = 0; i < ARRAY_LENGTH(ADCList); i++) {
        REG_WR32(ADCList[i] + DRF_ADC1_SR, 0);
    }
    halIrqClear(IRQ_ADC);
}

void halADCInit(struct ADCDriver *pD, struct ADCConfig *pCfg)
{
    if (pD == NULL || pCfg == NULL) {
        panic();
    }
    if (adcSemaphore != NULL || adcMutex != NULL) {
        panic();
    }

    if ((adcSemaphore = xSemaphoreCreateBinary()) == NULL) {
        panic();
    }
    if ((adcMutex = xSemaphoreCreateMutex()) == NULL) {
        panic();
    }

    REG_FLD_SET_DRF(_RCC, _APB2ENR, _ADC1EN, _ENABLED);
#if NUM_ADC > 1
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _ADC2EN, _ENABLED);
#endif
#if NUM_ADC > 2
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _ADC3EN, _ENABLED);
#endif

    uint32_t commonCCR = 0;

    // Set COMMON_CCR_ADCPRE
    const uint8_t divFactor =
        ROUND_UP(pCfg->adcParentClockSpeed, pCfg->adcClkMaxFreq);
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
        REG_WR32(ADCList[i] + DRF_ADC1_CR1,
                 DRF_DEF(_ADC1, _CR1, _EOCIE, _ENABLED));
        REG_WR32(ADCList[i] + DRF_ADC1_CR2,
                 DRF_DEF(_ADC1, _CR2, _ADON, _ENABLED) |
                     DRF_DEF(_ADC1, _CR2, _EOCS, _EACH_CONVERSION));
    }

    halIrqPrioritySet(IRQ_ADC, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    halIrqEnable(IRQ_ADC);
}

uint8_t halADCGetResolutionBits(struct ADCDriver *pD)
{
    return 12;
}

uint16_t halADCConvertSingle(struct ADCDriver *pD, uint32_t streamId)
{
    uint16_t convertResult = 0;
    uint32_t channel = DRF_VAL(_HAL_ADC, _STREAM, _CHN, streamId);
    if (xSemaphoreTake(adcMutex, portMAX_DELAY) != pdTRUE) {
        panic();
    }

    REG_WR32(s_getADCBaseFromStream(streamId) + DRF_ADC1_SQR1,
             DRF_NUM(_ADC1, _SQR1, _L, 1));
    REG_WR32(s_getADCBaseFromStream(streamId) + DRF_ADC1_SQR3,
             DRF_NUM(_ADC1, _SQR3, _SQ1, channel));
    xSemaphoreTake(adcSemaphore, portMAX_DELAY);
    convertResult =
        (uint16_t)(REG_RD32(s_getADCBaseFromStream(streamId) + DRF_ADC1_DR)
                   << 4U);

    xSemaphoreGive(adcMutex);

    return convertResult;
}