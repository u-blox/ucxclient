# ucx-windows-app - Windows Console Application for u-connectXpress

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
   - Use the Windows x64 Installer
   - ✅ During install, select "Add CMake to system PATH"

3. **Git** (for cloning)
   - Download: https://git-scm.com/download/win

### Hardware
- **FTDI USB device** (NORA-W36 or NORA-B26 module)
- FTDI drivers are embedded in the executable (auto-extracted to temp folder)

### Quick Setup Steps
```powershell
# 1. Clone the repository
git clone https://github.com/u-blox/ucxclient.git
cd ucxclient

# 2. Launch (auto-builds on first run)
.\launch-ucx-windows-app.cmd

# That's it! The script handles CMake configuration and building.
```

### What the Launch Script Does
- Detects if CMake is configured (runs `cmake -S examples/ucx-windows-app -B build` if needed)
- Builds the executable if missing or outdated (source newer than exe)
- Embeds FTDI DLL into the executable as a resource (auto-extracted at runtime)
- Creates settings file (`ucx-windows-app.ini`) in project root if missing
- Smart version selection: Runs signed version if newer than unsigned
- Launches the application (ucx-windows-app.exe or ucx-windows-app-signed.exe)
- Can code-sign executables with certificate thumbprint for distribution
- Can build all configurations at once with the `all` command

### Launch Script Commands
```powershell
# Default: Auto-select signed if newer, else unsigned
.\launch-ucx-windows-app.cmd

# Debug build
.\launch-ucx-windows-app.cmd debug

# Force signed version
.\launch-ucx-windows-app.cmd signed

# Build all configurations (Debug + Release)
.\launch-ucx-windows-app.cmd all

# Clean build artifacts
.\launch-ucx-windows-app.cmd clean [debug|release]

# Rebuild from scratch
.\launch-ucx-windows-app.cmd rebuild [debug|release]

# Code sign the Release build
.\launch-ucx-windows-app.cmd sign YOUR_CERT_THUMBPRINT
```

### Code Signing (Optional)
For production releases, you can digitally sign the executable:

```powershell
# Find your certificate thumbprint
# 1. Open Certificate Manager: certmgr.msc
# 2. Personal > Certificates > Your code signing cert > Details > Thumbprint
# 3. Copy the thumbprint (remove spaces)

# Sign Release build (creates ucx-windows-app-signed.exe in release/ folder)
.\launch-ucx-windows-app.cmd sign YOUR_CERT_THUMBPRINT_HERE

# Example
.\launch-ucx-windows-app.cmd sign EF3FD135F1CD669E0D7F4F2CF14FE1334EECD16E
```

**Output Location:**
- Signed executable: `examples/ucx-windows-app/release/ucx-windows-app-signed.exe`
- Unsigned build: `examples/ucx-windows-app/bin/ucx-windows-app.exe`
- The launcher auto-selects the signed version if it's newer

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
.\launch-ucx-windows-app.cmd

# Debug build
.\launch-ucx-windows-app.cmd debug

# Build all configurations (Debug + Release)
.\launch-ucx-windows-app.cmd all
```

The launch script will:
- ✅ Auto-build if executable doesn't exist or source is newer
- ✅ Embed FTDI DLL as resource in executable
- ✅ Smart version selection (signed vs unsigned)
- ✅ Launch the application
- ✅ Handle all dependencies
- ✅ Build both configurations with `all` command

### Building Manually

#### Using CMake
```bash
# Configure
cmake -S examples/ucx-windows-app -B build

# Build Release
cmake --build build --config Release --target ucx-windows-app

