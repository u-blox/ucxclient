cmake_minimum_required(VERSION 3.4)
project(at_client)
string(TOLOWER  ${CMAKE_SYSTEM_NAME} OS_NAME)

file(GLOB UCX_API_SRC ucx_api/*.c)

if (DEFINED ENV{UBXLIB_DIR})
  # Ubxlib based AT client app
  set(APP_NAME_UBXLIB at_client)
  add_executable(${APP_NAME_UBXLIB}
    test/u_main_ubxlib.c
  )
  # Get and build the ubxlib library
  set(UBXLIB_BASE $ENV{UBXLIB_DIR})
  include(${UBXLIB_BASE}/port/platform/${OS_NAME}/${OS_NAME}.cmake)
  target_link_libraries(${APP_NAME_UBXLIB} ubxlib ${UBXLIB_REQUIRED_LINK_LIBS})
  target_include_directories(${APP_NAME_UBXLIB} PUBLIC ${UBXLIB_INC} ${UBXLIB_PUBLIC_INC_PORT} inc ucx_api)
else()
  message(WARNING "Ubxlib location not defined (UBXLIB_DIR) - not building ubxlib app")
endif()


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
