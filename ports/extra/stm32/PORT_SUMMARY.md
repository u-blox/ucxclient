# STM32F407VG Port Summary

This document summarizes the STM32F407VG port implementation for ucxclient.

## Overview

The STM32F407VG port provides a complete FreeRTOS-based implementation for running ucxclient on ARM Cortex-M4 microcontrollers. It includes:

- **FreeRTOS OS port** (`ports/os/u_port_freertos.*`)
- **STM32F4 UART port** (`ports/uart/u_port_uart_stm32f4.*`)
- **Build system integration** (CMake toolchain and configuration)
- **Example application** (HTTP GET request demo)

## Quick Start

### Building with Docker

```bash
# From project root
cd docker
./build_stm32.sh

# With custom WiFi credentials
WIFI_SSID="MyNetwork" WIFI_PSK="MyPassword" ./build_stm32.sh
```

### Flashing

```bash
st-flash write stm32/build_stm32/http_example_stm32.bin 0x08000000
```

## Port Layer Implementation

### FreeRTOS OS Port

**Files**: `ports/os/u_port_freertos.{h,c}`

**Features**:
- Mutex support using FreeRTOS semaphores
- Background RX task for automatic UART handling
- Tick-based timing (1ms resolution)
- Configurable task priority and stack size

**Key Functions**:
- `uPortInit()` / `uPortDeinit()` - Port initialization
- `uPortGetTickTimeMs()` - Millisecond timer
- `uPortMutexTryLock()` - Mutex with timeout
- `uPortBgRxTaskCreate()` - Creates RX polling task
- `uPortBgRxTaskDestroy()` - Cleans up RX task

**Configuration**:
```c
#define U_PORT_FREERTOS_RX_TASK_STACK_SIZE    2048  // in words
#define U_PORT_FREERTOS_RX_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
```

### STM32F4 UART Port

**Files**: `ports/uart/u_port_uart_stm32f4.{h,c}`

**Features**:
- Interrupt-driven RX with circular buffer (2048 bytes)
- Blocking TX using HAL
- Hardware flow control (RTS/CTS) support
- Configurable UART instance (USART1-6, UART4-5)
- Thread-safe buffer operations

**Key Functions**:
- `uPortUartOpen()` - Open and configure UART
- `uPortUartClose()` - Close UART
- `uPortUartWrite()` - Blocking write
- `uPortUartRead()` - Non-blocking/blocking read with timeout

**Configuration**:
```c
#define U_PORT_UART_USE_USART2           // Select UART instance
#define U_PORT_UART_RX_BUFFER_SIZE 2048  // RX buffer size
```

## Build System

### CMake Files

1. **cmake/arm-none-eabi-gcc.cmake** - ARM GCC toolchain definition
2. **cmake/stm32f407vg.cmake** - STM32F407VG specific settings (CPU flags, HAL paths)
3. **stm32/CMakeLists.txt** - Main build configuration

### Key Settings

**CPU Flags**:
```cmake
-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard
```

**Linker Script**: `stm32/STM32F407VGTx_FLASH.ld`
- Flash: 1024 KB @ 0x08000000
- RAM: 128 KB @ 0x20000000
- CCM RAM: 64 KB @ 0x10000000

## Hardware Configuration

### Default UART Pins (USART2)

| Pin  | Function | u-blox Module |
|------|----------|---------------|
| PA2  | TX       | RX            |
| PA3  | RX       | TX            |
| PA0  | CTS      | RTS (optional)|
| PA1  | RTS      | CTS (optional)|

### System Clock

- **External oscillator**: 8 MHz HSE
- **System clock**: 168 MHz (PLL configured in `system_stm32f4xx.c`)
- **AHB**: 168 MHz
- **APB1**: 42 MHz (UART2-5)
- **APB2**: 84 MHz (USART1, USART6)

## Memory Usage

Typical for HTTP example:

| Section | Size      | Location   |
|---------|-----------|------------|
| Flash   | ~120 KB   | 0x08000000 |
| RAM     | ~25 KB    | 0x20000000 |
| Heap    | 32 KB     | Configurable |
| Stack   | 1 KB      | Per task |

## Example Application

**File**: `stm32/main_stm32.c`

Creates a FreeRTOS task that:
1. Initializes port layer
2. Configures WiFi credentials
3. Calls `http_example_main()` from `examples/http_example.c`
4. Performs HTTP GET request
5. Prints response

**WiFi Configuration**:
Edit in `main_stm32.c`:
```c
char *argv[] = {
    "http_example",
    NULL,
    "YourSSID",     // Change this
    "YourPassword"  // Change this
};
```

## Integration Points

### Adding New Examples

1. Create `main_<example>.c` similar to `main_stm32.c`
2. Define `<example>_main()` function in example source
3. Add build target in `stm32/CMakeLists.txt`

### Using Different UART

1. Define `U_PORT_UART_USE_USARTx` in CMakeLists.txt
2. Update GPIO configuration in main file
3. Ensure correct interrupt handler in `stm32f4xx_it.c`

### Porting to Other STM32

1. Create new `cmake/stm32xxxxx.cmake` with CPU flags
2. Update linker script for memory layout
3. Adjust HAL paths for STM32 family
4. Modify startup file reference

## Testing

Build and test on STM32F407VG Discovery board:

```bash
# Build
cd docker
./build_stm32.sh

# Flash
st-flash write ../stm32/build_stm32/http_example_stm32.bin 0x08000000

# Monitor (optional - requires debug UART setup)
minicom -D /dev/ttyUSB0 -b 115200
```

## Dependencies

**External Libraries** (fetched by Docker):
- STM32CubeF4 (HAL + CMSIS)
- FreeRTOS Kernel

**Build Tools** (in Docker image):
- gcc-arm-none-eabi
- cmake
- make

## Supported Features

✅ Full AT client support
✅ Background RX task
✅ Hardware flow control
✅ Multiple UART instances
✅ FreeRTOS integration
✅ Interrupt-driven RX
✅ All ucxclient examples compatible

## Limitations

- Single UART instance at runtime (only one `uPortUartOpen()` call)
- No DMA support (yet - can be added)
- Printf redirected to null (stub implementation)

## Future Enhancements

- DMA support for UART TX/RX
- Low-power modes integration
- Multiple UART instances
- UART over USB CDC
- STM32CubeMX project file

## References

- [STM32F407VG Datasheet](https://www.st.com/resource/en/datasheet/stm32f407vg.pdf)
- [FreeRTOS Documentation](https://www.freertos.org/Documentation/RTOS_book.html)
- [STM32CubeF4 GitHub](https://github.com/STMicroelectronics/STM32CubeF4)
- [Port Layer Documentation](../ports/README.md)
