if(CONFIG_UCXCLIENT)
  include(../ucxclient.cmake)
  zephyr_library()
  zephyr_include_directories(
    ${UCXCLIENT_INC}
    ${UCXCLIENT_PORT_DIR}
    ${UCXCLIENT_PORT_DIR}/uart
  )
  zephyr_library_sources(
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
    ${UCXCLIENT_PORT_DIR}/os/u_port_zephyr.c
    ${UCXCLIENT_PORT_DIR}/uart/u_port_uart_zephyr.c
  )
endif() # CONFIG_UCXCLIENT
