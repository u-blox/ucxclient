cmake_minimum_required(VERSION 3.4)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

include(ucxclient.cmake)

set(U_EXAMPLE_UART "/dev/ttyUSB0" CACHE STRING "The UART device to use for the examples")
set(U_EXAMPLE_SSID "ubx" CACHE STRING "The Wi-Fi SSID to use for the examples")
set(U_EXAMPLE_WPA_PSK "" CACHE STRING "The Wi-Fi WPA PSK to use for the examples")

# HTTP Example
add_executable(http_example
  examples/http_example.c
  examples/port/u_port_posix.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
)
target_link_libraries(http_example PRIVATE Threads::Threads)
target_compile_options(http_example PRIVATE -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -pedantic -DU_CX_LOG_DEBUG=1 -DU_PORT_POSIX)
target_include_directories(http_example PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})

# No OS Example
add_executable(http_example_no_os
  examples/http_example_no_os.c
  examples/port/u_port_no_os.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
)
target_compile_options(http_example_no_os PRIVATE -Wall -Wextra -Werror -Wconversion -Wsign-conversion -Wshadow -pedantic -DU_CX_LOG_DEBUG=1 -DU_PORT_NO_OS)
target_compile_definitions(http_example_no_os PRIVATE
  -DU_EXAMPLE_UART="${U_EXAMPLE_UART}"
  -DU_EXAMPLE_SSID="${U_EXAMPLE_SSID}"
  -DU_EXAMPLE_WPA_PSK="${U_EXAMPLE_WPA_PSK}"
)
target_include_directories(http_example_no_os PUBLIC ${UCXCLIENT_INC} ${UCXCLIENT_PORT_DIR})
