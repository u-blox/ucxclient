cmake_minimum_required(VERSION 3.10)
project(windows-demo)

# Include ucxclient library definitions
include(../../ucxclient.cmake)

# ============================================================================
# Auto-versioning: Get build number from Git commit count
# ============================================================================
set(APP_VERSION_MAJOR 3)
set(APP_VERSION_MINOR 2)
set(APP_VERSION_PATCH 0)
set(APP_VERSION_BUILD 0)

# Try to get Git commit count for build number
find_package(Git QUIET)
if(GIT_FOUND)
  execute_process(
    COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_COUNT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
  )
  if(GIT_COMMIT_COUNT)
    set(APP_VERSION_BUILD ${GIT_COMMIT_COUNT})
    message(STATUS "Build version: ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}.${APP_VERSION_BUILD} (Git commit count)")
  else()
    message(STATUS "Build version: ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}.${APP_VERSION_BUILD} (Git not available)")
  endif()
else()
  message(STATUS "Build version: ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR}.${APP_VERSION_PATCH}.${APP_VERSION_BUILD} (Git not found)")
endif()

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
  message(FATAL_ERROR "windows-demo requires Windows")
endif()

set(U_EXAMPLE_UART "COM3" CACHE STRING "The UART device to use")

# Port layer files for Windows
set(PORT_OS_SRC ../../ports/os/u_port_windows.c)
set(PORT_UART_SRC ../../ports/uart/u_port_uart_windows.c)

# FTDI D2XX library support
set(FTDI_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ftdi")
set(FTDI_LIBRARY "${CMAKE_CURRENT_SOURCE_DIR}/third-party/ftdi/ftd2xx.lib")

# Windows Demo Application (Monolithic)
add_executable(windows-demo
  windows-demo.c
  ${PORT_OS_SRC}
  ${PORT_UART_SRC}
  windows-demo.rc
  third-party/qrcodegen/qrcodegen.c
  ../../src/u_cx_xmodem.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
  ${UCXCLIENT_HEADERS}
)

target_compile_options(windows-demo PRIVATE /W4 /FS /wd4200)

target_compile_definitions(windows-demo PRIVATE
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

target_include_directories(windows-demo PUBLIC 
  ${UCXCLIENT_INC} 
  ${UCXCLIENT_PORT_DIR} 
  ${FTDI_INCLUDE_DIR} 
  ${CMAKE_CURRENT_SOURCE_DIR} 
  ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(windows-demo PRIVATE setupapi ${FTDI_LIBRARY})

# Set different output names for Debug and Release builds
set_target_properties(windows-demo PROPERTIES
  OUTPUT_NAME_DEBUG "windows-demo-debug"
  OUTPUT_NAME_RELEASE "windows-demo"
)
