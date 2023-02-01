#include <stdint.h>

#include "hal.h"

static volatile uint32_t *const NVIC_ISER = (volatile uint32_t *) 0xE000E100; // Set Enable
static volatile uint32_t *const NVIC_ICER = (volatile uint32_t *) 0xE000E180; // Clear Enable
static volatile uint32_t *const NVIC_ICPR = (volatile uint32_t *) 0xE000E280; // Clear Pending

static volatile uint8_t *const NVIC_IPR = (volatile uint8_t *) 0xE000E400; // Interrupt Priority Register

void halInterruptDisable(void) 
{
    __asm__ volatile (
        "cpsid i \n"
        "cpsid f \n"
    );
}

void halInterruptEnable(void) 
{
    __asm__ volatile (
        "cpsie i \n"
        "cpsie f \n"
    );
}

void halIrqClear(uint32_t irq) 
{
    NVIC_ICPR[irq / 32U] = 1U << (irq % 32U);
}

void halIrqEnable(uint32_t irq) 
{
    NVIC_ISER[irq / 32U] = 1U << (irq % 32U);
}

void halIrqDisable(uint32_t irq)
{
    NVIC_ICER[irq / 32U] = 1U << (irq % 32U);
}

void halIrqPrioritySet(uint32_t irq, uint8_t priority)
{
    NVIC_IPR[irq] = priority;
}