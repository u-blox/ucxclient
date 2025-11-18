# ucxclient

This repo contains a small footprint AT command client for talking to u-blox u-connectXpress short-range modules, including:

* NORA-W36
* And other u-connectXpress modules (dynamically detected)

The client can run on both bare-metal and OS systems using a tiny porting layer (see [Porting and Configuration](#porting-and-configuration))

There are two levels of APIs included in this repo; the lower [uAtClient API](#uatclient-api) and the upper [u-connectXpress API](#u-connectxpress-api).

If you need even more features you can checkout [ubxlib](https://github.com/u-blox/ubxlib) which uses the ucxclient for communicating with the new u-connectXpress modules.

**Please note: The code in this repo is in experimental status and changes to the APIs are to be expected.**

## Cross-Platform GUI Application

This repository includes a complete **GUI application** (Windows, Linux, macOS) inspired by **u-blox s-center classic**, providing an intuitive interface for working with u-connectXpress modules. The GUI is **100% dynamically generated** from product YAML configurations - no hardcoded commands or UI layouts.

### Features

#### 🎨 s-center-Inspired Dynamic Interface

* **Fully Dynamic GUI Generation**: Entire interface auto-generated from loaded product YAML
  * Read/Write forms with parameter validation for every command
  * Type-aware input widgets (text, integer, enum dropdowns)
  * Automatic default values (e.g., MAC address = 000000000000)
  * Real-time response display per command

* **📶 Dedicated WiFi Control Tab**
  * Network scanner with sortable results table (SSID, BSSID, Channel, RSSI, Security)
  * One-click connection panel with SSID/password inputs
  * Auto-fill from scan results
  * Real-time connection status monitoring
  * Connect/Disconnect with visual feedback

* **🔵 Dedicated Bluetooth Control Tab**
  * Device discovery scanner with results table (Name, Address, RSSI, Type)
  * One-click connection panel
  * Auto-fill device address from scan
  * Real-time connection status monitoring
  * Connect/Disconnect with visual feedback

* **📋 Auto-Generated Command Group Tabs**
  * One tab per functional group (System, Security, IP, HTTP, MQTT, GATT, SPS, etc.)
  * Each command in its own labeled frame with:
    - **Read Forms**: Button + scrollable response display
    - **Write Forms**: Parameter inputs with tooltips + execute button  
    - **Action Forms**: Execute button for parameter-less commands

#### ⚙️ Core Capabilities

* **Product Selection**: Choose from available NORA products (W36, B26, W46, W56) and versions from GitHub
* **Zero Hardcoded Commands**: 100% dynamic system - all commands loaded from YAML product configurations
* **Smart Parameter Handling**: 
  * Automatic default values from YAML metadata
  * Type-based validation (integer, string, MAC address, etc.)
  * Range checking and enum constraints
* **COM Port Management**: Easy selection with automatic port detection and refresh
* **Flow Control Support**: Configurable (disabled by default for u-connectXpress modules)
* **Baud Rate Support**: 9600 to 4,000,000 bps
* **AT Command Terminal**: Direct command input with history and autocomplete
* **Real-time Logging**: All operations logged with timestamps

### Architecture

The Windows GUI uses a fully dynamic architecture where all AT commands and API mappings are loaded from product-specific YAML configurations:

```text
Product Selection → YAML Load → Dynamic Mapping Build → API Execution
      ↓                ↓              ↓                    ↓
  GitHub API    compiled_product   at_to_api_mapper   UCX API Calls
                     .yaml          (159 mappings)      (ucx_client)
```

**Key Benefits**:

* No hardcoded AT commands anywhere in the codebase
* Automatic adaptation to new product versions from GitHub
* Dynamic API mapping generation (e.g., 159 mappings for NORA-W36 v3.1.0)
* Future-proof design supporting any u-connectXpress product

**Why This Approach?**

Traditional tools hardcode AT commands and UI layouts, requiring code changes for each new product. This GUI:
* ✅ **Auto-adapts** to new products/versions from GitHub (no code changes needed)
* ✅ **Type-safe** - Uses UCX C API instead of raw AT strings
* ✅ **Self-documenting** - Parameter types, defaults, and descriptions from YAML
* ✅ **s-center familiar** - Similar workflow to u-blox s-center classic
* ✅ **Future-proof** - Supports products not yet released

### Quick Start

#### Windows

1. **Build the library**:
   ```cmd
   build_windows.cmd           REM Build Release and Debug
   ```

   **Build output**: `build/Release/ucxclient_windows.dll`

2. **Launch the GUI**:

   ```cmd
   .\launch_gui.cmd
   ```

#### Linux/macOS

1. **Build the library**:
   
   **Option A: Native build on Linux/macOS**:
   ```bash
   mkdir -p build && cd build
   cmake -DCMAKE_BUILD_TYPE=Release ..
   cmake --build .
   ```

   **Option B: Build Linux library from Windows** (using WSL or Docker):
   ```powershell
   # Using Docker (easiest):
   .\build_linux_docker.cmd
   
   # Or using WSL:
   wsl
   cd /mnt/c/u-blox/ucxclient
   mkdir -p build && cd build
   cmake ..
   cmake --build .
   ```
   
   See [BUILD_LINUX_FROM_WINDOWS.md](BUILD_LINUX_FROM_WINDOWS.md) for detailed instructions.

   **Build output**: `build/libucxclient.so` (Linux) or `build/libucxclient.dylib` (macOS)

2. **Install Python dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install python3-tk
   pip3 install pyserial
   
   # Fedora/CentOS
   sudo dnf install python3-tkinter
   pip3 install pyserial
   
   # macOS
   pip3 install pyserial  # tkinter included with Python
   ```

3. **Launch the GUI**:
   ```bash
   chmod +x launch_gui.sh
   ./launch_gui.sh
   ```

#### Cross-Platform Python Launch

All platforms can also launch directly with Python:
```bash
cd examples/python_gui
python3 launcher.py  # or python launcher.py on Windows
```

**The GUI automatically**:
* 🔍 Detects your EVK (NORA-W36, B26, W46, W56) and pre-selects port
* 📋 Shows product selector with latest version pre-selected
* 🔗 Auto-connects after you click "Load" (or press Enter)

3. **Manual Override** (if needed):
   * No EVK? Select COM port manually from dropdown
   * Different version? Choose from version dropdown
   * Press Enter or click "Load" to continue

### Typical Workflow (s-center-inspired)

**Startup (automatic)**:
1. Launch GUI → EVK auto-detected, product pre-selected
2. Press Enter or click "Load" → GUI builds, auto-connects
3. Start using features immediately

**Daily Usage**:

1. **WiFi Operations**:
   * Go to 📶 WiFi tab
   * Click "🔍 Scan Networks" to discover access points
   * Select network from table (auto-fills SSID)
   * Enter password, click "🔗 Connect"
   * Monitor connection status in real-time

2. **Bluetooth Operations**:
   * Go to 🔵 Bluetooth tab
   * Click "🔍 Scan Devices" to discover nearby devices
   * Select device from table (auto-fills address)
   * Click "🔗 Connect"
   * Monitor connection status

3. **System Configuration**:
   * Navigate to System tab
   * Find command (e.g., AT+USYUS - UART Settings)
   * Use Read button to get current value
   * Modify parameters in Write form
   * Click Write button to apply changes

4. **Advanced Commands**:
   * Each command group (IP, HTTP, MQTT, Security, etc.) has its own tab
   * Read/Write forms automatically generated with proper input types
   * Response displayed immediately below each command

### GUI Architecture Components

* **Dynamic Product GUI** (`dynamic_product_gui.py`): Core GUI builder that generates entire interface from YAML
  * Auto-creates WiFi and Bluetooth tabs with scan/connect panels
  * Generates Read/Write/Action forms for each command
  * Handles parameter validation and default values
  * Real-time command execution with response display

* **Main Window** (`main_window.py`): Application shell with connection management
  * COM port selection and configuration
  * Product selector integration
  * Terminal tab for direct AT command input
  * Logging and status monitoring

* **YAML Parser** (`yaml_parser.py`): Product configuration loader
  * GitHub API integration for dynamic product discovery
  * Command and parameter parsing from compiled_product.yaml
  * Parameter default value inference (e.g., MAC addresses = 0)
  * 159 AT-to-API mappings for NORA-W36 v3.1.0

* **AT-to-API Mapper** (`at_to_api_mapper.py`): Dynamic mapping engine
  * Builds mappings entirely from YAML (zero hardcoded commands)
  * Converts YAML api_name to UCX function names (e.g., uCxWifiStationScanDefaultBegin)
  * Determines call types (BEGIN_END vs SIMPLE) from YAML metadata
  * Parameter extraction and validation

* **UCX API Executor** (`ucx_api_executor.py`): Command execution engine
  * Translates AT commands to UCX API function calls
  * Handles BEGIN_END pattern for multi-line responses
  * Response parsing and error handling
  * ctypes integration with ucxclient C library

* **Product Selector Dialog** (`product_selector_dialog.py`): Startup configuration
  * GitHub product/version selection
  * Local YAML file loading option
  * Caching for performance

### Requirements

* Python 3.6+
* PyYAML 6.0.1 (for YAML parsing)
* tkinter (usually included with Python)
* pyserial (for COM port communication)
* Windows with available COM ports
* Built ucxclient Windows library (`ucxclient_windows.dll`)
* Optional: portenum library for advanced EVK detection

## Project Structure

```text
ucxclient/
├── README.md                      # This file
├── CMakeLists.txt                 # Main build configuration
├── inc/                           # uAtClient API headers
├── src/                           # uAtClient API implementation
├── ucx_api/                       # u-connectXpress API (C functions)
├── examples/                      # Example applications and ports
│   ├── python_gui/                # Python GUI Application
│   │   ├── launcher.py            # GUI application launcher
│   │   ├── main_window.py         # Main window with connection management
│   │   ├── dynamic_product_gui.py # ⭐ Dynamic GUI builder (s-center-inspired)
│   │   ├── yaml_parser.py         # ⭐ YAML product config parser + GitHub API
│   │   ├── at_to_api_mapper.py    # ⭐ Dynamic AT→API mapping engine
│   │   ├── ucx_api_executor.py    # ⭐ UCX API command executor
│   │   └── product_selector_dialog.py # Product/version selection dialog
├── test/                          # Unit tests
│   ├── ucx_wrapper.py             # Python wrapper for ucxclient DLL
│   ├── dynamic_wrapper.py         # Device-agnostic wrapper factory
│   └── dynamic_at_gui.py          # Legacy AT command interface
└── build/                         # Build output directory
    └── Release/
        └── ucxclient_windows.dll  # Windows library (C)
```

⭐ = Core components of the new dynamic system

## Dynamic API Mapping System

The GUI uses a sophisticated **AT-to-API mapping engine** that dynamically translates AT commands to UCX C API function calls:

### How It Works

1. **YAML Parsing**: Product configuration loaded from GitHub or local file
   * 188 commands for NORA-W36 v3.1.0
   * 159 have `api_name` mappings to UCX functions
   * Parameter definitions with types, defaults, and validation

2. **Dynamic Mapping Generation**: AT commands automatically mapped to API functions
   * `AT+UWSSC` (WiFi Scan) → `uCxWifiStationScanDefaultBegin()`
   * `AT+USYLA` (Local Address) → `uCxSystemGetLocalAddress()` / `uCxSystemSetLocalAddress()`
   * Module inference from command group (Wi-Fi → uCxWifi, System → uCxSystem, etc.)

3. **Automatic Function Name Construction**:
   ```
   YAML: api_name = "StationScanDefault"
   Group: "Wi-Fi"
   Result: uCxWifiStationScanDefaultBegin()
   ```

4. **Call Type Detection**: Automatically determines if command requires BEGIN_END pattern
   * Multiline responses → BEGIN_END pattern
   * Single response → SIMPLE call
   * All inferred from YAML metadata

### Example Mapping

From YAML for AT+UWSSC:
```yaml
api_name: "StationScanDefault"
group: "Wi-Fi"
multiline_response: true
```

Generated Mapping:
```python
APIMapping(
    at_command="AT+UWSSC",
    api_function="uCxWifiStationScanDefaultBegin",
    api_type=APICallType.BEGIN_END,
    parameters=[],
    description="WiFi station scan"
)
```

### Coverage Statistics

**NORA-W36 v3.1.0**:
* Total AT commands: 188
* Commands with API mappings: 159 (85%)
* Command groups: 15 (General, System, Wi-Fi, Bluetooth, GATT, SPS, IP, HTTP, MQTT, Security, etc.)
* Auto-generated GUI components: ~200+ (forms, buttons, inputs)

## GATT Server and Client Features

The ucxclient Windows application (`ucxclient-x64.c`) includes comprehensive **GATT Server** and **GATT Client** implementations, demonstrating advanced Bluetooth Low Energy capabilities with multiple profile support.

### GATT Server Profiles

The application can act as a GATT Server implementing multiple standard profiles simultaneously:

| Profile | Service UUID | Characteristics | Features |
| ------- | ------------ | --------------- | -------- |
| **Heart Rate Service** | 0x180D | Heart Rate Measurement (0x2A37)<br>Body Sensor Location (0x2A38) | Heart rate notifications<br>Sensor location (chest) |
| **HID over GATT (Keyboard)** | 0x1812 | Protocol Mode (0x2A4E)<br>Report Map (0x2A4B)<br>HID Information (0x2A4A)<br>HID Control Point (0x2A4C)<br>Boot Keyboard Input (0x2A22)<br>Boot Keyboard Output (0x2A32)<br>Keyboard Input (0x2A4D - Report)<br>Keyboard Output (0x2A4D - LED) | Simulates USB keyboard over BLE<br>Boot and Report protocol modes<br>LED status feedback (Num/Caps/Scroll Lock)<br>Supports Suspend/Resume commands<br>6-key rollover keyboard reports |
| **Automation IO** | 0x1815 | Digital (0x2A56)<br>Analog (0x2A58) | Digital I/O bitfield (read/write/notify)<br>Analog value (read/notify, 16-bit signed)<br>Real-time I/O state notifications |
| **Battery Service** | 0x180F | Battery Level (0x2A19) | Battery percentage notifications (0-100%) |
| **Environmental Sensing** | 0x181A | Temperature (0x2A6E)<br>Humidity (0x2A6F) | Temperature in Celsius (×100)<br>Humidity percentage (×100)<br>Environmental data notifications |
| **Nordic UART Service (NUS)** | 6E400001-B5A3-F393-E0A9-E50E24DCCA9E | TX (6E400003...)<br>RX (6E400002...) | Bidirectional UART-like data transfer<br>Notify on TX, Write on RX |
| **Serial Port Service (SPS)** | 2456E1B9-26E2-8F83-E744-F34F01E9D701 | FIFO (2456E1B9...)<br>Credits (2456E1B9...) | Flow-controlled serial data transfer<br>Credit-based flow control |
| **Location and Navigation** | 0x1819 | LN Feature (0x2A6A)<br>Location and Speed (0x2A67)<br>Position Quality (0x2A69) | GPS position notifications<br>Speed and heading data<br>Satellite count and accuracy |
| **Current Time Service** | 0x1805 | Current Time (0x2A2B) | Date/time distribution (YYYY-MM-DD HH:MM:SS)<br>Adjustable time field (day of week) |

### GATT Client Capabilities

The application can act as a GATT Client to discover and interact with remote GATT servers:

| Profile | Capabilities | Functions |
| ------- | ------------ | --------- |
| **Current Time Service** | Read current time from server<br>Subscribe to time update notifications | Discovery of CTS service<br>Automatic CCCD subscription<br>Time format parsing |
| **Environmental Sensing** | Read temperature and humidity<br>Subscribe to environmental notifications | Multi-sensor discovery<br>Decimal conversion (×100 format)<br>Real-time sensor monitoring |
| **Location and Navigation** | Read GPS position and speed<br>Subscribe to location updates<br>Read position quality | Feature discovery<br>Coordinate parsing (lat/long)<br>Speed and heading extraction |
| **Nordic UART Service** | Bidirectional data transfer<br>Subscribe to incoming data notifications | Custom UUID discovery<br>Data transmission via RX characteristic<br>Notification routing |
| **Serial Port Service** | Flow-controlled data transfer<br>Credit-based flow management | FIFO and Credits discovery<br>Credit monitoring<br>Data segmentation |
| **Battery Service** | Read battery level<br>Subscribe to battery notifications | Standard service discovery<br>Percentage parsing (0-100) |
| **Device Information** | Read manufacturer, model, serial number, firmware version | Multi-characteristic discovery<br>String parsing for device metadata |
| **Automation IO** | Read digital bitfield and analog values<br>Subscribe to I/O change notifications<br>Write digital outputs | Service discovery (0x1815)<br>Bitfield decoder (D0-D7 display)<br>16-bit analog parsing<br>Bidirectional I/O control |

### Menu Navigation

The application uses an **intuitive letter-based menu system** for easy access to GATT examples:

```
GATT Examples Menu:
  [h] Heart Rate Service
  [k] HID Keyboard Service  
  [a] Automation IO Service
  [b] Battery Service
  [e] Environmental Sensing Service
  [u] Nordic UART Service
  [s] Serial Port Service (SPS)
  [l] Location and Navigation Service
  [c] Current Time Service (Server)
  [t] Current Time Service (Client)
```

Each letter mnemonic matches the service name for easy recall.

### Automation IO Details

The **Automation IO** implementation demonstrates industrial I/O control over BLE:

**Server Capabilities**:
* **Digital Characteristic (0x2A56)**: 
  * 1-byte bitfield representing 8 digital I/O pins (D0-D7)
  * Supports Read, Write, and Notify operations
  * Clients can write to control outputs
  * Server auto-notifies subscribed clients on state changes
  * Default state: 0x00 (all outputs low)

* **Analog Characteristic (0x2A58)**:
  * 16-bit signed integer (little-endian)
  * Supports Read and Notify operations
  * Example range: 0-1000 (application-defined units)
  * Default value: 500
  * Real-time analog value monitoring

**Client Capabilities**:
* Automatic service discovery by UUID (0x1815)
* Characteristic discovery for Digital (0x2A56) and Analog (0x2A58)
* CCCD discovery and subscription (handle + 1 calculation)
* Real-time notification parsing:
  * Digital: Hex display + per-bit decoder (e.g., "D0=1, D1=0, D2=1...")
  * Analog: Little-endian SINT16 conversion
* Bidirectional control: Read current state, Write new outputs

**Example Workflow**:
1. Select `[a] Automation IO` from GATT Examples menu
2. Service is created with Digital and Analog characteristics
3. Advertising starts automatically
4. Clients can connect and discover service UUID 0x1815
5. Clients subscribe to notifications for real-time updates
6. Digital writes from client trigger immediate notification to all subscribers

### HID Keyboard Features

The **HID over GATT** implementation provides full keyboard emulation:

**Protocol Support**:
* Boot Protocol: Basic 8-byte keyboard reports (BIOS-compatible)
* Report Protocol: Full 6-key rollover with modifier keys
* Dynamic switching via Protocol Mode characteristic (0x2A4E)

**LED Feedback**:
* Keyboard Output characteristic (0x2A4D) receives LED states from host
* Logs Num Lock, Caps Lock, Scroll Lock states
* Separate Boot Keyboard Output (0x2A32) for Boot protocol mode

**Control Point**:
* Suspend command (0x00): Keyboard enters low-power state
* Exit Suspend command (0x01): Keyboard resumes normal operation

**Report Map**:
* Standard HID descriptor for USB keyboard
* Defines Input (keys), Output (LEDs), Feature reports
* 63-byte descriptor compatible with all major operating systems

### Security and Compatibility

All GATT characteristics use **U_SECURITY_READ_NONE/WRITE_NONE** for maximum compatibility during development and testing. For production deployments, update to appropriate security levels (UNAUTHENTICATED, AUTHENTICATED, or AUTHORIZED) based on requirements.

### Usage Examples

**Starting a GATT Server**:
```
Main Menu → [g] GATT Examples → [a] Automation IO Service
```
Service is created, advertising starts, ready for client connections.

**Starting a GATT Client**:
```
Main Menu → [y] GATT Client → [a] Automation IO (AIO)
```
Discovers remote AIO service, reads values, subscribes to notifications.

**Testing Bidirectional I/O**:
1. Start AIO Server on Device A
2. Start AIO Client on Device B (discovers and subscribes)
3. Client writes new digital state → Server receives write → Server notifies all subscribers
4. Real-time I/O state synchronization across devices

## uAtClient API

This API contains an AT client implementation that handles transmission of AT commands, reception and parsing of AT responses and URCs. You will find the uAtClient API in the [inc/](inc) directory.

### Example

```c
#include "u_cx_at_client.h"

static int32_t myReadFunction(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    // TODO: Implement
}

static int32_t myWriteFunction(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    // TODO: Implement
}

void main(void)
{
  int32_t status;
  static char rxBuf[1024];
  static char urcBuf[1024];
  static uCxAtClientConfig_t config = {
      .pRxBuffer = &rxBuf[0],
      .rxBufferLen = sizeof(rxBuf),
      .pUrcBuffer = &urcBuf[0],
      .urcBufferLen = sizeof(urcBuf),
      .pStreamHandle = NULL,
      .write = myWriteFunction,
      .read = myReadFunction
  };

  uCxAtClient_t client;
  uCxAtClientInit(&config, &client);

  // Example of executing AT command without any AT response

  // To execute command without any response use the uCxAtClientExecSimpleCmd(F)
  // uCxAtClientExecSimpleCmdF() uses a format string for the AT command params where
  // each character represent a parameter type (a bit like printf)
  status = uCxAtClientExecSimpleCmdF(&client, "AT+USYUS=", "dd",
                                     115200, 0, U_CX_AT_UTIL_PARAM_LAST);
  printf("'AT+USYUS=' status: %d\n", status);


  // Example of executing AT command with an AT response

  // Execute cmd 'AT+USYUS?
  uCxAtClientCmdBeginF(&client, "AT+USYUS?", "", U_CX_AT_UTIL_PARAM_LAST);

  // Read out the AT response
  int32_t baudrate;
  int32_t flowControl;
  status = uCxAtClientCmdGetRspParamsF(&client, "+USYUS:", NULL, NULL, "dd",
                                       &baudrate, &flowControl, U_CX_AT_UTIL_PARAM_LAST);
  printf("Response params: %d\n", status);

  // All AT client APIs that ends with 'Begin' (such as uCxAtClientCmdBeginF())
  // must be terminated by calling uCxAtClientCmdEnd().
  // This where you'll get the AT command status
  status = uCxAtClientCmdEnd(&client);
  printf("'AT+USYUS?' status: %d, baudrate: %d, flowControl: %d\n", status, baudrate, flowControl);
}
```

## u-connectXpress API

This API is a higher level API that that simplifies communication with new u-connectXpress u-blox modules (only NORA-W36 at the moment).
Using this API eliminates the need of manually sending AT commands to the module.
You will find the u-connectXpress API in the [ucx_api/](ucx_api) directory.

### Example

```c
#include "u_cx_at_client.h"
#include "u_cx.h"
#include "u_cx_system.h"

void main(void)
{
  int32_t status;
  uCxAtClient_t client;
  uCxHandle_t ucxHandle;

  // You need to initialize the AT client in same way as in the uAtClient API example (part of this
  // has been left out here for simplicity)
  uCxAtClientInit(&config, &client);

  uCxInit(&client, &ucxHandle);

  // This will send the "AT+USYUS=" AT command
  status = uCxSystemSetUartSettings2(&ucxHandle, 115200, 0);
  printf("uCxSystemSetUartSettings2(): %d\n", status);

  // This will send the "AT+USYUS?" AT command and parse the AT response params to &settings
  uCxSystemGetUartSettings_t settings;
  status = uCxSystemGetUartSettings(&ucxHandle, &settings);
  printf("uCxSystemGetUartSettings(): %d, baudrate: %d, flow control: %d\n",
         status, settings.baud_rate, settings.flow_control);
}
```

## Porting and Configuration

All configuration and porting config is located in [inc/u_cx_at_config.h](inc/u_cx_at_config.h).
Make a copy of this file and place it in your code base where you can modify each config to your likings.
When compiling you can specify the name of this local file with `U_CX_AT_CONFIG_FILE` (for GCC you could pass `-DU_CX_AT_CONFIG_FILE=\"my_u_cx_at_config.h\"`).

### Minimum Porting

Some things are not required for successfully running the AT client (such as U_CX_PORT_PRINTF for logging, U_CX_AT_PORT_ASSERT), but the following are required:

| Function | Description |
| -------- | ----------- |
| U_CX_PORT_GET_TIME_MS | Must return a 32 bit timestamp in milliseconds.|
| read()                | Passed as argument to uCxAtClientInit(). Should read data from UART with a timeout in millisec. Must return the number of bytes received, 0 if there are no data available within the timeout or negative value on error. |
| write()               | Passed as argument to uCxAtClientInit(). Should write data to the UART. Must return the number of actual bytes written or negative number on error. |

For systems running RTOS you will also need to port the mutex API below - for bare-metal systems you can use [examples/port/u_port_no_os.h](examples/port/u_port_no_os.h):

| Define   | Example (Posix) | Description |
| -------- | --------------- | ----------- |
| U_CX_MUTEX_HANDLE            | `pthread_mutex_t`                  | Define this to the mutex type of your system. |
| U_CX_MUTEX_CREATE(mutex)     | `pthread_mutex_init(&mutex, NULL)` | If your system need to call a function before the mutex can be used, then define it here. |
| U_CX_MUTEX_DELETE(mutex)     | `pthread_mutex_destroy(&mutex)`    | If your system has a function to de-allocate a mutex, then define it here. |
| U_CX_MUTEX_LOCK(mutex)       | `pthread_mutex_lock(&mutex)`       | Define this to corresponding "lock"/"take" function of your system. No return value is expected (any return value will be ignored). |
| U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) | `uPortMutexTryLock(&mutex, timeoutMs)`<sup>1</sup> | Define this to a function that tries to lock/take the mutex but with a timeout `timeoutMs` in millisec. Must return 0 if the mutex is successfully taken/locked and can return any negative value on timeout. |
| U_CX_MUTEX_UNLOCK(mutex)     | `pthread_mutex_unlock(&mutex)`     | Define this to corresponding "unlock"/"give" function of your system. No return value is expected (any return value will be ignored). |

<sup>1</sup> See [examples/port/u_port_posix.c](examples/port/u_port_posix.c)

### Example Ports

You will find some example ports in [examples/port](examples/port). These ports are used by the [example code](examples/README.md) and you will find more information in [examples/port/README.md](examples/port/README.md)

## Disclaimer

Copyright &#x00a9; u-blox

u-blox reserves all rights in this deliverable (documentation, software, etc.,
hereafter “Deliverable”).

u-blox grants you the right to use, copy, modify and distribute the
Deliverable provided hereunder for any purpose without fee.

THIS DELIVERABLE IS BEING PROVIDED "AS IS", WITHOUT ANY EXPRESS OR IMPLIED
WARRANTY. IN PARTICULAR, NEITHER THE AUTHOR NOR U-BLOX MAKES ANY
REPRESENTATION OR WARRANTY OF ANY KIND CONCERNING THE MERCHANTABILITY OF THIS
DELIVERABLE OR ITS FITNESS FOR ANY PARTICULAR PURPOSE.

In case you provide us a feedback or make a contribution in the form of a
further development of the Deliverable (“Contribution”), u-blox will have the
same rights as granted to you, namely to use, copy, modify and distribute the
Contribution provided to us for any purpose without fee.
