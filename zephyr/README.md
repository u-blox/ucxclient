# Zephyr Module

u-connectClient can be used as a Zephyr module for easy integration to your Zephyr application.

## Adding u-connectClient to Your Zephyr App

There are several ways of including u-connectClient to your Zephyr application.

### Using `west.yml` manifest

If you use a `west.yml` manifest for your application then you can add u-connectClient to the list of projects like this:

```yml
manifest:
  remotes:
    - name: zephyrproject-rtos
      url-base: https://github.com/zephyrproject-rtos
    - name: u-blox
      url-base: https://github.com/u-blox

  projects:
    - name: zephyr
      remote: zephyrproject-rtos
      revision: main

    - name: u-connectClient
      remote: u-blox
      repo-path: u-connectClient.git
      revision: master
```

There is a very useful Zephyr example app illustrating how to create folder structure etc for a Zephyr application using a `west.yml` manifest file available here:  
[https://github.com/zephyrproject-rtos/example-application](https://github.com/zephyrproject-rtos/example-application)

### Using `ZEPHYR_MODULES` CMake Variable

With this method you need to clone u-connectClient manually or use git submodule or similar.
After that you need to add the path of u-connectClient to the `ZEPHYR_MODULES` CMake variable.
This can be done in several ways as described here:  
[https://docs.zephyrproject.org/latest/develop/modules.html#without-west](https://docs.zephyrproject.org/latest/develop/modules.html#without-west)

## Config

To build u-connectClient with your Zephyr application you must add the following to your `prj.conf`:

```ini
CONFIG_SERIAL=y
CONFIG_UART_INTERRUPT_DRIVEN=y
CONFIG_RING_BUFFER=y
CONFIG_U_CONNECT_CLIENT=y
```

The current Zephyr port only support the interrupt driven UART API.

Further configuration of u-connectClient is also possible.
Use [menuconfig](https://docs.zephyrproject.org/latest/build/kconfig/menuconfig.html) to view and configure these.
