cmake_minimum_required(VERSION 3.4)
project(ucxclient)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

find_package(Threads REQUIRED)

file(GLOB UCX_API_SRC ucx_api/*.c)
file(GLOB AT_API_SRC src/*.c)

# HTTP Example
add_executable(http_example
  examples/http_example.c
  examples/port/u_port_posix.c
  ${UCX_API_SRC}
  ${AT_API_SRC}
)
target_link_libraries(http_example PRIVATE Threads::Threads)
target_compile_options(http_example PRIVATE -Wall -Werror)
target_include_directories(http_example PUBLIC inc ucx_api examples/port)
