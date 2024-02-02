if(CONFIG_UCXCLIENT)
  include(../ucxclient.cmake)
  zephyr_library()
  zephyr_include_directories(
    ${UCXCLIENT_INC}
    ${UCXCLIENT_PORT_DIR}
  )
  zephyr_library_sources(
    ${UCXCLIENT_UCX_API_SRC}
    ${UCXCLIENT_AT_API_SRC}
    ${UCXCLIENT_PORT_DIR}/u_port_zephyr.c
  )
endif() # CONFIG_UCXCLIENT
