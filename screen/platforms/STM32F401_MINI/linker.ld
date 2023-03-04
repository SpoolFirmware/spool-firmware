ENTRY(Reset_Handler);
MEMORY {
  flash(rx)   : ORIGIN = 0x08000000, LENGTH = 256k
  sram(rwx)   : ORIGIN = 0x20000000, LENGTH = 64k
}
__main_stack_end__ = ORIGIN(sram) + LENGTH(sram);

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
    __bss_base__ = .;              /* .bss section start */
    *(.bss SORT(.bss.*) COMMON)
    __bss_end__ = .;               /* .bss section end */
  } > sram

  . = ALIGN(8);
  _end = .;     /* for cmsis_gcc.h  */

  /DISCARD/ : {
    *(.init .fini)
    *(.ARM.attributes)
    *(.comment)
  }
}
