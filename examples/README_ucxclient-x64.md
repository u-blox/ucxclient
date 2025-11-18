# ucxclient-x64 - Windows Console Application

## Overview

A **comprehensive C application** with text-based menu interface for u-connectXpress devices (NORA-B26, NORA-W36).

This is a **clean alternative** to the Python GUI, avoiding complexity and reliability issues:
- ✅ **No Python** - Direct C code
- ✅ **No DLL complexity** - Standard linking
- ✅ **No GC crashes** - No garbage collection
- ✅ **Easy to debug** - Standard C debugging tools
- ✅ **Simple and maintainable** - Clear, linear code flow
- ✅ **Auto-build support** - Launch script handles everything

## Prerequisites for a Clean PC

To build and run this application from a fresh clone, you need:

### System Requirements
- **Windows 10 or Windows 11** (64-bit)
- Windows 7/8/8.1 may work but are not tested
- 32-bit Windows is not supported (requires 64-bit FTDI DLL)

### Required Software
1. **Visual Studio 2022 Build Tools** (or full Visual Studio 2022)
   - ✅ **Build Tools only** (minimal, ~2-3 GB): Includes MSVC compiler without IDE
     - Download: https://aka.ms/vs/17/release/vs_BuildTools.exe
     - During install, select "Desktop development with C++"
   - ✅ **Full Visual Studio 2022** (complete IDE, ~10-20 GB): If you want the IDE
     - Download: https://visualstudio.microsoft.com/downloads/
     - Install the "Desktop development with C++" workload
   - Both include MSVC compiler and Windows SDK
   - **Visual Studio 2019 also works** (use Build Tools or full IDE)

2. **CMake 3.15 or later**
   - CMake generates the Visual Studio project files
   - Download: https://cmake.org/download/
   - ✅ During install, select "Add CMake to system PATH"

3. **Git** (for cloning)
   - Download: https://git-scm.com/download/win

### Hardware
- **FTDI USB device** (NORA-W36 or NORA-B26 module)
- FTDI drivers are included (`examples/ftdi/ftd2xx64.dll`)

### Quick Setup Steps
```powershell
# 1. Clone the repository
git clone https://github.com/u-blox/ucxclient.git
cd ucxclient

# 2. Launch (auto-builds on first run)
.\launch_ucxclient-x64.cmd

# That's it! The script handles CMake configuration and building.
```

### What the Launch Script Does
- Detects if CMake is configured (runs `cmake -S . -B build` if needed)
- Builds the executable if missing (runs `cmake --build build --config Debug`)
- Copies FTDI DLL to the output directory
- Launches the application (ucxclient-x64.exe)
- Can code-sign executables with certificate thumbprint (creates ucxclient-x64-signed.exe)
- Can build all configurations at once with the `all` command

### Code Signing (Optional)
For production releases, you can digitally sign the executable:

```powershell
# Find your certificate thumbprint
# 1. Open Certificate Manager: certmgr.msc
# 2. Personal > Certificates > Your code signing cert > Details > Thumbprint
# 3. Copy the thumbprint (remove spaces)

# Sign Release build (creates ucxclient-x64-signed.exe)
.\launch_ucxclient-x64.cmd sign release YOUR_CERT_THUMBPRINT_HERE

# Sign Debug build (for testing)
.\launch_ucxclient-x64.cmd sign debug YOUR_CERT_THUMBPRINT_HERE
```

**Requirements for signing:**
- Windows SDK 10 (for signtool.exe)
- Code signing certificate (USB token or machine certificate store)
- SafeNet Authentication Client (if using USB token)

### Automatic Checks
The launch script automatically verifies:
- ✅ Windows 10/11 (64-bit)
- ✅ CMake installed and in PATH
- ✅ Visual Studio with C++ tools installed

