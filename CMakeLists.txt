cmake_minimum_required(VERSION 3.4)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

include(ucxclient.cmake)

set(U_EXAMPLE_UART "/dev/ttyUSB0" CACHE STRING "The UART device to use for the examples")
set(U_EXAMPLE_SSID "ubx" CACHE STRING "The Wi-Fi SSID to use for the examples")
set(U_EXAMPLE_WPA_PSK "" CACHE STRING "The Wi-Fi WPA PSK to use for the examples")

# Common source files for examples
set(EXAMPLE_COMMON_SRC
  examples/example_utils.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
)

# Common compile options for all examples
set(EXAMPLE_COMPILE_OPTIONS
  -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -pedantic -DU_CX_LOG_DEBUG=1
)

# HTTP Example (POSIX with OS support)
add_executable(http_example
  examples/http_example.c
  ports/os/u_port_posix.c
  ports/uart/u_port_uart_linux.c
  ${EXAMPLE_COMMON_SRC}
)
target_link_libraries(http_example PRIVATE Threads::Threads)
target_compile_options(http_example PRIVATE ${EXAMPLE_COMPILE_OPTIONS} -DU_PORT_POSIX)
target_include_directories(http_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

# HTTP Example No-OS (same code as http_example, different port)
# This demonstrates that examples can work in both OS and no-OS environments
# by simply changing the port layer and compile-time defines
add_executable(http_example_no_os
  examples/http_example.c
  ports/os/u_port_no_os.c
  ports/uart/u_port_uart_linux.c
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(http_example_no_os PRIVATE ${EXAMPLE_COMPILE_OPTIONS} -DU_PORT_NO_OS)
target_compile_definitions(http_example_no_os PRIVATE
  U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
  U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
)
target_include_directories(http_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

# Firmware Upgrade Example (POSIX with OS support)
add_executable(fw_upgrade_example
  examples/fw_upgrade_example.c
  ports/os/u_port_posix.c
  ports/uart/u_port_uart_linux.c
  src/u_cx_xmodem.c
  ${EXAMPLE_COMMON_SRC}
)
target_link_libraries(fw_upgrade_example PRIVATE Threads::Threads)
target_compile_options(fw_upgrade_example PRIVATE ${EXAMPLE_COMPILE_OPTIONS} -DU_PORT_POSIX -DU_CX_XMODEM_FILE_SUPPORT=1)
target_include_directories(fw_upgrade_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

# Firmware Upgrade Example No-OS (same code as fw_upgrade_example, different port)
add_executable(fw_upgrade_example_no_os
  examples/fw_upgrade_example.c
  ports/os/u_port_no_os.c
  ports/uart/u_port_uart_linux.c
  src/u_cx_xmodem.c
  ${EXAMPLE_COMMON_SRC}
)
target_compile_options(fw_upgrade_example_no_os PRIVATE ${EXAMPLE_COMPILE_OPTIONS} -DU_PORT_NO_OS -DU_CX_XMODEM_FILE_SUPPORT=1)
target_include_directories(fw_upgrade_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
