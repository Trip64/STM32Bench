/*
******************************************************************************
* Linker script for STM32H723ZGTx Device
* 1024K FLASH, 564K RAM total
* Cortex-M7 with DP-FPU, DSP, CORDIC, FMAC, Ethernet
******************************************************************************
*/

ENTRY(Reset_Handler)

_estack = ORIGIN(DTCMRAM) + LENGTH(DTCMRAM); /* Top of stack at end of DTCM */
_Min_Heap_Size = 0x8000;      /* 32 KB heap */
_Min_Stack_Size = 0x4000;     /* 16 KB stack */

MEMORY
{
  ITCMRAM (xrw)   : ORIGIN = 0x00000000, LENGTH = 64K
  FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 1024K
  DTCMRAM (xrw)   : ORIGIN = 0x20000000, LENGTH = 128K
  RAM_AXI (xrw)   : ORIGIN = 0x24000000, LENGTH = 320K
  RAM_D2_SRAM1 (xrw) : ORIGIN = 0x30000000, LENGTH = 128K
  RAM_D2_SRAM2 (xrw) : ORIGIN = 0x30020000, LENGTH = 128K
  RAM_D3_SRAM4 (xrw) : ORIGIN = 0x38000000, LENGTH = 16K
}

SECTIONS
{
  /* The startup code into FLASH */
  .isr_vector :
  {
    . = ALIGN(4);
    KEEP(*(.isr_vector))
    . = ALIGN(4);
  } >FLASH

  /* The program code and other data into FLASH */
  .text :
  {
    . = ALIGN(4);
    *(.text)
    *(.text*)
    *(.glue_7)
    *(.glue_7t)
    *(.eh_frame)

    KEEP (*(.init))
    KEEP (*(.fini))

    . = ALIGN(4);
    _etext = .;
  } >FLASH

  /* Constant data into FLASH */
  .rodata :
  {
    . = ALIGN(4);
    *(.rodata)
    *(.rodata*)
    . = ALIGN(4);
  } >FLASH

  .ARM.extab :
  {
    *(.ARM.extab* .gnu.linkonce.armextab.*)
  } >FLASH

  .ARM :
  {
    __exidx_start = .;
    *(.ARM.exidx*)
    __exidx_end = .;
  } >FLASH

  .preinit_array :
  {
    PROVIDE_HIDDEN (__preinit_array_start = .);
    KEEP (*(.preinit_array*))
    PROVIDE_HIDDEN (__preinit_array_end = .);
  } >FLASH

  .init_array :
  {
    PROVIDE_HIDDEN (__init_array_start = .);
    KEEP (*(SORT(.init_array.*)))
    KEEP (*(.init_array*))
    PROVIDE_HIDDEN (__init_array_end = .);
  } >FLASH

  .fini_array :
  {
    PROVIDE_HIDDEN (__fini_array_start = .);
    KEEP (*(SORT(.fini_array.*)))
    KEEP (*(.fini_array*))
    PROVIDE_HIDDEN (__fini_array_end = .);
  } >FLASH

  /* Used by the startup to initialize itcm code */
  _siitcm = LOADADDR(.itcm_text);

  .itcm_text :
  {
    . = ALIGN(4);
    _sitcm = .;
    *(.itcm_text)
    *(.itcm_text*)
    . = ALIGN(4);
    _eitcm = .;
  } >ITCMRAM AT> FLASH

  /* Used by the startup to initialize data */
  _sidata = LOADADDR(.data);

  /* Initialized data sections into RAM_AXI */
  .data :
  {
    . = ALIGN(4);
    _sdata = .;
    *(.data)
    *(.data*)
    *(.RamFunc)
    *(.RamFunc*)
    . = ALIGN(4);
    _edata = .;
  } >RAM_AXI AT> FLASH

  /* Uninitialized data section into RAM_AXI */
  . = ALIGN(4);
  .bss :
  {
    _sbss = .;
    __bss_start__ = _sbss;
    *(.bss)
    *(.bss*)
    *(COMMON)
    . = ALIGN(4);
    _ebss = .;
    __bss_end__ = _ebss;
  } >RAM_AXI

  /* Hardware DMA Buffers in D2 SRAM1 (AHB accessible region) */
  .eth_dma (NOLOAD) :
  {
    . = ALIGN(32);
    *(.eth_desc)
    . = ALIGN(32);
    *(.eth_buffers)
    . = ALIGN(32);
    *(.d2_sram)
    *(.d2_sram*)
    . = ALIGN(32);
  } >RAM_D2_SRAM1

  /* User heap and stack validation */
  ._user_heap_stack :
  {
    . = ALIGN(8);
    PROVIDE ( end = . );
    PROVIDE ( _end = . );
    . = . + _Min_Heap_Size;
    . = ALIGN(8);
  } >RAM_AXI

  /* DTCM data section for stack and benchmark variables */
  .dtcm (NOLOAD) :
  {
    . = ALIGN(8);
    _sdtcm = .;
    *(.dtcm)
    *(.dtcm*)
    . = ALIGN(8);
    _edtcm = .;
  } >DTCMRAM

  /* Discard unused sections */
  /DISCARD/ :
  {
    libc.a ( * )
    libm.a ( * )
    libgcc.a ( * )
  }

  .ARM.attributes 0 : { *(.ARM.attributes) }
}