### Troubleshooting
If you get errors:
- **"64-bit Windows required"** → This app requires 64-bit Windows (uses 64-bit FTDI DLL)
- **"Windows 10 or 11 recommended"** → Older Windows versions may have issues
- **"cmake not found"** → Install CMake and add to PATH, then restart PowerShell
- **"MSVC not found"** → Install Visual Studio 2022 Build Tools with "Desktop development with C++" workload
- **"Cannot find COM port"** → Check Device Manager for FTDI device

### Why Build Tools vs Full IDE?
- **Build Tools** (recommended for CI/servers): Minimal install, command-line only, faster
- **Full IDE**: If you want Visual Studio's editor, debugger, and GUI features
- Both work identically with CMake and the launch script!

## Features

### ✅ Current Implementation

#### 1. **Device Connection**
   - Auto-detect and list available COM ports
   - Smart FTDI device detection (shows NORA-W36, NORA-B26, etc.)
   - Quick connect to last used device
   - Auto-reconnect with Wi-Fi credentials
   - Settings saved next to executable

#### 2. **Basic Commands**
   - Dynamic API command listing (fetch from GitHub)
   - AT test (basic communication)
   - ATI9 (device information)
   - Module reboot/switch off
   - Toggle UCX logging on/off

#### 3. **Bluetooth Operations**
   - Show Bluetooth status
   - Scan for nearby devices
   - Connect to devices
   - List active connections
   - SPS (Serial Port Service) support

#### 4. **Wi-Fi Operations** (NORA-W36)
   - Show Wi-Fi status with RSSI
   - Scan Wi-Fi networks
   - Connect to networks (WPA2/Open)
   - Disconnect from networks
   - Credentials saved and reused

#### 5. **Socket Operations** (TCP/UDP)
   - Create TCP/UDP sockets
   - Connect to remote servers
   - Send and receive data
   - List socket status
   - Close sockets

#### 6. **SPS Operations** (Bluetooth Serial)
   - Enable SPS service
   - Connect SPS over Bluetooth
   - Send and receive serial data

#### 7. **Firmware Update**
   - XMODEM protocol support
   - Progress bar during update
   - Auto-reconnect after update

#### 8. **User Experience**
   - Welcome guide for first-time users
   - Comprehensive help system ([h] key)
   - Universal quit ([q] key)
   - Input validation
   - Auto-save settings
   - Status indicators (Wi-Fi/BT availability)
   - Color-coded log messages

### 🚧 In Progress Features
- **[c] MQTT** - Publish/subscribe messaging
- **[d] HTTP Client** - REST API operations (GET/POST/PUT/DELETE)
- **[e] Security/TLS** - Certificate management

## Quick Start

### Easy Launch (Recommended)
```bash
# From project root - builds automatically if needed
launch_ucxclient-x64.cmd

# For Release build
launch_ucxclient-x64.cmd release

# Build all configurations (Debug + Release)
launch_ucxclient-x64.cmd all
```

The launch script will:
- ✅ Auto-build if executable doesn't exist
- ✅ Copy FTDI DLL to build directory
- ✅ Launch the application
- ✅ Handle all dependencies
- ✅ Build both configurations with `all` command

### Building Manually

#### Using CMake
```bash
cd build
cmake ..
cmake --build . --config Debug --target ucxclient-x64
```

The executable will be in `build/Debug/ucxclient-x64.exe`

#### Using Visual Studio
Open `build/ucxclient-x64.sln` and build the `ucxclient-x64` project.

## File Structure

```
ucxclient/
├── launch_ucxclient-x64.cmd         # Launch script (auto-builds)
├── examples/
│   ├── ucxclient-x64.c              # Main application
│   └── ftdi/
│       └── ftd2xx64.dll             # FTDI driver DLL
├── ucxclient-x64_settings.ini       # Settings (auto-created in workspace root)
└── build/
    ├── Debug/                       # Debug build output
    │   ├── ucxclient-x64.exe        # Executable
    │   └── ftd2xx64.dll             # FTDI DLL (auto-copied)
    ├── Release/                     # Release build output
    │   ├── ucxclient-x64.exe        # Unsigned executable
    │   └── ftd2xx64.dll             # FTDI DLL (auto-copied)
    └── Release_Signed/              # Signed Release (for distribution)
        ├── ucxclient-x64-signed.exe # Signed executable (code-signed)
        └── ftd2xx64.dll             # FTDI DLL (auto-copied)
```

