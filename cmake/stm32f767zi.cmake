# STM32F767ZI specific settings (NUCLEO-F767ZI board)
# Cortex-M7 @ 216MHz, 2MB Flash, 512KB RAM

set(STM32_CHIP "STM32F767xx")
set(STM32_FAMILY "STM32F7xx")

# CPU specific flags for Cortex-M7 with FPU
# Using soft-float for nano.specs compatibility (can enable hard-float if needed)
set(CPU_FLAGS "-mcpu=cortex-m7 -mthumb -mfloat-abi=soft")

# Compiler flags
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

# STM32 HAL path - STM32CubeF7 (separate from F4)
if(NOT DEFINED STM32_HAL_PATH)
    set(STM32_HAL_PATH "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32f7/STM32CubeF7" CACHE PATH "Path to STM32CubeF7")
endif()

# FreeRTOS path (from STM32CubeF7 middleware)
if(NOT DEFINED FREERTOS_PATH)
    set(FREERTOS_PATH "${STM32_HAL_PATH}/Middlewares/Third_Party/FreeRTOS/Source" CACHE PATH "Path to FreeRTOS Source")
endif()

# STM32 HAL includes
set(STM32_HAL_INCLUDE_DIRS
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Inc
    ${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F7xx/Include
    ${STM32_HAL_PATH}/Drivers/CMSIS/Include
)

# STM32 HAL sources (add only what's needed)
set(STM32_HAL_SOURCES
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_cortex.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_rcc_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_gpio.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_uart.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_dma.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_pwr_ex.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim.c
    ${STM32_HAL_PATH}/Drivers/STM32F7xx_HAL_Driver/Src/stm32f7xx_hal_tim_ex.c
)

# FreeRTOS includes - Using ARM_CM3 port for soft-float compatibility
# ARM_CM3 port works for both CM4 and CM7 with soft-float ABI
# Include CMSIS_RTOS for cmsis_os.h compatibility
set(FREERTOS_INCLUDE_DIRS
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable/GCC/ARM_CM3
    ${FREERTOS_PATH}/CMSIS_RTOS
)

# FreeRTOS sources
set(FREERTOS_SOURCES
    ${FREERTOS_PATH}/croutine.c
    ${FREERTOS_PATH}/event_groups.c
    ${FREERTOS_PATH}/list.c
    ${FREERTOS_PATH}/queue.c
    ${FREERTOS_PATH}/stream_buffer.c
    ${FREERTOS_PATH}/tasks.c
    ${FREERTOS_PATH}/timers.c
    ${FREERTOS_PATH}/portable/GCC/ARM_CM3/port.c
    ${FREERTOS_PATH}/portable/MemMang/heap_4.c
    ${FREERTOS_PATH}/CMSIS_RTOS/cmsis_os.c
)

# Startup file and linker script
# Using startup from SW4STM32 directory (GCC compatible)
set(STM32_STARTUP_FILE "${STM32_HAL_PATH}/Projects/STM32F767ZI-Nucleo/Applications/EEPROM/EEPROM_Emulation/SW4STM32/startup_stm32f767xx.s")
set(STM32_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32f7/scripts/STM32F767ZITx_FLASH.ld")

# System file - from a NUCLEO-F767ZI project for proper clock config
set(STM32_SYSTEM_FILE "${STM32_HAL_PATH}/Projects/STM32F767ZI-Nucleo/Applications/EEPROM/EEPROM_Emulation/Src/system_stm32f7xx.c")

message(STATUS "STM32F767ZI configuration loaded")
message(STATUS "  HAL Path: ${STM32_HAL_PATH}")
message(STATUS "  FreeRTOS: ${FREERTOS_PATH}")
