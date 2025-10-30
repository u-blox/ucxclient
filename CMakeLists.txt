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
  
  # HTTP Example for Windows
  add_executable(http_example_windows
    examples/http_example.c
    examples/port/u_port_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(http_example_windows PRIVATE /W4)
  target_compile_definitions(http_example_windows PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(http_example_windows PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  target_link_libraries(http_example_windows PRIVATE setupapi)

  # Windows Basic Application
  add_executable(windows_basic
    examples/windows_basic.c
    examples/port/u_port_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(windows_basic PRIVATE /W4)
  target_compile_definitions(windows_basic PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
    U_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
    U_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
  )
  target_include_directories(windows_basic PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  target_link_libraries(windows_basic PRIVATE setupapi)

  # Windows Console Application (Simple Menu-based App)
  add_executable(windows_app
    examples/windows_app.c
    examples/port/u_port_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(windows_app PRIVATE /W4 /FS)
  target_compile_definitions(windows_app PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  )
  target_include_directories(windows_app PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  target_link_libraries(windows_app PRIVATE setupapi)

  # Firmware Update Standalone Application
  add_executable(firmware_update
    examples/firmware_update.c
    examples/port/u_port_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(firmware_update PRIVATE /W4 /FS)
  target_compile_definitions(firmware_update PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
    U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  )
  target_include_directories(firmware_update PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  target_link_libraries(firmware_update PRIVATE setupapi)

  # Windows shared library for Python integration
  add_library(ucxclient_windows SHARED
    examples/port/u_port_windows.c
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
  )
  target_compile_options(ucxclient_windows PRIVATE /W4)
  target_compile_definitions(ucxclient_windows PRIVATE
    U_PORT_WINDOWS
    U_CX_AT_CONFIG_FILE="u_cx_at_config_windows.h"
    _CRT_SECURE_NO_WARNINGS
  )
  target_include_directories(ucxclient_windows PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
  target_link_libraries(ucxclient_windows PRIVATE setupapi)
  
  # Export all symbols for Python ctypes
  set_target_properties(ucxclient_windows PROPERTIES
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
