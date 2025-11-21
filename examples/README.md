# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

All examples are designed to work with both OS and no-OS configurations by using the shared utilities in `example_utils.c/h`. This demonstrates the portability of ucxclient across different execution environments.

| Files               | Description |
| ------------------- | ----------- |
| http_example.c      | Example of doing a HTTP GET request using the uCx API. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| fw_upgrade_example.c | Example of performing firmware upgrade using AT+USYFWUS command and XMODEM protocol. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| example_utils.c/h   | Common utility functions that work with both OS and no-OS configurations, providing AT client initialization, event handling, and sleep functionality. |

## Building

The examples use [PyInvoke](https://www.pyinvoke.org/) for build automation. Install it with:

```sh
pip install invoke
```

Then build from the `examples/` directory:

```sh
invoke all              # Build all examples
invoke http             # Build http_example only
invoke fw-upgrade       # Build fw_upgrade_example only
invoke clean            # Clean build artifacts
invoke all --clean      # Clean and rebuild
```

Or from the project root:

```sh
invoke build.examples
```

Executables are placed in `examples/bin/`.

### Manual CMake Build

If you prefer to use CMake directly:

```sh
# From examples/ directory
cmake -S . -B build
cmake --build build
```

## Running

### http_example

After building, the executable is located in `bin/http_example`.
To start it you will need to pass some arguments:

```
http_example <device> <SSID> <WPA_PSK>
  device:  the UART device that is connected to a u-connectXpress module
  SSID:    the Wi-Fi SSID to connect to
  WPA_PSK: the Wi-Fi WPA PSK
```

Example:

```sh
bin/http_example /dev/ttyUSB0 MySSID MyWiFiPasswd
```

### http_example_no_os

The no-OS variant of http_example is built from the same source code but uses a different port layer (u_port_no_os.c). The UART device, Wi-Fi SSID and PSK are configured using CMake defines since command-line arguments are not available in typical bare-metal environments.

To set these defines, you can either use `cmake-gui`:

![cmake-gui](/images/cmake-gui.png)

or configure from command line:

```sh
cd examples
cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_SSID="MySSID" -D U_EXAMPLE_WPA_PSK="MyWiFiPasswd"
invoke all
```

Then run:

```sh
bin/http_example_no_os
```

Note: Both http_example and http_example_no_os are compiled from the same http_example.c source file, demonstrating how ucxclient examples can work seamlessly in both OS and no-OS environments.

### fw_upgrade_example

This example demonstrates upgrading module firmware using the AT+USYFWUS command followed by XMODEM protocol transfer.

To run the firmware upgrade example:

```sh
bin/fw_upgrade_example <device> <firmware_file>
```

Example:

```sh
bin/fw_upgrade_example /dev/ttyUSB0 NORA-W36X-SW-1.0.0.bin
```

The example will:
1. Check communication with the module
2. Issue the AT+USYFWUS command to enter bootloader mode
3. Transfer the firmware file using XMODEM protocol at 921600 baud
4. Wait for the module to reboot
5. Confirm the module is back online

### fw_upgrade_example_no_os

The no-OS variant of fw_upgrade_example is built from the same source code but uses a different port layer (u_port_no_os.c). The UART device and firmware file path are configured using CMake defines.

To set these defines:

```sh
cd examples
cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_FW_FILE="NORA-W36X-SW-1.0.0.bin"
invoke all
```

Then run:

```sh
bin/fw_upgrade_example_no_os
```

Note: Both fw_upgrade_example and fw_upgrade_example_no_os are compiled from the same fw_upgrade_example.c source file.
