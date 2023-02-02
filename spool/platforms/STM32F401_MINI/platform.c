#include <stddef.h>
#include "dbgprintf.h"
#include "platform/platform.h"
#include "hal/hal.h"
#include "hal/cortexm/hal.h"
#include "hal/stm32/hal.h"
#include "manual/irq.h"
#include "manual/mcu.h"
#include "stream_buffer.h"

#include "FreeRTOS.h"

const static struct HalClockConfig halClockConfig = {
    .hseFreqHz = 25000000,
    .q = 7,
    .p = 4,
    .n = 336,
    .m = 25,

    .apb2Prescaler = 1,
    .apb1Prescaler = 2,
    .ahbPrescaler = 1,
};

const static struct UARTConfig uart1Cfg = {
    .baudrate = 115200,
    .useRxInterrupt = 1,
    .useTxInterrupt = 0,
};

const static struct IOLine statusLED = { .group = GPIOC, .pin = 13 };
struct UARTDriver printUart = { 0 };

static char uartCommandBuffer[128];
static StreamBufferHandle_t cmdmgmtBufferHandle;
static StaticStreamBuffer_t cmdmgmtBufferMeta;

size_t platformRecvCommand(char *pBuffer, size_t bufferSize,
                           TickType_t ticksToWait)
{
    return xStreamBufferReceive(cmdmgmtBufferHandle, pBuffer, bufferSize,
                                ticksToWait);
}

static void setupTimer(void);

void platformPostInit(void)
{
    setupTimer();

    halUartStart(&printUart);
    halIrqEnable(IRQ_USART1);
}

void enableStepper(uint8_t stepperMask)
{
}

void platformInit(struct PlatformConfig *config)
{
    halClockInit(&halClockConfig);

    uint32_t rcc = REG_RD32(DRF_REG(_RCC, _AHB1ENR));
    rcc = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOCEN, _ENABLED, rcc);
    rcc = FLD_SET_DRF(_RCC, _AHB1ENR, _GPIOAEN, _ENABLED, rcc);
    REG_WR32(DRF_REG(_RCC, _AHB1ENR), rcc);

    uint32_t apb1enr = REG_RD32(DRF_REG(_RCC, _APB1ENR));
    apb1enr = FLD_SET_DRF(_RCC, _APB1ENR, _TIM2EN, _ENABLED, apb1enr);
    REG_WR32(DRF_REG(_RCC, _APB1ENR), apb1enr);

    uint32_t apb2enr = REG_RD32(DRF_REG(_RCC, _APB2ENR));
    apb2enr = FLD_SET_DRF(_RCC, _APB2ENR, _USART1EN, _ENABLED, apb2enr);
    REG_WR32(DRF_REG(_RCC, _APB2ENR), apb2enr);

    halGpioSetMode(statusLED, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _OUTPUT) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                                  DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _HIGH));

    struct IOLine uartTx = { .group = GPIOA, .pin = 9 };
    halGpioSetMode(uartTx, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                               DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                               DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                               DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));
    struct IOLine uartRx = { .group = GPIOA, .pin = 10 };
    halGpioSetMode(uartRx, DRF_DEF(_HAL_GPIO, _MODE, _MODE, _AF) |
                               DRF_DEF(_HAL_GPIO, _MODE, _TYPE, _PUSHPULL) |
                               DRF_DEF(_HAL_GPIO, _MODE, _SPEED, _VERY_HIGH) |
                               DRF_NUM(_HAL_GPIO, _MODE, _AF, 7));

    // Setup Queues
    halUartInit(&printUart, &uart1Cfg, DRF_BASE(DRF_USART1),
                halClockApb2FreqGet(&halClockConfig));

    cmdmgmtBufferHandle = xStreamBufferCreateStatic(
        sizeof(uartCommandBuffer), 1, (uint8_t *)uartCommandBuffer,
        &cmdmgmtBufferMeta);
}

IRQ_HANDLER_TIM2(void)
{
    if (FLD_TEST_DRF(_TIM2, _SR, _UIF, _UPDATE_PENDING,
                     REG_RD32(DRF_REG(_TIM2, _SR)))) {
        REG_WR32(DRF_REG(_TIM2, _SR),
                 ~DRF_DEF(_TIM2, _SR, _UIF, _UPDATE_PENDING));
        /* halGpioToggle(statusLED); */
    }
    halIrqClear(IRQ_TIM2);
}

IRQ_HANDLER_USART1(void)
{
    uint8_t byte;
    if (!halUartRecvByte(&printUart, &byte)) {
        xStreamBufferSendFromISR(cmdmgmtBufferHandle, &byte, 1, NULL);
    }
    halIrqClear(IRQ_USART1);
}

static void setupTimer(void)
{
    // Set Interrupt Priority
    // 28 is TIM2
    halGpioSet(statusLED);
    halIrqPrioritySet(IRQ_TIM2, configMAX_SYSCALL_INTERRUPT_PRIORITY + 1);
    halIrqEnable(IRQ_TIM2);

    // Enable Counter
    REG_WR32(DRF_REG(_TIM2, _PSC), 400);
    REG_WR32(DRF_REG(_TIM2, _ARR), 0xFFFF);
    REG_WR32(DRF_REG(_TIM2, _CNT), 0);
    REG_WR32(DRF_REG(_TIM2, _CR1), DRF_DEF(_TIM2, _CR1, _CEN, _ENABLED));
    REG_WR32(DRF_REG(_TIM2, _DIER), DRF_DEF(_TIM2, _DIER, _UIE, _ENABLED));
}

void stepX(void)
{
}

void platformMotorStep(uint8_t motor_mask, uint8_t dir_mask)
{
}

__attribute__((always_inline)) inline struct IOLine platformGetStatusLED(void)
{
    return statusLED;
}

void platformDbgPutc(char c)
{
    halUartSendByte(&printUart, (uint8_t)c);
}
