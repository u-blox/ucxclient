# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

All examples are designed to work with both OS and no-OS configurations by using the shared utilities in `example_utils.c/h`. This demonstrates the portability of ucxclient across different execution environments.

| Files               | Description |
| ------------------- | ----------- |
| http_example.c      | Example of doing a HTTP GET request using the uCx API. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| fw_upgrade_example.c | Example of performing firmware upgrade using AT+USYFWUS command and XMODEM protocol. This example can be compiled for both OS (POSIX) and no-OS (bare-metal) configurations. |
| example_utils.c/h   | Common utility functions that work with both OS and no-OS configurations, providing AT client initialization, event handling, and sleep functionality. |

## Building

To build all the examples on a Linux based system, just execute the following in repo root if you prefer Makefile:

```sh
> cmake -S . -B build -G "Unix Makefiles"
> make -C build
```

or if you prefer Ninja:

```sh
> cmake -S . -B build -G "Ninja"
> ninja -C build
```

## Running

### http_example

If you have built http_example using the steps above it will be built for Linux and located in `build/http_example`.
To start it you will need to pass some arguments:

```
http_example <device> <SSID> <WPA_PSK>
  device:  the UART device that is connected to a u-connectXpress module
  SSID:    the Wi-Fi SSID to connect to
  WPA_PSK: the Wi-Fi WPA PSK
```

Example:

```sh
> build/http_example /dev/ttyUSB0 MySSID MyWiFiPasswd
```

### http_example_no_os

The no-OS variant of http_example is built from the same source code but uses a different port layer (u_port_no_os.c). The UART device, Wi-Fi SSID and PSK are configured using CMake defines since command-line arguments are not available in typical bare-metal environments.

To set these defines using CMake you can either use `cmake-gui`:

![cmake-gui](/images/cmake-gui.png)

or from command line:

```sh
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_SSID="MySSID" -D U_EXAMPLE_WPA_PSK="MyWiFiPasswd"
```

Now you should be able to start the example using:

```sh
> build/http_example_no_os
```

Note: Both http_example and http_example_no_os are compiled from the same http_example.c source file, demonstrating how ucxclient examples can work seamlessly in both OS and no-OS environments.

### fw_upgrade_example

This example demonstrates upgrading module firmware using the AT+USYFWUS command followed by XMODEM protocol transfer.

To run the firmware upgrade example:

```sh
> build/fw_upgrade_example <device> <firmware_file>
```

Example:

```sh
> build/fw_upgrade_example /dev/ttyUSB0 NORA-W36X-SW-1.0.0.bin
```

The example will:
1. Check communication with the module
2. Issue the AT+USYFWUS command to enter bootloader mode
3. Transfer the firmware file using XMODEM protocol at 921600 baud
4. Wait for the module to reboot
5. Confirm the module is back online

### fw_upgrade_example_no_os

The no-OS variant of fw_upgrade_example is built from the same source code but uses a different port layer (u_port_no_os.c). The UART device and firmware file path are configured using CMake defines.

To set these defines using CMake:

```sh
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_FW_FILE="NORA-W36X-SW-1.0.0.bin"
```

Then run:

```sh
> build/fw_upgrade_example_no_os
```

Note: Both fw_upgrade_example and fw_upgrade_example_no_os are compiled from the same fw_upgrade_example.c source file.
