set(REPO_DIR ${CMAKE_CURRENT_LIST_DIR})

# UCX_MODULE selects which module-specific generated API to use
# Available modules have their own subdirectory in ucx_api/generated/
if(NOT DEFINED UCX_MODULE)
    message(WARNING "UCX_MODULE not set, defaulting to NORA-W36X")
    set(UCX_MODULE "NORA-W36X")
endif()

# Verify the module directory exists
set(UCX_MODULE_DIR ${REPO_DIR}/ucx_api/generated/${UCX_MODULE})
if(NOT EXISTS ${UCX_MODULE_DIR})
    message(FATAL_ERROR "UCX_MODULE '${UCX_MODULE}' not found. "
        "Directory '${UCX_MODULE_DIR}' does not exist. "
        "Available modules can be found in ucx_api/generated/")
endif()

message(STATUS "Using UCX_MODULE: ${UCX_MODULE}")

file(GLOB UCXCLIENT_UCX_API_SRC
    ${REPO_DIR}/ucx_api/*.c
    ${UCX_MODULE_DIR}/*.c
)
file(GLOB UCXCLIENT_AT_API_SRC ${REPO_DIR}/src/*.c)
set(UCXCLIENT_INC ${REPO_DIR}/inc
    ${REPO_DIR}/ucx_api
    ${UCX_MODULE_DIR}
    ${REPO_DIR}/ports
)
set(UCXCLIENT_PORT_DIR ${REPO_DIR}/ports)
