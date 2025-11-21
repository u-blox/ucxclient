# Building ucxclient for STM32F407VG

This guide explains how to build and run the HTTP example on STM32F407VG microcontroller with FreeRTOS.

## Prerequisites

The Docker image you built includes all necessary tools:
- `gcc-arm-none-eabi` - ARM cross-compiler
- `cmake` - Build system
- STM32CubeF4 HAL library
- FreeRTOS kernel

## Project Structure

```
ucxclient/
├── stm32/                      # STM32-specific files
│   ├── CMakeLists.txt         # STM32 build configuration
│   ├── build.sh               # Build script
│   ├── main_stm32.c           # Main entry point
│   ├── system_stm32f4xx.c     # System initialization
│   ├── stm32f4xx_hal_conf.h   # HAL configuration
│   ├── stm32f4xx_it.c         # Interrupt handlers
│   ├── FreeRTOSConfig.h       # FreeRTOS configuration
│   └── STM32F407VGTx_FLASH.ld # Linker script
├── ports/
│   ├── os/u_port_freertos.*   # FreeRTOS OS port
│   └── uart/u_port_uart_stm32f4.* # STM32 UART port
├── cmake/
│   ├── arm-none-eabi-gcc.cmake # ARM toolchain file
│   └── stm32f407vg.cmake      # STM32F407VG settings
└── examples/
    └── http_example.c         # HTTP example (works on all platforms)
```

## Building with Docker

### 1. Quick build using the provided script

```bash
cd docker
./build_stm32.sh
```

### 2. With custom WiFi credentials

Set WiFi credentials as environment variables:

```bash
cd docker
WIFI_SSID="YourWiFiSSID" WIFI_PSK="YourWiFiPassword" ./build_stm32.sh
```

### 3. Using Docker Compose directly

```bash
cd docker

# Build the image
docker compose build

# Run the build
docker compose run --rm stm32f4-builder bash -c "cd stm32 && ./build.sh"

# Or with WiFi credentials
WIFI_SSID="MyNetwork" WIFI_PSK="MyPassword" \
  docker compose run --rm stm32f4-builder bash -c "cd stm32 && ./build.sh"
```

### 4. Interactive container session

```bash
cd docker
docker compose run --rm stm32f4-builder bash

# Inside container:
cd stm32
./build.sh
```

Or edit `stm32/main_stm32.c` directly:

```c
char *argv[] = {
    "http_example",
    NULL,
    "YourActualSSID",      // Change this
    "YourActualPassword"   // Change this
};
```

## Build Output

After successful build, you'll find in `stm32/build_stm32/`:

- `http_example_stm32.elf` - ELF file for debugging
- `http_example_stm32.hex` - Intel HEX format
- `http_example_stm32.bin` - Binary file for flashing
- `http_example_stm32.map` - Memory map

## Flashing to STM32F407VG

### Using st-flash (st-link tools)

```bash
st-flash write stm32/build_stm32/http_example_stm32.bin 0x08000000
```

### Using OpenOCD

```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
  -c "program stm32/build_stm32/http_example_stm32.elf verify reset exit"
```

### Using STM32CubeProgrammer

1. Open STM32CubeProgrammer
2. Connect to ST-LINK
3. Load `http_example_stm32.hex` or `.bin` file
4. Program and verify

## Hardware Setup

### UART Connection (USART2)

Connect your u-blox module to STM32F407VG USART2:

| STM32F407VG | u-blox Module | Description |
|-------------|---------------|-------------|
| PA2         | RX            | UART TX     |
| PA3         | TX            | UART RX     |
| GND         | GND           | Ground      |
| PA0*        | CTS           | Flow Control (optional) |
| PA1*        | RTS           | Flow Control (optional) |

*Only needed if hardware flow control is enabled

### Power Supply

- Ensure both devices share common ground
- Power the u-blox module appropriately (usually 3.3V or separate supply)
- STM32F407VG runs at 3.3V logic level