### Signed Release Folder
The `build/Release_Signed/` folder contains the code-signed executable for distribution:
- **Created by**: Code signing command (`launch_ucxclient-x64.cmd sign release THUMBPRINT`)
- **Can be committed to Git**: Signed executable remains stable for distribution
- **Smart auto-select**: Launch script automatically uses signed build if it exists and is up-to-date
- **Priority fallback**: If Debug or Release builds are newer (recompiled), they take priority over the signed version
- **Best practice**: Sign and commit the Release_Signed build for distribution, but keep developing with Debug/Release

### Settings File
The `ucxclient-x64_settings.ini` file is automatically created **in the workspace root directory** and stores:
- Last COM port used
- Last device model
- Wi-Fi SSID and password (obfuscated)
- Last remote server address

## Usage

### Launch Methods
```bash
# Method 1: Use launch script (recommended)
launch_ucxclient-x64.cmd

# Method 2: Direct execution
cd build\Debug
ucxclient-x64.exe

# Method 3: Specify COM port
ucxclient-x64.exe COM4
```

### Main Menu
```
--- Main Menu ---
  Device:      COM31 (NORA-W36 3.2.0-046)
  Wi-Fi:        Available (use [8] to connect)
  Bluetooth:   Available (use [7] for operations)
  UCX Logging: ENABLED

  [1] Connect to UCX device
  [2] Disconnect from device
  [3] List API commands
  [4] AT test (basic communication)
  [5] ATI9 (device info)
  [6] Module reboot/switch off
  [7] Bluetooth menu
  [8] Wi-Fi menu
  [9] Toggle UCX logging (AT traffic)
  [a] Socket menu (TCP/UDP) (requires Wi-Fi)
  [b] SPS menu (Bluetooth Serial) (requires BT)
  [c] MQTT menu (publish/subscribe) [IN PROGRESS]
  [d] HTTP Client menu (GET/POST/PUT) [IN PROGRESS]
  [e] Security/TLS menu (certificates) [IN PROGRESS]
  [f] Firmware update (XMODEM)
  [h] Help - Getting started guide
  [q] Quit application
```

### Special Keys
- **[h]** - Show comprehensive help anytime
- **[q]** - Quit from any menu
- **[0]** - Return to previous menu

## Architecture

### Clean and Simple
```
ucxclient-x64.c (3899 lines)
├── main()                          // Entry point
├── connectDevice()                 // Initialize and connect
├── disconnectDevice()              // Clean shutdown
├── executeAtTest()                 // AT command
├── executeAti9()                   // Device info
├── showBluetoothStatus()           // BT status
├── showWifiStatus()                // Wi-Fi status
├── wifiMenu()                      // Wi-Fi operations
├── bluetoothMenu()                 // Bluetooth operations
├── socketMenu()                    // TCP/UDP sockets
├── spsMenu()                       // Bluetooth Serial Port
├── firmwareUpdate()                // XMODEM update
└── Menu handling functions
```

### Direct API Calls
No wrapper layers - just call ucxclient functions directly:
```c
// Initialize
uPortAtInit(&gAtClient);
uPortAtOpen(&gAtClient, "COM31", 115200, false);
uCxInit(&gAtClient, &gUcxHandle);

// Execute commands
uCxGeneralGetIdentInfoBegin(&gUcxHandle, &info);
uCxBluetoothGetMode(&gUcxHandle, &btMode);
uCxWifiStationStatusBegin(&gUcxHandle, statusId, &status);
```

## Advantages Over Python GUI

