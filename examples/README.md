# Examples

This directory contains application examples of how to use ucxclient. The examples make use of the [example ports](port/README.md).

| Files                | Description | Port |
| -------------------- | ----------- | ---- |
| http_example.c       | Example of doing a HTTP GET request using the uCx API. | Linux (POSIX) |
| http_example_no_os.c | Same example as http_example.c but illustrating how it could be done on a bare-metal system with no OS. | No OS (bare-metal) |
| ucxclient-x64.c    | **Comprehensive Windows console application** with menu-driven interface for Bluetooth, Wi-Fi, HTTP, Location Services, Network operations, and more. | Windows |
| windows_app.c        | Legacy Windows console application (superseded by ucxclient-x64). | Windows |
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
- `build\Release\ucxclient-x64.exe` - **Comprehensive application with Location Services** (Recommended)
- `build\Debug\windows_app.exe` - Legacy interactive menu-driven application
- `build\Debug\windows_basic.exe` - Simple test application
- `build\Debug\http_example_windows.exe` - HTTP example for Windows
- `build\Debug\ucxclient_dll.dll` - Shared library for Python integration

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
> build/http_example /dev/ttyUSB0 MySSID MyWi-FiPasswd
```

### Linux: http_example_no_os

Just like http_example this example will also be compiled for Linux, but in this case the UART device, Wi-Fi SSID and PSK are configured using defines.
To set these defines using CMake you can either use `cmake-gui`:

![cmake-gui](/images/cmake-gui.png)

or from command line:

```sh
> cmake -S . -B build -D U_EXAMPLE_UART="/dev/ttyUSB0" -D U_EXAMPLE_SSID="MySSID" -D U_EXAMPLE_WPA_PSK="MyWi-FiPasswd"
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

3. **Wi-Fi Operations**
   - View Wi-Fi status and signal strength
   - Scan for Wi-Fi networks
   - Connect to Wi-Fi (credentials saved securely)
   - Disconnect from Wi-Fi

3.5. **Time Synchronization**
   - **NTP Time Sync**: Synchronize system time with NTP servers (±1 second accuracy)
   - **TimeAPI.io Integration**: 7 comprehensive endpoints for time and timezone data
     * Get current time by timezone (e.g., Europe/Stockholm)
     * Get current time by IP address (auto-detect external IP)
     * Get current time by coordinates (latitude/longitude)
     * Get timezone info by name (IANA timezone database)
     * Get timezone info by IP address
     * Get timezone info by coordinates
     * Health check (API status verification)
   - **Automatic Time Tracking**: Updates from multiple sources (PC, HTTP headers, NTP, TimeAPI.io)
   - **Source Provenance**: Displays time source and accuracy (e.g., "±1s, NTP")
   - **Timezone-to-Position**: Estimates geographic position from timezone (60+ IANA zones mapped)
   - **HTTPS Support**: TLS 1.2 for secure time synchronization

3.6. **Location Services**
   - **External IP Detection**: Discover public IP address via ipify.org
   - **IP Geolocation**: Approximate location from IP address (±20 km accuracy via ip-api.com)
   - **Wi-Fi Positioning**: Precise positioning using nearby access points (±48 m accuracy via Combain API)
   - **Timezone Position Estimation**: Geographic position from timezone data (±300-2000 km depending on zone)
   - **Position Tracking**: Real-time position display with source and accuracy metadata
   - **Status Dashboard**: Shows current position: "59.3293, 18.0686 (±500m, Wi-Fi)"

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
- Wi-Fi SSID and password (obfuscated)
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
  [7] Wi-Fi menu
  [8] Toggle UCX logging (AT traffic)
  [9] Socket menu (TCP/UDP)
  [a] SPS menu (Bluetooth Serial)
  [0] Exit

Choice: 7

--- Wi-Fi Menu ---
  [1] Show Wi-Fi status
  [2] Scan networks
  [3] Connect to network
  [4] Disconnect from network
  [0] Back to main menu

