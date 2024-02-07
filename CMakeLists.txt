cmake_minimum_required(VERSION 3.4)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

include(ucxclient.cmake)

# HTTP Example
add_executable(http_example
  examples/http_example.c
  examples/port/u_port_posix.c
  ${UCXCLIENT_UCX_API_SRC}
  ${UCXCLIENT_AT_API_SRC}
)
target_link_libraries(http_example PRIVATE Threads::Threads)
target_compile_options(http_example PRIVATE -Wall -Werror -DU_CX_LOG_DEBUG=1)
target_include_directories(http_example PUBLIC ${UCXCLIENT_INC})
