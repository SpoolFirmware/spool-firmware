    .syntax unified
    .extern __copy_data
    .cpu    cortex-m3

    .thumb
    .section .text
    .text

    .globl _crt0_entry
_crt0_entry:

/* BSS INIT */
    movs    r0, #0
    ldr     r1, =__bss_base__
    ldr     r2, =__bss_end__
bloop:
    cmp     r1, r2
    itt     lo
    strlo   r0, [r1], #4
    blo     bloop

    bl      __copy_data

main_exit_loop:
    b main_exit_loop