Choice: 3
```

### Windows: ucxclient-x64.exe (Recommended)

The **ucxclient-x64** is a comprehensive Windows console application with an enhanced menu system and extensive feature set.

#### Quick Start

**Using the launcher (recommended):**
```powershell
.\launch_ucxclient-x64.cmd
```
The launcher automatically:
- Checks build status and rebuilds if needed
- Detects available COM ports
- Auto-connects to the last used device
- Provides color-coded status messages

**Direct execution:**
```powershell
.\build\Release\ucxclient-x64.exe [COM_PORT]
```

#### Features

**Bluetooth Operations:**
- Device scanning and pairing
- Connection management with multiple devices
- GATT Client/Server operations
- Serial Port Service (SPS)
- HID over GATT (keyboard/mouse)
- Service discovery and characteristics

**Wi-Fi Operations:**
- Network scanning with detailed information
- Profile management (save/load credentials)
- Connection with automatic reconnect
- Status monitoring (signal strength, IP, gateway)
- Access Point mode with QR code generation
- Network connectivity testing (ping)

**HTTP Client Examples:**
- HTTP GET with file save option
- HTTP POST with data or file upload
- Response header and body reading
- Connection parameter configuration

**Location Services Examples:**
- External IP Detection (ipify.org API)
- IP Geolocation (ip-api.com) - Country, city, ISP, coordinates
- Wi-Fi Positioning (Combain API) - Indoor/outdoor positioning using access points

**Network Operations:**
- Socket client/server (TCP/UDP)
- MQTT publish/subscribe
- TLS/SSL certificate management
- NTP time synchronization

**Developer Tools:**
- Interactive AT Terminal with command history
- Real-time AT command logging
- API command reference with GitHub integration
- Firmware update via XMODEM
- URC (Unsolicited Result Code) monitoring

#### Menu Structure

```
Main Menu
├── [w] Wi-Fi Station
│   ├── Scan networks
│   ├── Connect/Disconnect
│   ├── Profile management
│   └── Status monitoring
├── [o] Wi-Fi Access Point
│   ├── Enable/Disable AP
│   ├── QR code generation
│   └── AP configuration
├── [b] Bluetooth
│   ├── Scan devices
│   ├── Pairing
│   └── Connection management
├── [s] Serial Port Service (SPS)
├── [g] GATT Client/Server
├── [n] Network
│   ├── Sockets (TCP/UDP)
│   ├── MQTT
│   └── HTTP
├── [x] Security/TLS
│   └── Certificate management
├── [e] GATT Examples
│   ├── Heartbeat service
│   └── HID over GATT
├── [p] HTTP Examples
│   ├── GET request
│   └── POST request
├── [k] Location Examples ⭐ NEW
│   ├── External IP Detection
│   ├── IP Geolocation
│   └── Wi-Fi Positioning
├── [y] NTP Examples
│   └── Time synchronization
├── [f] Firmware Update
├── [t] AT Terminal
└── [l] Toggle Logging
```

#### Settings Persistence

The application automatically saves and restores:
- Last used COM port and device model
- Wi-Fi network profiles (SSID, password, security settings)
- Recent connection addresses
- Logging preferences

Settings stored in: `ucxclient-x64_settings.ini`

#### Example: Location Services

The Location Examples menu (`[k]`) provides three positioning methods:

**1. External IP Detection:**
```
Uses ipify.org API to retrieve your public IP address
- No authentication required
- Simple JSON response
- Useful for NAT/firewall detection
```

**2. IP Geolocation:**
```
Uses ip-api.com to determine location from IP
- Country, region, city
- ISP and organization
- Latitude/longitude coordinates
- Timezone information
- Free tier: 45 requests/minute
```

**3. Wi-Fi Positioning:**
```
Uses Combain Positioning Service with Wi-Fi scan
- Performs Wi-Fi scan to collect nearby access points
- Sends MAC addresses, SSIDs, and signal strengths
- Returns outdoor position (lat/lon, accuracy)
- Indoor positioning if building data available
- No GPS hardware required
```

Example flow:
1. Connect to Wi-Fi network
2. Press `[k]` for Location menu
3. Choose `[3]` for Wi-Fi Positioning
4. App scans access points automatically
5. Builds JSON payload with scan results
6. Sends to Combain API
7. Displays position and accuracy

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
.\build\Debug\http_example_windows.exe COM31 MySSID MyWi-FiPassword
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
