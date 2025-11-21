# STM32F407VG specific settings
set(STM32_CHIP "STM32F407xx")
set(STM32_FAMILY "STM32F4xx")

# CPU specific flags - Using march=armv7e-m for proper multilib matching
set(CPU_FLAGS "-march=armv7e-m -mthumb -mfloat-abi=soft")

# Compiler flags
set(CMAKE_C_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_CXX_FLAGS_INIT "${CPU_FLAGS} -fdata-sections -ffunction-sections -Wall")
set(CMAKE_ASM_FLAGS_INIT "${CPU_FLAGS} -x assembler-with-cpp")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CPU_FLAGS} -specs=nano.specs -specs=nosys.specs -Wl,--gc-sections")

# STM32 HAL path (should be set from environment or command line)
if(NOT DEFINED STM32_HAL_PATH)
    set(STM32_HAL_PATH "/opt/STM32CubeF4" CACHE PATH "Path to STM32CubeF4")
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

# Startup file (local copy from STM32CubeF4/Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc)
# Path is relative to the cmake directory
set(STM32_STARTUP_FILE "${CMAKE_CURRENT_LIST_DIR}/../ports/extra/stm32/startup_stm32f407xx.s")

# Compile definitions
add_compile_definitions(
    ${STM32_CHIP}
    USE_HAL_DRIVER
    HSE_VALUE=8000000
)
