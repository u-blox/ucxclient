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

The FreeRTOS port (`u_port_freertos`) provides integration with FreeRTOS RTOS for embedded platforms.

### Supported Platforms

The port includes complete UART implementations for the following platforms:

| Platform | Define | MCU Families | SDK/HAL |
|----------|--------|--------------|---------|
| **STM32** | `U_PORT_STM32_HAL` | F0/F1/F2/F3/F4/F7/G0/G4/H7/L0/L1/L4/L5/U5/WB/WL | STM32 HAL |
| **ESP32** | `U_PORT_ESP32` | ESP32, ESP32-S2/S3/C2/C3/C6 | ESP-IDF |
| **NXP** | `U_PORT_NXP_SDK` | Kinetis, LPC, i.MX RT | MCUXpresso SDK |
| **Microchip/Atmel** | `U_PORT_ATMEL_ASF` | SAM D/E/C/S/L | Atmel ASF |
| **Texas Instruments** | `U_PORT_TI_DRIVERLIB` | MSP432, Tiva C (TM4C) | TI DriverLib |
| **Silicon Labs** | `U_PORT_SILABS_GECKO` | EFM32, EFR32 | Gecko SDK |
| **Renesas** | `U_PORT_RENESAS_FSP` | RA series | Renesas FSP |
| **ARM Cortex-A** | `U_PORT_ARM_CORTEXA` | i.MX, Zynq, Versatile, etc. | FreeRTOS+POSIX |
| **RISC-V** | `U_PORT_RISCV` | SiFive, GigaDevice, Nuclei | Memory-mapped UART |

### Features
- Uses FreeRTOS semaphores (`xSemaphoreCreateMutex`) for thread-safe mutex operations
- Uses FreeRTOS tick count (`xTaskGetTickCount`) for timing
- Supports timeout-based mutex locking
- Ready for multi-threaded applications
- Complete UART implementations with:
  - Configurable baud rate
  - Hardware flow control (CTS/RTS) support
  - Timeout-based blocking and non-blocking reads
  - Buffer flushing

### Platform-Specific Setup

#### STM32 (U_PORT_STM32_HAL)

**Build Configuration:**
```c
// In your compiler defines:
#define U_PORT_FREERTOS
#define U_PORT_STM32_HAL
#define STM32F4  // Or your STM32 family: F0, F1, F7, H7, L4, etc.
```

**Hardware Setup:**
1. Use STM32CubeMX to generate UART initialization code
2. The port expects `huart1`, `huart2`, `huart3` etc. as extern variables
3. GPIO and clock configuration is handled by CubeMX init code

**Example:**
```c
#include "u_port.h"
#include "u_cx.h"

void ucxTask(void *pvParameters)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    
    uPortAtInit(&client);
    
    // "UART1" or "USART1" both work
    if (!uPortAtOpen(&client, "UART1", 115200, true)) {
        // Handle error
        return;
    }
    
    uCxInit(&client, &ucxHandle);
    
    // Use the API...
    
    uPortAtClose(&client);
}
```

#### ESP32 (U_PORT_ESP32)

**Build Configuration:**
```c
// In your component CMakeLists.txt or project config:
target_compile_definitions(${COMPONENT_LIB} PRIVATE
    U_PORT_FREERTOS
    U_PORT_ESP32
)
```

**Hardware Setup:**
```c
// In your main app_main():
// Configure UART pins before using uPortAtOpen
uart_set_pin(UART_NUM_1, TX_PIN, RX_PIN, RTS_PIN, CTS_PIN);
uart_driver_install(UART_NUM_1, 1024, 1024, 0, NULL, 0);
```

**Example:**
```c
uPortAtInit(&client);
uPortAtOpen(&client, "UART1", 115200, true);  // Uses UART_NUM_1
```

#### NXP (U_PORT_NXP_SDK)

**Build Configuration:**
```c
#define U_PORT_FREERTOS
#define U_PORT_NXP_SDK
```

**Hardware Setup:**
1. Initialize UART clocks in your board init code
2. Use device names: "UART0", "UART1", "UART2"

#### Other Platforms

Similar configuration applies:
- **Atmel**: `U_PORT_ATMEL_ASF` - Configure SERCOM pins
- **TI**: `U_PORT_TI_DRIVERLIB` - Configure EUSCI module
- **Silicon Labs**: `U_PORT_SILABS_GECKO` - Configure USART pins and clocks
- **Renesas**: `U_PORT_RENESAS_FSP` - Configure UART instance in FSP configurator

