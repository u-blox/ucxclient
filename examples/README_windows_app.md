# Windows Console Application (windows_app.c)

## Overview

A **simple, straightforward C application** with text-based menu interface for u-connectXpress devices (NORA-W36).

This is a **clean alternative** to the Python GUI, avoiding complexity and reliability issues:
- ✅ **No Python** - Direct C code
- ✅ **No DLL complexity** - Standard linking
- ✅ **No GC crashes** - No garbage collection
- ✅ **Easy to debug** - Standard C debugging tools
- ✅ **Simple and maintainable** - Clear, linear code flow

## Features

### Current Implementation
1. **Device Connection**
   - Connect to COM port
   - Initialize UCX handle
   - Disconnect cleanly

2. **Basic Commands**
   - List available API commands
   - AT test (basic communication)
   - ATI9 (device information)

3. **Status Commands**
   - Bluetooth status (mode + active connections)
   - WiFi status (connection state, SSID, RSSI)

4. **Menu System**
   - Main menu
   - Bluetooth submenu
   - WiFi submenu
   - Simple text-based interface

### Planned Features
- Bluetooth scan and connect
- WiFi network scan and connect
- Configuration management (store, reset)
- System commands (reboot, factory reset)

## Building

### Using CMake
```bash
cd build
cmake ..
cmake --build . --config Debug
```

The executable will be in `build/Debug/windows_app.exe`

### Manual Build (Visual Studio)
```bash
cl /I..\inc /I..\ucx_api windows_app.c ucxclient_windows.lib
```

## Usage

### Basic Usage
```bash
# Auto-connect to default COM port (COM31)
windows_app.exe

# Specify COM port
windows_app.exe COM4
```

### Menu Navigation
```
--- Main Menu ---
  [1] Connect to device
  [2] Disconnect  
  [3] List API commands
  [4] AT test
  [5] ATI9 (device info)
  [6] Bluetooth menu
  [7] WiFi menu
  [0] Exit
```

## Architecture

### Clean and Simple
```
windows_app.c
├── main()                          // Entry point
├── connectDevice()                 // Initialize and connect
├── disconnectDevice()              // Clean shutdown
├── executeAtTest()                 // AT command
├── executeAti9()                   // Device info
├── showBluetoothStatus()           // BT status
├── showWifiStatus()                // WiFi status
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
   - WiFi scan/connect
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
========================================
  u-connectXpress Console App v1.0.0
========================================
Simple C application for NORA-W36
No Python, no DLL complexity!

Attempting to connect to COM31...
COM port opened successfully
UCX initialized successfully
Connected successfully!

--- Main Menu ---
  Connected to: COM31

  [1] Connect to device
  [2] Disconnect
  [3] List API commands
  [4] AT test (basic communication)
  [5] ATI9 (device info)
  [6] Bluetooth menu
  [7] WiFi menu
  [0] Exit

Choice: 5

--- ATI9 Device Information ---
Application Version: 4.0.0
Unique Identifier: 3032385247343431

Choice: 6

--- Bluetooth Menu ---
  [1] Show BT status
  [2] Scan for devices
  [3] List connections
  [0] Back to main menu

Choice: 1

--- Bluetooth Status ---
Bluetooth Mode: Central + Peripheral

Active Connections:
  No devices connected
```

## Philosophy

**Keep It Simple!**
- Direct C calls
- Clear error messages
- Linear execution flow
- Easy to understand and maintain
- No over-engineering

This is what we should have built from the start! 🎯
