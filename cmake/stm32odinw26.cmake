# ODIN-W26 specific settings (u-blox custom STM32F439-based module)
# Cortex-M4 @ 168MHz, 2MB Flash, 256KB RAM (192KB SRAM + 64KB CCM)
#
# Memory Layout:
#   SRAM:  112KB (0x20000000 - 0x2001BFFF)
#   SRAM2:  16KB (0x2001C000 - 0x2001FFFF)  
#   SRAM3:  64KB (0x20020000 - 0x2002FFFF)
#   CCM:    64KB (0x10000000 - 0x1000FFFF) - CPU core coupled, no DMA
#   Total: 192KB standard SRAM + 64KB CCM = 256KB
#
# Flash Layout:
#   Bootloader: 64KB (0x08000000 - 0x0800FFFF)
#   Application: 1984KB (0x08010000 onwards)
#
# Key differences from standard F439:
#   - 168MHz clock (not 180MHz) for compatibility with ODIN firmware
#   - HSE = 24MHz external crystal
#   - Same STM32F439 die but different configuration
#   - Vector table offset 0x10000 for bootloader
#   - No built-in Wi-Fi/BLE used (NORA-W36 provides all networking)
#
# UART Configuration:
#   - USART1 (PA9/PA10 + HW flow control): NORA-W36 AT commands
#   - USART3 (PD8/PD9): Debug/console @ 115200
#
# Clock Configuration (matches original ODIN-W26 firmware):
#   - HSE: 24MHz
#   - PLL: M=24, N=336, P=2, Q=7
#   - SYSCLK: 168MHz
#   - AHB: 168MHz (DIV1)
#   - APB1: 42MHz (DIV4)
#   - APB2: 84MHz (DIV2)
#   - Flash: 5 wait states @ 168MHz

set(STM32_CHIP "STM32F439xx")
set(STM32_FAMILY "STM32F4xx")

# Additional define to distinguish ODIN-W26 from generic F439
add_compile_definitions(ODIN_W26)

# CPU specific flags - Using march=armv7e-m for proper multilib matching
# F439 has FPU but we use soft-float for nano.specs compatibility
set(CPU_FLAGS "-march=armv7e-m -mthumb -mfloat-abi=soft")

# Compiler flags
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

# STM32 HAL path (should be set from environment or command line)
if(NOT DEFINED STM32_HAL_PATH)
    set(STM32_HAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32f4/STM32CubeF4" CACHE PATH "Path to STM32CubeF4")
endif()

# FreeRTOS path (now from STM32CubeF4 middleware)
if(NOT DEFINED FREERTOS_PATH)
    set(FREERTOS_PATH "${STM32_HAL_PATH}/Middlewares/Third_Party/FreeRTOS/Source" CACHE PATH "Path to FreeRTOS Source")
endif()

# STM32 HAL includes
set(STM32_HAL_INCLUDE_DIRS
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Inc
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
)

# STM32 HAL sources (add only what's needed)
set(STM32_HAL_SOURCES
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_uart.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c
    ${STM32_HAL_PATH}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c
)

# FreeRTOS includes - Using ARM_CM3 port for soft-float
set(FREERTOS_INCLUDE_DIRS
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable/GCC/ARM_CM3
    ${FREERTOS_PATH}/CMSIS_RTOS
)

# FreeRTOS sources - Using ARM_CM3 port
set(FREERTOS_SOURCES
    ${FREERTOS_PATH}/tasks.c
    ${FREERTOS_PATH}/queue.c
    ${FREERTOS_PATH}/list.c
    ${FREERTOS_PATH}/timers.c
    ${FREERTOS_PATH}/portable/GCC/ARM_CM3/port.c
    ${FREERTOS_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_PATH}/CMSIS_RTOS/cmsis_os.c
)

# Startup file - Using F439xx startup from STM32CubeF4
set(STM32_STARTUP_FILE "${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f439xx.s")

# Linker script - ODIN-W26 NO BOOTLOADER (direct flash at 0x08000000)
# For bootloader-aware builds, use STM32F439_ODINW26_FLASH.ld instead
set(STM32_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32f4/scripts/STM32F439_ODINW26_NOBOOT_FLASH.ld")

# Compile definitions - ODIN-W26 uses HSI (internal 16MHz) per original u-blox firmware
# NOTE: No VECT_TAB_OFFSET needed - direct flash at 0x08000000
add_compile_definitions(
    ${STM32_CHIP}
    USE_HAL_DRIVER
)
