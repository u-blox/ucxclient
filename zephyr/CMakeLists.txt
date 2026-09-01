if(CONFIG_U_CONNECT_CLIENT)
  include(../u-connectClient.cmake)
  zephyr_library()
  zephyr_include_directories(
    ${U_CONNECT_CLIENT_INC}
    ${U_CONNECT_CLIENT_PORT_DIR}
    ${U_CONNECT_CLIENT_PORT_DIR}/uart
  )
  zephyr_library_sources(
    ${U_CONNECT_CLIENT_UCX_API_SRC}
    ${U_CONNECT_CLIENT_AT_API_SRC}
    ${U_CONNECT_CLIENT_PORT_DIR}/os/u_port_zephyr.c
    ${U_CONNECT_CLIENT_PORT_DIR}/uart/u_port_uart_zephyr.c
  )
endif() # CONFIG_U_CONNECT_CLIENT
