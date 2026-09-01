# Port Examples

This directory contains the port layer abstraction that enables u-connectClient to run on different platforms.

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
| os/u_port_freertos | FreeRTOS port using FreeRTOS mutexes, timers, and tasks for background RX handling. |

See also: [STM32F4 port](extra/stm32f4/README.md) for embedded ARM Cortex-M4 with FreeRTOS.

## UART Ports

| Files                     | Description |
| ------------------------- | ----------- |
| uart/u_port_uart.h        | UART abstraction API (open, read, write, close). |
| uart/u_port_uart_linux    | Linux termios-based UART implementation. Used by both POSIX and no-OS ports. |
| uart/u_port_uart_windows  | Windows COM port UART implementation using Windows API. |
| uart/u_port_uart_zephyr   | Zephyr interrupt-driven UART with ring buffer. |
| uart/u_port_uart_stm32f4_irq | STM32F4 HAL-based UART, interrupt-driven RX (default, proven). |
| uart/u_port_uart_stm32f4_dma | STM32F4 HAL-based UART, circular-DMA RX (opt-in via `STM32_UART_USE_DMA=ON`, needed for 2 Mbaud+). |
| uart/u_port_uart_stm32h7  | STM32H7 HAL-based UART implementation with DMA support. |

## Background RX Task

The port layer optionally implements `uPortBgRxTaskCreate()` and `uPortBgRxTaskDestroy()`:

* **POSIX port**: Creates a pthread that polls `uCxAtClientHandleRx()` every 10ms
* **Windows port**: Creates a Windows thread that polls `uCxAtClientHandleRx()` every 10ms
* **Zephyr port**: Uses work queue that is triggered by UART ISR
* **FreeRTOS port**: Creates a FreeRTOS task that polls `uCxAtClientHandleRx()` every 10ms
* **No-OS port**: Stub implementation - user must call `uCxAtClientHandleRx()` manually in their main loop

These functions are called automatically by `uCxAtClientInit()` and `uCxAtClientDeinit()`.

## Using an Example Port

You can tell u-connectClient which port to use by using the following defines during build:

| Port           | Define            |
| -------------- | ----------------- |
| u_port_posix   | `U_PORT_POSIX`    |
| u_port_windows | `U_PORT_WINDOWS`  |
| u_port_no_os   | `U_PORT_NO_OS`    |
| u_port_freertos | `U_PORT_FREERTOS` |
| u_port_zephyr  | No define needed; it will be selected automatically if you use u-connectClient as a Zephyr module (see [/zephyr/README.md](/zephyr/README.md)). |

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

## Performance & Latency Tuning

Projects with high-throughput/low-latency requirements (e.g. Matter/Thread
controllers) should review these compile-time settings:

| Flag | Where | Default | Effect |
| ---- | ----- | ------- | ------ |
| `U_CX_EVENT_DRIVEN_IO` | inc/u_cx_at_config.h | `1` | Bulk, blocking UART reads via a 2048-byte read-ahead buffer instead of byte-per-byte polling; TX commands coalesced into one write. Set to `0` only to fall back to the original byte-per-byte/polled behaviour if a port has compatibility issues. |
| `U_CX_USE_URC_QUEUE` | inc/u_cx_at_config.h | `1` | Lets you issue AT commands from inside a URC callback. Costs one extra copy per URC (rxBuffer -> URC queue buffer). Leave enabled unless memory-constrained and URC-in-callback usage isn't needed. |
| `U_CX_LOG_DEBUG` | inc/u_cx_at_config.h | `0` | Debug logging is off by default; every enabled log line adds UART-blocking `printf()` time on the calling thread. Keep `0` in latency-sensitive builds. |
| `STM32_UART_USE_DMA` | cmake/stm32.cmake (STM32F4 only) | `OFF` | Selects `u_port_uart_stm32f4_dma.c` (circular DMA RX, higher throughput, required for 2 Mbaud+) instead of the default `u_port_uart_stm32f4_irq.c` (interrupt-driven, proven, sufficient up to ~1 Mbaud). Pass `-DSTM32_UART_USE_DMA=ON` when high baud rate is required. STM32H7 only has the DMA implementation (no switch). |

Additional latency-relevant behavior (not compile flags, but good to know):

* **Windows port**: automatically sets the FTDI USB-serial `LatencyTimer` registry
  value to 1 ms on port open (factory default is 16 ms) to remove hidden USB
  buffering delay. Only affects FTDI-based adapters; other USB-UART chips are
  unaffected (registry key won't exist, error is ignored).
* **Windows port read timeouts**: with `U_CX_EVENT_DRIVEN_IO=1`, `ReadFile()` is
  configured to return immediately when data is already buffered and to wait
  at most 1 ms when idle (vs. a 10 ms poll interval when `U_CX_EVENT_DRIVEN_IO=0`).
* **RX buffer sizing**: `u_port_uart_stm32f4_irq.c` / `_dma.c` use
  `U_PORT_UART_RX_BUFFER_SIZE` (default 2048 / 8192 bytes respectively);
  override via compile definition if a board needs a different size.

