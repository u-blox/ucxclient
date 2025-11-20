cmake_minimum_required(VERSION 3.10)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

include(ucxclient.cmake)

set(U_EXAMPLE_UART "/dev/ttyUSB0" CACHE STRING "The UART device to use for the examples")
set(U_EXAMPLE_SSID "ubx" CACHE STRING "The Wi-Fi SSID to use for the examples")
set(U_EXAMPLE_WPA_PSK "" CACHE STRING "The Wi-Fi WPA PSK to use for the examples")

# POSIX Examples (Linux/Unix only)
if(NOT WIN32)
  # HTTP Example
  add_executable(http_example
    examples/http_example.c
    examples/port/u_port_posix.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_link_libraries(http_example PRIVATE Threads::Threads)
  target_compile_options(http_example PRIVATE -Wall -Wextra -Werror)
  target_compile_definitions(http_example PRIVATE U_CX_LOG_DEBUG=1 U_PORT_POSIX)
  target_include_directories(http_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

  # No OS Example
  add_executable(http_example_no_os
    examples/http_example_no_os.c
    examples/port/u_port_no_os.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(http_example_no_os PRIVATE -Wall -Wextra -Werror)
  target_compile_definitions(http_example_no_os PRIVATE
    U_CX_LOG_DEBUG=1
    U_PORT_NO_OS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(http_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

  # Linux/macOS shared library for Python integration
  add_library(ucxclient SHARED
    examples/port/u_port_posix.c
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
endif()

# Windows Example (when building on Windows)
if(WIN32)
  set(U_EXAMPLE_UART "COM3" CACHE STRING "The COM port to use for Windows examples")
  
  # Add /FS flag globally for Windows to fix PDB file locking issues with parallel builds
  add_compile_options(/FS)
  
  # FTDI D2XX library support (optional - uncomment USE_UART_FTDI in u_port_windows.c to enable)
  set(FTDI_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/examples/ftdi")
  set(FTDI_LIBRARY "${CMAKE_SOURCE_DIR}/examples/ftdi/ftd2xx.lib")
  
  # HTTP Example for Windows
  add_executable(http_example_windows
    examples/http_example.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(http_example_windows PRIVATE /W4 /wd4200)
  target_compile_definitions(http_example_windows PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="os/u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(http_example_windows PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(http_example_windows PRIVATE setupapi ${FTDI_LIBRARY})

  # Windows Basic WiFi Application
  add_executable(ucxclient_basic_wifi
    examples/ucxclient_basic_wifi.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(ucxclient_basic_wifi PRIVATE /W4 /wd4200)
  target_compile_definitions(ucxclient_basic_wifi PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="os/u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(ucxclient_basic_wifi PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(ucxclient_basic_wifi PRIVATE setupapi ${FTDI_LIBRARY})

  # Windows Console Application - ucxclient-x64 (Monolithic)
  add_executable(ucxclient-x64
    examples/ucxclient-x64.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    examples/ucxclient-x64.rc
    examples/qrcodegen/qrcodegen.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(ucxclient-x64 PRIVATE /W4 /FS /wd4200)
  target_compile_definitions(ucxclient-x64 PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="os/u_cx_at_config_windows.h"
    U_CX_PORT_HEADER_FILE="os/u_port_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  )
  target_include_directories(ucxclient-x64 PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR} ${CMAKE_SOURCE_DIR}/examples)
  target_link_libraries(ucxclient-x64 PRIVATE setupapi ${FTDI_LIBRARY})

  # Firmware Upgrade Standalone Application
  add_executable(fw_upgrade_example
    examples/fw_upgrade_example.c
    examples/example_utils.c
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(fw_upgrade_example PRIVATE /W4 /FS /wd4200)
  target_compile_definitions(fw_upgrade_example PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="os/u_cx_at_config_windows.h"
    U_CX_PORT_HEADER_FILE="os/u_port_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  )
  target_include_directories(fw_upgrade_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR} ${FTDI_INCLUDE_DIR})
  target_link_libraries(fw_upgrade_example PRIVATE setupapi ${FTDI_LIBRARY})

  # Windows shared library for Python integration
  add_library(ucxclient_dll SHARED
    ports/os/u_port_windows.c
    ports/uart/u_port_uart_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(ucxclient_dll PRIVATE /W4 /wd4200)
  target_compile_definitions(ucxclient_dll PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="os/u_cx_at_config_windows.h"
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
