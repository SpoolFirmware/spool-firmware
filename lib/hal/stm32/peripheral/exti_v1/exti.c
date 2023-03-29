#include "exti.h"
#include "hal/cortexm/hal.h"
#include "drf.h"
#include "manual/mcu.h"
#include "manual/irq.h"
#include "error.h"
#include "FreeRTOS.h"
#include "bitops.h"
#include "dbgprintf.h"

static struct ExtiDriver *handlers[HAL_EXTI_NR_GROUPS];

/* different from gpio group */
uint32_t halExtiGroupNumFromGpio(struct IOLine line)
{
    return line.pin;
}

void halExtiInit(struct ExtiDriver *drv, const struct ExtiConfig *config)
{
	BUG_ON(!config->handler);
	BUG_ON(config->groupNum >= HAL_EXTI_NR_GROUPS);

	drv->config = *config;
	drv->enabled = false;
}

status_t halExtiEnable(struct ExtiDriver *drv)
{
	WARN_ON(drv->enabled);
    portENTER_CRITICAL();
    if (handlers[drv->config.groupNum]) {
	portEXIT_CRITICAL();
	return StatusExtiGroupConflict;
    }
    handlers[drv->config.groupNum] = drv;
    drv->enabled = true;
    portEXIT_CRITICAL();
    return StatusOk;
}

void halExtiDisable(struct ExtiDriver *drv)
{
	WARN_ON(!drv->enabled);
	portENTER_CRITICAL();
	WARN_ON(handlers[drv->config.groupNum] != drv);
	handlers[drv->config.groupNum] = NULL;
	drv->enabled = false;
	portEXIT_CRITICAL();
}

static void handleIrq(uint32_t groupNum)
{
	uint32_t pr = REG_RD32(DRF_REG(_EXTI,_PR));

	if (!(pr & BIT(groupNum))) {
		return;
	}
	REG_WR32(DRF_REG(_EXTI, _PR), BIT(groupNum));

	struct ExtiDriver *drv = handlers[groupNum];
	if (drv)
		drv->config.handler(drv);
}

IRQ_HANDLER_EXTI0(void)
{
    handleIrq(0);
}

IRQ_HANDLER_EXTI1(void)
{
    handleIrq(1);
}

IRQ_HANDLER_EXTI2(void)
{
    handleIrq(2);
}

IRQ_HANDLER_EXTI3(void)
{
    handleIrq(3);
}

IRQ_HANDLER_EXTI4(void)
{
    handleIrq(4);
}

IRQ_HANDLER_EXTI9_5(void)
{
	for (uint32_t i = 5; i <= 9; ++i) {
		handleIrq(i);
	}
}

IRQ_HANDLER_EXTI15_10(void)
{
	for (uint32_t i = 10; i <= 15; ++i) {
		handleIrq(i);
	}
}
