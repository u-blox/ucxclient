# FreeRTOS Port

This directory contains the FreeRTOS port implementation for ucxclient.

## Features

- **FreeRTOS mutex support** using semaphores
- **Background RX task** for automatic AT command handling
- **Configurable task parameters** (stack size, priority)
- **Tick-based timing** using FreeRTOS tick counter
- **Thread-safe** mutex operations with timeout support

## Requirements

- FreeRTOS kernel (tested with FreeRTOS V10.x and V11.x)
- STM32 HAL or equivalent RTOS-compatible HAL

## Configuration

### Task Parameters

You can customize the RX task parameters using compile-time defines:

```c
// Default stack size (in words, not bytes)
#define U_PORT_FREERTOS_RX_TASK_STACK_SIZE    (2048)

// Default priority (higher value = higher priority)
#define U_PORT_FREERTOS_RX_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
```

**CMake example:**
```cmake
target_compile_definitions(your_target PRIVATE
    U_PORT_FREERTOS_RX_TASK_STACK_SIZE=4096
    U_PORT_FREERTOS_RX_TASK_PRIORITY=10
)
```

### FreeRTOS Configuration

Ensure your `FreeRTOSConfig.h` has these settings enabled:

```c
#define configUSE_MUTEXES                1
#define configUSE_TIMERS                 0  // Not required
#define configSUPPORT_DYNAMIC_ALLOCATION 1  // Required for task creation
```

## Usage

### Basic Initialization

```c
#include "FreeRTOS.h"
#include "task.h"
#include "u_port.h"
#include "u_cx_at_client.h"

void your_task(void *pvParameters)
{
    // Initialize port layer (must be called before any port functions)
    uPortInit();

    // Open UART (see UART port documentation)
    uPortUartHandle_t uart = uPortUartOpen(NULL, 115200, false);

    // Initialize AT client
    uCxAtClient_t client;
    uCxAtClientConfig_t config = {
        .pUartHandle = uart,
        .pRxBuffer = rxBuffer,
        .rxBufferLen = sizeof(rxBuffer),
        .pUrcQueue = &urcQueue,
        .pRxEventCallback = rxEventCallback,
        .pRxEventCallbackParam = NULL,
        .instance = 0
    };

    int32_t result = uCxAtClientInit(&config, &client);
    if (result == 0) {
        // Background RX task is automatically created
        // You can now use the AT client from this task

        // ... your application code ...

        // Cleanup (automatically destroys RX task)
        uCxAtClientDeinit(&client);
    }

    uPortUartClose(uart);
    uPortDeinit();

    vTaskDelete(NULL);
}

int main(void)
{
    // Initialize hardware
    HAL_Init();
    SystemClock_Config();

    // Create your application task
    xTaskCreate(your_task, "ucx", 2048, NULL, 5, NULL);

    // Start FreeRTOS scheduler
    vTaskStartScheduler();

    // Should never reach here
    while (1);
}
```

### Background RX Task

The port automatically creates a background task when you call `uCxAtClientInit()`:

- **Task Name**: "ucxRx"
- **Function**: Polls UART every 10ms and processes received data
- **Lifecycle**: Created in `uCxAtClientInit()`, destroyed in `uCxAtClientDeinit()`

**Important**: The background RX task is essential for:
- Processing unsolicited result codes (URCs)
- Receiving responses to AT commands
- Maintaining communication with the module

### Mutex Operations

The port provides FreeRTOS-based mutex operations:

```c
U_CX_MUTEX_HANDLE mutex;
U_CX_MUTEX_CREATE(mutex);

// Lock with timeout
int32_t result = U_CX_MUTEX_TRY_LOCK(mutex, 1000);  // 1000ms timeout
if (result == 0) {
    // Critical section
    U_CX_MUTEX_UNLOCK(mutex);
}

U_CX_MUTEX_DELETE(mutex);
```

### Time Functions

Get elapsed time since `uPortInit()`:

```c
int32_t timeMs = uPortGetTickTimeMs();
```

**Note**: This uses FreeRTOS tick counter, so resolution depends on `configTICK_RATE_HZ`.

## Integration with STM32F4 UART Port

Complete CMake integration example:

