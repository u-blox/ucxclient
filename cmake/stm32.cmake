# STM32 Example Build Helper
#
# This file provides a function to build STM32-based examples.
# It should be included from examples/CMakeLists.txt when BUILD_STM32_EXAMPLES is enabled.
#
# Usage:
#   cmake -DBUILD_STM32_EXAMPLES=ON -DCMAKE_TOOLCHAIN_FILE=../cmake/arm-none-eabi-gcc.cmake ..

if(NOT CMAKE_CROSSCOMPILING)
    message(FATAL_ERROR "STM32 builds require cross-compilation. Set CMAKE_TOOLCHAIN_FILE to cmake/arm-none-eabi-gcc.cmake")
endif()

# Select board configuration (default: STM32F407G-DISC1)
# Pass -DSTM32_BOARD=nucleo_f439zi for the NUCLEO-F439ZI board.
if(NOT DEFINED STM32_BOARD)
    set(STM32_BOARD "f407_disc" CACHE STRING "STM32 board: f407_disc or nucleo_f439zi")
endif()

if(STM32_BOARD STREQUAL "nucleo_f439zi")
    include(${CMAKE_CURRENT_LIST_DIR}/stm32f439zi.cmake)
else()
    include(${CMAKE_CURRENT_LIST_DIR}/stm32f407vg.cmake)
endif()

# STM32 port directory
set(STM32_PORT_EXTRA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../ports/extra/stm32f4)

# STM32-specific common sources
set(STM32_COMMON_SRC
    ${STM32_PORT_EXTRA_DIR}/src/system_stm32f4xx.c
    ${STM32_PORT_EXTRA_DIR}/src/stm32f4xx_it.c
    ${STM32_PORT_EXTRA_DIR}/src/stm32f4xx_hal_timebase_tim.c
    ${STM32_PORT_EXTRA_DIR}/src/sysmem.c
    ${STM32_PORT_EXTRA_DIR}/src/syscalls.c
    ${STM32_STARTUP_FILE}
    ${STM32_HAL_SOURCES}
    ${FREERTOS_SOURCES}
)

# STM32 port layer sources
set(STM32_PORT_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/../ports/os/u_port_freertos.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../ports/uart/u_port_uart_stm32f4.c
)

# STM32 include directories
set(STM32_INCLUDE_DIRS
    ${STM32_PORT_EXTRA_DIR}/src
    ${STM32_HAL_INCLUDE_DIRS}
    ${FREERTOS_INCLUDE_DIRS}
)

# STM32 compile definitions
# WiFi credentials (U_EXAMPLE_SSID, U_EXAMPLE_WPA_PSK) come from config.local.h
set(STM32_COMPILE_DEFINITIONS
    U_PORT_FREERTOS
    U_EXAMPLE_UART="USART3"
    U_CX_LOG_DEBUG=1
)

# STM32 compile options
set(STM32_COMPILE_OPTIONS
    -march=armv7e-m
    -mthumb
    -mfloat-abi=soft
    -Wall
    -Wextra
    -Og
    -g3
)

# Function to add an STM32 example target
#
# add_stm32_example(
#   NAME <target_name>
#   SOURCES <source_files...>
# )
function(add_stm32_example)
    set(options "")
    set(oneValueArgs NAME)
    set(multiValueArgs SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT ARG_NAME)
        message(FATAL_ERROR "add_stm32_example: NAME is required")
    endif()

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_stm32_example: SOURCES is required")
    endif()

    # Create the executable (target name already has .elf suffix)
    add_executable(${ARG_NAME}.elf
        ${STM32_PORT_EXTRA_DIR}/src/main_stm32.c
        ${ARG_SOURCES}
        ${EXAMPLE_COMMON_SRC}
        ${STM32_PORT_SOURCES}
        ${STM32_COMMON_SRC}
    )

    # Set output directory and name
    set_target_properties(${ARG_NAME}.elf PROPERTIES
        OUTPUT_NAME "${ARG_NAME}"
        SUFFIX ".elf"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
        LINK_FLAGS "${CPU_FLAGS} -T${STM32_LINKER_SCRIPT} -Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/../bin/${ARG_NAME}.map,--no-warn-rwx-segments"
    )

    # Apply compile definitions
    target_compile_definitions(${ARG_NAME}.elf PRIVATE
        ${STM32_COMPILE_DEFINITIONS}
    )

    # Apply compile options
    target_compile_options(${ARG_NAME}.elf PRIVATE ${STM32_COMPILE_OPTIONS})

    # Set include directories
    target_include_directories(${ARG_NAME}.elf PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${UCXCLIENT_INC}
        ${UCXCLIENT_PORT_DIR}
        ${STM32_INCLUDE_DIRS}
    )

    # Ensure assembly files use the same CPU flags
    set_source_files_properties(${STM32_STARTUP_FILE} PROPERTIES
        COMPILE_FLAGS "-march=armv7e-m -mthumb -mfloat-abi=soft"
    )

    # Find ARM toolchain utilities
    find_program(ARM_OBJCOPY arm-none-eabi-objcopy REQUIRED)
    find_program(ARM_SIZE arm-none-eabi-size REQUIRED)

    # Generate additional output files
    add_custom_command(TARGET ${ARG_NAME}.elf POST_BUILD
        COMMAND ${ARM_OBJCOPY} -O ihex $<TARGET_FILE:${ARG_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/../bin/${ARG_NAME}.hex
        COMMAND ${ARM_OBJCOPY} -O binary $<TARGET_FILE:${ARG_NAME}.elf> ${CMAKE_CURRENT_BINARY_DIR}/../bin/${ARG_NAME}.bin
        COMMAND ${ARM_SIZE} $<TARGET_FILE:${ARG_NAME}.elf>
        COMMENT "Building HEX and BIN files for ${ARG_NAME}"
    )
endfunction()
