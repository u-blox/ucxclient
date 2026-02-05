/* STM32F767ZITx Linker Script - NUCLEO-F767ZI Board */
/* Memory: 2MB Flash, 512KB RAM total */
/* 
 * RAM Layout:
 *   DTCM:  128KB @ 0x20000000 (tightly coupled, fast, no DMA)
 *   SRAM1: 368KB @ 0x20020000 (AXI bus, DMA accessible)
 *   SRAM2:  16KB @ 0x2007C000 (backup capable)
 *   ITCM:   16KB @ 0x00000000 (instruction TCM, optional)
 *
 * For simplicity, we use DTCM + SRAM1 as main RAM (496KB usable)
 * SRAM2 can be used for backup data if needed
 */

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K   /* 2MB Flash */
    DTCM (xrw)      : ORIGIN = 0x20000000, LENGTH = 128K    /* DTCM - fast, no DMA */
    RAM (xrw)       : ORIGIN = 0x20020000, LENGTH = 368K    /* SRAM1 - main RAM */
    SRAM2 (xrw)     : ORIGIN = 0x2007C000, LENGTH = 16K     /* SRAM2 - backup capable */
}

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack - put stack in DTCM for speed */
_estack = ORIGIN(DTCM) + LENGTH(DTCM);

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x200;       /* Required heap: 512 bytes (malloc not used extensively) */
_Min_Stack_Size = 0x1000;     /* Required stack: 4KB */

SECTIONS
{
    /* Startup code */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Program code and constants */
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

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)
        *(.rodata*)
        . = ALIGN(4);
    } >FLASH

    /* ARM exception handling */
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

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data - copied from Flash to RAM at startup */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* DTCM section for fast variables and stack */
    .dtcm :
    {
        . = ALIGN(4);
        _sdtcm = .;
        *(.dtcm)
        *(.dtcm*)
        . = ALIGN(4);
        _edtcm = .;
    } >DTCM AT> FLASH

    /* Uninitialized data */
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
    } >RAM

    /* User heap and stack */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Remove debug info from final image */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
