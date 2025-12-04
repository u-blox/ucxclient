# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](../ports/README.md).

All examples are designed to work with both OS and no-OS configurations by using the shared utilities in `example_utils.c/h`. This demonstrates the portability of ucxclient across different execution environments.

| Files               | Description |
| ------------------- | ----------- |
| http_example.c      | Example of doing a HTTP GET request using the uCx API. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| fw_upgrade_example.c | Example of performing firmware upgrade using AT+USYFWUS command and XMODEM protocol. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| ble_scan_example.c  | Example of performing a BLE device discovery scan. Demonstrates how to start/stop BLE scanning and handle scan result URCs. |
| ble_advertise_example.c | Example of BLE advertising with custom advertising data. Demonstrates how to configure and start BLE advertising. |
| wifi_scan_example.c | Example of scanning for nearby WiFi access points. Lists available networks with signal strength and security information. |
| wifi_ap_example.c   | Example of starting a WiFi access point (AP mode). Demonstrates AP configuration and handling station connect/disconnect events via URCs. |
| socket_example.c    | Example of raw TCP socket communication. Demonstrates socket creation, connection, data transfer, and URC-driven event handling. |
| example_utils.c/h   | Common utility functions that work with both OS and no-OS configurations, providing AT client initialization, event handling, and sleep functionality. |

## Configuration

All configuration is done via `config.local.h`. Copy the template and edit it:

```sh
cp config.local.h.template config.local.h
```

Then edit the macros:

```c
#define U_EXAMPLE_SSID "your-wifi-ssid"
#define U_EXAMPLE_WPA_PSK "your-wifi-password"
// Uncomment if you need a different UART:
// #define U_EXAMPLE_UART "/dev/ttyUSB1"
```

This file is `.gitignore`d, so your credentials stay out of commits. All examples automatically include this file when it exists.

## Building