# Build Debug
cmake --build build --config Debug --target ucx-windows-app
```

The executable will be in `examples/ucx-windows-app/bin/ucx-windows-app.exe`

#### Using Visual Studio
Open `build/ucx-windows-app.sln` and build the `ucx-windows-app` project.

## File Structure

```
ucxclient/
├── launch-ucx-windows-app.cmd          # Launch script (auto-builds)
├── ucx-windows-app.ini                 # Settings (auto-created in project root)
├── examples/
│   └── ucx-windows-app/
│       ├── ucx-windows-app.c           # Main application (~25,000 lines)
│       ├── ucx-windows-app.rc          # Windows resource file (icon, version, FTDI DLL)
│       ├── version.h.in             # Version template (CMake)
│       ├── bin/
│       │   ├── ucx-windows-app.exe         # Release build (unsigned, gitignored)
│       │   └── ucx-windows-app-debug.exe   # Debug build (gitignored)
│       ├── release/
│       │   └── ucx-windows-app-signed.exe  # Code-signed release (tracked in git)
│       ├── third-party/
│       │   └── ftdi/
│       │       └── ftd2xx64.dll     # FTDI driver (embedded as resource)
│       └── images/
│           └── ShortRange.ico       # Application icon
└── build/
    ├── ucx-windows-app.sln             # Visual Studio solution
    ├── ucx-windows-app.vcxproj         # Visual Studio project
    ├── CMakeCache.txt               # CMake configuration
    └── [build artifacts]            # Intermediate files (.obj, .pdb, etc.)
```

### Folder Organization
- **`bin/`** - Temporary build outputs (unsigned executables, auto-built, gitignored)
- **`release/`** - Production releases (code-signed executable, tracked in git for distribution)
- **`build/`** - CMake build directory (intermediate files, project files, gitignored)

### Settings File Location Strategy
The `ucx-windows-app.ini` file uses smart location detection:

1. **Launcher usage** (recommended):
   - Launcher creates/uses `ucx-windows-app.ini` in project root
   - Shared settings across all executions
   - Easy to find and edit

2. **Direct exe usage** (standalone):
   - Application checks `../../ucx-windows-app.ini` (navigate from bin/release to root)
   - If found: Uses root settings (same as launcher)
   - If not found: Creates `ucx-windows-app.ini` next to exe (standalone mode)

**Settings include:**
- Last COM port and device model
- Wi-Fi profiles (SSID, password, IP prefix for auto-detection)
- Bluetooth device profiles (address, name)
- Remote server addresses
- Menu mode preference (detailed/compact)
- Logging and timestamp settings
- API keys (Combain location service)
- Regulatory domain preference

## Usage

### Launch Methods
```bash
# Method 1: Use launch script (recommended)
.\launch-ucx-windows-app.cmd

# Method 2: Force signed version
.\launch-ucx-windows-app.cmd signed

# Method 3: Debug build
.\launch-ucx-windows-app.cmd debug

# Method 4: Direct execution (from bin folder)
cd examples\ucx-windows-app\bin
.\ucx-windows-app.exe

# Method 5: Specify COM port (any method)
.\launch-ucx-windows-app.cmd COM4
# or
.\ucx-windows-app.exe COM4
```

### Main Menu
```
═════════════════════════════════════════════════════════════════
             u-blox ucxclient Windows Demo
     using ucxclient API v3.2.0  |  Application v3.2.0.257
═════════════════════════════════════════════════════════════════

=== STATUS DASHBOARD ===
┌─────────────────────────────────────────────────────────────────┐
│ Device  │ COM10                                                  │
│ Module  │ NORA-W36                                               │
│ Version │ 3.2.0                                                  │
├─────────────────────────────────────────────────────────────────┤
│ Wi-Fi   │ ● Connected - ubx (Ch 1, -42 dBm)                     │
│ IP      │ 10.12.71.133                                           │
│ BT      │ ○ Not Advertising                                     │
└─────────────────────────────────────────────────────────────────┘

=== MAIN MENU ===
POWER & SYSTEM
  [r] Reboot Module        [j] Factory Reset       [p] Power Management

BLUETOOTH
  [b] Bluetooth Operations [s] SPS - BLE Serial    [t] GATT Client      
  [u] GATT Server

WI-FI (NORA-W36)
  [w] Wi-Fi Station        [o] Access Point        [d] Network Diagnostics

EXAMPLES
  Bluetooth: [e] GATT Server (9)  [g] GATT Client (9)
  Wi-Fi:     [h] HTTP  [m] MQTT  [y] Time Sync  [k] Location

