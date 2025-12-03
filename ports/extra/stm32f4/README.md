# STM32F4 Example Implementation

This directory contains the STM32F407 example application and build infrastructure for running ucxclient with FreeRTOS.

The actual port layer implementations are:

- `ports/os/u_port_freertos.c` - FreeRTOS OS abstraction (mutex, time, background RX task)
- `ports/uart/u_port_uart_stm32f4.c` - STM32F4 UART driver

## What This Directory Provides

This directory provides the complete build infrastructure and application code needed to run ucxclient examples on STM32F407 hardware:

- Example application entry point with FreeRTOS task setup
- STM32 HAL initialization and interrupt handlers
- CMake build system integration
- Hardware startup code and linker scripts
- Renode emulation support for development without hardware

## Directory Structure

```text
stm32f4/
├── src/                   # Source files (.c, .h, .s)
│   ├── main_stm32.c      # Application entry point
│   ├── u_port_*.c/h      # Platform abstraction implementations
│   ├── system_*.c        # STM32 HAL system files
│   └── startup_*.s       # ARM Cortex-M4 startup code
├── scripts/               # Build and debug scripts
│   ├── *.resc            # Renode emulation scripts
│   ├── *.ld              # Linker scripts
│   └── *.sh              # Setup scripts
└── README.md             # This file
```

## Supported Hardware

- **Target MCU**: STM32F407IGHx (Cortex-M4F @ 168 MHz)
- **Development Board**: STM32F407G-DISC1 (or compatible)
- **RTOS**: FreeRTOS v10.3.1
- **HAL**: STM32Cube HAL drivers

### UART Pin Configuration (STM32F407G-DISC1)

The default configuration uses **USART3** for communication with the u-blox module:

| Signal | Pin  | Notes |
|--------|------|-------|
| TX     | PB10 | USART3_TX |
| RX     | PB11 | USART3_RX |
| CTS    | PB13 | USART3_CTS (hardware flow control) |
| RTS    | PB14 | USART3_RTS (hardware flow control) |

**Console UART** (debug output) uses USART2:
- TX: PA2, RX: PA3 (no flow control)

The port automatically configures CTS/RTS pins when hardware flow control is enabled via the `useFlowControl` parameter in `uPortUartOpen()`.

## Building Examples

```bash
cd examples
inv stm32.http    # Build HTTP example for STM32F4
```

### WiFi Credentials

You can embed WiFi credentials directly into the binary at build time:

```bash
inv stm32.http --wifi-ssid="YourNetwork" --wifi-psk="YourPassword"
```

Without these parameters, the binary will have empty credentials and display a warning at runtime.

The build produces:

- `examples/bin/http_example_stm32.elf` - Executable
- `examples/bin/http_example_stm32.hex` - Flash image
- `examples/bin/http_example_stm32.bin` - Binary image

## Running and Debugging

### Emulation with Renode

```bash
cd examples
inv stm32.renode --example http
inv stm32.uart-console              # In another terminal
```

Or use VS Code debug configuration: **"STM32 HTTP (Renode GDB)"**

### Physical Hardware

Connect STM32F407G-DISC1 via ST-Link and use VS Code debug configuration: **"STM32 HTTP (Hardware ST-Link)"**

## Integration

This port is integrated into the ucxclient build system:

- Build via `-DBUILD_STM32_EXAMPLES=ON` in CMake
- Configured in `cmake/stm32.cmake` and `cmake/stm32f407vg.cmake`
- Examples in `examples/` use `add_stm32_example()` function

## More Information

See the main [ports README](../../README.md) for general port layer documentation.
