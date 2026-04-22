/* STM32F439xI Linker Script - ODIN-W26 Module (NO BOOTLOADER) */
/* 
 * Memory Configuration for DIRECT PROGRAMMING (no bootloader):
 *   Flash: 2MB total, application at 0x08000000
 *   SRAM:  192KB (SRAM1 112KB + SRAM2 16KB + SRAM3 64KB) @ 0x20000000
 *   CCM:   64KB (Core Coupled Memory, no DMA) @ 0x10000000
 *   Total RAM: 256KB (192KB + 64KB CCM)
 *
 * Use this linker script when:
 *   - Flashing directly via ST-Link/SWD
 *   - No bootloader present on module
 *   - Program starts at 0x08000000 (standard STM32 startup)
 *
 * For bootloader-aware builds, use STM32F439_ODINW26_FLASH.ld instead.
 */

MEMORY
{
    /* Flash: Full 2MB available (no bootloader) */
    FLASH (rx)      : ORIGIN = 0x08000000, LENGTH = 2048K
    
    /* SRAM: 192KB contiguous region */
    /* SRAM1: 112KB (0x20000000 - 0x2001BFFF) */
    /* SRAM2:  16KB (0x2001C000 - 0x2001FFFF) */
    /* SRAM3:  64KB (0x20020000 - 0x2002FFFF) */
    RAM (xrw)       : ORIGIN = 0x20000000, LENGTH = 192K
    
    /* CCM: 64KB Core Coupled Memory - fast, CPU-only, not DMA accessible */
    /* Good for stack, fast variables, not for DMA buffers */
    CCMRAM (xrw)    : ORIGIN = 0x10000000, LENGTH = 64K
    
    /* Backup SRAM: 4KB battery-backed (not used in linker, accessed via HAL) */
    /* BKPSRAM (rw)  : ORIGIN = 0x40024000, LENGTH = 4K */
}

/* Entry Point */
ENTRY(Reset_Handler)

/* Stack in CCM for best performance (CPU-only access is fine for stack) */
_estack = ORIGIN(CCMRAM) + LENGTH(CCMRAM);    /* Top of CCM */

/* Heap end - top of RAM (used by _sbrk for malloc) */
_heap_end = ORIGIN(RAM) + LENGTH(RAM);        /* Top of main SRAM */

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x8000;       /* Required heap: 32KB (reduced for Matter SDK) */
_Min_Stack_Size = 0x2000;      /* Required stack: 8KB (in CCM) */

SECTIONS
{
    /* Startup code - must be at start of flash */
    .isr_vector :
    {
        . = ALIGN(4);
        KEEP(*(.isr_vector))
        . = ALIGN(4);
    } >FLASH

    /* Program code and read-only data */
    .text :
    {
        . = ALIGN(4);
        *(.text)           /* .text sections (code) */
        *(.text*)          /* .text* sections (code) */
        *(.glue_7)         /* glue arm to thumb code */
        *(.glue_7t)        /* glue thumb to arm code */
        *(.eh_frame)

        KEEP(*(.init))
        KEEP(*(.fini))

        . = ALIGN(4);
        _etext = .;        /* end of code */
    } >FLASH

    /* Read-only data */
    .rodata :
    {
        . = ALIGN(4);
        *(.rodata)         /* .rodata sections (constants, strings, etc.) */
        *(.rodata*)        /* .rodata* sections (constants, strings, etc.) */
        . = ALIGN(4);
    } >FLASH

    /* ARM exception/unwind tables */
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

    /* C++ constructors/destructors */
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .);
        KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } >FLASH
    
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .);
        KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*))
        PROVIDE_HIDDEN(__init_array_end = .);
    } >FLASH
    
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .);
        KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*))
        PROVIDE_HIDDEN(__fini_array_end = .);
    } >FLASH

    /* Used by startup to initialize data */
    _sidata = LOADADDR(.data);

    /* Initialized data in RAM */
    .data :
    {
        . = ALIGN(4);
        _sdata = .;        /* start of .data section */
        *(.data)           /* .data sections */
        *(.data*)          /* .data* sections */
        . = ALIGN(4);
        _edata = .;        /* end of .data section */
    } >RAM AT> FLASH

    /* CCM RAM section - explicitly placed in CCM */
    /* Use __attribute__((section(".ccmram"))) for CCM variables */
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } >CCMRAM AT> FLASH
    
    /* CCM data initializer */
    _siccmram = LOADADDR(.ccmram);

    /* Uninitialized data (zeroed by startup) */
    .bss :
    {
        _sbss = .;         /* start of bss (used by startup) */
        __bss_start__ = _sbss;
        *(.bss)
        *(.bss*)
        *(COMMON)
        . = ALIGN(4);
        _ebss = .;         /* end of bss */
        __bss_end__ = _ebss;
    } >RAM

    /* User heap section - grows upward from here */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE(end = .);
        PROVIDE(_end = .);
        . = . + _Min_Heap_Size;
        . = ALIGN(8);
    } >RAM

    /* Stack in CCM - check that it fits */
    ._ccm_stack :
    {
        . = ALIGN(8);
        . = . + _Min_Stack_Size;
        . = ALIGN(8);
    } >CCMRAM

    /* Discard exception handling for newlib-nano */
    /DISCARD/ :
    {
        libc.a(*)
        libm.a(*)
        libgcc.a(*)
    }

    /* ARM attributes */
    .ARM.attributes 0 : { *(.ARM.attributes) }
}
