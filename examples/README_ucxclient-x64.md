# ucxclient-x64 - Windows Console Application

## Overview

A comprehensive Windows application for testing and configuring u-connectXpress devices (NORA-W36, NORA-B26). Built with native C code for optimal performance, stability, and ease of debugging.

**Key Features:**
- ✅ **Native C Implementation** - Direct API calls without wrapper layers
- ✅ **Professional Interface** - Dual menu modes (detailed/compact)
- ✅ **Auto-versioning** - Git-based build numbers aligned with UCX API
- ✅ **Secure Firmware Updates** - SHA256 verification with XMODEM protocol
- ✅ **Easy Deployment** - Launch script handles building and dependencies
- ✅ **Production Ready** - Code signing support for distribution

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

### Device Management
- **Auto-detect** FTDI devices with smart COM port detection
- **Quick connect** to last used device with saved settings
- **Auto-reconnect** with Wi-Fi/Bluetooth credentials
- **Device information** display (ATI9) with detailed module specs
- **Firmware updates** via XMODEM with SHA256 verification

### Bluetooth Operations
- Show Bluetooth status and mode
- Scan for nearby devices with RSSI
- Connect/disconnect to devices
- Bond management (pair/unpair)
- List active connections
- SPS (Serial Port Service) - Bluetooth serial communication
- GATT Client - Read/write characteristics, discover services
- GATT Server - Host custom services and characteristics

### Wi-Fi Operations (NORA-W36)
- Show Wi-Fi status with signal strength (RSSI)
- Scan networks with channel and security info
- Connect to networks (WPA2/WPA3/Open/Enterprise)
- Station mode - Connect to access points
- Access Point mode - Create hotspot
- Network diagnostics (ping)
- Credentials saved and auto-reconnect

### Network Services
- **Socket Operations** - TCP/UDP client/server, send/receive data
- **HTTP Client** - GET/POST/PUT/DELETE requests with headers
- **MQTT Client** - Publish/subscribe messaging with QoS
- **Security/TLS** - Certificate management and encrypted connections
- **Time Sync** - NTP time synchronization
- **Location Services** - Combain Wi-Fi/Cell positioning

### User Experience
- **Dual Menu Modes** - Toggle between detailed and compact views
- **Quick Access Keys** - Single letter shortcuts for common functions
- **Context-sensitive Help** - Press [?] anytime for help
- **Universal Quit** - Press [q] from any menu
- **Smart Input Validation** - Error checking and user guidance
- **Auto-save Settings** - Preferences persist across sessions
- **Status Indicators** - Clear display of Wi-Fi/Bluetooth availability
- **Logging Control** - Toggle AT command logging with timestamps

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
The `ucxclient-x64_settings.ini` file is automatically created in the workspace root directory and stores:
- Last COM port and device model
- Wi-Fi SSID and password (obfuscated)
- Bluetooth bonded devices
- Menu mode preference (detailed/compact)
- Logging and timestamp settings
- API keys (Combain location service)
- Last remote server addresses

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
==============================================================
 ucxclient-x64 - u-blox u-connectXpress Test Application
               UCX API v3.2.0  |  App v3.2.0.257
==============================================================

 Device: COM31 (NORA-W36 3.2.0) | Wi-Fi: Available | BT: Available
 Logging: ON | Timestamps: OFF

POWER & SYSTEM
  [r] Reboot  [j] Factory reset  [p] Power Management

BLUETOOTH
  [b] Scan, connect, pair  [s] Serial Port Service (SPS)
  [t] GATT Client  [u] GATT Server

WI-FI (NORA-W36)
  [w] Station - Scan, connect  [o] Access Point - Hotspot
  [d] Network Diagnostics

EXAMPLES
  Bluetooth: [e] GATT Server (9)  [g] GATT Client (9)
  Wi-Fi:     [h] HTTP  [m] MQTT  [y] Time Sync  [k] Location

NETWORK SERVICES
  [n] Network menu - Socket, HTTP, MQTT, TLS/Security

TOOLS & SETTINGS
  [l] Toggle logging  [z] Toggle timestamps  [c] Toggle menu mode
  [v] List UCX API commands  [?] Help & Tips

FIRMWARE
  [f] Update module firmware (XMODEM)

  [q] Quit
  [1-94] Direct menu choice

Enter choice:
```

### Compact Menu Mode
Press **[c]** to toggle between detailed and compact menu modes. The compact mode displays multiple items per line, reducing vertical space by 50% for easier viewing on smaller terminals.

## Architecture

### Clean Design
```
ucxclient-x64.c (~21,000 lines)
├── Main application loop
├── Device management (connect/disconnect)
├── Menu system (detailed/compact modes)
├── Power & System (reboot, factory reset, power mgmt)
├── Bluetooth operations (scan, connect, bond, SPS, GATT)
├── Wi-Fi operations (scan, connect, AP mode, diagnostics)
├── Network services (Socket, HTTP, MQTT, TLS)
├── Example implementations (9 BT + 4 Wi-Fi examples)
├── Firmware update (XMODEM with SHA256 verification)
├── Settings management (load/save configuration)
└── Utility functions (logging, input validation, help)
```

### Direct API Integration
Native C code directly calls ucxclient API functions:
```c
// Initialize device
uPortAtInit(&gAtClient);
uPortAtOpen(&gAtClient, "COM31", 115200, false);
uCxInit(&gAtClient, &gUcxHandle);

