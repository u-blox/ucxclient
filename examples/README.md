# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

| Files                | Description | Port |
| -------------------- | ----------- | ---- |
| http_example.c       | Example of doing a HTTP GET request using the uCx API. | Linux (POSIX) |
| http_example_no_os.c | Same example as http_example.c but illustrating how it could be done on a bare-metal system with no OS. | No OS (bare-metal) |
| windows_app.c        | Interactive Windows console application with menu-driven interface for Bluetooth, WiFi, Socket, and SPS operations. | Windows |
| windows_basic.c      | Simple Windows test application for basic AT command validation. | Windows |

## Available Ports

ucxclient includes several port implementations to support different platforms:

| Port | File | Platform | Description |
|------|------|----------|-------------|
| **POSIX** | `u_port_posix.*` | Linux, macOS | Uses POSIX threads and standard serial I/O |
| **Windows** | `u_port_windows.*` | Windows | Native Windows API with CreateFile/ReadFile/WriteFile |
| **No OS** | `u_port_no_os.*` | Bare-metal | Minimal port for systems without OS |
| **FreeRTOS** | `u_port_freertos.*` | Embedded | Complete port for FreeRTOS with 9 platform implementations |
| **Zephyr** | `u_port_zephyr.*` | Zephyr RTOS | Integration with Zephyr (see [zephyr/README.md](../zephyr/README.md)) |

For detailed port documentation, see [port/README.md](port/README.md).

## Building

### Linux (POSIX Port)

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

### Windows (Native Port)

#### Prerequisites
- Visual Studio 2019 or later (with C++ Desktop Development workload)
- CMake 3.15 or later

#### Build Steps

1. Open PowerShell or Command Prompt in the repository root

2. Generate Visual Studio solution:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022"
```
For Visual Studio 2019, use `"Visual Studio 16 2019"`

3. Build using CMake:
```powershell
cmake --build build --config Debug
```
or for Release:
```powershell
cmake --build build --config Release
```

4. Alternatively, open the generated solution in Visual Studio:
```powershell
start build\ucxclient.sln
```

#### Windows Executables

After building, you'll find:
- `build\Debug\windows_app.exe` - Interactive menu-driven application
- `build\Debug\windows_basic.exe` - Simple test application
- `build\Debug\http_example_windows.exe` - HTTP example for Windows
- `build\Debug\ucxclient_windows.dll` - Shared library

### FreeRTOS Port

The FreeRTOS port requires integration into your embedded project. See [port/README.md](port/README.md) for detailed setup instructions for:
- STM32 (all families)
- ESP32/ESP-IDF
- NXP MCUXpresso SDK
- Microchip/Atmel ASF
- Texas Instruments DriverLib
- Silicon Labs Gecko SDK
- Renesas FSP
- ARM Cortex-A with FreeRTOS+POSIX
- RISC-V

**Quick Start for STM32:**
```c
// In your CMakeLists.txt or Makefile:
// add_compile_definitions(U_PORT_FREERTOS U_PORT_STM32_HAL STM32F4)
// add_sources(${UCXCLIENT_ROOT}/examples/port/u_port_freertos.c)

#include "u_port.h"
#include "u_cx.h"

void ucxTask(void *pvParameters)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    
    uPortAtInit(&client);
    uPortAtOpen(&client, "UART1", 115200, true);
    uCxInit(&client, &ucxHandle);
    
    // Use the API...
    
    uPortAtClose(&client);
}
```

### Zephyr Port

See [zephyr/README.md](../zephyr/README.md) for complete Zephyr integration instructions.

## Running

### Linux: http_example

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

### Linux: http_example_no_os

Just like http_example this example will also be compiled for Linux, but in this case the UART device, Wi-Fi SSID and PSK are configured using defines.
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

### Windows: windows_app.exe

The Windows app is an interactive menu-driven application that provides easy access to all ucxclient features.

#### Starting the Application

**Auto-detection mode (recommended):**
```powershell
.\build\Debug\windows_app.exe
```
The app will scan for available COM ports and let you choose.

**Specify COM port:**
```powershell
.\build\Debug\windows_app.exe COM31
```

#### Features

The Windows app provides:

1. **Device Connection**
   - Auto-detect available COM ports
   - Connect/disconnect from u-connectXpress module
   - Saves last used COM port to `windows_app_settings.ini`

2. **Bluetooth Operations**
   - View Bluetooth status
   - Scan for nearby devices
   - Connect to Bluetooth devices
   - List active connections

3. **WiFi Operations**
   - View WiFi status and signal strength
   - Scan for WiFi networks
   - Connect to WiFi (credentials saved securely)
   - Disconnect from WiFi

4. **Socket Operations (TCP/UDP)**
   - Create TCP or UDP sockets
   - Connect to remote servers
   - Send and receive data
   - Event-driven data reception with URC handlers
   - List socket status
   - Close sockets

5. **SPS (Serial Port Service over Bluetooth)**
   - Enable SPS service
   - Connect SPS on Bluetooth connection
   - Send and receive data over SPS
   - Event-driven data reception

6. **API Command Reference**
   - Fetch latest API commands from GitHub
   - View all available AT commands and their UCX API equivalents
   - Offline static command list as fallback

7. **Debug Features**
   - Toggle AT command logging on/off
   - View raw AT traffic (commands and responses)
   - Real-time URC (Unsolicited Result Code) event monitoring

#### Settings Persistence

The app automatically saves:
- Last used COM port
- WiFi SSID and password (obfuscated)
- Last remote address/hostname

Settings are stored in `windows_app_settings.ini` in the same directory as the executable.

#### Example Session

```
========================================
  u-connectXpress ucxclient App v1.0.0