The examples use [PyInvoke](https://www.pyinvoke.org/) for build automation. Install it with:

```sh
> pip install invoke
```

### Available platforms

Build tasks are organized by platform namespace:

| Namespace | Platform | Host requirement |
| --------- | -------- | ---------------- |
| `linux.*` | Linux (POSIX) | Linux host |
| `win32.*` | Windows | Windows host |
| `stm32.*` | STM32F4 (FreeRTOS) | ARM toolchain or `--docker` flag |

### Listing available tasks

To see all available build tasks, run from the `examples/` directory:

```sh
> invoke --list
Available tasks:

  linux.all             Build all examples for Linux (only available on Linux host).
  linux.ble-advertise   Build ble_advertise_example for Linux (only available on Linux host).
  linux.ble-scan        Build ble_scan_example for Linux (only available on Linux host).
```

To get help for a specific task:

```sh
> invoke --help stm32.http
Usage: inv[oke] [--core-opts] stm32.http [--options] [other tasks here ...]

Docstring:
  Build http_example for STM32.

  WiFi credentials are configured in config.local.h.
  For CI, run 'inv generate-config' before building to create the config file.

Options:
  -c, --clean                     Clean build directory before building
  -d, --docker                    Build inside Docker container (no local ARM toolchain required)
  -j INT, --jobs=INT              Number of parallel jobs (default: CPU cores)
```

### Selecting a target module

Use `invoke ucx-module` to select the target u-connectXpress module. The selection persists across builds:

```sh
invoke ucx-module                      # Show current module and available options
invoke ucx-module --set NORA-W36X      # Set module for all subsequent builds
```

### Building examples

Build from the `examples/` directory using `invoke <platform>.<example>`:

```sh
invoke linux.http                   # Build http_example for Linux
invoke stm32.http --docker          # Build http_example for STM32 using Docker
invoke stm32.all --docker           # Build all STM32 examples using Docker
```

Or from the project root (prefix with `examples.`):

```sh
invoke examples.linux.http
```

Executables are placed in `examples/bin/`.

### Manual CMake Build

If you prefer to use CMake directly:

```sh
# From examples/ directory
> cmake -S . -B build
> cmake --build build

# Build for a specific module
> cmake -S . -B build -D UCX_MODULE=NORA-W36X
> cmake --build build
```

The `UCX_MODULE` variable selects the module-specific generated API from `ucx_api/generated/<module>/`. If not specified, it defaults to `NORA-W36X`.

## Running

### http_example

After building, the executable is located in `bin/http_example`.
All arguments are optional and default to values from `config.local.h`:

```text
> http_example [device] [SSID] [WPA_PSK]
  device:  the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
  SSID:    the Wi-Fi SSID to connect to (default: U_EXAMPLE_SSID)
  WPA_PSK: the Wi-Fi WPA PSK (default: U_EXAMPLE_WPA_PSK)
```

Example:

```sh
> bin/http_example                                         # Use all defaults from config.local.h
> bin/http_example /dev/ttyUSB0                            # Override device only
> bin/http_example /dev/ttyUSB0 MySSID MyWiFiPasswd        # Override all
```

### http_example_no_os

The no-OS variant of http_example is built from the same source code but uses a different port layer (u_port_no_os.c). Because command-line arguments are typically unavailable on bare-metal targets, the UART device, Wi-Fi SSID and PSK are read from compile-time macros defined in `config.local.h`.

To configure, copy `config.local.h.template` to `config.local.h` and edit the values:

```c
#define U_EXAMPLE_SSID "your-wifi-ssid"
#define U_EXAMPLE_WPA_PSK "your-wifi-password"
```

Then build and run:

```sh
> cmake -S . -B build
> cmake --build build
> bin/http_example_no_os
```

Note: Both http_example and http_example_no_os are compiled from the same http_example.c source file, demonstrating how ucxclient examples can work seamlessly in both OS and no-OS environments.

### fw_upgrade_example

This example demonstrates upgrading module firmware using the AT+USYFWUS command followed by XMODEM protocol transfer.

To run the firmware upgrade example:

```text
> fw_upgrade_example <firmware_file> [device]
  firmware_file: path to the firmware binary file (required)
  device:        the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
```

Example:

```sh
> bin/fw_upgrade_example NORA-W36X-SW-1.0.0.bin                    # Use default device
> bin/fw_upgrade_example NORA-W36X-SW-1.0.0.bin /dev/ttyUSB0       # Specify device
```

The example will:

1. Check communication with the module
2. Issue the AT+USYFWUS command to enter bootloader mode
3. Transfer the firmware file using XMODEM protocol at 921600 baud
4. Wait for the module to reboot
5. Confirm the module is back online

### fw_upgrade_example_no_os

The no-OS variant of fw_upgrade_example is built from the same source code but uses a different port layer (u_port_no_os.c). The UART device and firmware file path are configured via compile-time macros. Define them in `config.local.h` for a persistent setup, or provide them through CMake when configuring the build.

To pass the values through CMake:

```sh
> cd examples
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_FW_FILE="NORA-W36X-SW-1.0.0.bin"
> invoke all
```

Then run:

```sh
> bin/fw_upgrade_example_no_os
```

Note: Both fw_upgrade_example and fw_upgrade_example_no_os are compiled from the same fw_upgrade_example.c source file.

### ble_scan_example

This example demonstrates BLE device discovery scanning.

To run the BLE scan example:

```text
> ble_scan_example [device]
  device: the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
```

Example:

```sh
> bin/ble_scan_example                      # Use default device from config.local.h
> bin/ble_scan_example /dev/ttyUSB0         # Specify device
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Query and display the module's Bluetooth MAC address
3. Start BLE scanning
4. Display discovered devices in real-time via URC callbacks
5. Stop scanning after the specified duration

### ble_advertise_example

This example demonstrates BLE advertising with custom advertising data.

To run the BLE advertise example:

```text
> ble_advertise_example [device] [device_name]
  device:      the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
  device_name: the BLE device name to advertise (default: "uCxExample")
```

Example:

```sh
> bin/ble_advertise_example                         # Use all defaults
> bin/ble_advertise_example /dev/ttyUSB0            # Override device only
> bin/ble_advertise_example /dev/ttyUSB0 MyDevice   # Override device and name
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Query and display the module's Bluetooth MAC address
3. Set custom advertising data (Flags + Complete Local Name "ucxclient")
4. Start BLE advertising
5. Stop advertising after the specified duration

### wifi_scan_example

This example demonstrates scanning for nearby WiFi access points.

To run the WiFi scan example:

```text
> wifi_scan_example [device]
  device: the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
```

Example:

```sh
> bin/wifi_scan_example                     # Use default device from config.local.h
> bin/wifi_scan_example /dev/ttyUSB0        # Specify device
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Scan for available WiFi networks
3. Display each network's BSSID, SSID, channel, RSSI, and security type
4. Show a summary of total networks found

### wifi_ap_example

This example demonstrates starting a WiFi access point (AP mode).

To run the WiFi AP example:

```text
> wifi_ap_example [device] [SSID] [password]
  device:   the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
  SSID:     the AP network name (default: U_EXAMPLE_SSID)
  password: the AP password (default: U_EXAMPLE_WPA_PSK, open network if < 8 chars)
```

Example:

```sh
> bin/wifi_ap_example                                       # Use all defaults from config.local.h
> bin/wifi_ap_example /dev/ttyUSB0                          # Override device only
> bin/wifi_ap_example /dev/ttyUSB0 MyAP MyPassword123       # Override all
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Configure and start the WiFi access point
3. Display the AP's IP address
4. Monitor and display station connect/disconnect events via URCs
5. Run for 60 seconds then stop the AP

### socket_example

This example demonstrates raw TCP socket communication.

To run the socket example:

```text
> socket_example [device] [SSID] [WPA_PSK]
  device:  the UART device connected to a u-connectXpress module (default: U_EXAMPLE_UART)
  SSID:    the Wi-Fi SSID to connect to (default: U_EXAMPLE_SSID)
  WPA_PSK: the Wi-Fi WPA PSK (default: U_EXAMPLE_WPA_PSK)
```

Example:

```sh
> bin/socket_example                                        # Use all defaults from config.local.h
> bin/socket_example /dev/ttyUSB0 MySSID MyWiFiPasswd       # Override all
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Connect to the specified WiFi network
3. Create a TCP socket and connect to httpbin.org:80
4. Send an HTTP GET request
5. Receive and display the response using URC-driven data availability events
6. Close the socket and disconnect from WiFi
