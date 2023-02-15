#include "platform_private.h"
#include "lib/thermistor/thermistors.h"
#include "semphr.h"
#include "error.h"

static SemaphoreHandle_t conversionSemaphoreHandle;
void thermalInit(void)
{
    halGpioSetMode(halGpioLineConstruct(DRF_BASE(DRF_GPIOF), 4),
                   DRF_DEF(_HAL_GPIO, _MODE, _MODE, _ANALOG));
    REG_FLD_SET_DRF(_RCC, _APB2ENR, _ADC3EN, _ENABLED);

    REG_FLD_SET_DRF(_ADC_COMMON, _CCR, _ADCPRE, _DIV8);

    // Configure ADC
    REG_WR32(DRF_REG(_ADC3, _CR1), DRF_DEF(_ADC3, _CR1, _EOCIE, _ENABLED));
    REG_WR32(DRF_REG(_ADC3, _CR2),
             DRF_DEF(_ADC3, _CR2, _ADON, _ENABLED) |
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
    xSemaphoreGiveFromISR(conversionSemaphoreHandle, NULL);
    REG_WR32(DRF_REG(_ADC3, _SR), 0);
    halIrqClear(IRQ_ADC);
}

fix16_t platformReadTemp(int8_t idx)
{
    uint16_t scaledValue = 0;
    if (idx == 0) {
        REG_FLD_SET_DRF(_ADC3, _CR2, _SWSTART, _START);
        xSemaphoreTake(conversionSemaphoreHandle, portMAX_DELAY);
        scaledValue = (uint16_t)(REG_RD32(DRF_REG(_ADC3, _DR))) << 4U;
    } else {
        panic();
    }


    return thermistorEvaulate(&t100k_4k7_table, scaledValue);
}
