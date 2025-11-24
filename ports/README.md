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
| os/u_port_windows | Windows port using Windows API for mutex, threads, and time. |
| os/u_port_no_os   | "No OS" port for bare-metal systems. Provides stub mutex and no background RX task - user must call uCxAtClientHandleRx() manually. |
| os/u_port_zephyr  | Zephyr RTOS port using work queues for background RX handling. |

## UART Ports

| Files                     | Description |
| ------------------------- | ----------- |
| uart/u_port_uart.h        | UART abstraction API (open, read, write, close). |
| uart/u_port_uart_linux    | Linux termios-based UART implementation. Used by both POSIX and no-OS ports. |
| uart/u_port_uart_windows  | Windows COM port UART implementation using Windows API. |
| uart/u_port_uart_zephyr   | Zephyr interrupt-driven UART with ring buffer. |

## Background RX Task

The port layer optionally implements `uPortBgRxTaskCreate()` and `uPortBgRxTaskDestroy()`:

* **POSIX port**: Creates a pthread that polls `uCxAtClientHandleRx()` every 10ms
* **Windows port**: Creates a Windows thread that polls `uCxAtClientHandleRx()` every 10ms
* **Zephyr port**: Uses work queue that is triggered by UART ISR
* **No-OS port**: Stub implementation - user must call `uCxAtClientHandleRx()` manually in their main loop

These functions are called automatically by `uCxAtClientInit()` and `uCxAtClientDeinit()`.

## Using an Example Port

You can tell ucxclient which port to use by using the following defines during build:

| Port           | Define            |
| -------------- | ----------------- |
| u_port_posix   | `U_PORT_POSIX`    |
| u_port_windows | `U_PORT_WINDOWS`  |
| u_port_no_os   | `U_PORT_NO_OS`    |
| u_port_zephyr  | No define needed; it will be selected automatically if you use ucxclient as a Zephyr module (see [/zephyr/README.md](/zephyr/README.md)). |

You will also need to add corresponding .c files to your build (not needed for Zephyr).

## Port API Requirements

Each port must implement the following macros and functions:

### Mutex Abstraction

```c
U_CX_MUTEX_HANDLE                     // Type for mutex handle
U_CX_MUTEX_CREATE(mutex)              // Initialize mutex
U_CX_MUTEX_DELETE(mutex)              // Destroy mutex
U_CX_MUTEX_LOCK(mutex)                // Lock mutex (blocking)
U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) // Try to lock with timeout
U_CX_MUTEX_UNLOCK(mutex)              // Unlock mutex
```

### Time Abstraction

```c
U_CX_PORT_GET_TIME_MS()   // Get current time in milliseconds
U_CX_PORT_SLEEP_MS(ms)    // Sleep for specified milliseconds
```

### Core Functions

```c
void uPortInit(void);                           // Initialize port layer
void uPortDeinit(void);                         // Deinitialize port layer
void uPortBgRxTaskCreate(uCxAtClient_t *);      // Create background RX task (optional)
void uPortBgRxTaskDestroy(uCxAtClient_t *);     // Destroy background RX task (optional)
```

### Optional Overrides

```c
U_CX_AT_PORT_ASSERT(COND)  // Assert macro (default: assert())
U_CX_PORT_PRINTF           // Printf function (default: printf)
```

