    .syntax unified
    .cpu    cortex-m4
// if FPU
    .fpu    fpv4-sp-d16

    .thumb
    .section .text
    .text

    .globl _crt0_entry
_crt0_entry:
    
    // FPU Init (From ARM DOCS)
    // CPACR is located at address 0xE000ED88
    ldr.w   r0, =0xE000ED88
    ldr     r1, [r0]
    // Set bits 20-23 to enable CP10 and CP11 coprocessors
    orr     r1, r1, #(0xF << 20)
    str     r1, [r0]
    dsb
    isb

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
