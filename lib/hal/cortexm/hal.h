#pragma once
#include <stdint.h>

/** Global processor interrupt disable */
void halInterruptDisable(void);
/** Global processor interrupt enable */
void halInterruptEnable(void);


// IRQ Related
void halIrqPrioritySet(uint32_t irq, uint8_t priority);

void halIrqEnable(uint32_t irq);
void halIrqDisable(uint32_t irq);

void halIrqClear(uint32_t irq);