cmake_minimum_required(VERSION 3.10)
project(ucx-windows-app)

# Include ucxclient library definitions
include(../../ucxclient.cmake)

# ============================================================================
# Auto-versioning: Use timestamp-based build number
# ============================================================================
set(APP_VERSION_MAJOR 3)
set(APP_VERSION_MINOR 2)
set(APP_VERSION_PATCH 0)
set(APP_VERSION_BUILD 0)

# Generate build number from current date: YYDDD (e.g., 25338 = 2025, day 338)
# This is consistent across branches and increases over time
string(TIMESTAMP BUILD_YEAR "%y")
string(TIMESTAMP BUILD_DAY_OF_YEAR "%j")
math(EXPR APP_VERSION_BUILD "${BUILD_YEAR}${BUILD_DAY_OF_YEAR}")

message(STATUS "Build version: ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}.${APP_VERSION_BUILD} (timestamp-based)")

# Generate version header
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/version.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/version.h
  @ONLY
)

# Set binary output directory to bin/
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_CURRENT_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_CURRENT_SOURCE_DIR}/bin)

# Windows-specific settings
if(NOT WIN32)
  message(FATAL_ERROR "ucx-windows-app requires Windows")
endif()

set(U_EXAMPLE_UART "COM3" CACHE STRING "The UART device to use")

# Port layer files for Windows
set(PORT_OS_SRC ../../ports/os/u_port_windows.c)
set(PORT_UART_SRC ../../ports/uart/u_port_uart_windows.c)

# FTDI D2XX library support
set(FTDI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ftdi")
set(FTDI_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ftdi/ftd2xx.lib")

# ucx-windows-app Application (Monolithic)
add_executable(ucx-windows-app
  ucx-windows-app.c
  ${PORT_OS_SRC}
  ${PORT_UART_SRC}
  ucx-windows-app.rc
  third-party/qrcodegen/qrcodegen.c
  ../../src/u_cx_xmodem.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
  ${UCXCLIENT_HEADERS}
)

target_compile_options(ucx-windows-app PRIVATE /W4 /FS /wd4200)

target_compile_definitions(ucx-windows-app PRIVATE
  U_PORT_WINDOWS
  U_CX_PORT_HEADER_FILE="os/u_port_windows.h"
  U_CX_XMODEM_FILE_SUPPORT=1
  _CRT_SECURE_NO_WARNINGS
  U_EXAMPLE_UART="${U_EXAMPLE_UART}"
  APP_VERSION_MAJOR=${APP_VERSION_MAJOR}
  APP_VERSION_MINOR=${APP_VERSION_MINOR}
  APP_VERSION_PATCH=${APP_VERSION_PATCH}
  APP_VERSION_BUILD=${APP_VERSION_BUILD}
)

target_include_directories(ucx-windows-app PUBLIC 
  ${UCXCLIENT_INC} 
  ${UCXCLIENT_PORT_DIR} 
  ${FTDI_INCLUDE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(ucx-windows-app PRIVATE setupapi ${FTDI_LIBRARY})

# Set different output names for Debug and Release builds
set_target_properties(ucx-windows-app PROPERTIES
  OUTPUT_NAME_DEBUG "ucx-windows-app-debug"
  OUTPUT_NAME_RELEASE "ucx-windows-app"
)