| Aspect | Python GUI | C Console App |
|--------|-----------|---------------|
| **Complexity** | Python→DLL→C chain | Direct C calls |
| **Reliability** | GC crashes, threading issues | Stable, predictable |
| **Debugging** | Multi-layer debugging | Standard C debugging |
| **Performance** | Python/ctypes overhead | Native performance |
| **Maintenance** | Complex state management | Simple linear flow |
| **Portability** | Python dependency | Standard C compiler |

## Next Steps

1. **Test basic functionality**
   - Connect to device
   - Execute AT commands
   - Check status commands

2. **Add missing features**
   - Bluetooth scan/connect
   - Wi-Fi scan/connect
   - Configuration commands

3. **Enhance UX**
   - Better error messages
   - Input validation
   - Progress indicators

4. **Document patterns**
   - API usage examples
   - Error handling
   - Best practices

## Example Session

```
==============================================================
 Windows UCX Client - NORA-W36/NORA-B26 AT Test Application
==============================================================
Auto-detecting COM ports with FTDI devices...
Found FTDI device on COM31

Settings loaded from: build\Debug\windows_app_settings.ini
Attempting to auto-connect to COM31...
Connected successfully!

Device info: NORA-W36 (3.2.0-046)

╔════════════════════════════════════════════════════════╗
║                      Main Menu                         ║
╚════════════════════════════════════════════════════════╝
  Device:      COM31 (NORA-W36 3.2.0-046)
  Wi-Fi:        Available
  Bluetooth:   Available
  UCX Logging: ENABLED

  [1] Connect to UCX device
  [2] Disconnect from device
  [3] List API commands (188 commands)
  [4] AT test (basic communication)
  [5] ATI9 (device info)
  [6] Module reboot/switch off
  [7] Bluetooth menu
  [8] Wi-Fi menu
  [9] Toggle UCX logging (AT traffic)
  [a] Socket menu (TCP/UDP)
  [b] SPS menu (Bluetooth Serial)
  [c] MQTT menu [IN PROGRESS]
  [d] HTTP Client menu [IN PROGRESS]
  [e] Security/TLS menu [IN PROGRESS]
  [f] Firmware update (XMODEM)
  [h] Help - Getting started guide
  [q] Quit application

Enter choice: 8

╔════════════════════════════════════════════════════════╗
║                      Wi-Fi Menu                         ║
╚════════════════════════════════════════════════════════╝
  Current Status: Disconnected

  [1] Wi-Fi scan
  [2] Wi-Fi connect
  [3] Wi-Fi disconnect
  [4] Wi-Fi status
  [0] Back to main menu
  [q] Quit application

Enter choice: 1

Scanning for Wi-Fi networks...

Found 5 Wi-Fi networks:
  1. MyHomeNetwork     (CH: 6, RSSI: -45 dBm) [WPA2-Personal]
  2. GuestNetwork      (CH: 11, RSSI: -62 dBm) [WPA2-Personal]
  3. CoffeeShop        (CH: 1, RSSI: -75 dBm) [Open]
  4. Neighbor5G        (CH: 36, RSSI: -68 dBm) [WPA3-Personal]
  5. IoT_Devices       (CH: 6, RSSI: -58 dBm) [WPA2-Personal]

Press any key to continue...

Enter choice: 2

Enter Wi-Fi SSID: MyHomeNetwork
Enter Wi-Fi password: ********
Connecting to 'MyHomeNetwork'...
Wi-Fi connected successfully!

Enter choice: a

╔════════════════════════════════════════════════════════╗
║                     Socket Menu                        ║
╚════════════════════════════════════════════════════════╝
  Wi-Fi Status: Connected to MyHomeNetwork

  [1] TCP connect
  [2] TCP send data
  [3] TCP close
  [4] UDP send/receive
  [0] Back to main menu
```

## Philosophy

**Keep It Simple!**
- Direct C calls
- Clear error messages
- Linear execution flow
- Easy to understand and maintain
- No over-engineering

This is what we should have built from the start! 🎯

