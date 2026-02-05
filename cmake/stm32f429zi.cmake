# STM32F429ZI specific settings (Discovery board)
# Cortex-M4 @ 180MHz, 2MB Flash, 256KB RAM (192KB SRAM + 64KB CCM)

set(STM32_CHIP "STM32F429xx")
set(STM32_FAMILY "STM32F4xx")

# CPU specific flags - Using march=armv7e-m for proper multilib matching
# F429 has FPU but we use soft-float for nano.specs compatibility
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
set(STM32_STARTUP_FILE "${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/startup_stm32f429xx.s")
set(STM32_LINKER_SCRIPT "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32f4/scripts/STM32F429ZITx_FLASH.ld")

# System file
set(STM32_SYSTEM_FILE "${STM32_HAL_PATH}/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/system_stm32f4xx.c")

message(STATUS "STM32F429ZI configuration loaded")
message(STATUS "  HAL Path: ${STM32_HAL_PATH}")
message(STATUS "  FreeRTOS: ${FREERTOS_PATH}")
