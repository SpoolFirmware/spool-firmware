ENTRY(_reset_vector);
MEMORY {
  flash(rx)  : ORIGIN = 0x08000000, LENGTH = 256k
  sram(rwx) : ORIGIN = 0x20000000, LENGTH = 64k  /* remaining 64k in a separate address space */
}
__main_stack_end__     = ORIGIN(sram) + LENGTH(sram);    /* stack points to end of SRAM */

SECTIONS {
  .vectors  : { KEEP(*(.vectors)) }   > flash
  .text     : { *(.text*) }           > flash
  .rodata   : { *(.rodata*) }         > flash

  .data : {
    _sdata = .;   /* .data section start */
    *(.first_data)
    *(.data SORT(.data.*))
    _edata = .;  /* .data section end */
  } > sram AT > flash
  _sidata = LOADADDR(.data);

  .bss : {
    _bss_start = .;              /* .bss section start */
    *(.bss SORT(.bss.*) COMMON)
    _bss_end = .;              /* .bss section end */
  } > sram

  . = ALIGN(8);
  _end = .;     /* for cmsis_gcc.h  */
}
