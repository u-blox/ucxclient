# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

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

## Local configuration overrides

Examples automatically include `config.local.h` when it exists in this directory. Copy `config.local.h.template` to `config.local.h` and edit the macros (for instance `U_EXAMPLE_SSID` / `U_EXAMPLE_WPA_PSK`) to store credentials locally without passing them via command-line arguments or CMake definitions. The file is `.gitignore`d so sensitive data stays out of commits, and any macros you place there take precedence over the built-in defaults.

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
  Build http_example for STM32 with optional WiFi credentials.

  WiFi credentials can be provided via --wifi-ssid/--wifi-psk arguments
  or via WIFI_SSID/WIFI_PSK environment variables (useful in CI).

Options:
  -c, --clean                     Clean build directory before building
  -d, --docker                    Build inside Docker container (no local ARM toolchain required)
  -i STRING, --wifi-psk=STRING    WiFi WPA-PSK password to embed in the binary (or set WIFI_PSK env var)
  -w STRING, --wifi-ssid=STRING   WiFi SSID to embed in the binary (or set WIFI_SSID env var)
```

### Building examples

Build from the `examples/` directory using `invoke <platform>.<example>`:

```sh
invoke linux.http          # Build http_example for Linux
invoke stm32.http --docker # Build http_example for STM32 using Docker
invoke stm32.all --docker  # Build all STM32 examples using Docker
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
```

## Running

### http_example

After building, the executable is located in `bin/http_example`.
To start it you will need to pass some arguments:

```text
> http_example <device> <SSID> <WPA_PSK>
  device:  the UART device that is connected to a u-connectXpress module
  SSID:    the Wi-Fi SSID to connect to
  WPA_PSK: the Wi-Fi WPA PSK
```

Example:

```sh
> bin/http_example /dev/ttyUSB0 MySSID MyWiFiPasswd
```

### http_example_no_os

The no-OS variant of http_example is built from the same source code but uses a different port layer (u_port_no_os.c). Because command-line arguments are typically unavailable on bare-metal targets, the UART device, Wi-Fi SSID and PSK must be provided as compile-time macros. You can define them once in `config.local.h`, or pass them through CMake when configuring the build.

To use CMake defines, you can either use `cmake-gui`:

![cmake-gui](/images/cmake-gui.png)

or configure from command line:

```sh
> cd examples
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_SSID="MySSID" -D U_EXAMPLE_WPA_PSK="MyWiFiPasswd"
> cmake --build build
```

When relying on `config.local.h`, skip the `-D` arguments—the copied file is included automatically and will provide the macro definitions for every future build.

Then run:

```sh
> bin/http_example_no_os
```

Note: Both http_example and http_example_no_os are compiled from the same http_example.c source file, demonstrating how ucxclient examples can work seamlessly in both OS and no-OS environments.

### fw_upgrade_example

This example demonstrates upgrading module firmware using the AT+USYFWUS command followed by XMODEM protocol transfer.

To run the firmware upgrade example:

```sh
> bin/fw_upgrade_example <device> <firmware_file>
```

Example:

```sh
> bin/fw_upgrade_example /dev/ttyUSB0 NORA-W36X-SW-1.0.0.bin
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

```sh
> bin/ble_scan_example <device> [scan_duration_seconds]
```

Example:

```sh
> bin/ble_scan_example /dev/ttyUSB0        # Scan for 10 seconds (default)
> bin/ble_scan_example /dev/ttyUSB0 30     # Scan for 30 seconds
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

```sh
> bin/ble_advertise_example <device> [advertise_duration_seconds]
```

Example:

```sh
> bin/ble_advertise_example /dev/ttyUSB0      # Advertise for 30 seconds (default)
> bin/ble_advertise_example /dev/ttyUSB0 60   # Advertise for 60 seconds
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

```sh
> bin/wifi_scan_example <device>
```

Example:

```sh
> bin/wifi_scan_example /dev/ttyUSB0
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Scan for available WiFi networks
3. Display each network's BSSID, SSID, channel, RSSI, and security type
4. Show a summary of total networks found

### wifi_ap_example

This example demonstrates starting a WiFi access point (AP mode).

To run the WiFi AP example:

```sh
> bin/wifi_ap_example <device> [SSID] [password]
```

Example:

```sh
> bin/wifi_ap_example /dev/ttyUSB0                           # Use default SSID "ucxclient_ap" (open)
> bin/wifi_ap_example /dev/ttyUSB0 MyAP                      # Custom SSID (open network)
> bin/wifi_ap_example /dev/ttyUSB0 MyAP MyPassword123        # Custom SSID with WPA2 password
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

```sh
> bin/socket_example <device> <SSID> <WPA_PSK>
```

Example:

```sh
> bin/socket_example /dev/ttyUSB0 MySSID MyWiFiPasswd
```

The example will:

1. Initialize the AT client and u-connectXpress API
2. Connect to the specified WiFi network
3. Create a TCP socket and connect to httpbin.org:80
4. Send an HTTP GET request
5. Receive and display the response using URC-driven data availability events
6. Close the socket and disconnect from WiFi
