#include "platform_private.h"
#include "semphr.h"

static SemaphoreHandle_t conversionSemaphoreHandle;
void thermalInit(void)
{
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _ADC3EN, _ENABLED);

    REG_FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV8);

    // Configure ADC
    REG_WR32(DRF_REG(_ADC3, _CR1), DRF_DEF(_ADC3, _CR1, _EOCIE, _ENABLED));
    REG_WR32(DRF_REG(_ADC3, _CR2), DRF_DEF(_ADC3, _CR2, _ADON, _ENABLED) | 
        DRF_DEF(_ADC3, _CR2, _EOCS, _EACH_CONVERSION));

    // Configure Sequence
    REG_WR32(DRF_REG(_ADC3, _SQR1), DRF_NUM(_ADC3, _SQR1, _L, 1));
    REG_WR32(DRF_REG(_ADC3, _SQR3), DRF_NUM(_ADC3, _SQR3, _SQ1, 14));

    conversionSemaphoreHandle = xSemaphoreCreateBinary();
}

void thermalPostInit(void)
{
    halIrqPrioritySet(IRQ_ADC, configMAX_SYSCALL_INTERRUPT_PRIORITY);
    REG_WR32(DRF_REG(_ADC3, _SR), 0);
    halIrqEnable(IRQ_ADC);
}

IRQ_HANDLER_ADC(void)
{
    xSemaphoreGive(conversionSemaphoreHandle);
    halIrqClear(IRQ_ADC);
}

uint16_t platformReadTemp(uint8_t idx)
{
    REG_FLD_SET_DRF(_ADC3, _CR2, _SWSTART, _START);
    xSemaphoreTake(conversionSemaphoreHandle, portMAX_DELAY);
    return (uint16_t)REG_RD32(DRF_REG(_ADC3, _DR));
}