NETWORK SERVICES
  [n] Network Menu - Socket, HTTP, MQTT, Security, Time

TOOLS & SETTINGS
  [l] Logging: ON          [z] Timestamps: OFF     [c] Menu: Detailed
  [i] Device Info (ATI9)   [v] List UCX API        [?] Help & Tips

FIRMWARE
  [f] Update Module Firmware (XMODEM + SHA256)

  [q] Quit

═════════════════════════════════════════════════════════════════

Enter choice:
```

### Compact Menu Mode
Press **[c]** to toggle between detailed and compact menu modes. The compact mode displays multiple items per line, reducing vertical space by 50% for easier viewing on smaller terminals.

## Architecture

### Clean Design
```
ucx-windows-app.c (~25,000 lines)
├── Main application loop
├── Device management (connect/disconnect/auto-detect)
├── Menu system (detailed/compact modes, status dashboard)
├── Power & System (reboot, factory reset, power mgmt)
├── Bluetooth operations (scan, connect, bond, SPS, GATT)
├── Wi-Fi operations (scan, connect, AP mode, diagnostics, profiles)
├── Network services (Socket, HTTP, MQTT, TLS)
├── Example implementations (9 BT + 4 Wi-Fi examples)
├── Firmware update (XMODEM with SHA256 verification)
├── Settings management (load/save, Wi-Fi/BT profiles, encryption)
├── Connection tracking (symmetric for all types: BT, SPS, Socket, HTTP, MQTT)
└── Utility functions (logging, input validation, help, IP detection)
```

### Connection Tracking (Symmetric Design)
All connection types have consistent tracking:
- **Bluetooth**: Connect/disconnect URCs → `gBtConnections[]` array
- **SPS**: Connect/disconnect URCs → `gActiveSpsConnectionHandle`
- **Socket**: Connect/closed URCs → `gCurrentSocket`
- **HTTP**: Request status/disconnect URCs → `gHttpConnected` + `gHttpLastSessionId`
- **MQTT**: Connect/disconnect URCs → `gActiveMqttClientId`

Each type has:
- URC event flags for synchronization
- Pending data structures for data-available events
- Registration/unregistration in init/cleanup functions

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
═════════════════════════════════════════════════════════════════
             u-blox ucxclient Windows Demo
     using ucxclient API v3.2.0  |  Application v3.2.0.257
═════════════════════════════════════════════════════════════════

Settings file: C:\u-blox\ucxclient\ucx-windows-app.ini

Auto-detecting COM ports with FTDI devices...
Found FTDI device on COM10

Settings loaded from: C:\u-blox\ucxclient\ucx-windows-app.ini
Last device: NORA-W36 on COM10
Attempting to auto-connect to COM10...

[00:00:00.012][AT TX][0] AT
[00:00:00.125][AT RX][0] OK
[00:00:00.125][AT TX][0] ATI9
[00:00:00.251][AT RX][0] 
Manufacturer: u-blox
Model: NORA-W36
Serial Number: 358219090048631
Firmware: 3.2.0
OK

Connected to: NORA-W36 (u-connectXpress 3.2.0)

=== STATUS DASHBOARD ===
┌─────────────────────────────────────────────────────────────────┐
│ Device  │ COM10                                                  │
│ Module  │ NORA-W36                                               │
│ Version │ 3.2.0                                                  │
├─────────────────────────────────────────────────────────────────┤
│ Wi-Fi   │ ○ Not Connected                                       │
│ BT      │ ○ Not Advertising                                     │
└─────────────────────────────────────────────────────────────────┘

Enter choice: w

--- Wi-Fi Connect ---
PC IP address: 10.12.71.133

Auto-detected: Profile 'Office_Wi-Fi' matches your network
Use profile 'Office_Wi-Fi' (SSID: ubx)? (Y/n): 

Using profile 'Office_Wi-Fi'
Connecting to 'ubx'...

[00:00:05.012][AT TX][0] AT+UWSC=0,2,"ubx"
[00:00:05.125][AT RX][0] OK
[00:00:05.125][AT TX][0] AT+UWSC=0,8,4,"password"
[00:00:05.251][AT RX][0] OK
[00:00:05.251][AT TX][0] AT+UWSCA=0,3
[00:00:06.512][AT RX][0] +UUWLE:0,2
[00:00:06.512][AT RX][0] OK

✓ Connected to 'ubx'
Getting network details...

[00:00:06.625][AT TX][0] AT+UWSSTAT
[00:00:06.751][AT RX][0] +UWSSTAT:0,"ubx",1,-42,0
[00:00:06.751][AT RX][0] OK
[00:00:06.751][AT TX][0] AT+UNSTAT=0
[00:00:06.875][AT RX][0] +UNSTAT:0,"10.12.71.133","255.255.255.0","10.12.71.1"
[00:00:06.875][AT RX][0] OK

Wi-Fi Status:
  SSID: ubx
  Channel: 1
  RSSI: -42 dBm (Excellent)
  IP: 10.12.71.133
  Subnet: 255.255.255.0
  Gateway: 10.12.71.1

Press Enter to continue...

[Returned to main menu]

Enter choice: h

--- HTTP Client ---
Enter URL (or press Enter for example.com): http://httpbin.org/get

Sending GET request to http://httpbin.org/get...

[00:00:10.012][AT TX][0] AT+UHTTP=0
[00:00:10.125][AT RX][0] OK
[00:00:10.125][AT TX][0] AT+UHTTP=0,1,"httpbin.org"
[00:00:10.251][AT RX][0] OK
[00:00:10.251][AT TX][0] AT+UHTTPC=0,1,"/get","response.txt"
[00:00:11.512][AT RX][0] +UEHTCRS:0,1,200
[00:00:11.512][AT RX][0] OK

HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 312

Response body:
{
  "args": {}, 
  "headers": {
    "Host": "httpbin.org",
    "User-Agent": "u-blox NORA-W36"
  }, 
  "url": "http://httpbin.org/get"
}

✓ Request completed successfully
Press Enter to continue...
```

