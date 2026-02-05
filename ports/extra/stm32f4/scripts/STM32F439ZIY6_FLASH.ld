/* STM32F439ZIY6 Linker Script - ODIN-W2 Module */
/* Memory: 2MB Flash, 256KB SRAM + 64KB CCM = 320KB total RAM */
/* CCM (Core Coupled Memory) is fast but not DMA-accessible */

MEMORY
{
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K   /* 2MB Flash */
    /* Contiguous RAM: SRAM1(112KB) + SRAM2(16KB) + SRAM3(64KB) = 192KB @ 0x20000000 */
    /* But F439 can access all 192KB as contiguous region */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 256K    /* SRAM1+SRAM2+SRAM3 = 256KB usable */
    CCMRAM (xrw)    : ORIGIN = 0x10000000, LENGTH = 64K     /* CCM - fast, no DMA */
}

/* Note: F439 has 256KB contiguous SRAM at 0x20000000-0x2003FFFF
 * Plus 64KB CCM at 0x10000000 (CPU-only, no DMA)
 * We use 256KB as main heap, CCM for stack or fast variables
 */

/* Entry Point */
ENTRY(Reset_Handler)

/* Highest address of the user mode stack */
_estack = ORIGIN(RAM) + LENGTH(RAM);    /* end of RAM */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x18000;      /* Required heap: 96KB */
_Min_Stack_Size = 0x2000;      /* Required stack: 8KB */

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

    /* Pre-init and init arrays */
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

    /* Used by startup code to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data - in RAM, loaded from Flash */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;
        *(.data)
        *(.data*)
        . = ALIGN(4);
        _edata = .;
    } >RAM AT> FLASH

    /* CCM section for performance-critical data (no DMA!) */
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } >CCMRAM AT> FLASH

    _siccmram = LOADADDR(.ccmram);

    /* Uninitialized data */
    .bss :
    {
        . = ALIGN(4);
        _sbss = .;
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;
        __bss_end__ = _ebss;
    } >RAM

    /* User heap */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >RAM

    /* Remove debug info from output */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