### Using Different UART

To use a different UART (e.g., USART1), modify:

1. **stm32/CMakeLists.txt**:
   ```cmake
   target_compile_definitions(http_example_stm32.elf PRIVATE
       U_PORT_UART_USE_USART1  # Change this
       ...
   )
   ```

2. **stm32/main_stm32.c** - Update `GPIO_Init()` with correct pins

## Debugging

### Serial Debug Output

By default, `printf()` is stubbed out. To enable debug output via SWO or another UART:

1. Implement `_write()` in `stm32/main_stm32.c`
2. Configure SWO in your debugger
3. Or redirect to another UART instance

### Using GDB with OpenOCD

```bash
# Terminal 1: Start OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg

# Terminal 2: Start GDB
arm-none-eabi-gdb stm32/build_stm32/http_example_stm32.elf
(gdb) target remote localhost:3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

### Common Issues

**Build fails with missing headers**
- Ensure Docker container has STM32CubeF4 and FreeRTOS in `/opt/`
- Check paths in `cmake/stm32f407vg.cmake`

**No UART communication**
- Verify GPIO pins are correctly configured
- Check baud rate matches module configuration (default: 115200)
- Ensure common ground connection
- Try disabling flow control first

**Hard fault on startup**
- Check stack size in `FreeRTOSConfig.h` (default: 32KB heap)
- Verify FreeRTOS task stack size in `main_stm32.c` (default: 4096 words)
- Enable stack overflow detection in `FreeRTOSConfig.h`

**Module not responding**
- Check module power supply
- Verify correct UART pins
- Try manual AT commands first: `AT\r\n`
- Increase RX task polling interval for debugging

## Customization

### Change System Clock

Edit `stm32/system_stm32f4xx.c`:
```c
#define PLL_M      8    // Input divider
#define PLL_N      336  // Multiplier
#define PLL_P      2    // Output divider
// Result: 8MHz * 336 / 8 / 2 = 168 MHz
```

### Adjust FreeRTOS Heap

Edit `stm32/FreeRTOSConfig.h`:
```c
#define configTOTAL_HEAP_SIZE  ( ( size_t ) ( 32 * 1024 ) )
```

### Change UART Buffer Size

Edit compile definitions in `stm32/CMakeLists.txt`:
```cmake
target_compile_definitions(http_example_stm32.elf PRIVATE
    U_PORT_UART_RX_BUFFER_SIZE=4096
)
```

### Enable More HAL Modules

Edit `stm32/stm32f4xx_hal_conf.h` to enable additional modules (SPI, I2C, etc.)

## Memory Usage

Typical memory usage for HTTP example:

- **Flash**: ~100-150 KB (depends on enabled features)
- **RAM**: ~20-30 KB (includes FreeRTOS heap, stacks, buffers)
- **Heap**: 32 KB (configurable)
- **Main task stack**: 16 KB (configurable)
- **RX task stack**: 8 KB (configurable)

Check actual usage after build:
```bash
arm-none-eabi-size stm32/build_stm32/http_example_stm32.elf
```

## Performance

At 168 MHz with optimized builds (`-O2`):
- UART interrupt latency: < 5 µs
- FreeRTOS task switch: ~1 µs
- HTTP request handling: ~50-200 ms (network dependent)

## Building Other Examples

The same approach works for other examples. For `fw_upgrade_example`:

1. Copy `stm32/main_stm32.c` to `main_fw_upgrade.c`
2. Change `http_example_main` to `fw_upgrade_example_main`
3. Add to `stm32/CMakeLists.txt`:
   ```cmake
   add_executable(fw_upgrade_stm32.elf
       main_fw_upgrade.c
       ../examples/fw_upgrade_example.c
       ../src/u_cx_xmodem.c
       ...
   )
   ```

## License

Copyright 2025 u-blox

Licensed under the Apache License, Version 2.0.

