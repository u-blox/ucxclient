/* STM32F439xI Linker Script - ODIN-W26 Module */
/* 
 * Memory Configuration:
 *   Flash: 2MB total, 64KB reserved for bootloader
 *   SRAM:  192KB (SRAM1 112KB + SRAM2 16KB + SRAM3 64KB) @ 0x20000000
 *   CCM:   64KB (Core Coupled Memory, no DMA) @ 0x10000000
 *   Total RAM: 256KB (192KB + 64KB CCM)
 *
 * Note: Application starts at 0x08010000 after 64KB bootloader
 *       Vector table offset (VECT_TAB_OFFSET) must be set to 0x10000
 */

MEMORY
{
    /* Flash: Skip first 64KB for bootloader, 1984KB available for app */
    FLASH (rx)      : ORIGIN = 0x08010000, LENGTH = 1984K
    
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

/* Generate a link error if heap and stack don't fit into RAM */
_Min_Heap_Size = 0x10000;      /* Required heap: 64KB */
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

    /* Used by startup to init data */
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

    /* CCM section for fast variables (optional, user must explicitly place) */
    .ccmram :
    {
        . = ALIGN(4);
        _sccmram = .;
        *(.ccmram)
        *(.ccmram*)
        . = ALIGN(4);
        _eccmram = .;
    } >CCMRAM

    /* Uninitialized data (zeroed at startup) */
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

    /* User heap - grows upward from end of BSS */
    ._user_heap_stack :
    {
        . = ALIGN(8);
        PROVIDE ( end = . );
        PROVIDE ( _end = . );
        . = . + _Min_Heap_Size;
        . = ALIGN(8);
    } >RAM

    /* Stack check - ensure minimum stack fits in CCM */
    ._check_stack :
    {
        . = . + _Min_Stack_Size;
    } >CCMRAM

    /* Remove debug info from final binary */
    /DISCARD/ :
    {
        libc.a ( * )
        libm.a ( * )
        libgcc.a ( * )
    }

    .ARM.attributes 0 : { *(.ARM.attributes) }
}
