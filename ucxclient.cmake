set(REPO_DIR ${CMAKE_CURRENT_LIST_DIR})
file(GLOB UCXCLIENT_UCX_API_SRC
    ${REPO_DIR}/ucx_api/*.c
    ${REPO_DIR}/ucx_api/generated/*.c
)
file(GLOB UCXCLIENT_AT_API_SRC ${REPO_DIR}/src/*.c)
set(UCXCLIENT_INC ${REPO_DIR}/inc
    ${REPO_DIR}/ucx_api
    ${REPO_DIR}/ucx_api/generated
    ${REPO_DIR}/ports
)
set(UCXCLIENT_PORT_DIR ${REPO_DIR}/ports)
