# =============================================================================
# STM32H743ZI Specific Settings
# =============================================================================
#
# Target: STM32H743ZI (Cortex-M7 @ 480MHz, 1MB RAM, 2MB Flash)
# Board:  Nucleo-H743ZI or custom
#
# =============================================================================

set(STM32_CHIP "STM32H743xx")
set(STM32_FAMILY "STM32H7xx")

# CPU specific flags - Cortex-M7 with FPU (hardware float)
# Using hard-float ABI for better performance with crypto operations
set(CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfloat-abi=hard -mfpu=fpv5-d16")

# Compiler flags
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

# =============================================================================
# STM32CubeH7 HAL Path
# =============================================================================
# Note: STM32CubeH7 must be downloaded separately from ST
# https://www.st.com/en/embedded-software/stm32cubeh7.html

if(NOT DEFINED STM32_HAL_PATH)
    # Check if STM32CubeH7 is in the ports/extra directory
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32h7/STM32CubeH7")
        set(STM32_HAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32h7/STM32CubeH7" CACHE PATH "Path to STM32CubeH7")
    elseif(EXISTS "$ENV{STM32CUBEH7_PATH}")
        set(STM32_HAL_PATH "$ENV{STM32CUBEH7_PATH}" CACHE PATH "Path to STM32CubeH7")
    else()
        message(WARNING "STM32CubeH7 not found. Set STM32_HAL_PATH or STM32CUBEH7_PATH environment variable.")
        set(STM32_HAL_PATH "" CACHE PATH "Path to STM32CubeH7")
    endif()
endif()

# FreeRTOS path (from STM32CubeH7 middleware)
if(NOT DEFINED FREERTOS_PATH)
    if(STM32_HAL_PATH)
        set(FREERTOS_PATH "${STM32_HAL_PATH}/Middlewares/Third_Party/FreeRTOS/Source" CACHE PATH "Path to FreeRTOS Source")
    endif()
endif()

# =============================================================================
# STM32 HAL Include Directories
# =============================================================================

if(STM32_HAL_PATH)
    set(STM32_HAL_INCLUDE_DIRS
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Inc
        ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32H7xx/Include
        ${STM32_HAL_PATH}/Drivers/CMSIS/Include
    )

    # =============================================================================
    # STM32 HAL Sources (add only what's needed)
    # =============================================================================

    set(STM32_HAL_SOURCES
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_cortex.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_rcc_ex.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_gpio.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_uart_ex.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_dma.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_pwr_ex.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim.c
        ${STM32_HAL_PATH}/Drivers/STM32H7xx_HAL_Driver/Src/stm32h7xx_hal_tim_ex.c
    )

    # =============================================================================
    # FreeRTOS Configuration
    # =============================================================================
    # Using ARM_CM7/r0p1 port (Cortex-M7 with FPU)

    if(FREERTOS_PATH)
        set(FREERTOS_INCLUDE_DIRS
            ${FREERTOS_PATH}/include
            ${FREERTOS_PATH}/portable/GCC/ARM_CM7/r0p1
            ${FREERTOS_PATH}/CMSIS_RTOS
        )

        set(FREERTOS_SOURCES
            ${FREERTOS_PATH}/tasks.c
            ${FREERTOS_PATH}/queue.c
            ${FREERTOS_PATH}/list.c
            ${FREERTOS_PATH}/timers.c
            ${FREERTOS_PATH}/portable/GCC/ARM_CM7/r0p1/port.c
            ${FREERTOS_PATH}/portable/MemMang/heap_4.c
            ${FREERTOS_PATH}/CMSIS_RTOS/cmsis_os.c
        )
    endif()
endif()

# =============================================================================
# Startup and Linker Script
# =============================================================================
# These are in platform/stm32/h7/ directory

set(STM32_PORT_H7_DIR "${CMAKE_CURRENT_LIST_DIR}/../../platform/stm32/h7")

set(STM32_STARTUP_FILE "${STM32_PORT_H7_DIR}/startup_stm32h743xx.s")
set(STM32_LINKER_SCRIPT "${STM32_PORT_H7_DIR}/STM32H743ZITx_FLASH.ld")

# =============================================================================
# Compile Definitions
# =============================================================================

set(STM32_COMPILE_DEFINITIONS
    ${STM32_CHIP}
    USE_HAL_DRIVER
    # HSE = 8MHz on Nucleo-H743ZI (from ST-Link MCO)
    HSE_VALUE=8000000
    # Use 480MHz system clock (max for H743)
    SYSCLK_FREQ_480MHz
)

# =============================================================================
# Compile Options
# =============================================================================

set(STM32_COMPILE_OPTIONS
    ${CPU_FLAGS}
    -Wall
    -Wextra
    -Os
    -g1
)

# =============================================================================
# Include Directories
# =============================================================================

set(STM32_INCLUDE_DIRS
    ${STM32_PORT_H7_DIR}
    ${STM32_HAL_INCLUDE_DIRS}
    ${FREERTOS_INCLUDE_DIRS}
)
