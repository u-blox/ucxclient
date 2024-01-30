cmake_minimum_required(VERSION 3.4)
project(at_client)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

file(GLOB UCX_API_SRC ucx_api/*.c)

# Stdio AT client for testing
set(APP_NAME_TEST at_client_test)
add_executable(${APP_NAME_TEST}
  src/u_cx_at_client.c
  src/u_cx_at_util.c
  src/u_cx_at_params.c
  src/u_cx_at_urc_queue.c
  src/u_cx_log.c
  test/u_main_test.c
  ${UCX_API_SRC}
)
find_package(Threads REQUIRED)
target_link_libraries(${APP_NAME_TEST} PRIVATE Threads::Threads)
target_compile_options(${APP_NAME_TEST} PRIVATE -Wall -Werror)

target_include_directories(${APP_NAME_TEST} PUBLIC ${UBXLIB_INC} ${UBXLIB_PUBLIC_INC_PORT} inc ucx_api)
