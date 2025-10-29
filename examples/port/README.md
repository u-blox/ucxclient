# Port Examples

This directory contains example ports that you can either use out-of-box or use as inspiration.

| Files           | Description |
| --------------- | ----------- |
| u_port.h        | A common API mainly for being able to run the [example code](/examples/README.md) using any of the ports. |
| u_port_posix    | A Linux port example using Posix threads. |
| u_port_no_os    | A "no OS" port example to illustrate how ucxclient could be ported to a bare-metal system. The UART and time porting layer is using Linux API for this example so you will need to adjust it for your specific target. |
| u_port_freertos | A FreeRTOS port example for embedded systems like STM32. Uses FreeRTOS semaphores for mutexes and tick count for timing. You need to implement the UART functions for your specific hardware. |
| u_port_zephyr   | A Zephyr port example. You will find details on how to use it in [/zephyr/README.md](/zephyr/README.md). |

## Using an Example Port

You can tell ucxclient which port to use by using the following defines during build:

| Port            | Define           |
| --------------- | ---------------- |
| u_port_posix    | `U_PORT_POSIX`   |
| u_port_no_os    | `U_PORT_NO_OS`   |
| u_port_freertos | `U_PORT_FREERTOS`|
| u_port_zephyr   | No define needed; it will be selected automatically if you use ucxclient as a Zephyr module (see [/zephyr/README.md](/zephyr/README.md)). |

You will also need to add corresponding .c file to your build (not needed for Zephyr).

## FreeRTOS Port Details

The FreeRTOS port (`u_port_freertos`) provides integration with FreeRTOS RTOS for embedded platforms like STM32, ESP32, and others.

### Features
- Uses FreeRTOS semaphores (`xSemaphoreCreateMutex`) for thread-safe mutex operations
- Uses FreeRTOS tick count (`xTaskGetTickCount`) for timing
- Supports timeout-based mutex locking
- Ready for multi-threaded applications

### Hardware Integration Required

The FreeRTOS port includes **placeholder implementations** for UART functions that you must implement for your specific hardware:

- `uPortUartOpen()` - Initialize and configure UART
- `uPortUartClose()` - Deinitialize UART
- `uPortUartWrite()` - Write data to UART
- `uPortUartRead()` - Read data from UART (with timeout support)
- `uPortUartFlush()` - Flush UART buffers

Example implementations are provided in `u_port_freertos.c` for STM32 HAL, but you should adapt them to your specific hardware and UART driver.

### Example for STM32 with HAL

```c
// In u_port_freertos.c, implement the UART functions:

static void *uPortUartOpen(const char *pDevName, int baudRate, bool useFlowControl)
{
    UART_HandleTypeDef *pHuart = &huart1;  // Your UART handle
    
    pHuart->Init.BaudRate = baudRate;
    pHuart->Init.WordLength = UART_WORDLENGTH_8B;
    pHuart->Init.StopBits = UART_STOPBITS_1;
    pHuart->Init.Parity = UART_PARITY_NONE;
    pHuart->Init.Mode = UART_MODE_TX_RX;
    pHuart->Init.HwFlowCtl = useFlowControl ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
    
    if (HAL_UART_Init(pHuart) != HAL_OK) {
        return NULL;
    }
    return pHuart;
}
```

### Usage in Your Application

```c
#include "u_port.h"
#include "u_cx.h"

void myTask(void *pvParameters)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    
    // Initialize the port layer
    uPortAtInit(&client);
    
    // Open UART connection
    if (!uPortAtOpen(&client, "UART1", 115200, true)) {
        // Handle error
        return;
    }
    
    // Initialize uCx API
    uCxInit(&client, &ucxHandle);
    
    // Use the API...
    
    // Cleanup
    uPortAtClose(&client);
}
```
