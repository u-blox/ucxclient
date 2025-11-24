set(REPO_DIR ${CMAKE_CURRENT_LIST_DIR})
file(GLOB UCXCLIENT_UCX_API_SRC
    ${REPO_DIR}/ucx_api/*.c
    ${REPO_DIR}/ucx_api/generated/*.c
)
file(GLOB UCXCLIENT_AT_API_SRC ${REPO_DIR}/src/*.c)

# Collect header files to ensure CMake tracks them for rebuild dependencies
file(GLOB UCXCLIENT_HEADERS
    ${REPO_DIR}/inc/*.h
    ${REPO_DIR}/ucx_api/*.h
    ${REPO_DIR}/ucx_api/generated/*.h
)

set(UCXCLIENT_INC ${REPO_DIR}/inc
    ${REPO_DIR}/ucx_api
    ${REPO_DIR}/ucx_api/generated
    ${REPO_DIR}/ports
)
set(UCXCLIENT_PORT_DIR ${REPO_DIR}/ports)
