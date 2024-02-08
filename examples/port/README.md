# Port Examples

This directory contains example ports that you can either use out-of-box or use as inspiration.

| Files         | Description |
| ------------- | ----------- |
| u_port.h      | A common API mainly for being able to run the [example code](/examples/README.md) using any of the ports. |
| u_port_posix  | A Linux port example using Posix threads. |
| u_port_no_os  | A "no OS" port example to illustrate how ucxclient could be ported to a bare-metal system. The UART and time porting layer is using Linux API for this example so you will need to adjust it for your specific target. |
| u_port_zephyr | A Zephyr port example. You will find details on how to use it in [/zephyr/README.md](/zephyr/README.md). |

## Using an Example Port

You can tell ucxclient which port to use by using the following defines during build:

| Port          | Define         |
| ------------- | -------------- |
| u_port_posix  | `U_PORT_POSIX` |
| u_port_no_os  | `U_PORT_NO_OS` |
| u_port_zephyr | No define needed; it will be selected automatically if you use ucxclient as a Zephyr module (see [/zephyr/README.md](/zephyr/README.md)). |

You will also need to add corresponding .c file to your build (not needed for Zephyr).