#### ARM Cortex-A (U_PORT_ARM_CORTEXA)

**Build Configuration:**
```c
#define U_PORT_FREERTOS
#define U_PORT_ARM_CORTEXA
```

**Platform Details:**
- For more powerful applications using ARM Cortex-A processors with FreeRTOS+POSIX
- Common platforms: NXP i.MX series, Xilinx Zynq, ARM Versatile Express
- Uses POSIX serial port API (termios, open, read, write)
- Device names are platform-specific (e.g., "/dev/ttyS0", "/dev/ttyAMA0", "/dev/ttyUSB0")

**Hardware Setup:**
1. Ensure FreeRTOS+POSIX layer is enabled in your FreeRTOS configuration
2. Serial device drivers must be available in your system
3. Device nodes should be accessible via standard POSIX open()

**Example:**
```c
#include "u_port.h"
#include "u_cx.h"

void ucxTask(void *pvParameters)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    
    uPortAtInit(&client);
    
    // Use POSIX device name
    if (!uPortAtOpen(&client, "/dev/ttyS0", 115200, true)) {
        // Handle error
        return;
    }
    
    uCxInit(&client, &ucxHandle);
    
    // Use the API...
    
    uPortAtClose(&client);
}
```

**Notes:**
- Supports standard baud rates: 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600
- Hardware flow control (CTS/RTS) is supported via termios CRTSCTS flag
- Timeout-based reads use poll() for efficient waiting

#### RISC-V (U_PORT_RISCV)

**Build Configuration:**
```c
#define U_PORT_FREERTOS
#define U_PORT_RISCV
```

**Platform Details:**
- For RISC-V cores with official FreeRTOS support
- Common platforms: SiFive FE310, GigaDevice GD32VF103, Nuclei cores
- Uses memory-mapped UART registers (16550-compatible or platform-specific)
- Default UART base addresses: 0x10013000 (UART0), 0x10023000 (UART1)

**Hardware Setup:**
1. Adjust UART base addresses for your RISC-V SoC:
   ```c
   #define RISCV_UART0_BASE 0x10013000  // SiFive FE310 UART0
   #define RISCV_UART1_BASE 0x10023000  // SiFive FE310 UART1
   ```

2. Adjust clock frequency if different from 32 MHz:
   ```c
   #define RISCV_CLOCK_FREQ 32000000  // 32 MHz default
   ```

3. Verify UART status register bit definitions match your hardware:
   ```c
   #define UART_STATUS_RXNE (1 << 0)  // RX not empty
   #define UART_STATUS_TXE  (1 << 1)  // TX empty
   ```

**Example:**
```c
#include "u_port.h"
#include "u_cx.h"

void ucxTask(void *pvParameters)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    
    uPortAtInit(&client);
    
    // Use generic device name
    if (!uPortAtOpen(&client, "UART0", 115200, true)) {
        // Handle error
        return;
    }
    
    uCxInit(&client, &ucxHandle);
    
    // Use the API...
    
    uPortAtClose(&client);
}
```

**Notes:**
- GPIO and clock initialization must be done in your board init code
- The implementation assumes a standard UART register layout
- You may need to adjust register offsets and bit definitions for your specific RISC-V SoC
- Hardware flow control support depends on your UART peripheral

### Hardware Integration Required

While the UART functions are fully implemented, you still need to:

1. **Initialize GPIO pins** for UART TX/RX/CTS/RTS
2. **Enable clocks** for the UART peripheral
3. **Configure interrupts** if using interrupt-driven mode (optional)
4. **Declare UART handles** as extern in your main.c (for HAL-based platforms)

Most of this is handled automatically by:
- **STM32CubeMX** for STM32
- **ESP-IDF menuconfig** for ESP32
- **MCUXpresso Config Tools** for NXP
- Platform configurators for other vendors

### Build System Integration

**CMake Example:**
```cmake
# Add FreeRTOS port
target_sources(your_app PRIVATE
    ${UCXCLIENT_DIR}/examples/port/u_port_freertos.c
)

target_compile_definitions(your_app PRIVATE
    U_PORT_FREERTOS
    U_PORT_STM32_HAL
    STM32F4
)
```

**Makefile Example:**
```makefile
CFLAGS += -DU_PORT_FREERTOS -DU_PORT_STM32_HAL -DSTM32F4
SOURCES += $(UCXCLIENT_DIR)/examples/port/u_port_freertos.c
```