// Execute commands
uCxGeneralGetDeviceInfo(&gUcxHandle, &info);
uCxBluetoothGetMode(&gUcxHandle, &btMode);
uCxWifiStationConnect(&gUcxHandle, ssid, passphrase);
uCxSocketCreate(&gUcxHandle, protocol, &socketHandle);
```

### Versioning System
- **Format**: `3.2.0.BUILD` where BUILD = Git commit count
- **Alignment**: Major.Minor.Patch follows UCX API version
- **Auto-increment**: Build number increases with each commit
- **Display**: Shown in menu header and Windows file properties
- **Implementation**: CMake generates `version.h` from template at build time

## Example Session

```
==============================================================
 ucxclient-x64 - u-blox u-connectXpress Test Application
               UCX API v3.2.0  |  App v3.2.0.257
==============================================================

Auto-detecting COM ports with FTDI devices...
Found FTDI device on COM31

Settings loaded from: C:\u-blox\ucxclient\ucxclient-x64_settings.ini
Attempting to auto-connect to COM31...

Connected to: NORA-W36 (u-connectXpress 3.2.0)
Firmware: 3.2.0-046

 Device: COM31 (NORA-W36 3.2.0) | Wi-Fi: Available | BT: Available

Enter choice: w

╔══════════════════════════════════════════════════════╗
║                  Wi-Fi Station Menu                  ║
╚══════════════════════════════════════════════════════╝

  [1] Scan for networks
  [2] Connect to network
  [3] Disconnect
  [4] Show status
  [0] Return to main menu

Enter choice: 1

Scanning for Wi-Fi networks...

Found 4 networks:
  1. HomeNetwork      (CH:  6, RSSI: -42 dBm) [WPA2-Personal]
  2. Office_5G        (CH: 36, RSSI: -58 dBm) [WPA3-Personal]
  3. Guest_Network    (CH: 11, RSSI: -65 dBm) [WPA2-Personal]
  4. CoffeeShop       (CH:  1, RSSI: -72 dBm) [Open]

Press Enter to continue...

Enter choice: 2

Enter SSID: HomeNetwork
Enter password: ********

Connecting to 'HomeNetwork'...
Wi-Fi connected successfully!
IP Address: 192.168.1.45

Enter choice: 0

[Back to main menu]

Enter choice: h

╔══════════════════════════════════════════════════════╗
║                   HTTP Client Menu                   ║
╚══════════════════════════════════════════════════════╝

  [1] GET request
  [2] POST request
  [3] Show response
  [4] Custom headers
  [0] Return to main menu

Enter choice: 1

Enter URL: http://httpbin.org/get

Sending GET request to http://httpbin.org/get...
Response: 200 OK
Content-Length: 312

Press Enter to continue...
```

## Advanced Features

### Firmware Update with SHA256 Verification
When updating firmware, the application:
1. Downloads latest firmware release information from GitHub
2. Verifies SHA256 checksum matches the release notes
3. If SHA256 not found, prompts for manual verification
4. Transfers firmware via XMODEM protocol with progress bar
5. Auto-reconnects after successful update

### Certificate Management
Upload and manage certificates for TLS/HTTPS connections:
- Import PEM/DER format certificates
- Configure TLS version (1.2, 1.3, or negotiate)
- Client certificate authentication
- CA certificate validation

### MQTT Client Features
- Connect to MQTT brokers with authentication
- Publish messages with QoS 0/1/2
- Subscribe to topics with wildcard support
- Receive and display incoming messages
- TLS/SSL encrypted connections

### Location Services
Get device location using Combain Wi-Fi/Cell positioning:
- Automatic Wi-Fi AP scanning
- Location query to Combain API
- Latitude/longitude with accuracy radius
- Optional API key configuration

## Production Deployment

### Building Release Version
```powershell
# Build Release configuration
.\launch_ucxclient-x64.cmd release

# Code sign the executable (requires certificate)
.\launch_ucxclient-x64.cmd sign release YOUR_CERT_THUMBPRINT

# Distribute the signed executable
# Location: build\Release_Signed\ucxclient-x64-signed.exe
```

### Distribution Package
Include these files when distributing:
- `ucxclient-x64-signed.exe` (or `ucxclient-x64.exe`)
- `ftd2xx64.dll` (FTDI driver)
- Optional: Sample `ucxclient-x64_settings.ini`

### System Requirements
- Windows 10/11 (64-bit)
- FTDI USB device (NORA-W36 or NORA-B26)
- No additional runtime dependencies

## Support

For issues, feature requests, or contributions:
- GitHub: https://github.com/u-blox/ucxclient
- Documentation: See `examples/` directory
- UCX API Reference: Latest version included in firmware releases

---

**Version**: 3.2.0.257 (aligned with UCX API 3.2.0)  
**Build**: Auto-versioned using Git commit count  
**Platform**: Windows 10/11 (64-bit)  
**License**: See LICENSE.txt

