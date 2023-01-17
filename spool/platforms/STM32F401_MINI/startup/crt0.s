    .syntax unified
    .cpu    cortex-m4
// if FPU
    .fpu    fpv4-sp-d16

    .thumb
    .section .text
    .text

    .globl _crt0_entry
_crt0_entry:
    /*
    // FPU FPCCR initialization.
    movw    r0, #CRT0_FPCCR_INIT & 0xFFFF
    movt    r0, #CRT0_FPCCR_INIT >> 16
    movw    r1, #SCB_FPCCR & 0xFFFF
    movt    r1, #SCB_FPCCR >> 16
    str     r0, [r1]
    dsb
    isb

    // CPACR initialization
    movw    r0, #CRT0_CPACR_INIT & 0xFFFF
    movt    r0, #CRT0_CPACR_INIT >> 16
    movw    r1, #SCB_CPACR & 0xFFFF
    movt    r1, #SCB_CPACR >> 16
    str     r0, [r1]
    dsb
    isb

    // FPU FPSCR initially cleared
    mov     r0, #0
    vmsr    FPSCR, r0

    // FPU FPDSCR initially cleared
    movw    r1, #SCB_FPDSCR & 0xFFFF
    movt    r1, #SCB_FPDSCR >> 16
    str     r0, [r1]

    // Enforcing FPCA bit in the CONTROL register
    movs    r0, #CRT0_CONTROL_INIT | CONTROL_FPCA
    */

/* BSS INIT */
    movs    r0, #0
    ldr     r1, =__bss_base__
    ldr     r2, =__bss_end__
bloop:
    cmp     r1, r2
    itt     lo
    strlo   r0, [r1], #4
    blo     bloop

    bl      main

main_exit_loop:
    b main_exit_loop