## Advanced Features

### Wi-Fi Profile Management
Automatic network selection based on PC IP address:
- Saves up to 10 Wi-Fi profiles with SSID, password, and IP prefix
- Auto-detects current PC IP address (skips loopback and APIPA addresses)
- Matches IP prefix (first 3 octets) to suggest correct profile
- Example: PC IP `10.12.71.133` → matches profile with `ip_prefix=10.12.71`
- Always displays PC IP address when checking for matches
- Falls back to active profile if no IP match found

Profile structure:
```ini
wifi_profile_count=3
wifi_active_profile=0
wifi_profile_0_name=Office_Wi-Fi
wifi_profile_0_ssid=ubx
wifi_profile_0_password=32331c350e382840031e5c1c22573412  # Encrypted
wifi_profile_0_ip_prefix=10.12.71
wifi_profile_1_name=Home
wifi_profile_1_ssid=home_network
wifi_profile_1_password=012d1e0e16210c16  # Encrypted
wifi_profile_1_ip_prefix=192.168.1
```

### Bluetooth Device Profiles
Save frequently used Bluetooth devices:
- Stores device name and address
- Quick connect to saved devices
- Supports both Classic and BLE addresses
- Address format: `XX:XX:XX:XX:XX:XX` or `XX:XX:XX:XX:XX:XX,type`

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
# Build Release configuration (auto-builds if needed)
.\launch-ucx-windows-app.cmd

# Or force rebuild
.\launch-ucx-windows-app.cmd rebuild

# Code sign the executable (creates ucx-windows-app-signed.exe in release/)
.\launch-ucx-windows-app.cmd sign YOUR_CERT_THUMBPRINT

# Launch signed version
.\launch-ucx-windows-app.cmd signed
```

### Distribution Package
For GitHub releases, distribute the signed executable:
- **File**: `examples/ucx-windows-app/release/ucx-windows-app-signed.exe`
- **Size**: ~2.5 MB (FTDI DLL embedded, icon, version info)
- **Dependencies**: None (self-contained)
- **Settings**: Auto-created on first run (`ucx-windows-app.ini`)
- **FTDI Driver**: Automatically extracted to temp folder at runtime

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


