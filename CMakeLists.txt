cmake_minimum_required(VERSION 3.10)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

include(ucxclient.cmake)

set(U_EXAMPLE_UART "/dev/ttyUSB0" CACHE STRING "The UART device to use for the examples")
set(U_EXAMPLE_SSID "ubx" CACHE STRING "The Wi-Fi SSID to use for the examples")
set(U_EXAMPLE_WPA_PSK "" CACHE STRING "The Wi-Fi WPA PSK to use for the examples")

# POSIX Examples (Linux/Unix only)
# if(NOT WIN32)
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

  # Linux/macOS shared library for Python integration
  add_library(ucxclient SHARED
    ports/os/u_port_posix.c
    ports/uart/u_port_uart_linux.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_link_libraries(ucxclient PRIVATE Threads::Threads)
  target_compile_options(ucxclient PRIVATE -Wall -Wextra -fPIC)
  target_compile_definitions(ucxclient PRIVATE U_PORT_POSIX)
  target_include_directories(ucxclient PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  
  # Set proper library naming
  set_target_properties(ucxclient PROPERTIES
    PREFIX "lib"
    OUTPUT_NAME "ucxclient"
  )
# endif()

# Windows Example (when building on Windows)
if(WIN32)
  set(U_EXAMPLE_UART "COM3" CACHE STRING "The COM port to use for Windows examples")
  
  # Add /FS flag globally for Windows to fix PDB file locking issues with parallel builds
  add_compile_options(/FS)
  
  # FTDI D2XX library support (optional - uncomment USE_UART_FTDI in u_port_windows.c to enable)
  set(FTDI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/examples/third-party/ftdi")
  set(FTDI_LIBRARY "${CMAKE_SOURCE_DIR}/examples/third-party/ftdi/ftd2xx.lib")
  
  # HTTP Example for Windows
  add_executable(http_example_windows
    examples/http_example.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
    ${UCXCLIENT_HEADERS}
  )
  target_compile_options(http_example_windows PRIVATE /W4 /wd4200)
  target_compile_definitions(http_example_windows PRIVATE
    U_PORT_WINDOWS
    U_CX_LOG_AT=1
    U_CX_LOG_ERROR=1
    U_CX_LOG_WARNING=1
    U_CX_LOG_DEBUG=1
    U_CX_LOG_USE_ANSI_COLOR=1
    U_CX_XMODEM_FILE_SUPPORT=1
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(http_example_windows PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(http_example_windows PRIVATE setupapi ${FTDI_LIBRARY})

  # Firmware Upgrade Standalone Application (Windows)
  add_executable(fw_upgrade_example_windows
    examples/fw_upgrade_example.c
    examples/example_utils.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    src/u_cx_xmodem.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
    ${UCXCLIENT_HEADERS}
  )
  target_compile_options(fw_upgrade_example_windows PRIVATE /W4 /FS /wd4200)
  target_compile_definitions(fw_upgrade_example_windows PRIVATE
    U_PORT_WINDOWS
    U_CX_LOG_AT=1
    U_CX_LOG_ERROR=1
    U_CX_LOG_WARNING=1
    U_CX_LOG_DEBUG=1
    U_CX_LOG_USE_ANSI_COLOR=1
    U_CX_PORT_HEADER_FILE="os/u_port_windows.h"
    U_CX_XMODEM_FILE_SUPPORT=1
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  )
  target_include_directories(fw_upgrade_example_windows PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(fw_upgrade_example_windows PRIVATE setupapi ${FTDI_LIBRARY})

  # Windows shared library for Python integration
  add_library(ucxclient_dll SHARED
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
    ${UCXCLIENT_HEADERS}
  )
  target_compile_options(ucxclient_dll PRIVATE /W4 /wd4200)
  target_compile_definitions(ucxclient_dll PRIVATE
    U_PORT_WINDOWS
    U_CX_LOG_AT=1
    U_CX_LOG_ERROR=1
    U_CX_LOG_WARNING=1
    U_CX_LOG_DEBUG=1
    U_CX_LOG_USE_ANSI_COLOR=1
    U_CX_XMODEM_FILE_SUPPORT=1
    U_CX_PORT_HEADER_FILE="os/u_port_windows.h"
    _CRT_SECURE_NO_WARNINGS
  )
  target_include_directories(ucxclient_dll PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(ucxclient_dll PRIVATE setupapi ${FTDI_LIBRARY})
  
  # Export all symbols for Python ctypes
  set_target_properties(ucxclient_dll PROPERTIES
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
  )
  
  # Add custom clean target that removes executables and DLLs
  add_custom_target(cleanall
    COMMAND ${CMAKE_COMMAND} --build . --target clean --config Release
    COMMAND ${CMAKE_COMMAND} --build . --target clean --config Debug
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/Release
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}/Debug
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build artifacts including executables and DLLs"
  )
endif()