========================================

Attempting to connect to COM31...
COM port opened successfully
Disabling AT echo...
UCX initialized successfully
Connected successfully!

--- Main Menu ---
  Connected to: COM31
  UCX Logging: ENABLED

  [1] Connect to device
  [2] Disconnect
  [3] List API commands
  [4] AT test (basic communication)
  [5] ATI9 (device info)
  [6] Bluetooth menu
  [7] WiFi menu
  [8] Toggle UCX logging (AT traffic)
  [9] Socket menu (TCP/UDP)
  [a] SPS menu (Bluetooth Serial)
  [0] Exit

Choice: 7

--- WiFi Menu ---
  [1] Show WiFi status
  [2] Scan networks
  [3] Connect to network
  [4] Disconnect from network
  [0] Back to main menu

Choice: 3
```

### Windows: windows_basic.exe

Simple test application for basic validation:

```powershell
.\build\Debug\windows_basic.exe COM31
```

This runs a series of automated tests to verify:
- Device communication (AT command)
- Device information retrieval
- UCX API functionality

### Windows: http_example_windows.exe

HTTP GET example for Windows:

```powershell
.\build\Debug\http_example_windows.exe COM31 MySSID MyWiFiPassword
```

## Troubleshooting

### Windows

**COM Port Issues:**
- Use Device Manager to find the correct COM port number
- Ensure no other application is using the COM port
- Check that the u-connectXpress module is powered and connected

**Build Errors:**
- Ensure Visual Studio C++ Desktop Development workload is installed
- Try cleaning the build: `Remove-Item -Recurse -Force build; cmake -S . -B build`
- Check CMake version: `cmake --version` (should be 3.15+)

**Application Crashes:**
- Check that UCX logging is working: You should see AT traffic in the console
- Try toggling logging off/on from the menu
- Verify baud rate (default 115200) matches your module configuration

### Linux

**Permission Denied:**
```sh
sudo usermod -a -G dialout $USER
```
Then log out and log back in.

**Device Not Found:**
- Check device is connected: `ls -l /dev/ttyUSB*` or `ls -l /dev/ttyACM*`
- Verify permissions: `ls -l /dev/ttyUSB0`

### FreeRTOS/Embedded

**Compilation Errors:**
- Verify platform define is set correctly (e.g., `U_PORT_STM32_HAL`)
- Check that HAL/SDK includes are accessible
- Ensure `U_PORT_FREERTOS` is defined

**UART Not Working:**
- Verify UART initialization in your hardware setup code
- Check clock configuration for UART peripheral
- Confirm GPIO pins are configured correctly
- Verify baud rate matches module configuration (115200)

**Stack Overflow:**
- Increase FreeRTOS task stack size (recommended: 4096 bytes minimum)
- Check `configMINIMAL_STACK_SIZE` in FreeRTOSConfig.h

## Platform-Specific Notes

### STM32
- Use STM32CubeMX to generate UART initialization code
- Ensure HSE/HSI clock is configured correctly
- DMA can be enabled for better performance

### ESP32
- Use `uart_driver_install()` to set up UART buffering
- Configure pins with `uart_set_pin()`
- Use `idf.py menuconfig` to set component options

### ARM Cortex-A
- Requires FreeRTOS+POSIX compatibility layer
- Device paths like `/dev/ttyS0`, `/dev/ttyAMA0`
- Standard POSIX termios configuration

### RISC-V
- Adjust UART base addresses for your SoC
- Configure clock frequency with `RISCV_CLOCK_FREQ`
- Verify status register bit definitions match your hardware

## Next Steps

- Read the [API documentation](../README.md) for detailed function reference
- Check [port/README.md](port/README.md) for platform-specific setup
- Review example code for integration patterns
- Join the u-blox community forum for support