```cmake
set(UCXCLIENT_ROOT ${CMAKE_CURRENT_LIST_DIR}/ucxclient)

# ucxclient includes
include_directories(
    ${UCXCLIENT_ROOT}/inc
    ${UCXCLIENT_ROOT}/ports
    ${UCXCLIENT_ROOT}/ucx_api
)

# ucxclient sources
set(UCXCLIENT_SOURCES
    # Core AT client
    ${UCXCLIENT_ROOT}/src/u_cx_at_client.c
    ${UCXCLIENT_ROOT}/src/u_cx_at_params.c
    ${UCXCLIENT_ROOT}/src/u_cx_at_urc_queue.c
    ${UCXCLIENT_ROOT}/src/u_cx_at_util.c
    ${UCXCLIENT_ROOT}/src/u_cx_log.c

    # UCX API
    ${UCXCLIENT_ROOT}/ucx_api/u_cx.c
    ${UCXCLIENT_ROOT}/ucx_api/u_cx_general.c
    ${UCXCLIENT_ROOT}/ucx_api/u_cx_wifi.c

    # Port layer - FreeRTOS + STM32F4
    ${UCXCLIENT_ROOT}/ports/os/u_port_freertos.c
    ${UCXCLIENT_ROOT}/ports/uart/u_port_uart_stm32f4.c
)

add_executable(your_firmware
    ${UCXCLIENT_SOURCES}
    # ... your files ...
)

target_compile_definitions(your_firmware PRIVATE
    U_PORT_FREERTOS
    U_PORT_UART_USE_USART2
    U_PORT_FREERTOS_RX_TASK_STACK_SIZE=2048
    U_PORT_FREERTOS_RX_TASK_PRIORITY=10
)

target_link_libraries(your_firmware
    freertos
    stm32f4_hal
)
```

## Task Priority Considerations

The RX task priority affects system responsiveness:

1. **Too Low**: URCs and responses may be delayed, timeouts may occur
2. **Too High**: May starve lower-priority tasks
3. **Recommended**: `configMAX_PRIORITIES - 2` (leaves room for critical ISR tasks)

Adjust based on your application requirements:

```c
#define U_PORT_FREERTOS_RX_TASK_PRIORITY  (configMAX_PRIORITIES - 2)
```

## Memory Usage

### Stack Memory

- **RX Task**: Default 2048 words (8 KB on ARM Cortex-M4)
- **AT Client**: Configured via `uCxAtClientConfig_t` (RX buffer, URC queue)

### Heap Memory

- Minimal dynamic allocation (task creation only)
- Most buffers are static or user-provided

## Debugging

### Enable FreeRTOS Stack Overflow Detection

In `FreeRTOSConfig.h`:

```c
#define configCHECK_FOR_STACK_OVERFLOW  2
```

Implement the callback:

```c
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    // Log or break here
    printf("Stack overflow in task: %s\n", pcTaskName);
    while(1);
}
```

### Monitor RX Task

Use FreeRTOS trace tools (e.g., Tracealyzer) to monitor:
- RX task CPU usage
- Context switches
- Blocked time

### Common Issues

**RX Task Not Running**
- Check that `vTaskStartScheduler()` is called
- Verify sufficient heap for task creation (`configTOTAL_HEAP_SIZE`)
- Check that `configSUPPORT_DYNAMIC_ALLOCATION` is enabled

**Mutex Deadlock**
- Ensure proper lock/unlock pairing
- Check that mutexes are created before use
- Verify timeout values are reasonable

**Timing Issues**
- `configTICK_RATE_HZ` affects time resolution (typical: 1000 Hz = 1ms)
- Ensure system clock is configured correctly

## Performance

### Typical Resource Usage (STM32F407VG @ 168 MHz)

- **RX Task CPU**: ~1-5% (depends on UART traffic)
- **Stack Usage**: ~500-1000 bytes typical, 2048 words allocated
- **Heap**: ~300 bytes for task control block

### Optimization Tips

1. **Adjust RX polling interval** in `u_port_freertos.c` (currently 10ms):
   ```c
   vTaskDelay(pdMS_TO_TICKS(10));  // Change to 5 for faster response
   ```

2. **Use higher UART baud rate** to reduce transfer time

3. **Enable HW flow control** for sustained high-speed data

4. **Increase RX buffer** for bursty traffic

## Thread Safety

The FreeRTOS port provides thread-safe mutex operations. However:

- **UART access**: Not inherently thread-safe. Use mutexes if multiple tasks access UART.
- **AT Client**: Thread-safe when using a single AT client instance from multiple tasks (uses internal mutexes).
- **RX Callbacks**: Executed in RX task context - keep callbacks short to avoid blocking.

## Migration from Other RTOS

If migrating from another RTOS, you'll need to:

1. Replace RTOS-specific mutex calls with FreeRTOS equivalents
2. Adjust task priorities (FreeRTOS: higher number = higher priority)
3. Update stack sizes (FreeRTOS uses words, not bytes)
4. Implement FreeRTOS hooks (idle, tick, stack overflow)

## License

Copyright 2025 u-blox

Licensed under the Apache License, Version 2.0.
