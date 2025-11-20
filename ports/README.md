# Port Examples

This directory contains the port layer abstraction that enables ucxclient to run on different platforms.

## Port Structure

The port layer is organized into:

* **OS abstraction** (`os/`): Mutex, time, and optional background RX task
* **UART abstraction** (`uart/`): Platform-specific UART I/O

## OS Ports

| Files             | Description |
| ----------------- | ----------- |
| u_port.h          | Common port API header with platform selection and abstractions. |
| os/u_port_posix   | Linux/POSIX port using pthreads for mutex and background RX thread. |
| os/u_port_no_os   | "No OS" port for bare-metal systems. Provides stub mutex and no background RX task - user must call uCxAtClientHandleRx() manually. |
| os/u_port_zephyr  | Zephyr RTOS port using work queues for background RX handling. |

## UART Ports

| Files                  | Description |
| ---------------------- | ----------- |
| uart/u_port_uart.h     | UART abstraction API (open, read, write, close). |
| uart/u_port_uart_linux | Linux termios-based UART implementation. Used by both POSIX and no-OS ports. |
| uart/u_port_uart_zephyr| Zephyr interrupt-driven UART with ring buffer. |

## Background RX Task

The port layer optionally implements `uPortBgRxTaskCreate()` and `uPortBgRxTaskDestroy()`:

* **POSIX port**: Creates a pthread that polls `uCxAtClientHandleRx()` every 10ms
* **Zephyr port**: Uses work queue that is triggered by UART ISR
* **No-OS port**: Stub implementation - user must call `uCxAtClientHandleRx()` manually in their main loop

These functions are called automatically by `uCxAtClientInit()` and `uCxAtClientDeinit()`.

## Using an Example Port

You can tell ucxclient which port to use by using the following defines during build:

| Port          | Define         |
| ------------- | -------------- |
| u_port_posix  | `U_PORT_POSIX` |
| u_port_no_os  | `U_PORT_NO_OS` |
| u_port_zephyr | No define needed; it will be selected automatically if you use ucxclient as a Zephyr module (see [/zephyr/README.md](/zephyr/README.md)). |

You will also need to add corresponding .c files to your build (not needed for Zephyr).
