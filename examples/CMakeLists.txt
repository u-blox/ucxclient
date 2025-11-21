cmake_minimum_required(VERSION 3.10)
project(ucxclient_examples)
string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

# Include ucxclient library definitions
include(../ucxclient.cmake)

# Set binary output directory to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Platform-specific port layer files
if(WIN32)
  set(U_EXAMPLE_UART "COM3" CACHE STRING "The UART device to use for the examples")
  set(PORT_OS_SRC ../ports/os/u_port_windows.c)
  set(PORT_UART_SRC ../ports/uart/u_port_uart_windows.c)
  set(PORT_DEFINES -DU_PORT_WINDOWS)
  set(PORT_LIBS "")
  set(EXAMPLE_COMPILE_OPTIONS /J /WX /wd4200 /wd4996)
else()
  set(U_EXAMPLE_UART "/dev/ttyUSB0" CACHE STRING "The UART device to use for the examples")
  set(PORT_OS_SRC ../ports/os/u_port_posix.c)
  set(PORT_UART_SRC ../ports/uart/u_port_uart_linux.c)
  set(PORT_DEFINES -DU_PORT_POSIX)
  set(PORT_LIBS Threads::Threads)
  set(EXAMPLE_COMPILE_OPTIONS -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -pedantic)
endif()

list(APPEND EXAMPLE_COMPILE_OPTIONS -DU_CX_LOG_DEBUG=1)

set(U_EXAMPLE_SSID "ubx" CACHE STRING "The Wi-Fi SSID to use for the examples")
set(U_EXAMPLE_WPA_PSK "" CACHE STRING "The Wi-Fi WPA PSK to use for the examples")

# Common source files for examples
set(EXAMPLE_COMMON_SRC
  example_utils.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
)

# HTTP Example (OS-based)
add_executable(http_example
  http_example.c
  ${PORT_OS_SRC}
  ${PORT_UART_SRC}
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(http_example PRIVATE ${EXAMPLE_COMPILE_OPTIONS} ${PORT_DEFINES})
target_include_directories(http_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
if(PORT_LIBS)
  target_link_libraries(http_example PRIVATE ${PORT_LIBS})
endif()

# HTTP Example No-OS
add_executable(http_example_no_os
  http_example.c
  ../ports/os/u_port_no_os.c
  ${PORT_UART_SRC}
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(http_example_no_os PRIVATE ${EXAMPLE_COMPILE_OPTIONS} ${PORT_DEFINES} -DU_PORT_NO_OS)
target_compile_definitions(http_example_no_os PRIVATE
  U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
  U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
)
target_include_directories(http_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

# Firmware Upgrade Example (OS-based)
add_executable(fw_upgrade_example
  fw_upgrade_example.c
  ${PORT_OS_SRC}
  ${PORT_UART_SRC}
  ../src/u_cx_xmodem.c
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(fw_upgrade_example PRIVATE ${EXAMPLE_COMPILE_OPTIONS} ${PORT_DEFINES} -DU_CX_XMODEM_FILE_SUPPORT=1)
target_include_directories(fw_upgrade_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
if(PORT_LIBS)
  target_link_libraries(fw_upgrade_example PRIVATE ${PORT_LIBS})
endif()

# Firmware Upgrade Example No-OS
add_executable(fw_upgrade_example_no_os
  fw_upgrade_example.c
  ../ports/os/u_port_no_os.c
  ${PORT_UART_SRC}
  ../src/u_cx_xmodem.c
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(fw_upgrade_example_no_os PRIVATE ${EXAMPLE_COMPILE_OPTIONS} ${PORT_DEFINES} -DU_PORT_NO_OS -DU_CX_XMODEM_FILE_SUPPORT=1)
target_include_directories(fw_upgrade_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
