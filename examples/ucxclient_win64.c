/*
 * Copyright 2025 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 * @brief Simple Windows Console Application for u-connectXpress
 * 
 * A straightforward C application with text-based menu for:
 * - Listing available API functions
 * - Basic AT commands (AT, ATI9)
 * - Status commands (Bluetooth, WiFi)
 * - Bluetooth operations (scan, connect)
 * - WiFi operations (scan, connect)
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>     // For tolower()
#include <stdbool.h>
#include <time.h>      // For time() to generate unique device names
// Winsock2 must be included BEFORE windows.h to avoid conflicts
#include <winsock2.h>  // For socket functions
#include <ws2tcpip.h>  // For getaddrinfo and related functions
#include <windows.h>  // For Sleep() and CreateFile()
#include <conio.h>  // For _kbhit() and _getch()
#include <winhttp.h>  // For HTTP client to fetch from GitHub
#include <setupapi.h>  // For device enumeration
#include <devguid.h>   // For GUID_DEVCLASS_PORTS
#include <regstr.h>    // For registry string constants

#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "setupapi.lib")
#pragma comment(lib, "ws2_32.lib")  // Windows Sockets library

// FTD2XX library dynamic loading
// Note: FT_HANDLE in FTD2XX is actually a pointer type despite being typedef'd as ULONG
typedef PVOID FT_HANDLE;
typedef ULONG FT_STATUS;

#define FT_OK 0
#define FT_LIST_NUMBER_ONLY     0x80000000
#define FT_LIST_BY_INDEX        0x40000000
#define FT_LIST_ALL             0x20000000
#define FT_OPEN_BY_SERIAL_NUMBER 1
#define FT_OPEN_BY_DESCRIPTION   2

typedef FT_STATUS (WINAPI *PFN_FT_ListDevices)(PVOID, PVOID, DWORD);
typedef FT_STATUS (WINAPI *PFN_FT_Open)(int, FT_HANDLE*);
typedef FT_STATUS (WINAPI *PFN_FT_OpenEx)(PVOID, DWORD, FT_HANDLE*);
typedef FT_STATUS (WINAPI *PFN_FT_GetComPortNumber)(FT_HANDLE, LPLONG);
typedef FT_STATUS (WINAPI *PFN_FT_Close)(FT_HANDLE);

static HMODULE gFtd2xxModule = NULL;
static PFN_FT_ListDevices gpFT_ListDevices = NULL;
static PFN_FT_Open gpFT_Open = NULL;
static PFN_FT_OpenEx gpFT_OpenEx = NULL;
static PFN_FT_GetComPortNumber gpFT_GetComPortNumber = NULL;
static PFN_FT_Close gpFT_Close = NULL;

// Disable MSVC warnings for auto-generated UCX API headers
// C4200: zero-sized array in struct (valid C99/C11 feature)
// C4201: nameless struct/union (valid C11 feature)
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4200 4201)
#endif

// Include ucxclient headers
#include "u_cx_at_client.h"
#include "u_cx_at_params.h"
#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_general.h"
#include "u_cx_system.h"
#include "u_cx_bluetooth.h"
#include "u_cx_wifi.h"
#include "u_cx_socket.h"
#include "u_cx_mqtt.h"
#include "u_cx_http.h"
#include "u_cx_network_time.h"
#include "u_cx_security.h"
#include "u_cx_diagnostics.h"
#include "u_cx_gatt_client.h"
#include "u_cx_gatt_server.h"
#include "u_cx_sps.h"
#include "u_cx_firmware_update.h"
#include "qrcodegen/qrcodegen.h"

// Port layer
#include "port/u_port.h"

// Bluetooth SIG name databases
#include "bluetooth-sig/bt_company_identifiers.h"
#include "bluetooth-sig/bt_service_uuids.h"
#include "bluetooth-sig/bt_appearance_values.h"
#include "bluetooth-sig/bt_characteristic_uuids.h"
#include "bluetooth-sig/bt_descriptor_uuids.h"
#include "bluetooth-sig/bt_units.h"

// Application version
#define APP_VERSION "1.0.0"

// Settings file (will be placed next to executable)
#define SETTINGS_FILENAME "ucxclient_win64_settings.ini"

// Buffer size constants
#define MAX_DATA_BUFFER 1000

// URC Event flags
#define URC_FLAG_NETWORK_UP         (1 << 0)  // Wi-Fi Station Network UP (IP assigned)
#define URC_FLAG_NETWORK_DOWN       (1 << 1)  // Wi-Fi Station Network DOWN (IP lost)
#define URC_FLAG_SOCK_CONNECTED     (1 << 2)
#define URC_FLAG_SOCK_DATA          (1 << 3)
#define URC_FLAG_SPS_CONNECTED      (1 << 4)
#define URC_FLAG_SPS_DISCONNECTED   (1 << 5)
#define URC_FLAG_SPS_DATA           (1 << 6)
#define URC_FLAG_STARTUP            (1 << 7)
#define URC_FLAG_PING_COMPLETE      (1 << 8)
#define URC_FLAG_WIFI_LINK_UP       (1 << 9)  // Wi-Fi Link UP (connected to AP)
#define URC_FLAG_WIFI_LINK_DOWN     (1 << 10) // Wi-Fi Link DOWN (disconnected from AP)
#define URC_FLAG_MQTT_CONNECTED     (1 << 11) // MQTT connected (+UEMQC)
#define URC_FLAG_MQTT_DATA          (1 << 12) // MQTT data received (+UEMQDA)
#define URC_FLAG_BT_CONNECTED       (1 << 13) // Bluetooth connected (+UEBTC)
#define URC_FLAG_BT_DISCONNECTED    (1 << 14) // Bluetooth disconnected (+UEBTDC)

// Global handles
static uCxAtClient_t gUcxAtClient;
static uCxHandle_t gUcxHandle;
static bool gUcxConnected = false;  // UCX client connection status (COM port)

// Socket tracking
static int32_t gCurrentSocket = -1;

// Bluetooth connection tracking
#define MAX_BT_CONNECTIONS 7  // u-connectXpress supports up to 7 concurrent BT connections
typedef struct {
    int32_t handle;
    uBtLeAddress_t address;
    bool active;
} BtConnection_t;

static BtConnection_t gBtConnections[MAX_BT_CONNECTIONS];
static int gBtConnectionCount = 0;

// GATT Client tracking
#define MAX_GATT_SERVICES 20
#define MAX_GATT_CHARACTERISTICS 50

typedef struct {
    int32_t connHandle;
    int32_t startHandle;
    int32_t endHandle;
    uint8_t uuid[16];
    int32_t uuidLength;
    char name[64];  // Optional friendly name
} GattService_t;

typedef struct {
    int32_t connHandle;
    int32_t serviceIndex;  // Index into gGattServices array
    int32_t valueHandle;
    int32_t properties;
    uint8_t uuid[16];
    int32_t uuidLength;
    char name[64];  // Optional friendly name
} GattCharacteristic_t;

static GattService_t gGattServices[MAX_GATT_SERVICES];
static int gGattServiceCount = 0;
static GattCharacteristic_t gGattCharacteristics[MAX_GATT_CHARACTERISTICS];
static int gGattCharacteristicCount = 0;
static int32_t gCurrentGattConnHandle = -1;  // Currently selected GATT connection
static int gLastCharacteristicIndex = -1;    // Last used characteristic index

// GATT Server tracking
static int32_t gGattServerServiceHandle = -1;      // Last created service handle
static int32_t gGattServerCharHandle = -1;         // Last created characteristic handle
static int32_t gHeartbeatServiceHandle = -1;       // Heartbeat service handle
static int32_t gHeartbeatCharHandle = -1;          // Heartbeat characteristic handle
static int32_t gHeartbeatCccdHandle = -1;          // Heartbeat CCCD handle
static bool gHeartbeatNotificationsEnabled = false; // Client subscribed to notifications
static uint8_t gHeartbeatCounter = 60;             // Heartbeat BPM value
static HANDLE gHeartbeatThread = NULL;             // Background thread handle
static volatile bool gHeartbeatThreadRunning = false; // Thread control flag

// Unified GATT Server notification system
static HANDLE gGattNotificationThread = NULL;      // Unified notification thread
static volatile bool gGattNotificationThreadRunning = false;
static bool gBatteryNotificationsEnabled = false;  // Battery notifications enabled
static uint8_t gBatteryLevel = 100;                // Battery level percentage
static bool gCtsServerNotificationsEnabled = false; // CTS notifications enabled
static bool gEssServerTempNotificationsEnabled = false; // ESS Temperature notifications enabled
static bool gEssServerHumNotificationsEnabled = false;  // ESS Humidity notifications enabled
static int16_t gEssServerTempValue = 2345;         // Temperature in 0.01°C (23.45°C)
static uint16_t gEssServerHumValue = 4500;         // Humidity in 0.01% (45.00%)
static int32_t gEnvServerTempCccdHandle = -1;      // ESS Temperature CCCD handle
static int32_t gEnvServerHumCccdHandle = -1;       // ESS Humidity CCCD handle

// HID over GATT (HoG) tracking
static int32_t gHidServiceHandle = -1;             // HID Service (0x1812)
static int32_t gHidInfoHandle = -1;                // HID Information characteristic
static int32_t gHidReportMapHandle = -1;           // Report Map characteristic
static int32_t gHidProtocolModeHandle = -1;        // Protocol Mode characteristic
static int32_t gHidBootKbdInputHandle = -1;        // Boot Keyboard Input Report characteristic
static int32_t gHidBootKbdCccdHandle = -1;         // Boot Keyboard Input CCCD handle
static int32_t gHidBootKbdOutputHandle = -1;       // Boot Keyboard Output Report characteristic
static int32_t gHidKeyboardInputHandle = -1;       // Keyboard Input Report characteristic
static int32_t gHidKeyboardCccdHandle = -1;        // Keyboard Input CCCD handle
static int32_t gHidKeyboardOutputHandle = -1;      // Keyboard Output Report characteristic
static int32_t gHidMediaInputHandle = -1;          // Media Control Input Report characteristic
static int32_t gHidMediaCccdHandle = -1;           // Media Input CCCD handle
static int32_t gHidControlPointHandle = -1;        // HID Control Point characteristic
static int32_t gBatteryServiceHandle = -1;         // Battery Service (0x180F)
static int32_t gBatteryLevelHandle = -1;           // Battery Level characteristic
static int32_t gBatteryCccdHandle = -1;            // Battery Level CCCD handle
static int32_t gGattServiceChangedHandle = -1;     // GATT Service Changed characteristic
static bool gHidBootKbdNotificationsEnabled = false;  // Boot Keyboard notifications enabled
static bool gHidKeyboardNotificationsEnabled = false; // HID Keyboard notifications enabled
static bool gHidMediaNotificationsEnabled = false;    // HID Media notifications enabled
static bool gUseBootKeyboard = false;              // True if Boot Keyboard is preferred for sending

// GATT Client notification tracking - handles discovered from remote servers
static int gHeartRateValueHandle = -1;             // Heart Rate Measurement (0x2A37)
static int gUartTxValueHandle = -1;                // UART TX from server (Notify)

// GATT Client - Current Time Service (CTS)
static int gCtsClientServiceIndex      = -1;
static int gCtsClientTimeCharIndex     = -1;
static int gCtsClientTimeValueHandle   = -1;
static int gCtsClientTimeCccdHandle    = -1;

// GATT Server - Automation IO Service (AIO) (0x1815)
static int32_t gAioServerServiceHandle      = -1;
static int32_t gAioServerDigitalCharHandle  = -1;   // Digital characteristic (0x2A56)
static int32_t gAioServerDigitalCccdHandle  = -1;
static uint8_t gAioServerDigitalState       = 0x00; // bit0 = "LED 0" (0=off, 1=on)
static int32_t gAioServerAnalogCharHandle   = -1;   // Analog characteristic (0x2A58)
static int32_t gAioServerAnalogCccdHandle   = -1;
static uint16_t gAioServerAnalogValue       = 500;  // Example: 0..1000 range (e.g. 0.0–10.00V)

// GATT Client - Automation IO (AIO) tracking
static int gAioClientServiceIndex      = -1;
static int gAioClientDigitalCharIndex  = -1;
static int gAioClientDigitalValueHandle= -1;
static int gAioClientDigitalCccdHandle = -1;
static int gAioClientAnalogCharIndex   = -1;
static int gAioClientAnalogValueHandle = -1;
static int gAioClientAnalogCccdHandle  = -1;

// GATT Client - Environmental Sensing Service (ESS)
static int gEssClientServiceIndex       = -1;
static int gEssClientTempCharIndex      = -1;
static int gEssClientTempValueHandle    = -1;
static int gEssClientTempCccdHandle     = -1;
static int gEssClientHumCharIndex       = -1;
static int gEssClientHumValueHandle     = -1;
static int gEssClientHumCccdHandle      = -1;

// GATT Client - Location & Navigation Service (LNS)
static int gLnsClientServiceIndex  = -1;
static int gLnsClientLocCharIndex  = -1;
static int gLnsClientLocValueHandle = -1;
static int gLnsClientLocCccdHandle  = -1;

// GATT Client - UART Service (NUS-style)
static int gUartClientServiceIndex = -1;
static int gUartClientTxCharIndex  = -1;
static int gUartClientTxValueHandle = -1;
static int gUartClientTxCccdHandle  = -1;
static int gUartClientRxCharIndex  = -1;
static int gUartClientRxValueHandle = -1;

// GATT Client - u-blox Serial Port Service (SPS)
static int gSpsClientServiceIndex = -1;
static int gSpsClientFifoCharIndex  = -1;
static int gSpsClientFifoValueHandle = -1;
static int gSpsClientFifoCccdHandle  = -1;
static int gSpsClientCreditsCharIndex  = -1;
static int gSpsClientCreditsValueHandle = -1;
static int gSpsClientCreditsCccdHandle   = -1;
static int gSpsClientLocalCredits = 10;   // Credits available to send
static int gSpsClientRemoteCredits = 0;  // Credits received from remote
static bool gSpsClientFlowControlEnabled = false; // Credit-based flow control

// GATT Client - Battery Service (BAS)
static int gBasClientServiceIndex = -1;
static int gBasClientCharIndex    = -1;
static int gBasClientValueHandle  = -1;
static int gBasClientCccdHandle   = -1;

// GATT Client - Device Information Service (DIS)
static int gDisClientServiceIndex = -1;
static int gDisClientCharCount = 0;
static struct {
    uint16_t uuid;
    int32_t  handle;
} gDisClientChars[8];

// DIS characteristic UUIDs
static const uint16_t kDisCharUuids[] = {
    0x2A29, // Manufacturer Name
    0x2A24, // Model Number
    0x2A25, // Serial Number
    0x2A26, // Firmware Revision
    0x2A27, // Hardware Revision
    0x2A28, // Software Revision
    0x2A23  // System ID
};
static const int kDisCharUuidCount = sizeof(kDisCharUuids) / sizeof(kDisCharUuids[0]);

// GATT Server - Location & Navigation Service
static int32_t gLnsServerServiceHandle = -1;
static int32_t gLnsServerLocSpeedHandle = -1;

// GATT Server - UART Service (NUS-style)
static int32_t gUartServerServiceHandle = -1;
static int32_t gUartServerTxHandle = -1;
static int32_t gUartServerTxCccdHandle = -1;
static int32_t gUartServerRxHandle = -1;
static bool gUartServerTxNotificationsEnabled = false;

// GATT Server - Serial Port Service (SPS - u-blox)
static int32_t gSpsServerServiceHandle = -1;
static int32_t gSpsServerFifoHandle = -1;
static int32_t gSpsServerFifoCccdHandle = -1;
static int32_t gSpsServerCreditsHandle = -1;
static int32_t gSpsServerCreditsCccdHandle = -1;
static bool gSpsServerFifoNotifyEnabled = false;
static bool gSpsServerCreditsNotifyEnabled = false;
static bool gSpsServerFlowControlActive = false;
static int gSpsServerRemoteCredits = 0;  // Credits received from client

// GATT Server - Environmental Sensing Service
static int32_t gEnvServerServiceHandle = -1;
static int32_t gEnvServerTempHandle = -1;
static int32_t gEnvServerHumHandle = -1;

// GATT Server - Current Time Service (CTS Server)
static int32_t gCtsServerServiceHandle    = -1;
static int32_t gCtsServerTimeValueHandle  = -1;
static int32_t gCtsServerTimeCccdHandle   = -1;
static ULONGLONG gCtsServerLastTick = 0;

// 128-bit UUIDs for UART service & chars (Nordic-like)
static const uint8_t kUartServiceUuid[16] = {
    0x6E,0x40,0x00,0x01,0xB5,0xA3,0xF3,0x93,
    0xE0,0xA9,0xE5,0x0E,0x24,0xDC,0xCA,0x9E
};
static const uint8_t kUartTxCharUuid[16] = {
    0x6E,0x40,0x00,0x03,0xB5,0xA3,0xF3,0x93,
    0xE0,0xA9,0xE5,0x0E,0x24,0xDC,0xCA,0x9E
};
static const uint8_t kUartRxCharUuid[16] = {
    0x6E,0x40,0x00,0x02,0xB5,0xA3,0xF3,0x93,
    0xE0,0xA9,0xE5,0x0E,0x24,0xDC,0xCA,0x9E
};

// 128-bit UUIDs for u-blox SPS service & characteristics
static const uint8_t kSpsServiceUuid[16] = {
    0x24,0x56,0xe1,0xb9,0x26,0xe2,0x8f,0x83,
    0xe7,0x44,0xf3,0x4f,0x01,0xe9,0xd7,0x01
};
static const uint8_t kSpsFifoCharUuid[16] = {
    0x24,0x56,0xe1,0xb9,0x26,0xe2,0x8f,0x83,
    0xe7,0x44,0xf3,0x4f,0x03,0xe9,0xd7,0x01
};
static const uint8_t kSpsCreditsCharUuid[16] = {
    0x24,0x56,0xe1,0xb9,0x26,0xe2,0x8f,0x83,
    0xe7,0x44,0xf3,0x4f,0x04,0xe9,0xd7,0x01
};

// Bluetooth state tracking
static bool gBluetoothAdvertising = false;         // Advertising/discoverable state
static char gBluetoothLocalAddress[18] = "";       // Local BT address (XX:XX:XX:XX:XX:XX)
static char gBluetoothPairedDevice[18] = "";       // Last paired device address

// Wi-Fi status tracking
static bool gWifiConnected = false;                // Wi-Fi connection status
static char gWifiSsid[64] = "";                    // Connected network SSID
static char gWifiIpAddress[16] = "";               // Device IP address
static int gWifiChannel = 0;                       // Wi-Fi channel (1-13)

// Settings (saved to file)
static char gComPort[16] = "COM31";           // Default COM port
static char gLastDeviceModel[64] = "";        // Last connected device model
static char gRemoteAddress[128] = "";         // Last remote address/hostname
static char gCombainApiKey[128] = "";         // Combain API key (obfuscated in settings)

// WiFi Profile Management (up to 10 saved networks)
typedef struct {
    char name[32];              // Profile name (e.g., "Office", "Home", "Lab")
    char ssid[64];              // Network SSID
    char password[64];          // Network password
    char ipPrefix[16];          // Expected IP prefix (first 3 octets, e.g., "10.12.4", "192.168.0")
} WiFiProfile_t;

#define MAX_WIFI_PROFILES 10
static WiFiProfile_t gWifiProfiles[MAX_WIFI_PROFILES];
static int gWifiProfileCount = 0;
static int gActiveProfileIndex = -1;  // Currently selected profile (-1 = none/manual)

// Dynamic firmware path storage per product
typedef struct {
    char productName[64];      // e.g., "NORA-W36", "NORA-B26"
    char lastFirmwarePath[256]; // Last used firmware path for this product
} ProductFirmwarePath_t;

#define MAX_PRODUCT_PATHS 10
static ProductFirmwarePath_t gProductFirmwarePaths[MAX_PRODUCT_PATHS];
static int gProductFirmwarePathCount = 0;

// Device information (populated after connection)
static char gDeviceModel[64] = "";            // Device model (e.g., "NORA-W36")
static char gDeviceFirmware[64] = "";         // Firmware version (e.g., "3.1.0")

// URC event handling
static U_CX_MUTEX_HANDLE gUrcMutex;
static volatile uint32_t gUrcEventFlags = 0;

// Passkey entry request tracking (set by URC, handled in main loop)
static volatile bool gPasskeyRequestPending = false;
static uBtLeAddress_t gPasskeyRequestAddress;

// Ping test results
static volatile int32_t gPingSuccess = 0;
static volatile int32_t gPingFailed = 0;
static volatile int32_t gPingAvgTime = 0;
#define MAX_PING_TIMES 10
static volatile int32_t gPingTimes[MAX_PING_TIMES];
static volatile int32_t gPingCount = 0;

// iPerf state tracking
static volatile bool gIperfRunning = false;
static char gIperfOutputBuffer[1024] = "";

// Reboot timing
static volatile ULONGLONG gStartupTimestamp = 0;
static volatile ULONGLONG gLastStartupConfigTime = 0;

// Menu state
typedef enum {
    MENU_MAIN,
    MENU_BLUETOOTH,
    MENU_BLUETOOTH_FUNCTIONS,
    MENU_GATT_EXAMPLES,
    MENU_WIFI,
    MENU_WIFI_AP,
    MENU_WIFI_FUNCTIONS,
    MENU_SOCKET,
    MENU_SPS,
    MENU_GATT_CLIENT,
    MENU_GATT_SERVER,
    MENU_HID,
    MENU_MQTT,
    MENU_HTTP,
    MENU_LOCATION,
    MENU_DIAGNOSTICS,
    MENU_SECURITY_TLS,
    MENU_FIRMWARE_UPDATE,
    MENU_API_LIST,
    MENU_AT_TERMINAL,
    MENU_EXIT
} MenuState_t;

static MenuState_t gMenuState = MENU_MAIN;

// API Command structure for dynamic listing
typedef struct {
    char atCommand[128];
    char ucxApi[128];
    char description[256];
    char chapter[128];  // Chapter/category name (e.g., "General", "WiFi", "Bluetooth")
} ApiCommand_t;

static ApiCommand_t *gApiCommands = NULL;
static int gApiCommandCount = 0;

// Settings file path (next to executable)
static char gSettingsFilePath[MAX_PATH] = "";

// ============================================================================
// FUNCTION INDEX
// ============================================================================
// 
// UTILITY & INITIALIZATION
//   - main()                           Application entry point
//   - getExecutableDirectory()         Get path to executable
//
// UI & MENU SYSTEM
//   - printHeader()                    Display welcome banner
//   - printWelcomeGuide()              First-time user guide
//   - printHelp()                      Comprehensive help system
//   - printMenu()                      Display menu for current state
//   - handleUserInput()                Main input dispatcher
//   - bluetoothMenu()                  Bluetooth operations menu
//   - wifiMenu()                       WiFi operations menu
//   - socketMenu()                     Socket operations menu
//   - mqttMenu()                       MQTT menu (placeholder)
//   - httpMenu()                       HTTP menu (placeholder)
//   - securityTlsMenu()                Security/TLS menu
//   - bluetoothFunctionsMenu()         BT functions submenu
//   - wifiFunctionsMenu()              WiFi functions submenu
//   - gattClientMenu()                 GATT client submenu
//   - gattServerMenu()                 GATT server submenu
//
// DEVICE CONNECTION & MANAGEMENT
//   - ucxclientConnect()               Connect to UCX device via COM port
//   - quickConnectToLastDevice()       Auto-connect to last device
//   - ucxclientDisconnect()            Disconnect and cleanup
//   - listAvailableComPorts()          Enumerate COM ports with device info
//   - selectComPortFromList()          Interactive COM port selection
//   - initFtd2xxLibrary()              Load FTDI D2XX DLL
//   - getFtdiDeviceInfo()              Get device info via FTDI library
//   - getDeviceInfoFromSetupAPI()      Get device info via Windows API
//   - getComPortFriendlyName()         Get friendly name for COM port
//
// AT COMMANDS & DIAGNOSTICS
//   - executeAtTest()                  Basic AT command test
//   - executeAti9()                    Query device information
//   - executeModuleReboot()            Reboot module with timing
//
// BLUETOOTH OPERATIONS
//   - showBluetoothStatus()            Display BT connection status
//   - bluetoothScan()                  Scan for BT devices
//   - bluetoothConnect()               Connect to BT device
//
// WIFI OPERATIONS
//   - showWifiStatus()                 Display WiFi status
//   - wifiScan()                       Scan WiFi networks with analysis
//   - wifiConnect()                    Connect to WiFi network
//   - wifiDisconnect()                 Disconnect from WiFi
//   - testConnectivity()               Test gateway/internet connectivity
//
// SOCKET OPERATIONS (TCP/UDP)
//   - socketCreateTcp()                Create TCP socket
//   - socketCreateUdp()                Create UDP socket
//   - socketConnect()                  Connect socket
//   - socketSendData()                 Send data on socket
//   - socketReadData()                 Read data from socket
//   - socketClose()                    Close socket
//   - socketListStatus()               List all sockets
//
// SPS (SERIAL PORT SERVICE)
//   - spsEnableService()               Enable SPS service
//   - spsConnect()                     Connect SPS on BT connection
//   - spsSendData()                    Send data via SPS
//   - spsReadData()                    Read data from SPS
//
// GATT CLIENT
//   - gattClientDiscoverServices()     Discover GATT services
//   - gattClientReadCharacteristic()   Read characteristic
//   - gattClientWriteCharacteristic()  Write characteristic
//
// GATT SERVER
//   - gattServerAddService()           Add GATT service
//   - gattServerSetCharacteristic()    Set characteristic value
//
// FIRMWARE UPDATE
//   - downloadFirmwareFromGitHub()     Download firmware from GitHub
//   - downloadFirmwareFromGitHubInteractive()  Interactive download
//   - extractProductFromFilename()     Parse product from filename
//   - extractZipFile()                 Extract ZIP archive
//   - saveBinaryFile()                 Save binary to disk
//   - firmwareUpdateProgress()         Progress callback
//   - getProductFirmwarePath()         Get saved firmware path
//   - setProductFirmwarePath()         Save firmware path
//
// API COMMANDS (GITHUB INTEGRATION)
//   - listAllApiCommands()             Display API command list
//   - fetchApiCommandsFromGitHub()     Download command list from GitHub
//   - fetchLatestVersion()             Get latest firmware version
//   - parseYamlCommands()              Parse YAML API definitions
//   - freeApiCommands()                Free API command memory
//
// HTTP CLIENT HELPERS
//   - httpGetRequest()                 HTTP GET (text)
//   - httpGetBinaryRequest()           HTTP GET (binary)
//
// SETTINGS MANAGEMENT
//   - loadSettings()                   Load settings from INI file
//   - saveSettings()                   Save settings to INI file
//   - obfuscatePassword()              Simple password obfuscation
//   - deobfuscatePassword()            Decode obfuscated password
//
// URC (UNSOLICITED RESULT CODE) HANDLERS
//   - wifiStationNetworkUpUrc()        WiFi network up (IP assigned)
//   - wifiStationNetworkDownUrc()      WiFi network down (IP lost)
//   - wifiLinkConnectedUrc()           WiFi link connected (AP)
//   - wifiLinkDisconnectedUrc()        WiFi link disconnected (AP)
//   - socketDataAvailableUrc()         Socket data received
//   - socketConnectUrc()               Socket connected
//   - spsConnectUrc()                  SPS connected
//   - spsDisconnectUrc()               SPS disconnected
//   - spsDataAvailableUrc()            SPS data available
//   - systemStartupUrc()               System startup notification
//   - pingResponseUrc()                Ping response received
//   - pingCompleteUrc()                Ping test complete
//
// ============================================================================

// Forward declarations
static void getExecutableDirectory(char *buffer, size_t bufferSize);
static void printHeader(void);
static void printWelcomeGuide(void);
static void printHelp(void);
static void printMenu(void);
static void handleUserInput(void);
static bool ucxclientConnect(const char *comPort);
static bool quickConnectToLastDevice(void);
static void ucxclientDisconnect(void);
static void moduleStartupInit(void);
static void listAvailableComPorts(char *recommendedPort, size_t recommendedPortSize, 
                                   char *recommendedDevice, size_t recommendedDeviceSize);
static char* selectComPortFromList(const char *recommendedPort);
static void listAllApiCommands(void);
static void firmwareUpdateProgress(size_t totalBytes, size_t bytesTransferred, 
                                   int32_t percentComplete, void *pUserData);
static bool fetchApiCommandsFromGitHub(const char *product, const char *version);
static void parseYamlCommands(const char *yamlContent);
static void freeApiCommands(void);
static char* fetchLatestVersion(const char *product);
static char* httpGetRequest(const wchar_t *server, const wchar_t *path);
static char* httpGetBinaryRequest(const wchar_t *server, const wchar_t *path, size_t *outSize);
static bool downloadFirmwareFromGitHub(const char *product, char *downloadedPath, size_t pathSize);
static const char* getProductFirmwarePath(const char *productName);
static void setProductFirmwarePath(const char *productName, const char *firmwarePath);
static char* extractProductFromFilename(const char *filename);
static bool downloadFirmwareFromGitHubInteractive(char *downloadedPath, size_t pathSize);
static bool extractZipFile(const char *zipPath, const char *destFolder);
static bool saveBinaryFile(const char *filepath, const char *data, size_t size);
static void enableAllUrcs(void);
static void disableAllUrcs(void);
static void executeAtTest(void);
static void executeAtTerminal(void);
static void executeAti9(void);
static void executeModuleReboot(void);
static void showLegacyAdvertisementStatus(void);
static bool ensureLegacyAdvertisementEnabled(void);
static void showGattServerConnectionInfo(void);
static void showBluetoothStatus(void);
static void showWifiStatus(void);
static void bluetoothMenu(void);
static void bluetoothScan(void);
static void bluetoothConnect(void);
static void bluetoothDisconnect(void);
static void bluetoothSyncConnections(void);
static void syncGattConnectionOnly(void);
static void decodeAdvertisingData(const uint8_t *data, size_t dataLen);
static void wifiMenu(void);
static void wifiScan(void);
static void wifiConnect(void);
static void wifiDisconnect(void);
static void wifiManageProfiles(void);
static void wifiSaveProfile(const char *name, const char *ssid, const char *password, const char *ipPrefix);
static int wifiSuggestProfile(void);
static void wifiListProfiles(void);
static void wifiApEnable(void);
static void wifiApDisable(void);
static void wifiApShowStatus(void);
static void wifiApGenerateQrCode(void);
static void wifiApConfigure(void);
static void getCurrentPCIPAddress(char *ipBuffer, size_t bufferSize);
static void loadSettings(void);
static void saveSettings(void);
static void obfuscatePassword(const char *input, char *output, size_t outputSize);
static void deobfuscatePassword(const char *input, char *output, size_t outputSize);
static void tlsSetVersion(void);
static void tlsShowConfig(void);
static void tlsListCertificates(void);
static void tlsShowCertificateDetails(void);
static void tlsUploadCertificate(void);
static void tlsDeleteCertificate(void);
static void socketCreateTcp(void);
static void socketCreateUdp(void);
static void socketConnect(void);
static void socketSendData(void);
static void socketReadData(void);
static void socketClose(void);
static void socketCloseByHandle(void);
static void socketListStatus(void);
static void spsEnableService(void);
static void spsConnect(void);
static void spsSendData(void);
static void spsReadData(void);
static void gattClientMenu(void);
static void gattClientDiscoverServices(void);
static void gattClientDiscoverCharacteristics(void);
static void gattClientReadCharacteristic(void);
static void gattClientWriteCharacteristic(void);
static void gattServerMenu(void);
static void gattServerAddService(void);
static void gattServerAddCharacteristic(void);
static void gattServerSetCharacteristic(void);
static void gattServerSendNotification(void);
static void gattServerSetupHeartbeat(void);
static void gattServerSetupHidKeyboard(void);
static void gattServerSetupBatteryOnly(void);
static void gattServerSetupEnvSensing(void);
static void gattServerSetupUartService(void);
static void gattServerSetupSpsService(void);
static void gattServerSetupLocationService(void);
static void gattClientReadCurrentTime(void);
static void gattServerSetupCtsService(void);
static void ctsNotifyIfEnabled(void);
static size_t ctsBuildTimePayload(uint8_t out[10]);
static void gattServerSendKeyPress(void);
static void gattServerSendMediaControl(void);
static void gattServerSendHelloWorld(void);
static void bluetoothSetAdvertising(void);
static void bluetoothShowStatus(void);
static void bluetoothSetPairing(void);
static void bluetoothListBondedDevices(void);
static void bluetoothPairUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, uBtBondStatus_t bond_status);
static void bluetoothUserConfirmationUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value);
static void bluetoothPasskeyDisplayUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t passkey);
static void bluetoothPasskeyRequestUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr);
static void bluetoothPhyUpdateUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t phy_status, int32_t tx_phy, int32_t rx_phy);
static void gattServerSetupHeartbeat(void);
static void gattServerSetupHidKeyboard(void);
static void gattServerSendKeyPress(void);
static void gattServerSendMediaControl(void);
static void gattServerCharWriteUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *value, uGattServerOptions_t options);
static DWORD WINAPI heartbeatThread(LPVOID lpParam);
static DWORD WINAPI gattNotificationThread(LPVOID lpParam);
static void gattClientNotificationUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t value_handle, uByteArray_t *hex_data);
static void handleHeartRateNotification(int connHandle, const uint8_t *data, size_t len);
static void handleUartRxNotification(int connHandle, const uint8_t *data, size_t len);
static void ctsParseAndPrint(const uint8_t *data, size_t len);
static bool gattClientFindCtsHandles(void);
static void gattClientReadCtsTime(void);
static void gattClientSubscribeCts(void);
static void gattClientCtsExample(void);
static void essParseTemperature(const uint8_t *data, size_t len);
static void essParseHumidity(const uint8_t *data, size_t len);
static bool gattClientFindEssHandles(void);
static void gattClientReadEssValues(void);
static void gattClientSubscribeEss(void);
static void gattClientEssExample(void);
static void lnsParseLocation(const uint8_t *data, size_t len);
static bool gattClientFindLnsHandles(void);
static void gattClientReadLns(void);
static void gattClientSubscribeLns(void);
static void gattClientLnsExample(void);
static void aioParseDigital(const uint8_t *data, size_t len);
static void aioParseAnalog(const uint8_t *data, size_t len);
static bool gattClientFindAioHandles(void);
static void gattClientReadAioValues(void);
static void gattClientSubscribeAio(void);
static void gattClientAioExample(void);
static void uartParseRxData(const uint8_t *data, size_t len);
static bool gattClientFindUartHandles(void);
static void gattClientSubscribeUart(void);
static void gattClientUartSend(const char *msg);
static void gattClientUartExample(void);
static void spsParseFifoData(const uint8_t *data, size_t len);
static void spsParseCredits(const uint8_t *data, size_t len);
static bool gattClientFindSpsHandles(void);
static void gattClientSubscribeSps(bool enableFlowControl);
static void gattClientSpsSend(const char *msg);
static void gattClientSpsSendCredits(int8_t credits);
static void gattClientSpsExample(void);
static void basParseBatteryLevel(const uint8_t *data, size_t len);
static bool gattClientFindBasHandles(void);
static void gattClientReadBattery(void);
static void gattClientSubscribeBattery(void);
static void gattClientBasExample(void);
static bool gattClientFindDisHandles(void);
static void disReadAndPrint(uint16_t uuid, int32_t handle);
static void gattClientDisExample(void);
static void bluetoothFunctionsMenu(void);
static void wifiFunctionsMenu(void);
static void socketMenu(void);
static void mqttMenu(void);
static void httpMenu(void);
static void diagnosticsMenu(void);
static void securityTlsMenu(void);
static void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel);

// Diagnostics functions
static void pingExample(void);
static void iperfClientExample(void);
static void iperfServerExample(void);
static void iperfStopExample(void);
static void dnsLookupExample(void);
static void testConnectivityWrapper(void);

// URC handlers for ping and iperf
static void pingResponseUrc(struct uCxHandle *puCxHandle, uDiagPingResponse_t ping_response, int32_t response_time);
static void pingCompleteUrc(struct uCxHandle *puCxHandle, int32_t transmitted_packets, 
                           int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time);
static void iperfOutputUrc(struct uCxHandle *puCxHandle, const char *iperf_output);

// ============================================================================
// BLUETOOTH HELPER FUNCTIONS
// ============================================================================

// Parse Bluetooth address from settings format: "XX:XX:XX:XX:XX:XX,type" or "XX:XX:XX:XX:XX:XX"
// Returns true if parsed successfully
static bool parseBluetoothAddress(const char *addrStr, uBtLeAddress_t *addr)
{
    if (!addrStr || !addr || strlen(addrStr) < 17) {
        return false;
    }
    
    // Parse MAC address (first 17 characters: XX:XX:XX:XX:XX:XX)
    int values[6];
    if (sscanf(addrStr, "%02x:%02x:%02x:%02x:%02x:%02x",
               &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) != 6) {
        return false;
    }
    
    for (int i = 0; i < 6; i++) {
        addr->address[i] = (uint8_t)values[i];
    }
    
    // Check if type is specified after comma
    const char *comma = strchr(addrStr, ',');
    if (comma && strlen(comma) > 1) {
        // Type specified: "XX:XX:XX:XX:XX:XX,random" or "XX:XX:XX:XX:XX:XX,public"
        if (strstr(comma + 1, "random") != NULL) {
            addr->type = U_BD_ADDRESS_TYPE_RANDOM;
        } else {
            addr->type = U_BD_ADDRESS_TYPE_PUBLIC;
        }
    } else {
        // No type specified - default to public
        addr->type = U_BD_ADDRESS_TYPE_PUBLIC;
    }
    
    return true;
}

// ============================================================================
// HTTP CLIENT HELPER FUNCTIONS
// ============================================================================

static char* httpGetRequest(const wchar_t *server, const wchar_t *path)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    char *result = NULL;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    DWORD totalSize = 0;
    char *buffer = NULL;
    
    // Initialize WinHTTP
    hSession = WinHttpOpen(L"ucxclient/1.0",
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) goto cleanup;
    
    // Connect to server
    hConnect = WinHttpConnect(hSession, server, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) goto cleanup;
    
    // Create request
    hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                                 NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    
    if (!hRequest) goto cleanup;
    
    // Send request
    if (!WinHttpSendRequest(hRequest,
                           WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0,
                           0, 0)) goto cleanup;
    
    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) goto cleanup;
    
    // Allocate initial buffer
    buffer = (char*)malloc(4096);
    if (!buffer) goto cleanup;
    
    // Read data
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        
        if (dwSize == 0) break;
        
        // Reallocate buffer if needed
        char *newBuffer = (char*)realloc(buffer, totalSize + dwSize + 1);
        if (!newBuffer) {
            free(buffer);
            goto cleanup;
        }
        buffer = newBuffer;
        
        if (!WinHttpReadData(hRequest, buffer + totalSize, dwSize, &dwDownloaded)) break;
        
        totalSize += dwDownloaded;
    } while (dwSize > 0);
    
    if (totalSize > 0) {
        buffer[totalSize] = '\0';
        result = buffer;
        buffer = NULL;  // Don't free it in cleanup
    }
    
cleanup:
    if (buffer) free(buffer);
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    
    return result;
}

static char* httpGetBinaryRequest(const wchar_t *server, const wchar_t *path, size_t *outSize)
{
    HINTERNET hSession = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    char *result = NULL;
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    size_t totalSize = 0;
    char *buffer = NULL;
    
    if (outSize) *outSize = 0;
    
    // Initialize WinHTTP
    hSession = WinHttpOpen(L"ucxclient/1.0",
                          WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME,
                          WINHTTP_NO_PROXY_BYPASS, 0);
    
    if (!hSession) goto cleanup;
    
    // Connect to server
    hConnect = WinHttpConnect(hSession, server, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) goto cleanup;
    
    // Create request
    hRequest = WinHttpOpenRequest(hConnect, L"GET", path,
                                 NULL, WINHTTP_NO_REFERER,
                                 WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    
    if (!hRequest) goto cleanup;
    
    // Enable automatic redirect handling for GitHub CDN redirects
    DWORD dwRedirectPolicy = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
    WinHttpSetOption(hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &dwRedirectPolicy, sizeof(dwRedirectPolicy));
    
    // Send request
    if (!WinHttpSendRequest(hRequest,
                           WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                           WINHTTP_NO_REQUEST_DATA, 0,
                           0, 0)) goto cleanup;
    
    // Receive response
    if (!WinHttpReceiveResponse(hRequest, NULL)) goto cleanup;
    
    // Allocate initial buffer
    buffer = (char*)malloc(4096);
    if (!buffer) goto cleanup;
    
    // Read data (binary-safe)
    do {
        dwSize = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) break;
        
        if (dwSize == 0) break;
        
        // Reallocate buffer if needed (no +1 for null terminator for binary data)
        char *newBuffer = (char*)realloc(buffer, totalSize + dwSize);
        if (!newBuffer) {
            free(buffer);
            goto cleanup;
        }
        buffer = newBuffer;
        
        if (!WinHttpReadData(hRequest, buffer + totalSize, dwSize, &dwDownloaded)) break;
        
        totalSize += dwDownloaded;
        
        // Show progress
        if (totalSize % 10240 == 0 || dwSize == 0) {
            printf("\rDownloaded: %zu KB", totalSize / 1024);
            fflush(stdout);
        }
    } while (dwSize > 0);
    
    if (totalSize > 0) {
        result = buffer;
        buffer = NULL;  // Don't free it in cleanup
        if (outSize) *outSize = totalSize;
        printf("\rDownloaded: %zu KB - Complete!\n", totalSize / 1024);
    }
    
cleanup:
    if (buffer) free(buffer);
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);
    
    return result;
}

static char* fetchLatestVersion(const char *product)
{
    wchar_t path[512];
    swprintf(path, 512, L"/repos/u-blox/u-connectXpress/contents/%S", product);
    
    char *response = httpGetRequest(L"api.github.com", path);
    if (!response) return NULL;
    
    // Simple parsing - look for directory names that match version pattern (e.g., "3.1.0")
    char *latestVersion = NULL;
    int maxMajor = 0, maxMinor = 0, maxPatch = 0;
    
    char *ptr = response;
    while ((ptr = strstr(ptr, "\"name\":\"")) != NULL) {
        ptr += 8;  // Skip past "name":"
        char name[32];
        int i = 0;
        while (*ptr != '"' && i < 31) {
            name[i++] = *ptr++;
        }
        name[i] = '\0';
        
        // Check if this looks like a version number
        int major, minor, patch;
        if (sscanf(name, "%d.%d.%d", &major, &minor, &patch) == 3) {
            if (major > maxMajor ||
                (major == maxMajor && minor > maxMinor) ||
                (major == maxMajor && minor == maxMinor && patch > maxPatch)) {
                maxMajor = major;
                maxMinor = minor;
                maxPatch = patch;
            }
        }
    }
    
    if (maxMajor > 0) {
        latestVersion = (char*)malloc(32);
        if (latestVersion) {
            sprintf(latestVersion, "%d.%d.%d", maxMajor, maxMinor, maxPatch);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate memory for version string");
        }
    }
    
    free(response);
    return latestVersion;
}

static bool saveBinaryFile(const char *filepath, const char *data, size_t size)
{
    FILE *fp = fopen(filepath, "wb");
    if (!fp) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to create file: %s", filepath);
        return false;
    }
    
    size_t written = fwrite(data, 1, size, fp);
    fclose(fp);
    
    if (written != size) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to write all data. Expected %zu, wrote %zu", size, written);
        return false;
    }
    
    return true;
}

// ============================================================================
// FIRMWARE UPDATE (GitHub Download, XMODEM Transfer)
// ============================================================================

static bool downloadFirmwareFromGitHub(const char *product, char *downloadedPath, size_t pathSize)
{
    printf("\nFetching latest firmware release from GitHub...\n");
    
    // Get latest release info from GitHub API
    wchar_t apiPath[512];
    swprintf(apiPath, 512, L"/repos/u-blox/u-connectXpress/releases/latest");
    
    char *releaseInfo = httpGetRequest(L"api.github.com", apiPath);
    if (!releaseInfo) {
        printf("ERROR: Failed to fetch release information from GitHub\n");
        return false;
    }
    
    // Parse release info to find firmware asset
    // Look for asset name containing the product name and ending with .bin
    char assetName[256] = {0};
    char downloadUrl[512] = {0};
    bool foundAsset = false;
    
    // Search for browser_download_url containing the product name
    char searchPattern[128];
    snprintf(searchPattern, sizeof(searchPattern), "\"browser_download_url\":\"");
    
    char *ptr = releaseInfo;
    while ((ptr = strstr(ptr, searchPattern)) != NULL) {
        ptr += strlen(searchPattern);
        
        // Extract URL
        char url[512];
        int i = 0;
        while (*ptr != '"' && i < 511) {
            url[i++] = *ptr++;
        }
        url[i] = '\0';
        
        // Check if this URL contains the product name and ends with .bin
        if (strstr(url, product) && strstr(url, ".bin")) {
            strncpy(downloadUrl, url, sizeof(downloadUrl) - 1);
            
            // Extract filename from URL
            char *lastSlash = strrchr(url, '/');
            if (lastSlash) {
                strncpy(assetName, lastSlash + 1, sizeof(assetName) - 1);
            }
            
            foundAsset = true;
            break;
        }
    }
    
    free(releaseInfo);
    
    if (!foundAsset) {
        printf("ERROR: Could not find firmware binary for %s in latest release\n", product);
        printf("Please visit https://github.com/u-blox/u-connectXpress/releases\n");
        printf("to download the firmware manually.\n");
        return false;
    }
    
    printf("Found firmware: %s\n", assetName);
    printf("Downloading from GitHub...\n");
    
    // Parse the download URL to extract server and path
    // URL format: https://github.com/u-blox/u-connectXpress/releases/download/v1.0.0/file.bin
    char *pathStart = strstr(downloadUrl, "github.com");
    if (!pathStart) {
        printf("ERROR: Invalid download URL format\n");
        return false;
    }
    
    pathStart = strchr(pathStart, '/');
    if (!pathStart) {
        printf("ERROR: Invalid download URL format\n");
        return false;
    }
    
    // Convert path to wide string
    wchar_t wPath[512];
    mbstowcs(wPath, pathStart, 512);
    
    // Download the firmware binary
    size_t firmwareSize = 0;
    char *firmwareData = httpGetBinaryRequest(L"github.com", wPath, &firmwareSize);
    if (!firmwareData || firmwareSize == 0) {
        printf("ERROR: Failed to download firmware file\n");
        if (firmwareData) free(firmwareData);
        return false;
    }
    
    printf("Downloaded %zu bytes\n", firmwareSize);
    
    // Save to local file
    snprintf(downloadedPath, pathSize, "%s_downloaded.bin", product);
    
    if (!saveBinaryFile(downloadedPath, firmwareData, firmwareSize)) {
        printf("ERROR: Failed to save firmware file\n");
        free(firmwareData);
        return false;
    }
    
    free(firmwareData);
    
    printf("Firmware saved to: %s\n", downloadedPath);
    return true;
}

static bool extractZipFile(const char *zipPath, const char *destFolder)
{
    printf("Extracting ZIP file...\n");
    
    // Use PowerShell to extract ZIP (works on Windows 10+)
    char command[1024];
    snprintf(command, sizeof(command),
             "powershell -Command \"Expand-Archive -Path '%s' -DestinationPath '%s' -Force\"",
             zipPath, destFolder);
    
    int result = system(command);
    if (result != 0) {
        printf("ERROR: Failed to extract ZIP file (exit code %d)\n", result);
        return false;
    }
    
    printf("ZIP file extracted successfully\n");
    return true;
}

static bool downloadFirmwareFromGitHubInteractive(char *downloadedPath, size_t pathSize)
{
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("Download Firmware from GitHub\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    // Fetch available products from GitHub repository
    printf("Fetching available products from GitHub...\n");
    
    wchar_t productApiPath[512];
    swprintf(productApiPath, 512, L"/repos/u-blox/u-connectXpress/contents");
    
    char *repoContents = httpGetRequest(L"api.github.com", productApiPath);
    if (!repoContents) {
        printf("ERROR: Failed to fetch repository contents from GitHub\n");
        printf("Please check your internet connection and try again.\n");
        return false;
    }
    
    // Parse directory listing to find product folders
    // Look for directories that contain product names (uppercase with hyphens)
    typedef struct {
        char name[64];
    } ProductInfo;
    
    ProductInfo products[20];
    int productCount = 0;
    
    // Parse JSON array looking for "name" fields that match product naming pattern
    // Product names are like NORA-W36, NORA-B27, etc. (uppercase, contains hyphen)
    char *ptr = repoContents;
    while (productCount < 20 && (ptr = strstr(ptr, "\"name\":\"")) != NULL) {
        ptr += 8; // skip "name":"
        
        char name[64];
        int i = 0;
        while (*ptr != '"' && i < 63) {
            name[i++] = *ptr++;
        }
        name[i] = '\0';
        
        // Check if this looks like a product name:
        // - Contains at least one hyphen
        // - First character is uppercase letter
        // - Contains uppercase letters and hyphens
        if (strlen(name) > 3 && isupper(name[0]) && strchr(name, '-') != NULL) {
            // Additional validation: should be mostly uppercase/digits/hyphens
            bool validProduct = true;
            for (size_t j = 0; j < strlen(name); j++) {
                if (!isupper(name[j]) && !isdigit(name[j]) && name[j] != '-') {
                    validProduct = false;
                    break;
                }
            }
            
            if (validProduct) {
                strncpy(products[productCount].name, name, sizeof(products[productCount].name) - 1);
                products[productCount].name[sizeof(products[productCount].name) - 1] = '\0';
                productCount++;
            }
        }
    }
    
    free(repoContents);
    
    if (productCount == 0) {
        printf("ERROR: No products found in repository\n");
        return false;
    }
    
    // Display available products
    printf("\nAvailable products:\n");
    for (int i = 0; i < productCount; i++) {
        printf("  [%d] %s\n", i + 1, products[i].name);
    }
    printf("  [0] Cancel\n");
    printf("\nSelect product: ");
    
    int productChoice;
    if (scanf("%d", &productChoice) != 1) {
        printf("ERROR: Invalid input\n");
        getchar(); // consume newline
        return false;
    }
    getchar(); // consume newline
    
    if (productChoice == 0) return false;
    if (productChoice < 1 || productChoice > productCount) {
        printf("ERROR: Invalid choice\n");
        return false;
    }
    
    const char *productName = products[productChoice - 1].name;
    
    printf("\nFetching available releases for %s from GitHub...\n", productName);
    
    // Fetch release list from GitHub API
    wchar_t apiPath[512];
    swprintf(apiPath, 512, L"/repos/u-blox/u-connectXpress/releases");
    
    char *releaseList = httpGetRequest(L"api.github.com", apiPath);
    if (!releaseList) {
        printf("ERROR: Failed to fetch release list from GitHub\n");
        printf("Please check your internet connection and try again.\n");
        return false;
    }
    
    // Parse and display available versions (filter by product)
    printf("\nAvailable versions:\n");
    typedef struct {
        char tag[64];
        char name[128];
    } ReleaseInfo;
    
    ReleaseInfo releases[20];
    int releaseCount = 0;
    
    // Parse JSON to extract tag_name and name, filtering by product
    char *releasePtr = releaseList;
    while (releaseCount < 20 && (releasePtr = strstr(releasePtr, "\"tag_name\":")) != NULL) {
        releasePtr += 12; // skip "tag_name":"
        
        // Extract tag
        char tag[64];
        int i = 0;
        while (*releasePtr != '"' && i < 63) {
            tag[i++] = *releasePtr++;
        }
        tag[i] = '\0';
        
        // Check if this release is for the selected product
        // Tags are like "NORA-W36X-3.1.0" or "NORA-B26X-3.0.1"
        // We want to match the product part (e.g., "NORA-W36" matches "NORA-W36X-...")
        bool matchesProduct = false;
        if (strncmp(tag, productName, strlen(productName)) == 0) {
            // Check that the next character is either 'X', '-', or end of string
            char nextChar = tag[strlen(productName)];
            if (nextChar == 'X' || nextChar == '-' || nextChar == '\0') {
                matchesProduct = true;
            }
        }
        
        if (matchesProduct) {
            // This release matches our product - add it to the list
            strncpy(releases[releaseCount].tag, tag, sizeof(releases[releaseCount].tag) - 1);
            releases[releaseCount].tag[sizeof(releases[releaseCount].tag) - 1] = '\0';
            
            // Find corresponding name
            char *namePtr = strstr(releasePtr, "\"name\":");
            if (namePtr) {
                namePtr += 8; // skip "name":"
                i = 0;
                while (*namePtr != '"' && i < 127) {
                    releases[releaseCount].name[i++] = *namePtr++;
                }
                releases[releaseCount].name[i] = '\0';
            } else {
                strcpy(releases[releaseCount].name, releases[releaseCount].tag);
            }
            
            printf("  [%d] %s - %s\n", releaseCount + 1, releases[releaseCount].tag, releases[releaseCount].name);
            releaseCount++;
        }
        
        releasePtr++;
    }
    
    free(releaseList);
    
    if (releaseCount == 0) {
        printf("ERROR: No releases found\n");
        return false;
    }
    
    printf("  [0] Cancel\n");
    printf("\nSelect version: ");
    
    int versionChoice;
    if (scanf("%d", &versionChoice) != 1) {
        printf("ERROR: Invalid input\n");
        getchar();
        return false;
    }
    getchar();
    
    if (versionChoice == 0) return false;
    if (versionChoice < 1 || versionChoice > releaseCount) {
        printf("ERROR: Invalid choice\n");
        return false;
    }
    
    const char *selectedTag = releases[versionChoice - 1].tag;
    printf("\nSelected: %s (%s)\n", selectedTag, releases[versionChoice - 1].name);
    
    // Fetch the specific release to get asset URLs
    printf("\nFetching release assets...\n");
    
    wchar_t releaseApiPath[512];
    swprintf(releaseApiPath, 512, L"/repos/u-blox/u-connectXpress/releases/tags/%S", selectedTag);
    
    char *releaseData = httpGetRequest(L"api.github.com", releaseApiPath);
    if (!releaseData) {
        printf("ERROR: Failed to fetch release information\n");
        return false;
    }
    
    // Find ZIP assets in the release (should be firmware files)
    char assetUrl[512] = {0};
    char assetName[128] = {0};
    bool foundAsset = false;
    
    char *assetPtr = releaseData;
    while ((assetPtr = strstr(assetPtr, "\"browser_download_url\":\"")) != NULL) {
        assetPtr += 24; // skip "browser_download_url":"
        
        char url[512];
        int i = 0;
        while (*assetPtr != '"' && i < 511) {
            url[i++] = *assetPtr++;
        }
        url[i] = '\0';
        
        // Check if URL ends with .zip (firmware files are typically in ZIP format)
        if (strstr(url, ".zip")) {
            strncpy(assetUrl, url, sizeof(assetUrl) - 1);
            
            // Extract filename
            char *lastSlash = strrchr(url, '/');
            if (lastSlash) {
                strncpy(assetName, lastSlash + 1, sizeof(assetName) - 1);
            }
            
            foundAsset = true;
            break;
        }
    }
    
    free(releaseData);
    
    if (!foundAsset) {
        printf("ERROR: No ZIP file found in release %s\n", selectedTag);
        printf("Please visit https://github.com/u-blox/u-connectXpress/releases/%s\n", selectedTag);
        return false;
    }
    
    printf("Found asset: %s\n", assetName);
    printf("\nDownloading %s from GitHub...\n", assetName);
    
    // Parse URL to extract server and path
    char *serverStart = strstr(assetUrl, "://");
    if (!serverStart) {
        printf("ERROR: Invalid asset URL\n");
        return false;
    }
    serverStart += 3;
    
    char *pathStart = strchr(serverStart, '/');
    if (!pathStart) {
        printf("ERROR: Invalid asset URL\n");
        return false;
    }
    
    // Extract server
    char server[256];
    size_t serverLen = pathStart - serverStart;
    if (serverLen >= sizeof(server)) {
        printf("ERROR: Server name too long\n");
        return false;
    }
    strncpy(server, serverStart, serverLen);
    server[serverLen] = '\0';
    
    // Prepare ZIP file path (will overwrite if exists)
    char zipPath[256];
    strncpy(zipPath, assetName, sizeof(zipPath) - 1);
    zipPath[sizeof(zipPath) - 1] = '\0';
    
    FILE *existingFile = fopen(zipPath, "rb");
    if (existingFile) {
        fclose(existingFile);
        printf("\nFile '%s' already exists - will overwrite.\n", zipPath);
    }
    
    // Convert to wide strings
    wchar_t wServer[256];
    wchar_t wPath[512];
    mbstowcs(wServer, server, 256);
    mbstowcs(wPath, pathStart, 512);
    
    // Download ZIP file
    size_t zipSize = 0;
    char *zipData = httpGetBinaryRequest(wServer, wPath, &zipSize);
    if (!zipData || zipSize == 0) {
        printf("ERROR: Failed to download firmware ZIP file\n");
        printf("The file may not exist for this product/version combination.\n");
        printf("Please visit https://github.com/u-blox/u-connectXpress/releases\n");
        if (zipData) free(zipData);
        return false;
    }
    
    printf("Downloaded %zu bytes\n", zipSize);
    
    // Save ZIP file (zipPath already declared and set earlier)
    if (!saveBinaryFile(zipPath, zipData, zipSize)) {
        printf("ERROR: Failed to save ZIP file\n");
        free(zipData);
        return false;
    }
    
    free(zipData);
    printf("ZIP file saved: %s\n", zipPath);
    
    // Extract ZIP file (use asset name without .zip extension as folder name)
    char extractFolder[256];
    strncpy(extractFolder, assetName, sizeof(extractFolder) - 1);
    extractFolder[sizeof(extractFolder) - 1] = '\0';
    char *zipExt = strstr(extractFolder, ".zip");
    if (zipExt) *zipExt = '\0'; // Remove .zip extension
    
    if (!extractZipFile(zipPath, extractFolder)) {
        printf("ERROR: Failed to extract ZIP file\n");
        return false;
    }
    
    // Find .bin file in extracted folder
    char findCommand[512];
    snprintf(findCommand, sizeof(findCommand),
             "dir /s /b \"%s\\*.bin\" > temp_bin_list.txt", extractFolder);
    system(findCommand);
    
    FILE *binList = fopen("temp_bin_list.txt", "r");
    if (!binList) {
        printf("ERROR: Could not find firmware .bin file in extracted ZIP\n");
        return false;
    }
    
    char binPath[512];
    if (fgets(binPath, sizeof(binPath), binList) == NULL) {
        printf("ERROR: No .bin file found in extracted ZIP\n");
        fclose(binList);
        DeleteFileA("temp_bin_list.txt");
        return false;
    }
    
    fclose(binList);
    DeleteFileA("temp_bin_list.txt");
    
    // Remove newline
    binPath[strcspn(binPath, "\r\n")] = '\0';
    
    printf("Found firmware file: %s\n", binPath);
    strncpy(downloadedPath, binPath, pathSize - 1);
    downloadedPath[pathSize - 1] = '\0';
    
    return true;
}

static bool fetchApiCommandsFromGitHub(const char *product, const char *version)
{
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Fetching API commands for %s version %s from GitHub...", product, version);
    
    wchar_t path[512];
    swprintf(path, 512, L"/u-blox/u-connectXpress/main/%S/%S/compiled_product.yaml", product, version);
    
    char *yamlContent = httpGetRequest(L"raw.githubusercontent.com", path);
    
    if (!yamlContent) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to fetch YAML file from GitHub");
        return false;
    }
    
    parseYamlCommands(yamlContent);
    free(yamlContent);
    
    return gApiCommandCount > 0;
}

static void parseYamlCommands(const char *yamlContent)
{
    // Free existing commands
    freeApiCommands();
    
    // Allocate initial array
    int capacity = 300;
    gApiCommands = (ApiCommand_t*)malloc(sizeof(ApiCommand_t) * capacity);
    if (!gApiCommands) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate memory for API commands array");
        return;
    }
    
    gApiCommandCount = 0;
    
    // Find the command_groups section
    const char *cmdGroups = strstr(yamlContent, "command_groups:");
    if (!cmdGroups) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Could not find command_groups in YAML");
        return;
    }
    
    // Parse each AT command entry (look for "      AT" with 6 spaces - the indentation level under commands:)
    const char *ptr = cmdGroups;
    char currentChapter[128] = "";
    
    while ((ptr = strstr(ptr + 1, "\n      AT")) != NULL) {
        // Try to find the chapter name by looking backwards for "  ChapterName:" at indentation level 2
        // YAML structure is:
        //   General:
        //     commands:
        //       AT:
        const char *chapterSearch = ptr;
        bool foundChapter = false;
        
        while (chapterSearch > cmdGroups && chapterSearch > ptr - 5000) {
            const char *lineStart = chapterSearch;
            while (lineStart > cmdGroups && *(lineStart - 1) != '\n') lineStart--;
            
            // Check if this line starts with exactly 2 spaces followed by a capital letter
            // and ends with a colon (e.g., "  General:", "  WiFi:", "  System:")
            if (lineStart[0] == ' ' && lineStart[1] == ' ' && 
                lineStart[2] >= 'A' && lineStart[2] <= 'Z') {
                
                const char *colonPos = strchr(lineStart + 2, ':');
                if (colonPos && (colonPos - lineStart) < 60) {
                    // Extract the chapter name (without the colon)
                    const char *nameStart = lineStart + 2;
                    int i = 0;
                    while (nameStart < colonPos && i < 127) {
                        currentChapter[i++] = *nameStart++;
                    }
                    currentChapter[i] = '\0';
                    foundChapter = true;
                    break;
                }
            }
            
            chapterSearch--;
        }
        
        // If no chapter found yet, this might be the first command - look forward from command_groups
        if (!foundChapter && gApiCommandCount == 0) {
            const char *forward = cmdGroups;
            while (*forward && forward < ptr) {
                if (forward[0] == ' ' && forward[1] == ' ' && 
                    forward[2] >= 'A' && forward[2] <= 'Z') {
                    const char *colonPos = strchr(forward + 2, ':');
                    if (colonPos && (colonPos - forward) < 60 && colonPos < ptr) {
                        const char *nameStart = forward + 2;
                        int i = 0;
                        while (nameStart < colonPos && i < 127) {
                            currentChapter[i++] = *nameStart++;
                        }
                        currentChapter[i] = '\0';
                        break;
                    }
                }
                forward++;
            }
        }
        
        // Extract AT command (from line start to ':')
        const char *lineStart = ptr + 1;
        while (*lineStart == ' ') lineStart++;
        
        const char *colonPos = strchr(lineStart, ':');
        if (!colonPos || colonPos - lineStart > 127) continue;
        
        char atCmd[128] = {0};
        strncpy(atCmd, lineStart, colonPos - lineStart);
        atCmd[colonPos - lineStart] = '\0';
        
        // Look for api_name and brief in the next ~3000 chars (within this command block)
        const char *searchEnd = ptr + 3000;
        const char *nextCmd = strstr(ptr + 10, "\n      AT");
        if (nextCmd && nextCmd < searchEnd) searchEnd = nextCmd;
        
        char ucxApi[128] = {0};
        char desc[256] = {0};
        
        // Find api_name within this command block
        const char *apiStart = ptr;
        while (apiStart < searchEnd) {
            apiStart = strstr(apiStart, "api_name:");
            if (!apiStart || apiStart >= searchEnd) break;
            
            apiStart += 9;
            while (*apiStart && (*apiStart == ' ' || *apiStart == '\t')) apiStart++;
            
            int i = 0;
            while (*apiStart && *apiStart != '\n' && *apiStart != '\r' && i < 127) {
                ucxApi[i++] = *apiStart++;
            }
            ucxApi[i] = '\0';
            break;
        }
        
        // Find brief description
        const char *descStart = ptr;
        const char *briefPtr = NULL;
        while (descStart < searchEnd) {
            briefPtr = strstr(descStart, "brief:");
            if (!briefPtr || briefPtr >= searchEnd) break;
            
            briefPtr += 6;
            while (*briefPtr && (*briefPtr == ' ' || *briefPtr == '\t')) briefPtr++;
            
            int i = 0;
            while (*briefPtr && *briefPtr != '\n' && *briefPtr != '\r' && i < 255) {
                desc[i++] = *briefPtr++;
            }
            desc[i] = '\0';
            break;
        }
        
        // Add to array if we have valid data
        if (atCmd[0] != '\0') {
            if (gApiCommandCount >= capacity) {
                capacity *= 2;
                ApiCommand_t *newCommands = (ApiCommand_t*)realloc(gApiCommands, sizeof(ApiCommand_t) * capacity);
                if (!newCommands) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to reallocate API commands array");
                    break;
                }
                gApiCommands = newCommands;
            }
            
            strncpy(gApiCommands[gApiCommandCount].atCommand, atCmd, 127);
            gApiCommands[gApiCommandCount].atCommand[127] = '\0';
            strncpy(gApiCommands[gApiCommandCount].ucxApi, ucxApi, 127);
            gApiCommands[gApiCommandCount].ucxApi[127] = '\0';
            strncpy(gApiCommands[gApiCommandCount].description, desc, 255);
            gApiCommands[gApiCommandCount].description[255] = '\0';
            strncpy(gApiCommands[gApiCommandCount].chapter, currentChapter, 127);
            gApiCommands[gApiCommandCount].chapter[127] = '\0';
            
            gApiCommandCount++;
        }
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Parsed %d API commands", gApiCommandCount);
}

static void freeApiCommands(void)
{
    if (gApiCommands) {
        free(gApiCommands);
        gApiCommands = NULL;
    }
    gApiCommandCount = 0;
}

// ============================================================================
// SETTINGS MANAGEMENT (Load/Save/Obfuscation)
// ============================================================================

// Simple XOR-based obfuscation (not cryptographically secure, but better than plaintext)
#define OBFUSCATION_KEY "uBloxUcxClient"

static void obfuscatePassword(const char *input, char *output, size_t outputSize)
{
    size_t inputLen = strlen(input);
    size_t keyLen = strlen(OBFUSCATION_KEY);
    
    if (inputLen == 0) {
        output[0] = '\0';
        return;
    }
    
    // Convert to hex with XOR
    size_t outIdx = 0;
    for (size_t i = 0; i < inputLen && outIdx < outputSize - 3; i++) {
        unsigned char c = input[i] ^ OBFUSCATION_KEY[i % keyLen];
        snprintf(&output[outIdx], outputSize - outIdx, "%02x", c);
        outIdx += 2;
    }
    output[outIdx] = '\0';
}

static void deobfuscatePassword(const char *input, char *output, size_t outputSize)
{
    size_t inputLen = strlen(input);
    size_t keyLen = strlen(OBFUSCATION_KEY);
    
    if (inputLen == 0 || inputLen % 2 != 0) {
        output[0] = '\0';
        return;
    }
    
    // Convert from hex with XOR
    size_t outIdx = 0;
    for (size_t i = 0; i < inputLen && outIdx < outputSize - 1; i += 2) {
        char hexByte[3] = {input[i], input[i+1], '\0'};
        unsigned char c = (unsigned char)strtol(hexByte, NULL, 16);
        output[outIdx] = c ^ OBFUSCATION_KEY[outIdx % keyLen];
        outIdx++;
    }
    output[outIdx] = '\0';
}

// ----------------------------------------------------------------
// URC Event Helper Functions
// ----------------------------------------------------------------

static bool waitEvent(uint32_t evtFlag, uint32_t timeoutS)
{
    int32_t timeoutMs = timeoutS * 1000;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "waitEvent(%d, %d)", evtFlag, timeoutS);
    do {
        U_CX_MUTEX_TRY_LOCK(gUrcMutex, 100);
        if (gUrcEventFlags & evtFlag) {
            gUrcEventFlags &= ~evtFlag;  // Clear the flag
            U_CX_MUTEX_UNLOCK(gUrcMutex);
            return true;
        }
        U_CX_MUTEX_UNLOCK(gUrcMutex);
        
        // Sleep to allow RX thread and AT client to process URCs
        Sleep(50);  // Check every 50ms instead of spinning
    } while (U_CX_PORT_GET_TIME_MS() - startTime < timeoutMs);

    U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Timeout waiting for: %d", evtFlag);
    return false;
}

static void signalEvent(uint32_t evtFlag)
{
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags |= evtFlag;
    U_CX_MUTEX_UNLOCK(gUrcMutex);
}

// ============================================================================
// URC (UNSOLICITED RESULT CODE) HANDLERS
// ============================================================================

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Network UP");
    signalEvent(URC_FLAG_NETWORK_UP);
}

static void networkDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Network DOWN");
    signalEvent(URC_FLAG_NETWORK_DOWN);
}

static void linkUpUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel)
{
    (void)wlan_handle;
    (void)bssid;
    (void)channel;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Link UP");
    signalEvent(URC_FLAG_WIFI_LINK_UP);
}

static void linkDownUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, int32_t reason)
{
    (void)wlan_handle;
    (void)reason;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Link DOWN");
    signalEvent(URC_FLAG_WIFI_LINK_DOWN);
}

static void apNetworkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi AP Network UP");
    printf("\n[EVENT] Wi-Fi Access Point Network is UP\n");
}

static void apNetworkDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi AP Network DOWN");
    printf("\n[EVENT] Wi-Fi Access Point Network is DOWN\n");
}

static void apUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi AP UP");
    printf("\n[EVENT] Wi-Fi Access Point is UP\n");
}

static void apDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi AP DOWN");
    printf("\n[EVENT] Wi-Fi Access Point is DOWN\n");
}

static void apStationAssociatedUrc(struct uCxHandle *puCxHandle, uMacAddress_t *mac)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Station Associated");
    printf("\n[EVENT] Station connected to AP - MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac->address[0], mac->address[1], mac->address[2],
           mac->address[3], mac->address[4], mac->address[5]);
}

static void apStationDisassociatedUrc(struct uCxHandle *puCxHandle, uMacAddress_t *mac)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Station Disassociated");
    printf("\n[EVENT] Station disconnected from AP - MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac->address[0], mac->address[1], mac->address[2],
           mac->address[3], mac->address[4], mac->address[5]);
}

static void sockConnected(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    (void)puCxHandle;
    (void)socket_handle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Socket connected: %d", socket_handle);
    signalEvent(URC_FLAG_SOCK_CONNECTED);
}

static void socketDataAvailable(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)socket_handle;
    (void)number_bytes;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Socket data available: %d bytes on socket %d", number_bytes, socket_handle);
    signalEvent(URC_FLAG_SOCK_DATA);
}

static void spsDataAvailable(struct uCxHandle *puCxHandle, int32_t connection_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)connection_handle;
    (void)number_bytes;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "SPS data available: %d bytes on connection %d", number_bytes, connection_handle);
    signalEvent(URC_FLAG_SPS_DATA);
}

static void spsConnected(struct uCxHandle *puCxHandle, int32_t connection_handle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** SPS Connection established! Connection handle: %d ***", connection_handle);
    signalEvent(URC_FLAG_SPS_CONNECTED);
}

static void spsDisconnected(struct uCxHandle *puCxHandle, int32_t connection_handle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** SPS Disconnected! Connection handle: %d ***", connection_handle);
    signalEvent(URC_FLAG_SPS_DISCONNECTED);
}

static void startupUrc(struct uCxHandle *puCxHandle)
{
    // Record timestamp when STARTUP is received
    ULONGLONG currentTime = GetTickCount64();
    gStartupTimestamp = currentTime;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** Module STARTUP detected ***");
    
    // Prevent re-entry: Only reconfigure if we haven't done so recently (within 2 seconds)
    if (gLastStartupConfigTime == 0 || (currentTime - gLastStartupConfigTime) > 2000) {
        gLastStartupConfigTime = currentTime;
        
        // Re-initialize module settings after restart
        printf("\n[Module restarted] Re-initializing settings...\n");
        
        // Disable echo
        if (uCxAtClientExecSimpleCmd(puCxHandle->pAtClient, "ATE0") == 0) {
            printf("  ✓ Echo disabled\n");
        } else {
            printf("  ✗ Failed to disable echo\n");
        }
        
        // Enable extended error codes
        if (uCxAtClientExecSimpleCmdF(puCxHandle->pAtClient, "AT+USYEE=", "d", 1, U_CX_AT_UTIL_PARAM_LAST) == 0) {
            printf("  ✓ Extended error codes enabled\n");
        } else {
            printf("  ✗ Failed to enable extended error codes\n");
        }
    }
    
    // Signal the event to wake up any waiting code
    signalEvent(URC_FLAG_STARTUP);
}

static void pingResponseUrc(struct uCxHandle *puCxHandle, uDiagPingResponse_t ping_response, int32_t response_time)
{
    (void)puCxHandle;
    if (ping_response == U_DIAG_PING_RESPONSE_TRUE) {
        gPingSuccess++;
        // Store individual ping time
        if (gPingCount < MAX_PING_TIMES) {
            gPingTimes[gPingCount] = response_time;
            gPingCount++;
        }
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Ping response: %d ms", response_time);
    } else {
        gPingFailed++;
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Ping failed");
    }
}

static void pingCompleteUrc(struct uCxHandle *puCxHandle, int32_t transmitted_packets, 
                           int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time)
{
    (void)puCxHandle;
    (void)packet_loss_rate;
    gPingSuccess = received_packets;
    gPingFailed = transmitted_packets - received_packets;
    gPingAvgTime = avg_response_time;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Ping complete: %d/%d packets, avg %d ms", 
                   received_packets, transmitted_packets, avg_response_time);
    signalEvent(URC_FLAG_PING_COMPLETE);
}

static void iperfOutputUrc(struct uCxHandle *puCxHandle, const char *iperf_output)
{
    (void)puCxHandle;
    
    // Display the iPerf output line
    printf("%s\n", iperf_output);
    
    // Check if this indicates test completion or error
    if (strstr(iperf_output, "Server Report:") || 
        strstr(iperf_output, "Client connecting") ||
        strstr(iperf_output, "ERROR") ||
        strstr(iperf_output, "failed")) {
        // Store in buffer for later reference if needed
        strncat(gIperfOutputBuffer, iperf_output, 
                sizeof(gIperfOutputBuffer) - strlen(gIperfOutputBuffer) - 2);
        strcat(gIperfOutputBuffer, "\n");
    }
    
    // Check for completion indicators
    if (strstr(iperf_output, "Server Report:") || strstr(iperf_output, "Completed")) {
        gIperfRunning = false;
    }
}

static void mqttConnectedUrc(struct uCxHandle *puCxHandle, int32_t mqtt_client_id)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "MQTT connected: client %d", mqtt_client_id);
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("MQTT CONNECTION ESTABLISHED\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Client ID: %d\n", mqtt_client_id);
    printf("Status:    Connected to broker\n");
    printf("─────────────────────────────────────────────────\n");
    
    signalEvent(URC_FLAG_MQTT_CONNECTED);
}

static void mqttDataAvailableUrc(struct uCxHandle *puCxHandle, int32_t mqtt_client_id, int32_t number_bytes)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "MQTT data received: %d bytes on client %d", number_bytes, mqtt_client_id);
    
    // Read the MQTT data immediately
    printf("\n─────────────────────────────────────────────────\n");
    printf("MQTT MESSAGE RECEIVED\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Client ID: %d\n", mqtt_client_id);
    printf("Data size: %d bytes\n", number_bytes);
    
    // Read the message data using AT+UMQTTRD
    const char *pTopic = NULL;
    uint8_t readBuffer[1024];
    
    int32_t bytesRead = uCxMqttReadBegin(puCxHandle, mqtt_client_id, readBuffer, sizeof(readBuffer), &pTopic);
    
    if (bytesRead >= 0) {
        if (pTopic) {
            printf("Topic: %s\n", pTopic);
        }
        printf("Message (%d bytes): ", bytesRead);
        
        // Try to print as text, but handle binary data
        bool isPrintable = true;
        for (int32_t i = 0; i < bytesRead; i++) {
            if (readBuffer[i] < 32 && readBuffer[i] != '\n' && readBuffer[i] != '\r' && readBuffer[i] != '\t') {
                isPrintable = false;
                break;
            }
        }
        
        if (isPrintable) {
            // Print as text
            printf("%.*s\n", bytesRead, readBuffer);
        } else {
            // Print as hex
            printf("\n");
            for (int32_t i = 0; i < bytesRead; i++) {
                printf("%02X ", readBuffer[i]);
                if ((i + 1) % 16 == 0) printf("\n");
            }
            if (bytesRead % 16 != 0) printf("\n");
        }
        
        uCxEnd(puCxHandle);
    } else {
        printf("ERROR: Failed to read MQTT message (error %d)\n", bytesRead);
    }
    printf("─────────────────────────────────────────────────\n");
    
    printf("*****************************\n\n");
    
    signalEvent(URC_FLAG_MQTT_DATA);
}

static void btConnected(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Bluetooth connected: handle %d", conn_handle);
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("BLUETOOTH CONNECTION ESTABLISHED\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Connection handle: %d\n", conn_handle);
    if (bd_addr) {
        printf("Device address:    %02X:%02X:%02X:%02X:%02X:%02X\n",
               bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
               bd_addr->address[3], bd_addr->address[4], bd_addr->address[5]);
        
        // Save to settings for quick reconnect (format: XX:XX:XX:XX:XX:XX,type)
        const char *addrType = (bd_addr->type == U_BD_ADDRESS_TYPE_PUBLIC) ? "public" : "random";
        snprintf(gRemoteAddress, sizeof(gRemoteAddress),
                 "%02X:%02X:%02X:%02X:%02X:%02X,%s",
                 bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
                 bd_addr->address[3], bd_addr->address[4], bd_addr->address[5],
                 addrType);
        saveSettings();
    }
    
    // Track the connection
    if (gBtConnectionCount < MAX_BT_CONNECTIONS) {
        gBtConnections[gBtConnectionCount].handle = conn_handle;
        if (bd_addr) {
            memcpy(&gBtConnections[gBtConnectionCount].address, bd_addr, sizeof(uBtLeAddress_t));
        }
        gBtConnections[gBtConnectionCount].active = true;
        gBtConnectionCount++;
    }
    
    // Remember this connection for GATT operations
    gCurrentGattConnHandle = conn_handle;
    
    printf("****************************************\n\n");
    
    // Note: Service Changed indication not needed - client will discover automatically
    
    // // Auto-discover GATT services and characteristics
    // printf("Auto-discovering GATT services and characteristics...\n");
    
    // // Discover services
    // uCxGattClientDiscoverPrimaryServicesBegin(puCxHandle, conn_handle);
    // gGattServiceCount = 0;
    
    // uCxGattClientDiscoverPrimaryServices_t service;
    // while (uCxGattClientDiscoverPrimaryServicesGetNext(puCxHandle, &service)) {
    //     if (gGattServiceCount < MAX_GATT_SERVICES) {
    //         GattService_t *stored = &gGattServices[gGattServiceCount];
    //         stored->connHandle = conn_handle;
    //         stored->startHandle = service.start_handle;
    //         stored->endHandle = service.end_handle;
    //         stored->uuidLength = (int32_t)service.uuid.length;
    //         memcpy(stored->uuid, service.uuid.pData, service.uuid.length);
    //         stored->name[0] = '\0';
            
    //         // Get friendly name for 16-bit UUIDs
    //         if (stored->uuidLength == 2) {
    //             uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
    //             const char *name = btGetServiceName(uuid16);
    //             if (name) {
    //                 strncpy(stored->name, name, sizeof(stored->name) - 1);
    //             }
    //         }
            
    //         gGattServiceCount++;
    //     }
    // }
    // uCxEnd(puCxHandle);
    // printf("  Found %d services:\n", gGattServiceCount);
    
    // // Display discovered services
    // for (int i = 0; i < gGattServiceCount; i++) {
    //     GattService_t *svc = &gGattServices[i];
    //     printf("    [%d] 0x%04X-0x%04X", i, svc->startHandle, svc->endHandle);
        
    //     if (svc->uuidLength == 2) {
    //         uint16_t uuid16 = (svc->uuid[0] << 8) | svc->uuid[1];
    //         printf(" UUID: 0x%04X", uuid16);
    //         if (svc->name[0] != '\0') {
    //             printf(" (%s)", svc->name);
    //         }
    //     } else {
    //         printf(" UUID: ");
    //         for (int j = 0; j < svc->uuidLength; j++) {
    //             printf("%02X", svc->uuid[j]);
    //         }
    //         // Check for u-blox SPS service UUID
    //         if (svc->uuidLength == 16) {
    //             const uint8_t ubloxSpsUuid[] = {0x24, 0x56, 0xE1, 0xB9, 0x26, 0xE2, 0x8F, 0x83,
    //                                              0xE7, 0x44, 0xF3, 0x4F, 0x01, 0xE9, 0xD7, 0x01};
    //             if (memcmp(svc->uuid, ubloxSpsUuid, 16) == 0) {
    //                 printf(" (u-blox SPS)");
    //             }
    //         }
    //     }
    //     printf("\n");
    // }
    
    // // Discover characteristics for all services
    // gGattCharacteristicCount = 0;
    
    // for (int svcIdx = 0; svcIdx < gGattServiceCount; svcIdx++) {
    //     GattService_t *svc = &gGattServices[svcIdx];
    //     uCxGattClientDiscoverServiceCharsBegin(puCxHandle, conn_handle, svc->startHandle, svc->endHandle);
        
    //     uCxGattClientDiscoverServiceChars_t characteristic;
    //     while (uCxGattClientDiscoverServiceCharsGetNext(puCxHandle, &characteristic)) {
    //         if (gGattCharacteristicCount < MAX_GATT_CHARACTERISTICS) {
    //             GattCharacteristic_t *stored = &gGattCharacteristics[gGattCharacteristicCount];
    //             stored->connHandle = conn_handle;
    //             stored->serviceIndex = svcIdx;
    //             stored->valueHandle = characteristic.value_handle;
    //             stored->properties = (characteristic.properties.length > 0) ? characteristic.properties.pData[0] : 0;
    //             stored->uuidLength = (int32_t)characteristic.uuid.length;
    //             memcpy(stored->uuid, characteristic.uuid.pData, characteristic.uuid.length);
    //             stored->name[0] = '\0';
                
    //             // Get friendly name for 16-bit UUIDs
    //             if (stored->uuidLength == 2) {
    //                 uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
    //                 const char *name = btGetCharacteristicName(uuid16);
    //                 if (name) {
    //                     strncpy(stored->name, name, sizeof(stored->name) - 1);
    //                 }
    //             }
                
    //             gGattCharacteristicCount++;
    //         }
    //     }
    //     uCxEnd(puCxHandle);
    // }
    // printf("  Found %d characteristics:\n", gGattCharacteristicCount);
    
    // // Display discovered characteristics
    // for (int i = 0; i < gGattCharacteristicCount; i++) {
    //     GattCharacteristic_t *ch = &gGattCharacteristics[i];
    //     printf("    [%d] Handle: 0x%04X", i, ch->valueHandle);
        
    //     if (ch->uuidLength == 2) {
    //         uint16_t uuid16 = (ch->uuid[0] << 8) | ch->uuid[1];
    //         printf(", UUID: 0x%04X", uuid16);
    //         if (ch->name[0] != '\0') {
    //             printf(" (%s)", ch->name);
    //         }
    //     } else {
    //         printf(", UUID: ");
    //         for (int j = 0; j < ch->uuidLength; j++) {
    //             printf("%02X", ch->uuid[j]);
    //         }
    //     }
        
    //     // Show properties
    //     printf(", Props: ");
    //     uint8_t props = (uint8_t)ch->properties;
    //     if (props & 0x02) printf("R");
    //     if (props & 0x08) printf("W");
    //     if (props & 0x10) printf("N");
    //     if (props & 0x20) printf("I");
    //     printf("\n");
    // }
    
    // printf("GATT discovery complete! You can now read/write characteristics.\n\n");
    printf("─────────────────────────────────────────────────\n");
    
    signalEvent(URC_FLAG_BT_CONNECTED);
}

static void btDisconnected(struct uCxHandle *puCxHandle, int32_t conn_handle)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Bluetooth disconnected: handle %d", conn_handle);
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("BLUETOOTH DISCONNECTED\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Connection handle: %d\n", conn_handle);
    printf("─────────────────────────────────────────────────\n");
    
    // Remove from tracked connections
    for (int i = 0; i < gBtConnectionCount; i++) {
        if (gBtConnections[i].handle == conn_handle) {
            // Shift remaining connections down
            for (int j = i; j < gBtConnectionCount - 1; j++) {
                gBtConnections[j] = gBtConnections[j + 1];
            }
            gBtConnectionCount--;
            break;
        }
    }
    
    // Clear connection handle
    if (gCurrentGattConnHandle == conn_handle) {
        gCurrentGattConnHandle = -1;
        
        // Stop unified notification thread when connection closes
        if (gGattNotificationThread) {
            gGattNotificationThreadRunning = false;
            WaitForSingleObject(gGattNotificationThread, 2000);
            CloseHandle(gGattNotificationThread);
            gGattNotificationThread = NULL;
            printf("  Stopped GATT notification thread\n");
        }
        
        // Reset notification flags
        gHeartbeatNotificationsEnabled = false;
        gBatteryNotificationsEnabled = false;
        gCtsServerNotificationsEnabled = false;
        
        // Only clear GATT discovery data if we're in client mode
        // (i.e., we discovered services from a remote device)
        // In server mode (HID, Heartbeat), keep the service data intact
        if (gGattServiceCount > 0 && gGattCharacteristicCount > 0) {
            // This looks like GATT Client discovery data - clear it
            gGattServiceCount = 0;
            gGattCharacteristicCount = 0;
            gLastCharacteristicIndex = -1;
            printf("  Cleared GATT Client discovery data\n");
        } else {
            // GATT Server mode - keep services intact for reconnection
            printf("  GATT Server services remain active (ready for reconnection)\n");
        }
        
        // Reset HID notification flags (client must re-enable CCCDs on reconnect)
        gHidBootKbdNotificationsEnabled = false;
        gHidKeyboardNotificationsEnabled = false;
        gHidMediaNotificationsEnabled = false;
    }
    
    printf("******************************\n\n");
    
    signalEvent(URC_FLAG_BT_DISCONNECTED);
}

// ----------------------------------------------------------------
// ============================================================================
// SOCKET OPERATIONS (TCP/UDP)
// ============================================================================

static void socketCreateTcp(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Create TCP Socket ---");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_SOCKET_PROTOCOL_TCP, &socketHandle);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully created TCP socket");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket handle: %d", socketHandle);
        gCurrentSocket = socketHandle;
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to create socket (code %d)", result);
    }
}

static void socketCreateUdp(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Create UDP Socket ---");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_SOCKET_PROTOCOL_UDP, &socketHandle);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully created UDP socket");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket handle: %d", socketHandle);
        gCurrentSocket = socketHandle;
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to create socket (code %d)", result);
    }
}

static void socketConnect(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    if (gCurrentSocket < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No socket created. Create a socket first.");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Connect Socket ---");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket handle: %d", gCurrentSocket);
    
    char hostname[129];
    int port;
    
    // Use saved remote address if available
    if (strlen(gRemoteAddress) > 0) {
        printf("Enter hostname/IP [%s]: ", gRemoteAddress);
    } else {
        printf("Enter hostname/IP: ");
    }
    
    if (fgets(hostname, sizeof(hostname), stdin)) {
        char *end = strchr(hostname, '\n');
        if (end) *end = '\0';
        
        // Use saved address if empty input
        if (strlen(hostname) == 0 && strlen(gRemoteAddress) > 0) {
            strncpy(hostname, gRemoteAddress, sizeof(hostname) - 1);
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Using saved address: %s", hostname);
        }
    }
    
    printf("Enter port (1-65535): ");
    if (scanf("%d", &port) != 1 || port < 1 || port > 65535) {
        printf("ERROR: Invalid port number. Must be 1-65535\n");
        // Clear input buffer
        while (getchar() != '\n');
        return;
    }
    getchar(); // consume newline
    
    // Validate hostname/IP not empty
    if (strlen(hostname) == 0) {
        printf("ERROR: Hostname/IP address cannot be empty\n");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Connecting to %s:%d...", hostname, port);
    
    int32_t result = uCxSocketConnect(&gUcxHandle, gCurrentSocket, hostname, port);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully connected");
        // Save the address
        strncpy(gRemoteAddress, hostname, sizeof(gRemoteAddress) - 1);
        gRemoteAddress[sizeof(gRemoteAddress) - 1] = '\0';
        saveSettings();
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to connect (code %d)", result);
    }
}

static void socketSendData(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    if (gCurrentSocket < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No socket created/connected. Connect a socket first.");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Send Socket Data ---");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket handle: %d", gCurrentSocket);
    printf("Enter data to send: ");
    
    char data[MAX_DATA_BUFFER + 1];
    if (fgets(data, sizeof(data), stdin)) {
        char *end = strchr(data, '\n');
        if (end) *end = '\0';
        
        size_t len = strlen(data);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Sending %zu bytes...", len);
        
        int32_t result = uCxSocketWrite(&gUcxHandle, gCurrentSocket, (uint8_t*)data, (int32_t)len);
        
        if (result >= 0) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully sent %d bytes", result);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to send data (code %d)", result);
        }
    }
}

static void socketReadData(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    if (gCurrentSocket < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No socket created/connected");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Read Socket Data ---");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket handle: %d", gCurrentSocket);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Waiting for data (timeout 5s)...");
    
    // Wait for data available event
    if (!waitEvent(URC_FLAG_SOCK_DATA, 5)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No data available (timeout)");
        return;
    }
    
    printf("Data available! Enter number of bytes to read (max %d): ", MAX_DATA_BUFFER);
    
    int length;
    scanf("%d", &length);
    getchar(); // consume newline
    
    if (length <= 0 || length > MAX_DATA_BUFFER) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid length. Must be 1-%d", MAX_DATA_BUFFER);
        return;
    }
    
    uint8_t buffer[MAX_DATA_BUFFER + 1];
    int32_t result = uCxSocketRead(&gUcxHandle, gCurrentSocket, length, buffer);
    
    if (result > 0) {
        buffer[result] = '\0';  // Null terminate for display
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Received %d bytes: %s", result, buffer);
    } else if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No data available");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read data (code %d)", result);
    }
}

static void socketClose(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentSocket < 0) {
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("ERROR: No socket created in this session\n");
        printf("─────────────────────────────────────────────────\n");
        printf("\nThis command closes the socket created in this session.\n");
        printf("Current session socket: None\n");
        printf("\nTo close sockets from previous sessions:\n");
        printf("  1. Use option [7] to list all sockets\n");
        printf("  2. Use option [8] to close any socket by handle\n");
        printf("─────────────────────────────────────────────────\n");
        return;
    }
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("CLOSE SOCKET\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Closing socket %d...\n", gCurrentSocket);
    
    int32_t result = uCxSocketClose(&gUcxHandle, gCurrentSocket);
    
    if (result == 0) {
        printf("✓ Socket %d closed successfully\n", gCurrentSocket);
        gCurrentSocket = -1;
    } else {
        printf("✗ Failed to close socket (error code: %d)\n", result);
    }
    
    printf("─────────────────────────────────────────────────\n");
}

static void socketCloseByHandle(void)
{
    char input[64];
    int socketHandle;
    
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("CLOSE SOCKET BY HANDLE\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\nEnter socket handle to close (0-9): ");
    
    if (!fgets(input, sizeof(input), stdin)) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    input[strcspn(input, "\n")] = 0;
    socketHandle = atoi(input);
    
    if (socketHandle < 0 || socketHandle > 9) {
        printf("ERROR: Invalid socket handle (must be 0-9)\n");
        printf("─────────────────────────────────────────────────\n");
        return;
    }
    
    printf("\nClosing socket %d...\n", socketHandle);
    
    int32_t result = uCxSocketClose(&gUcxHandle, socketHandle);
    
    if (result == 0) {
        printf("✓ Socket %d closed successfully\n", socketHandle);
        
        // If we just closed our current session socket, clear it
        if (socketHandle == gCurrentSocket) {
            gCurrentSocket = -1;
        }
    } else {
        printf("✗ Failed to close socket (error code: %d)\n", result);
        printf("\nPossible reasons:\n");
        printf("  - Socket %d does not exist\n", socketHandle);
        printf("  - Socket is already closed\n");
        printf("  - Invalid socket handle\n");
    }
    
    printf("─────────────────────────────────────────────────\n");
}

static void socketListStatus(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("SOCKET STATUS\n");
    printf("─────────────────────────────────────────────────\n");
    
    uCxSocketListStatusBegin(&gUcxHandle);
    
    uCxSocketListStatus_t status;
    int count = 0;
    
    while (uCxSocketListStatusGetNext(&gUcxHandle, &status)) {
        count++;
        const char *marker = (status.socket_handle == gCurrentSocket) ? " ← Current" : "";
        printf("Socket %d: %s, %s%s\n",
               status.socket_handle,
               status.protocol == U_SOCKET_PROTOCOL_TCP ? "TCP" : "UDP",
               status.status == 0 ? "Not Connected" :
               status.status == 1 ? "Listening" : "Connected",
               marker);
    }
    
    uCxEnd(&gUcxHandle);
    
    if (count == 0) {
        printf("No sockets open\n");
    } else {
        printf("\nTotal: %d socket%s\n", count, count > 1 ? "s" : "");
    }
    
    if (gCurrentSocket >= 0) {
        printf("\nCurrent session socket: %d\n", gCurrentSocket);
        printf("(This socket can be used with Send/Read/Close commands)\n");
    } else {
        printf("\nNo socket created in this session\n");
        printf("Use [1] or [2] to create a new socket\n");
        
        if (count > 0) {
            printf("\nNote: Socket(s) listed above were created outside this session.\n");
            printf("      Use option [8] to close any socket by handle.\n");
        }
    }
    
    printf("─────────────────────────────────────────────────\n");
}

// ----------------------------------------------------------------
// ============================================================================
// SPS (SERIAL PORT SERVICE)
// ============================================================================

static void spsEnableService(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Enable SPS Service ---");
    
    int32_t result = uCxSpsSetServiceEnable(&gUcxHandle, U_SPS_SERVICE_OPTION_ENABLE);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully enabled SPS service");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to enable SPS (code %d)", result);
    }
}

static void spsConnect(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Connect SPS ---");
    printf("Enter Bluetooth connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Connecting SPS on connection %d...", connHandle);
    
    int32_t result = uCxSpsConnect2(&gUcxHandle, connHandle, 0);  // No flow control
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully initiated SPS connection");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Wait for +UESPSC URC event...");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to connect SPS (code %d)", result);
    }
}

static void spsSendData(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Send SPS Data ---");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Enter data to send: ");
    char data[MAX_DATA_BUFFER + 1];
    if (fgets(data, sizeof(data), stdin)) {
        char *end = strchr(data, '\n');
        if (end) *end = '\0';
        
        size_t len = strlen(data);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Sending %zu bytes...", len);
        
        int32_t result = uCxSpsWrite(&gUcxHandle, connHandle, (uint8_t*)data, (int32_t)len);
        
        if (result >= 0) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully sent %d bytes", result);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to send data (code %d)", result);
        }
    }
}

static void spsReadData(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Read SPS Data ---");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Waiting for data (timeout 5s)...");
    
    // Wait for data available event
    if (!waitEvent(URC_FLAG_SPS_DATA, 5)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No data available (timeout)");
        return;
    }
    
    printf("Data available! Enter number of bytes to read (max %d): ", MAX_DATA_BUFFER);
    int length;
    scanf("%d", &length);
    getchar(); // consume newline
    
    if (length <= 0 || length > MAX_DATA_BUFFER) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid length. Must be 1-%d", MAX_DATA_BUFFER);
        return;
    }
    
    uint8_t buffer[MAX_DATA_BUFFER + 1];
    int32_t result = uCxSpsRead(&gUcxHandle, connHandle, length, buffer);
    
    if (result > 0) {
        buffer[result] = '\0';  // Null terminate for display
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Received %d bytes: %s", result, buffer);
    } else if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No data available");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read data (code %d)", result);
    }
}

// ----------------------------------------------------------------
// ============================================================================
// GATT CLIENT OPERATIONS
// ============================================================================

// ----------------------------------------------------------------
// GATT Helper Functions
// ----------------------------------------------------------------

// Helper: match 16-bit service UUID in stored services (big-endian in gGattServices)
static int findServiceByUuid16(uint16_t uuid16)
{
    for (int i = 0; i < gGattServiceCount; i++) {
        if (gGattServices[i].uuidLength == 2) {
            uint16_t sUuid = (gGattServices[i].uuid[0] << 8) | gGattServices[i].uuid[1];
            if (sUuid == uuid16) {
                return i; // index into gGattServices
            }
        }
    }
    return -1;
}

// Helper: match 16-bit characteristic inside a given service
static int findCharByUuid16InService(int serviceIndex, uint16_t uuid16)
{
    if (serviceIndex < 0 || serviceIndex >= gGattServiceCount) {
        return -1;
    }

    GattService_t *svc = &gGattServices[serviceIndex];

    for (int i = 0; i < gGattCharacteristicCount; i++) {
        GattCharacteristic_t *ch = &gGattCharacteristics[i];
        if (ch->connHandle != svc->connHandle) {
            continue;
        }
        if (ch->valueHandle < svc->startHandle || ch->valueHandle > svc->endHandle) {
            continue;
        }
        if (ch->uuidLength == 2) {
            uint16_t cUuid = (ch->uuid[0] << 8) | ch->uuid[1];
            if (cUuid == uuid16) {
                return i; // index into gGattCharacteristics
            }
        }
    }
    return -1;
}

// Helper: match 128-bit service UUID
static int findServiceByUuid128(const uint8_t uuid[16])
{
    for (int i = 0; i < gGattServiceCount; i++) {
        if (gGattServices[i].uuidLength == 16 &&
            memcmp(gGattServices[i].uuid, uuid, 16) == 0) {
            return i;
        }
    }
    return -1;
}

// ----------------------------------------------------------------
// GATT Connection Management
// ----------------------------------------------------------------

// Sync connection handle only (no discovery) - for GATT Server
static void syncGattConnectionOnly(void)
{
    // First, sync the Bluetooth connections from the module
    bluetoothSyncConnections();
    
    // If we already have a valid connection handle, verify it's still active
    if (gCurrentGattConnHandle >= 0) {
        bool stillActive = false;
        for (int i = 0; i < gBtConnectionCount; i++) {
            if (gBtConnections[i].handle == gCurrentGattConnHandle && gBtConnections[i].active) {
                stillActive = true;
                break;
            }
        }
        if (stillActive) {
            return;  // Current handle is still valid
        }
    }
    
    // No valid handle, try to find an active connection
    if (gBtConnectionCount > 0) {
        // Use the first active connection
        for (int i = 0; i < gBtConnectionCount; i++) {
            if (gBtConnections[i].active) {
                gCurrentGattConnHandle = gBtConnections[i].handle;
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "[GATT Server] Using connection handle: %d", gCurrentGattConnHandle);
                return;
            }
        }
    }
    
    // No active connections
    gCurrentGattConnHandle = -1;
}

// Sync GATT connection handle with active Bluetooth connections (GATT Client - with discovery)
static void syncGattConnection(void)
{
    // First, sync the Bluetooth connections from the module
    bluetoothSyncConnections();
    
    // If we already have a valid connection handle, verify it's still active
    if (gCurrentGattConnHandle >= 0) {
        bool stillActive = false;
        for (int i = 0; i < gBtConnectionCount; i++) {
            if (gBtConnections[i].handle == gCurrentGattConnHandle && gBtConnections[i].active) {
                stillActive = true;
                break;
            }
        }
        if (stillActive) {
            return;  // Current handle is still valid
        }
    }
    
    // No valid handle, try to find an active connection
    if (gBtConnectionCount > 0) {
        // Use the first active connection
        for (int i = 0; i < gBtConnectionCount; i++) {
            if (gBtConnections[i].active) {
                gCurrentGattConnHandle = gBtConnections[i].handle;
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "[Auto-sync] Using Bluetooth connection handle: %d", gCurrentGattConnHandle);
                
                // Check if we need to discover GATT services/characteristics
                bool needsDiscovery = false;
                if (gGattServiceCount == 0 || gGattCharacteristicCount == 0) {
                    needsDiscovery = true;
                } else if (gGattServices[0].connHandle != gCurrentGattConnHandle) {
                    needsDiscovery = true;
                }
                
                if (needsDiscovery) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "[Auto-sync] Discovering GATT services and characteristics...");
                    
                    // Discover services
                    uCxGattClientDiscoverPrimaryServicesBegin(&gUcxHandle, gCurrentGattConnHandle);
                    gGattServiceCount = 0;
                    
                    uCxGattClientDiscoverPrimaryServices_t service;
                    while (uCxGattClientDiscoverPrimaryServicesGetNext(&gUcxHandle, &service)) {
                        if (gGattServiceCount < MAX_GATT_SERVICES) {
                            GattService_t *stored = &gGattServices[gGattServiceCount];
                            stored->connHandle = gCurrentGattConnHandle;
                            stored->startHandle = service.start_handle;
                            stored->endHandle = service.end_handle;
                            stored->uuidLength = (int32_t)service.uuid.length;
                            memcpy(stored->uuid, service.uuid.pData, service.uuid.length);
                            stored->name[0] = '\0';
                            
                            if (stored->uuidLength == 2) {
                                uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                                const char *name = btGetServiceName(uuid16);
                                if (name) {
                                    strncpy(stored->name, name, sizeof(stored->name) - 1);
                                }
                            }
                            gGattServiceCount++;
                        }
                    }
                    uCxEnd(&gUcxHandle);
                    
                    printf("  Found %d services:\n", gGattServiceCount);
                    
                    // Display discovered services
                    for (int svcIdx = 0; svcIdx < gGattServiceCount; svcIdx++) {
                        GattService_t *svc = &gGattServices[svcIdx];
                        printf("    [%d] 0x%04X-0x%04X", svcIdx, svc->startHandle, svc->endHandle);
                        
                        if (svc->uuidLength == 2) {
                            uint16_t uuid16 = (svc->uuid[0] << 8) | svc->uuid[1];
                            printf(" UUID: 0x%04X", uuid16);
                            if (svc->name[0] != '\0') {
                                printf(" (%s)", svc->name);
                            }
                        } else {
                            printf(" UUID: ");
                            for (int j = 0; j < svc->uuidLength; j++) {
                                printf("%02X", svc->uuid[j]);
                            }
                            // Check for u-blox SPS service UUID
                            if (svc->uuidLength == 16) {
                                const uint8_t ubloxSpsUuid[] = {0x24, 0x56, 0xE1, 0xB9, 0x26, 0xE2, 0x8F, 0x83,
                                                                 0xE7, 0x44, 0xF3, 0x4F, 0x01, 0xE9, 0xD7, 0x01};
                                if (memcmp(svc->uuid, ubloxSpsUuid, 16) == 0) {
                                    printf(" (u-blox SPS)");
                                }
                            }
                        }
                        printf("\n");
                    }
                    
                    // Discover characteristics
                    gGattCharacteristicCount = 0;
                    for (int svcIdx = 0; svcIdx < gGattServiceCount; svcIdx++) {
                        GattService_t *svc = &gGattServices[svcIdx];
                        uCxGattClientDiscoverServiceCharsBegin(&gUcxHandle, gCurrentGattConnHandle, 
                                                              svc->startHandle, svc->endHandle);
                        
                        uCxGattClientDiscoverServiceChars_t characteristic;
                        while (uCxGattClientDiscoverServiceCharsGetNext(&gUcxHandle, &characteristic)) {
                            if (gGattCharacteristicCount < MAX_GATT_CHARACTERISTICS) {
                                GattCharacteristic_t *stored = &gGattCharacteristics[gGattCharacteristicCount];
                                stored->connHandle = gCurrentGattConnHandle;
                                stored->serviceIndex = svcIdx;
                                stored->valueHandle = characteristic.value_handle;
                                stored->properties = (characteristic.properties.length > 0) ? 
                                                    characteristic.properties.pData[0] : 0;
                                stored->uuidLength = (int32_t)characteristic.uuid.length;
                                memcpy(stored->uuid, characteristic.uuid.pData, characteristic.uuid.length);
                                stored->name[0] = '\0';
                                
                                if (stored->uuidLength == 2) {
                                    uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                                    const char *name = btGetCharacteristicName(uuid16);
                                    if (name) {
                                        strncpy(stored->name, name, sizeof(stored->name) - 1);
                                    }
                                }
                                gGattCharacteristicCount++;
                            }
                        }
                        uCxEnd(&gUcxHandle);
                    }
                    
                    printf("  Found %d characteristics:\n", gGattCharacteristicCount);
                    
                    // Display discovered characteristics
                    for (int charIdx = 0; charIdx < gGattCharacteristicCount; charIdx++) {
                        GattCharacteristic_t *ch = &gGattCharacteristics[charIdx];
                        printf("    [%d] Handle: 0x%04X", charIdx, ch->valueHandle);
                        
                        if (ch->uuidLength == 2) {
                            uint16_t uuid16 = (ch->uuid[0] << 8) | ch->uuid[1];
                            printf(", UUID: 0x%04X", uuid16);
                            if (ch->name[0] != '\0') {
                                printf(" (%s)", ch->name);
                            }
                        } else {
                            printf(", UUID: ");
                            for (int j = 0; j < ch->uuidLength; j++) {
                                printf("%02X", ch->uuid[j]);
                            }
                        }
                        
                        // Show properties
                        printf(", Props: ");
                        uint8_t props = (uint8_t)ch->properties;
                        if (props & 0x02) printf("R");
                        if (props & 0x08) printf("W");
                        if (props & 0x10) printf("N");
                        if (props & 0x20) printf("I");
                        printf("\n");
                    }
                    
                    printf("GATT discovery complete!\n");
                }
                return;
            }
        }
    }
    
    // No active connections found
    gCurrentGattConnHandle = -1;
}

// Helper function to get expected unit UUID for common characteristics
static uint16_t getExpectedUnitForCharacteristic(uint16_t charUuid)
{
    switch (charUuid) {
        // Battery Level (0x2A19) -> percentage (0x27AD)
        case 0x2A19: return 0x27AD;
        
        // Temperature characteristics -> Celsius (0x272F)
        case 0x2A1C: // Temperature Measurement
        case 0x2A1E: // Intermediate Temperature
        case 0x2A6E: // Temperature
            return 0x272F;
        
        // Pressure (0x2A6D) -> Pascal (0x2724)
        case 0x2A6D: return 0x2724;
        
        // Humidity (0x2A6F) -> percentage (0x27AD)
        case 0x2A6F: return 0x27AD;
        
        // Voltage (0x2B18) -> Volt (0x2728)
        case 0x2B18: return 0x2728;
        
        // Current (0x2AEE) -> Ampere (0x2704)
        case 0x2AEE: return 0x2704;
        
        // Energy (0x2AF3) -> Joule (0x2725)
        case 0x2AF3: return 0x2725;
        
        // Power (0x2AF4) -> Watt (0x2729)
        case 0x2AF4: return 0x2729;
        
        // Frequency (0x2AF5) -> Hertz (0x2727)
        case 0x2AF5: return 0x2727;
        
        // Illuminance (0x2AFB) -> Lux (0x2731)
        case 0x2AFB: return 0x2731;
        
        // No known unit
        default: return 0;
    }
}

static void gattClientDiscoverServices(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Discover Services ---");
    
    // Use the saved connection handle
    int connHandle = gCurrentGattConnHandle;
    if (connHandle < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No active GATT connection. Use Bluetooth menu to connect first.");
        return;
    }
    
    printf("Using connection handle: %d\n", connHandle);
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Starting service discovery on connection %d...", connHandle);
    
    // Clear previous services for this connection
    int oldCount = gGattServiceCount;
    gGattServiceCount = 0;
    
    // Call GATT service discovery command
    uCxGattClientDiscoverPrimaryServicesBegin(&gUcxHandle, connHandle);
    
    // Get services and store them
    uCxGattClientDiscoverPrimaryServices_t service;
    while (uCxGattClientDiscoverPrimaryServicesGetNext(&gUcxHandle, &service)) {
        if (gGattServiceCount < MAX_GATT_SERVICES) {
            GattService_t *stored = &gGattServices[gGattServiceCount];
            stored->connHandle = connHandle;
            stored->startHandle = service.start_handle;
            stored->endHandle = service.end_handle;
            stored->uuidLength = (int32_t)service.uuid.length;
            memcpy(stored->uuid, service.uuid.pData, service.uuid.length);
            stored->name[0] = '\0';
            
            printf("  [%d] Service: start=0x%04X, end=0x%04X\n", 
                   gGattServiceCount, stored->startHandle, stored->endHandle);
            printf("      UUID: ");
            
            // For 16-bit UUIDs, display as proper hex value
            if (stored->uuidLength == 2) {
                uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];  // Big endian (network order)
                printf("0x%04X", uuid16);
                
                const char *name = btGetServiceName(uuid16);
                if (name) {
                    printf(" (%s)", name);
                    strncpy(stored->name, name, sizeof(stored->name) - 1);
                }
            } else {
                // For 128-bit UUIDs, show raw bytes
                for (int i = 0; i < stored->uuidLength; i++) {
                    printf("%02X", stored->uuid[i]);
                }
                
                // Check if it's a known u-blox UUID
                if (stored->uuidLength == 16) {
                    // u-blox SPS Service: 2456E1B9-26E2-8F83-E744-F34F-01E9D701
                    const uint8_t ublox_sps[] = {0x01, 0xD7, 0xE9, 0x01, 0x4F, 0xF3, 0x44, 0xE7, 
                                                  0x83, 0x8F, 0xE2, 0x26, 0xB9, 0xE1, 0x56, 0x24};
                    if (memcmp(stored->uuid, ublox_sps, 16) == 0) {
                        printf(" (u-blox SPS Service)");
                        strncpy(stored->name, "u-blox SPS Service", sizeof(stored->name) - 1);
                    }
                }
            }
            printf("\n");
            
            gGattServiceCount++;
        }
    }
    
    int32_t result = uCxEnd(&gUcxHandle);
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Service discovery complete. Found %d services.", gGattServiceCount);
        if (oldCount > 0) {
            printf("Note: Replaced %d previously discovered services\n", oldCount);
        }
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Service discovery failed (code %d)", result);
    }
}

static void gattClientDiscoverCharacteristics(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Discover Characteristics ---");
    
    // Use the saved connection handle
    int connHandle = gCurrentGattConnHandle;
    if (connHandle < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No active GATT connection. Use Bluetooth menu to connect first.");
        return;
    }
    
    printf("Using connection handle: %d\n", connHandle);
    
    // Show discovered services
    if (gGattServiceCount == 0) {
        printf("No services discovered yet. Please discover services first (option 1).\n");
        return;
    }
    
    printf("\nDiscovered services:\n");
    for (int i = 0; i < gGattServiceCount; i++) {
        if (gGattServices[i].connHandle == connHandle) {
            printf("  [%d] 0x%04X-0x%04X", i, gGattServices[i].startHandle, gGattServices[i].endHandle);
            if (gGattServices[i].name[0] != '\0') {
                printf(" (%s)", gGattServices[i].name);
            }
            printf("\n");
        }
    }
    
    printf("\nEnter service index to discover [all]: ");
    
    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    // Trim newline
    input[strcspn(input, "\n")] = 0;
    
    int serviceIndex = -1; // Default to all
    
    // If not empty, parse the number
    if (strlen(input) > 0) {
        serviceIndex = atoi(input);
        if (serviceIndex < 0 || serviceIndex >= gGattServiceCount) {
            printf("ERROR: Invalid service index %d (must be 0-%d)\n", serviceIndex, gGattServiceCount - 1);
            return;
        }
    } else {
        printf("Discovering characteristics for all services...\n");
    }
    
    // Clear previous characteristics
    int oldCount = gGattCharacteristicCount;
    gGattCharacteristicCount = 0;
    
    int startIdx = (serviceIndex == -1) ? 0 : serviceIndex;
    int endIdx = (serviceIndex == -1) ? gGattServiceCount : (serviceIndex + 1);
    
    for (int svcIdx = startIdx; svcIdx < endIdx; svcIdx++) {
        if (svcIdx >= gGattServiceCount) continue;
        if (gGattServices[svcIdx].connHandle != connHandle) continue;
        
        GattService_t *svc = &gGattServices[svcIdx];
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Discovering characteristics for service %d (0x%04X-0x%04X)...", 
                     svcIdx, svc->startHandle, svc->endHandle);
        
        // Discover characteristics within this service range
        uCxGattClientDiscoverServiceCharsBegin(&gUcxHandle, connHandle, svc->startHandle, svc->endHandle);
        
        uCxGattClientDiscoverServiceChars_t characteristic;
        while (uCxGattClientDiscoverServiceCharsGetNext(&gUcxHandle, &characteristic)) {
            if (gGattCharacteristicCount < MAX_GATT_CHARACTERISTICS) {
                GattCharacteristic_t *stored = &gGattCharacteristics[gGattCharacteristicCount];
                stored->connHandle = connHandle;
                stored->serviceIndex = svcIdx;
                stored->valueHandle = characteristic.value_handle;
                // Properties is a byte array, extract first byte
                stored->properties = (characteristic.properties.length > 0) ? characteristic.properties.pData[0] : 0;
                stored->uuidLength = (int32_t)characteristic.uuid.length;
                memcpy(stored->uuid, characteristic.uuid.pData, characteristic.uuid.length);
                stored->name[0] = '\0';
                
                printf("    [%d] Char: handle=0x%04X, prop=0x%02X\n", 
                       gGattCharacteristicCount, stored->valueHandle, stored->properties);
                printf("        UUID: ");
                for (int i = 0; i < stored->uuidLength; i++) {
                    printf("%02X", stored->uuid[i]);
                }
                
                // Try to get friendly name for 16-bit UUIDs
                if (stored->uuidLength == 2) {
                    uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                    const char *name = btGetCharacteristicName(uuid16);
                    if (name) {
                        printf(" (%s)", name);
                        strncpy(stored->name, name, sizeof(stored->name) - 1);
                    }
                    
                    // Show expected unit if known
                    uint16_t unitUuid = getExpectedUnitForCharacteristic(uuid16);
                    if (unitUuid != 0) {
                        const char *unitSymbol = btGetUnitSymbol(unitUuid);
                        if (unitSymbol && unitSymbol[0] != '\0') {
                            printf(" [Unit: %s]", unitSymbol);
                        }
                    }
                }
                
                // Show properties
                printf("\n        Props: ");
                uint8_t props = (uint8_t)stored->properties;
                if (props & 0x02) printf("Read ");
                if (props & 0x04) printf("WriteWithoutResponse ");
                if (props & 0x08) printf("Write ");
                if (props & 0x10) printf("Notify ");
                if (props & 0x20) printf("Indicate ");
                printf("\n");
                
                gGattCharacteristicCount++;
            }
        }
        
        int32_t result = uCxEnd(&gUcxHandle);
        if (result != 0) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to discover characteristics for service %d (code %d)", 
                         svcIdx, result);
        }
    }
    
    printf("\nCharacteristic discovery complete. Found %d characteristics.\n", gGattCharacteristicCount);
    if (oldCount > 0) {
        printf("Note: Replaced %d previously discovered characteristics\n", oldCount);
    }
}

static void gattClientReadCharacteristic(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Read Characteristic ---");
    
    // Use the saved connection handle
    int connHandle = gCurrentGattConnHandle;
    if (connHandle < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No active GATT connection. Use Bluetooth menu to connect first.");
        return;
    }
    
    printf("Using connection handle: %d\n", connHandle);
    
    // Auto-discover characteristics if not already done
    if (gGattCharacteristicCount == 0 || 
        (gGattCharacteristicCount > 0 && gGattCharacteristics[0].connHandle != connHandle)) {
        printf("\nNo characteristics discovered yet. Auto-discovering...\n");
        
        // First check if we have services
        if (gGattServiceCount == 0 || 
            (gGattServiceCount > 0 && gGattServices[0].connHandle != connHandle)) {
            printf("Discovering services first...\n");
            
            // Discover services
            uCxGattClientDiscoverPrimaryServicesBegin(&gUcxHandle, connHandle);
            gGattServiceCount = 0;
            
            uCxGattClientDiscoverPrimaryServices_t service;
            while (uCxGattClientDiscoverPrimaryServicesGetNext(&gUcxHandle, &service)) {
                if (gGattServiceCount < MAX_GATT_SERVICES) {
                    GattService_t *stored = &gGattServices[gGattServiceCount];
                    stored->connHandle = connHandle;
                    stored->startHandle = service.start_handle;
                    stored->endHandle = service.end_handle;
                    stored->uuidLength = (int32_t)service.uuid.length;
                    memcpy(stored->uuid, service.uuid.pData, service.uuid.length);
                    stored->name[0] = '\0';
                    
                    // Get friendly name for 16-bit UUIDs
                    if (stored->uuidLength == 2) {
                        uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                        const char *name = btGetServiceName(uuid16);
                        if (name) {
                            strncpy(stored->name, name, sizeof(stored->name) - 1);
                        }
                    }
                    
                    gGattServiceCount++;
                }
            }
            uCxEnd(&gUcxHandle);
            printf("Found %d services.\n", gGattServiceCount);
        }
        
        // Now discover characteristics for all services
        printf("Discovering characteristics...\n");
        gGattCharacteristicCount = 0;
        
        for (int svcIdx = 0; svcIdx < gGattServiceCount; svcIdx++) {
            if (gGattServices[svcIdx].connHandle != connHandle) continue;
            
            GattService_t *svc = &gGattServices[svcIdx];
            uCxGattClientDiscoverServiceCharsBegin(&gUcxHandle, connHandle, svc->startHandle, svc->endHandle);
            
            uCxGattClientDiscoverServiceChars_t characteristic;
            while (uCxGattClientDiscoverServiceCharsGetNext(&gUcxHandle, &characteristic)) {
                if (gGattCharacteristicCount < MAX_GATT_CHARACTERISTICS) {
                    GattCharacteristic_t *stored = &gGattCharacteristics[gGattCharacteristicCount];
                    stored->connHandle = connHandle;
                    stored->serviceIndex = svcIdx;
                    stored->valueHandle = characteristic.value_handle;
                    stored->properties = (characteristic.properties.length > 0) ? characteristic.properties.pData[0] : 0;
                    stored->uuidLength = (int32_t)characteristic.uuid.length;
                    memcpy(stored->uuid, characteristic.uuid.pData, characteristic.uuid.length);
                    stored->name[0] = '\0';
                    
                    // Get friendly name for 16-bit UUIDs
                    if (stored->uuidLength == 2) {
                        uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                        const char *name = btGetCharacteristicName(uuid16);
                        if (name) {
                            strncpy(stored->name, name, sizeof(stored->name) - 1);
                        }
                    }
                    
                    gGattCharacteristicCount++;
                }
            }
            uCxEnd(&gUcxHandle);
        }
        printf("Found %d characteristics.\n\n", gGattCharacteristicCount);
    }
    
    // Show discovered characteristics if available
    int charHandle = -1;
    if (gGattCharacteristicCount > 0) {
        printf("\nDiscovered characteristics:\n");
        for (int i = 0; i < gGattCharacteristicCount; i++) {
            if (gGattCharacteristics[i].connHandle == connHandle) {
                GattCharacteristic_t *ch = &gGattCharacteristics[i];
                
                // Show index and handle
                printf("  [%d] Handle=0x%04X", i, ch->valueHandle);
                
                // Show UUID
                printf(", UUID=");
                for (int j = 0; j < ch->uuidLength; j++) {
                    printf("%02X", ch->uuid[j]);
                }
                
                // Show friendly name if available
                if (ch->name[0] != '\0') {
                    printf(" (%s)", ch->name);
                }
                
                // Show expected unit if available
                if (ch->uuidLength == 2) {
                    uint16_t uuid16 = (ch->uuid[0] << 8) | ch->uuid[1];
                    uint16_t unitUuid = getExpectedUnitForCharacteristic(uuid16);
                    if (unitUuid != 0) {
                        const char *unitSymbol = btGetUnitSymbol(unitUuid);
                        if (unitSymbol && unitSymbol[0] != '\0') {
                            printf(" [Unit: %s]", unitSymbol);
                        }
                    }
                }
                
                // Show properties
                printf("\n      Props: ");
                uint8_t props = (uint8_t)ch->properties;
                if (props & 0x02) printf("Read ");
                if (props & 0x04) printf("WriteWithoutResponse ");
                if (props & 0x08) printf("Write ");
                if (props & 0x10) printf("Notify ");
                if (props & 0x20) printf("Indicate ");
                printf("\n");
            }
        }
        
        // Prompt with default option
        if (gLastCharacteristicIndex >= 0 && gLastCharacteristicIndex < gGattCharacteristicCount) {
            printf("\nEnter characteristic [0-%d] (default: %d): ", 
                   gGattCharacteristicCount - 1, gLastCharacteristicIndex);
        } else {
            printf("\nEnter characteristic index [0-%d]: ", gGattCharacteristicCount - 1);
        }
        
        char input[32];
        if (fgets(input, sizeof(input), stdin)) {
            // Remove newline
            input[strcspn(input, "\n")] = '\0';
            
            if (strlen(input) == 0 && gLastCharacteristicIndex >= 0) {
                // Use default (last used)
                charHandle = gGattCharacteristics[gLastCharacteristicIndex].valueHandle;
                printf("Using characteristic %d (handle 0x%04X)\n", 
                       gLastCharacteristicIndex, charHandle);
            } else {
                // Parse as index
                int charIndex = atoi(input);
                if (charIndex >= 0 && charIndex < gGattCharacteristicCount &&
                    gGattCharacteristics[charIndex].connHandle == connHandle) {
                    charHandle = gGattCharacteristics[charIndex].valueHandle;
                    gLastCharacteristicIndex = charIndex;  // Remember this selection
                    printf("Selected characteristic %d (handle 0x%04X)\n", charIndex, charHandle);
                } else {
                    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid characteristic index: %d", charIndex);
                    return;
                }
            }
        }
    } else {
        // No characteristics discovered, ask for handle directly
        printf("No characteristics discovered yet. Please discover characteristics first (option 2).\n");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Reading characteristic handle 0x%04X...", charHandle);
    
    // Call GATT read characteristic command
    uByteArray_t data;
    uint8_t buffer[512];
    data.pData = buffer;
    data.length = 0;
    
    bool success = uCxGattClientReadBegin(&gUcxHandle, connHandle, charHandle, &data);
    int32_t result = uCxEnd(&gUcxHandle);
    
    if (success && result == 0) {
        printf("  Read %zd bytes: ", data.length);
        for (size_t i = 0; i < data.length; i++) {
            printf("%02X ", data.pData[i]);
        }
        printf("\n");
        
        // Find the characteristic to get its UUID and check for units
        uint16_t charUuid = 0;
        const char *unitSymbol = NULL;
        for (int i = 0; i < gGattCharacteristicCount; i++) {
            if (gGattCharacteristics[i].valueHandle == charHandle) {
                if (gGattCharacteristics[i].uuidLength == 2) {
                    charUuid = (gGattCharacteristics[i].uuid[0] << 8) | gGattCharacteristics[i].uuid[1];
                    uint16_t unitUuid = getExpectedUnitForCharacteristic(charUuid);
                    if (unitUuid != 0) {
                        unitSymbol = btGetUnitSymbol(unitUuid);
                    }
                }
                break;
            }
        }
        
        // Try to interpret as numeric value with unit
        if (data.length == 1 && unitSymbol && unitSymbol[0] != '\0') {
            printf("  Value: %u %s\n", data.pData[0], unitSymbol);
        } else if (data.length == 2 && unitSymbol && unitSymbol[0] != '\0') {
            uint16_t value = data.pData[0] | (data.pData[1] << 8);
            printf("  Value: %u %s\n", value, unitSymbol);
        }
        
        // Try to interpret as text if printable
        bool isPrintable = true;
        for (int i = 0; i < data.length; i++) {
            if (data.pData[i] < 32 || data.pData[i] > 126) {
                isPrintable = false;
                break;
            }
        }
        if (isPrintable && data.length > 0) {
            printf("  As text: %.*s\n", (int)data.length, data.pData);
        }
        
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Read successful.");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read characteristic (code %d)", result);
    }
}

static void gattClientWriteCharacteristic(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Write Characteristic ---");
    
    // Use the saved connection handle
    int connHandle = gCurrentGattConnHandle;
    if (connHandle < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No active GATT connection. Use Bluetooth menu to connect first.");
        return;
    }
    
    printf("Using connection handle: %d\n", connHandle);
    
    // Auto-discover characteristics if not already done
    if (gGattCharacteristicCount == 0 || 
        (gGattCharacteristicCount > 0 && gGattCharacteristics[0].connHandle != connHandle)) {
        printf("\nNo characteristics discovered yet. Auto-discovering...\n");
        
        // First check if we have services
        if (gGattServiceCount == 0 || 
            (gGattServiceCount > 0 && gGattServices[0].connHandle != connHandle)) {
            printf("Discovering services first...\n");
            
            // Discover services
            uCxGattClientDiscoverPrimaryServicesBegin(&gUcxHandle, connHandle);
            gGattServiceCount = 0;
            
            uCxGattClientDiscoverPrimaryServices_t service;
            while (uCxGattClientDiscoverPrimaryServicesGetNext(&gUcxHandle, &service)) {
                if (gGattServiceCount < MAX_GATT_SERVICES) {
                    GattService_t *stored = &gGattServices[gGattServiceCount];
                    stored->connHandle = connHandle;
                    stored->startHandle = service.start_handle;
                    stored->endHandle = service.end_handle;
                    stored->uuidLength = (int32_t)service.uuid.length;
                    memcpy(stored->uuid, service.uuid.pData, service.uuid.length);
                    stored->name[0] = '\0';
                    
                    // Get friendly name for 16-bit UUIDs
                    if (stored->uuidLength == 2) {
                        uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                        const char *name = btGetServiceName(uuid16);
                        if (name) {
                            strncpy(stored->name, name, sizeof(stored->name) - 1);
                        }
                    }
                    
                    gGattServiceCount++;
                }
            }
            uCxEnd(&gUcxHandle);
            printf("Found %d services.\n", gGattServiceCount);
        }
        
        // Now discover characteristics for all services
        printf("Discovering characteristics...\n");
        gGattCharacteristicCount = 0;
        
        for (int svcIdx = 0; svcIdx < gGattServiceCount; svcIdx++) {
            if (gGattServices[svcIdx].connHandle != connHandle) continue;
            
            GattService_t *svc = &gGattServices[svcIdx];
            uCxGattClientDiscoverServiceCharsBegin(&gUcxHandle, connHandle, svc->startHandle, svc->endHandle);
            
            uCxGattClientDiscoverServiceChars_t characteristic;
            while (uCxGattClientDiscoverServiceCharsGetNext(&gUcxHandle, &characteristic)) {
                if (gGattCharacteristicCount < MAX_GATT_CHARACTERISTICS) {
                    GattCharacteristic_t *stored = &gGattCharacteristics[gGattCharacteristicCount];
                    stored->connHandle = connHandle;
                    stored->serviceIndex = svcIdx;
                    stored->valueHandle = characteristic.value_handle;
                    stored->properties = (characteristic.properties.length > 0) ? characteristic.properties.pData[0] : 0;
                    stored->uuidLength = (int32_t)characteristic.uuid.length;
                    memcpy(stored->uuid, characteristic.uuid.pData, characteristic.uuid.length);
                    stored->name[0] = '\0';
                    
                    // Get friendly name for 16-bit UUIDs
                    if (stored->uuidLength == 2) {
                        uint16_t uuid16 = (stored->uuid[0] << 8) | stored->uuid[1];
                        const char *name = btGetCharacteristicName(uuid16);
                        if (name) {
                            strncpy(stored->name, name, sizeof(stored->name) - 1);
                        }
                    }
                    
                    gGattCharacteristicCount++;
                }
            }
            uCxEnd(&gUcxHandle);
        }
        printf("Found %d characteristics.\n\n", gGattCharacteristicCount);
    }
    
    // Show discovered characteristics if available
    int charHandle = -1;
    if (gGattCharacteristicCount > 0) {
        printf("\nDiscovered characteristics:\n");
        for (int i = 0; i < gGattCharacteristicCount; i++) {
            if (gGattCharacteristics[i].connHandle == connHandle) {
                GattCharacteristic_t *ch = &gGattCharacteristics[i];
                
                // Show index and handle
                printf("  [%d] Handle=0x%04X", i, ch->valueHandle);
                
                // Show UUID
                printf(", UUID=");
                for (int j = 0; j < ch->uuidLength; j++) {
                    printf("%02X", ch->uuid[j]);
                }
                
                // Show friendly name if available
                if (ch->name[0] != '\0') {
                    printf(" (%s)", ch->name);
                }
                
                // Show expected unit if available
                if (ch->uuidLength == 2) {
                    uint16_t uuid16 = (ch->uuid[0] << 8) | ch->uuid[1];
                    uint16_t unitUuid = getExpectedUnitForCharacteristic(uuid16);
                    if (unitUuid != 0) {
                        const char *unitSymbol = btGetUnitSymbol(unitUuid);
                        if (unitSymbol && unitSymbol[0] != '\0') {
                            printf(" [Unit: %s]", unitSymbol);
                        }
                    }
                }
                
                // Show properties (highlight writable ones)
                printf("\n      Props: ");
                uint8_t props = (uint8_t)ch->properties;
                if (props & 0x02) printf("Read ");
                if (props & 0x04) printf("WriteWithoutResponse ");
                if (props & 0x08) printf("Write ");
                if (props & 0x10) printf("Notify ");
                if (props & 0x20) printf("Indicate ");
                printf("\n");
            }
        }
        
        // Prompt with default option
        if (gLastCharacteristicIndex >= 0 && gLastCharacteristicIndex < gGattCharacteristicCount) {
            printf("\nEnter characteristic [0-%d] (default: %d): ", 
                   gGattCharacteristicCount - 1, gLastCharacteristicIndex);
        } else {
            printf("\nEnter characteristic index [0-%d]: ", gGattCharacteristicCount - 1);
        }
        
        char input[32];
        if (fgets(input, sizeof(input), stdin)) {
            // Remove newline
            input[strcspn(input, "\n")] = '\0';
            
            if (strlen(input) == 0 && gLastCharacteristicIndex >= 0) {
                // Use default (last used)
                charHandle = gGattCharacteristics[gLastCharacteristicIndex].valueHandle;
                printf("Using characteristic %d (handle 0x%04X)\n", 
                       gLastCharacteristicIndex, charHandle);
            } else {
                // Parse as index
                int charIndex = atoi(input);
                if (charIndex >= 0 && charIndex < gGattCharacteristicCount &&
                    gGattCharacteristics[charIndex].connHandle == connHandle) {
                    charHandle = gGattCharacteristics[charIndex].valueHandle;
                    gLastCharacteristicIndex = charIndex;  // Remember this selection
                    printf("Selected characteristic %d (handle 0x%04X)\n", charIndex, charHandle);
                } else {
                    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid characteristic index: %d", charIndex);
                    return;
                }
            }
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read input");
            return;
        }
    } else {
        // No characteristics discovered, ask for handle directly
        printf("No characteristics discovered yet. Please discover characteristics first (option 2).\n");
        return;
    }
    
    printf("\nEnter data to write (hex format, e.g., 01020304): ");
    char hexInput[MAX_DATA_BUFFER * 2 + 1];
    if (fgets(hexInput, sizeof(hexInput), stdin) == NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read input");
        return;
    }
    
    // Remove newline
    hexInput[strcspn(hexInput, "\n")] = '\0';
    
    // Convert hex string to bytes
    size_t hexLen = strlen(hexInput);
    if (hexLen % 2 != 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid hex data (must be even number of digits)");
        return;
    }
    
    uint8_t data[MAX_DATA_BUFFER];
    size_t dataLen = hexLen / 2;
    
    for (size_t i = 0; i < dataLen; i++) {
        char byteStr[3] = {hexInput[i*2], hexInput[i*2 + 1], '\0'};
        data[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Writing %zu bytes...", dataLen);
    
    // Call GATT write characteristic command
    int32_t result = uCxGattClientWrite(&gUcxHandle, connHandle, charHandle, data, (int32_t)dataLen);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Write successful.");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to write characteristic (code %d)", result);
    }
}

// ----------------------------------------------------------------
// ============================================================================
// GATT SERVER OPERATIONS
// ============================================================================

static void gattServerAddService(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Server: Define Service ---");
    printf("Enter service UUID (hex, e.g., 180A for 16-bit or full 128-bit): ");
    
    char uuidStr[33];
    if (fgets(uuidStr, sizeof(uuidStr), stdin) == NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read input");
        return;
    }
    uuidStr[strcspn(uuidStr, "\n")] = '\0';
    
    // Convert hex string to bytes
    size_t hexLen = strlen(uuidStr);
    if (hexLen % 2 != 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid UUID (must be even number of hex digits)");
        return;
    }
    
    uint8_t uuid[16];
    int32_t uuidLen = (int32_t)(hexLen / 2);
    for (int i = 0; i < uuidLen; i++) {
        char byteStr[3] = {uuidStr[i*2], uuidStr[i*2 + 1], '\0'};
        uuid[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Defining service...");
    
    // Call GATT server define service command
    int32_t serviceHandle;
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, uuid, uuidLen, &serviceHandle);
    
    if (result == 0) {
        printf("  Service defined with handle: %d\n", serviceHandle);
        printf("  Next steps:\n");
        printf("    1. Add characteristics to this service\n");
        printf("    2. Call uCxGattServerServiceActivate() to activate\n");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Service defined successfully.");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to define service (code %d)", result);
    }
}

static void gattServerSetCharacteristic(void)
{
    if (!gUcxConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Server: Set Attribute Value ---");
    printf("Enter attribute handle: ");
    
    int attrHandle;
    scanf("%d", &attrHandle);
    getchar(); // consume newline
    
    printf("Enter data (hex format, e.g., 01020304): ");
    char hexInput[MAX_DATA_BUFFER * 2 + 1];
    if (fgets(hexInput, sizeof(hexInput), stdin) == NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read input");
        return;
    }
    
    // Remove newline
    hexInput[strcspn(hexInput, "\n")] = '\0';
    
    // Convert hex string to bytes
    size_t hexLen = strlen(hexInput);
    if (hexLen % 2 != 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Invalid hex data (must be even number of digits)");
        return;
    }
    
    uint8_t data[MAX_DATA_BUFFER];
    size_t dataLen = hexLen / 2;
    
    for (size_t i = 0; i < dataLen; i++) {
        char byteStr[3] = {hexInput[i*2], hexInput[i*2 + 1], '\0'};
        data[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Setting attribute value (%zu bytes)...", dataLen);
    
    // Call GATT server set attribute value command
    int32_t result = uCxGattServerSetAttrValue(&gUcxHandle, attrHandle, data, (int32_t)dataLen);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Value set successfully.");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to set value (code %d)", result);
    }
}

static void gattServerAddCharacteristic(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gGattServerServiceHandle == -1) {
        printf("\nERROR: No service created yet. Use option [1] to add a service first.\n");
        printf("Tip: Or use option [5] for the Heartbeat service example.\n");
        return;
    }
    
    printf("\n--- GATT Server: Add Characteristic ---\n");
    printf("Service handle: %d\n\n", gGattServerServiceHandle);
    
    // Get UUID
    printf("Enter characteristic UUID (hex, e.g., 2A37 for Heart Rate Measurement): ");
    char uuidStr[33];
    if (fgets(uuidStr, sizeof(uuidStr), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    uuidStr[strcspn(uuidStr, "\n")] = '\0';
    
    // Convert hex string to bytes
    size_t hexLen = strlen(uuidStr);
    if (hexLen % 2 != 0) {
        printf("ERROR: Invalid UUID (must be even number of hex digits)\n");
        return;
    }
    
    uint8_t uuid[16];
    int32_t uuidLen = (int32_t)(hexLen / 2);
    for (int i = 0; i < uuidLen; i++) {
        char byteStr[3] = {uuidStr[i*2], uuidStr[i*2 + 1], '\0'};
        uuid[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    // Get properties
    printf("\nSelect properties (you can combine):\n");
    printf("  [1] Read\n");
    printf("  [2] Write\n");
    printf("  [3] Write Without Response\n");
    printf("  [4] Notify\n");
    printf("  [5] Indicate\n");
    printf("Enter property numbers (e.g., 14 for Read+Notify): ");
    
    char propInput[32];
    if (fgets(propInput, sizeof(propInput), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    // Parse properties
    int32_t properties = 0;
    for (int i = 0; propInput[i] != '\0' && propInput[i] != '\n'; i++) {
        switch (propInput[i]) {
            case '1': properties |= 0x02; break; // Read
            case '2': properties |= 0x08; break; // Write
            case '3': properties |= 0x04; break; // Write Without Response
            case '4': properties |= 0x10; break; // Notify
            case '5': properties |= 0x20; break; // Indicate
        }
    }
    
    if (properties == 0) {
        printf("ERROR: No valid properties selected\n");
        return;
    }
    
    printf("\nAdding characteristic to service %d...\n", gGattServerServiceHandle);
    
    // Properties as byte array
    uint8_t propBytes[] = {(uint8_t)properties};
    
    // Call uCxGattServerCharDefine6 with all parameters
    uCxGattServerCharDefine_t response;
    uint8_t defaultValue[] = {0x00}; // Default value
    
    int32_t result = uCxGattServerCharDefine6(&gUcxHandle, uuid, uuidLen,
                                              propBytes, 1, // properties as byte array
                                              U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE, // no security
                                              defaultValue, 1, // default value
                                              20, // max 20 bytes
                                              &response);
    
    if (result == 0) {
        gGattServerCharHandle = response.value_handle;
        printf("✓ Characteristic added successfully!\n");
        printf("  Characteristic handle: %d\n", gGattServerCharHandle);
        if (response.cccd_handle > 0) {
            printf("  CCCD handle: %d (for notifications)\n", response.cccd_handle);
        }
        printf("  Properties: 0x%02X\n", properties);
        printf("\nNext steps:\n");
        printf("  1. Repeat [2] to add more characteristics (optional)\n");
        printf("  2. Use [3] to set the characteristic value\n");
        printf("  3. Use [4] to send notifications (if Notify property is set)\n");
        printf("\nNote: Service must be activated before it's visible to clients.\n");
        printf("      This happens automatically when you set a characteristic value.\n");
    } else {
        printf("ERROR: Failed to add characteristic (code %d)\n", result);
    }
}

static void gattServerSendNotification(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentGattConnHandle == -1) {
        printf("\nERROR: No active Bluetooth connection.\n");
        printf("Tip: Use Bluetooth menu to connect first.\n");
        return;
    }
    
    if (gGattServerCharHandle == -1) {
        printf("\nERROR: No characteristic created yet.\n");
        printf("Tip: Use option [2] to add a characteristic with Notify property.\n");
        return;
    }
    
    printf("\n--- GATT Server: Send Notification ---\n");
    printf("Connection handle: %d\n", gCurrentGattConnHandle);
    printf("Characteristic handle: %d\n\n", gGattServerCharHandle);
    
    printf("Enter data (hex format, e.g., 01020304): ");
    char hexInput[MAX_DATA_BUFFER * 2 + 1];
    if (fgets(hexInput, sizeof(hexInput), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    // Remove newline
    hexInput[strcspn(hexInput, "\n")] = '\0';
    
    // Convert hex string to bytes
    size_t hexLen = strlen(hexInput);
    if (hexLen % 2 != 0) {
        printf("ERROR: Invalid hex data (must be even number of digits)\n");
        return;
    }
    
    uint8_t data[MAX_DATA_BUFFER];
    size_t dataLen = hexLen / 2;
    
    for (size_t i = 0; i < dataLen; i++) {
        char byteStr[3] = {hexInput[i*2], hexInput[i*2 + 1], '\0'};
        data[i] = (uint8_t)strtol(byteStr, NULL, 16);
    }
    
    printf("Sending notification (%zu bytes)...\n", dataLen);
    
    // Call GATT server send notification command
    int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle, 
                                                   gGattServerCharHandle, data, (int32_t)dataLen);
    
    if (result == 0) {
        printf("✓ Notification sent successfully.\n");
    } else {
        printf("ERROR: Failed to send notification (code %d)\n", result);
        printf("Make sure:\n");
        printf("  - Client is connected\n");
        printf("  - Client has subscribed to notifications (CCCD enabled)\n");
        printf("  - Characteristic has Notify property\n");
    }
}

static void gattServerSetupHeartbeat(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("GATT Server: Heartbeat Service Setup\n");
    printf("─────────────────────────────────────────────────\n\n");
    
    // Warn if service might already exist
    if (gHeartbeatServiceHandle > 0) {
        printf("⚠️  WARNING: Heart Rate Service was already created (handle %d)\n", gHeartbeatServiceHandle);
        printf("   Old services persist until module reboot.\n");
        printf("   Recommendation: Reboot module first for clean setup.\n\n");
        printf("   Continue anyway? (y/N): ");
        
        char response[10];
        if (fgets(response, sizeof(response), stdin)) {
            if (tolower(response[0]) != 'y') {
                printf("Setup cancelled. Use main menu option [5] to reboot module.\n");
                return;
            }
        }
        printf("\n");
    }
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");
    
    printf("This will create a Heart Rate Service (0x180D) with:\n");
    printf("  - Heart Rate Measurement characteristic (0x2A37)\n");
    printf("  - Properties: Notify\n");
    printf("  - Sends heartbeat value every second when client subscribes\n\n");
    
    // Heart Rate Service UUID: 0x180D
    uint8_t serviceUuid[] = {0x18, 0x0D};
    
    printf("Step 1: Defining Heart Rate Service (UUID: 0x180D)...\n");
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, serviceUuid, 2, &gHeartbeatServiceHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to define service (code %d)\n", result);
        return;
    }
    printf("✓ Service defined with handle: %d\n\n", gHeartbeatServiceHandle);
    
    // Heart Rate Measurement UUID: 0x2A37
    uint8_t charUuid[] = {0x2A, 0x37};
    
    printf("Step 2: Adding Heart Rate Measurement characteristic (UUID: 0x2A37)...\n");
    printf("  Properties: Notify (0x10)\n");
    
    // Properties as byte array: Notify (0x10)
    uint8_t propBytes[] = {0x10};
    uint8_t defaultValue[] = {0x00, 60}; // Flags=0, Initial BPM=60
    
    uCxGattServerCharDefine_t charResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, charUuid, 2,
                                      propBytes, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      defaultValue, 2,
                                      20, // max_length: Allow up to 20 bytes for Heart Rate Measurement
                                      &charResponse);
    
    if (result != 0) {
        printf("ERROR: Failed to add characteristic (code %d)\n", result);
        return;
    }
    gHeartbeatCharHandle = charResponse.value_handle;
    printf("✓ Characteristic added with handle: %d\n", gHeartbeatCharHandle);
    if (charResponse.cccd_handle > 0) {
        gHeartbeatCccdHandle = charResponse.cccd_handle;
        printf("  CCCD handle: %d (for notifications)\n\n", charResponse.cccd_handle);
    } else {
        printf("\n");
    }
    
    // Activate the service
    printf("Step 3: Activating service...\n");
    result = uCxGattServerServiceActivate(&gUcxHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to activate service (code %d)\n", result);
        return;
    }
    printf("✓ Service activated successfully!\n\n");
    
    // Initialize heartbeat counter (value will be sent via notifications)
    gHeartbeatCounter = 60; // Start at 60 BPM
    printf("  Initial heartbeat value: %d BPM (will be sent when client enables notifications)\n\n", gHeartbeatCounter);
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("Heartbeat Service Ready!\n");
    printf("─────────────────────────────────────────────────\n\n");
    
    printf("Next steps:\n");
    printf("  1. Connect a GATT client (phone app, etc.)\n");
    printf("  2. Client should discover Heart Rate Service\n");
    printf("  3. Client enables notifications on Heart Rate Measurement\n");
    printf("  4. Server automatically sends heartbeat updates every second (58-72 BPM)\n\n");
    
    printf("Auto-Notification System:\n");
    printf("  - Updates sent every 1 second when notifications enabled\n");
    printf("  - Shared thread handles Battery, Heartbeat, and CTS notifications\n");
    printf("  - Thread starts/stops automatically based on client subscriptions\n\n");
    
    // Store handles for later use
    gGattServerServiceHandle = gHeartbeatServiceHandle;
    gGattServerCharHandle = gHeartbeatCharHandle;
}

// GATT Server URC handler - called when client writes to a characteristic
static void gattServerCharWriteUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, 
                                   int32_t value_handle, uByteArray_t *value, uGattServerOptions_t options)
{
    (void)puCxHandle;
    (void)options;
    
    printf("\n[CCCD WRITE] conn=%d, handle=%d, len=%zu", 
           conn_handle, value_handle, value->length);
    
    // Print the value being written
    if (value->length >= 2) {
        uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
        printf(", value=0x%04X", cccdValue);
        if (cccdValue & 0x0001) printf(" (Notifications)");
        if (cccdValue & 0x0002) printf(" (Indications)");
    }
    printf("\n");
    
    // Log the current CCCD handles for debugging
    printf("[DEBUG] Known CCCD handles: BootKbd=%d, Keyboard=%d, Battery=%d, Heartbeat=%d, CTS=%d, ESS_Temp=%d, ESS_Hum=%d\n", 
           gHidBootKbdCccdHandle, gHidKeyboardCccdHandle, gBatteryCccdHandle, 
           gHeartbeatCccdHandle, gCtsServerTimeCccdHandle, 
           gEnvServerTempCccdHandle, gEnvServerHumCccdHandle);
    
    // Update current connection handle (client is connected!)
    if (gCurrentGattConnHandle != conn_handle) {
        gCurrentGattConnHandle = conn_handle;
        printf("[DEBUG] Updated GATT connection handle to %d\n", conn_handle);
    }
    
    // Check if this is a CCCD write for Boot Keyboard Input
    if (gHidBootKbdCccdHandle > 0 && value_handle == gHidBootKbdCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[HID Boot Keyboard] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gHidBootKbdNotificationsEnabled = true;
                gUseBootKeyboard = true;  // Prefer Boot Keyboard for sending
            } else {
                printf("\n[HID Boot Keyboard] Client disabled notifications\n");
                gHidBootKbdNotificationsEnabled = false;
                // If regular keyboard is still enabled, switch to it
                if (gHidKeyboardNotificationsEnabled) {
                    gUseBootKeyboard = false;
                } else {
                    gUseBootKeyboard = false;
                }
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for HID Keyboard Input
    if (gHidKeyboardCccdHandle > 0 && value_handle == gHidKeyboardCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[HID Keyboard] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gHidKeyboardNotificationsEnabled = true;
                // Only switch to regular keyboard if boot keyboard is not already preferred
                if (!gHidBootKbdNotificationsEnabled) {
                    gUseBootKeyboard = false;
                }
            } else {
                printf("\n[HID Keyboard] Client disabled notifications\n");
                gHidKeyboardNotificationsEnabled = false;
                // If boot keyboard is still enabled, keep using it
                if (gHidBootKbdNotificationsEnabled) {
                    gUseBootKeyboard = true;
                }
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for HID Media Input
    if (gHidMediaCccdHandle > 0 && value_handle == gHidMediaCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[HID Media] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gHidMediaNotificationsEnabled = true;
            } else {
                printf("\n[HID Media] Client disabled notifications\n");
                gHidMediaNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for Battery Level
    if (gBatteryCccdHandle > 0 && value_handle == gBatteryCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[Battery] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gBatteryNotificationsEnabled = true;
                gBatteryLevel = 100; // Reset to 100%
                
                // Start unified notification thread if not running
                if (gGattNotificationThread == NULL) {
                    gGattNotificationThreadRunning = true;
                    gGattNotificationThread = CreateThread(NULL, 0, gattNotificationThread, NULL, 0, NULL);
                    if (gGattNotificationThread) {
                        printf("[GATT] Unified notification thread started\n");
                    }
                }
                
                // Send initial battery level
                int32_t result = uCxGattServerSendNotification(&gUcxHandle, conn_handle, 
                                                               gBatteryLevelHandle, &gBatteryLevel, 1);
                if (result == 0) {
                    printf("[Battery] Sent initial battery level: 100%%\n");
                }
            } else {
                printf("\n[Battery] Client disabled notifications\n");
                gBatteryNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for CTS (Current Time Service)
    if (gCtsServerTimeCccdHandle > 0 && value_handle == gCtsServerTimeCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[CTS] Notifications ENABLED\n");
                gCtsServerNotificationsEnabled = true;
                
                // Start unified notification thread if not running
                if (gGattNotificationThread == NULL) {
                    gGattNotificationThreadRunning = true;
                    gGattNotificationThread = CreateThread(NULL, 0, gattNotificationThread, NULL, 0, NULL);
                    if (gGattNotificationThread) {
                        printf("[GATT] Unified notification thread started\n");
                    }
                }
            } else {
                printf("\n[CTS] Notifications DISABLED\n");
                gCtsServerNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // HID Protocol Mode (0 = Boot, 1 = Report)
    if (value_handle == gHidProtocolModeHandle && value->length >= 1) {
        uint8_t mode = value->pData[0];
        gUseBootKeyboard = (mode == 0x00);
        printf("[HID] Protocol mode set to %s (%u)\n",
               gUseBootKeyboard ? "BOOT" : "REPORT", mode);
        return;
    }
    
    // HID Keyboard Output Report (LEDs)
    if (value_handle == gHidKeyboardOutputHandle && value->length >= 1) {
        uint8_t leds = value->pData[0];
        printf("[HID] Keyboard LEDs updated: 0x%02X (Num=%d Caps=%d Scroll=%d)\n",
               leds,
               (leds & 0x01) != 0,
               (leds & 0x02) != 0,
               (leds & 0x04) != 0);
        // Optionally store to a global, update a console "LED" state, etc.
        return;
    }
    
    // Boot Keyboard Output Report (LEDs in boot mode)
    if (value_handle == gHidBootKbdOutputHandle && value->length >= 1) {
        uint8_t leds = value->pData[0];
        printf("[HID] Boot keyboard LEDs updated: 0x%02X\n", leds);
        return;
    }
    
    // HID Control Point (Suspend / Exit Suspend)
    if (value_handle == gHidControlPointHandle && value->length >= 1) {
        uint8_t ctrl = value->pData[0];   // 0 = Suspend, 1 = Exit Suspend
        printf("[HID] Control Point: %s (%u)\n",
               (ctrl == 0) ? "Suspend" : "Exit Suspend", ctrl);
        // You could mute key sending while suspended if you want.
        return;
    }
    
    // Automation IO: Digital CCCD (notifications)
    if (gAioServerDigitalCccdHandle > 0 && value_handle == gAioServerDigitalCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            printf("\n[AIO Digital] Client %s notifications (CCCD=0x%04X)\n",
                   (cccdValue & 0x0001) ? "ENABLED" : "DISABLED",
                   cccdValue);
        }
        return;
    }

    // Automation IO: Analog CCCD (notifications)
    if (gAioServerAnalogCccdHandle > 0 && value_handle == gAioServerAnalogCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            printf("\n[AIO Analog] Client %s notifications (CCCD=0x%04X)\n",
                   (cccdValue & 0x0001) ? "ENABLED" : "DISABLED",
                   cccdValue);
        }
        return;
    }

    // Automation IO: Digital value write (client controls virtual digital outputs)
    if (gAioServerDigitalCharHandle > 0 && value_handle == gAioServerDigitalCharHandle) {
        if (value->length < 1) {
            printf("\n[AIO Digital] Invalid write (len=%zu)\n", value->length);
            return;
        }

        uint8_t newState = value->pData[0];
        uint8_t oldState = gAioServerDigitalState;
        gAioServerDigitalState = newState;

        printf("\n[AIO Digital] New digital state: 0x%02X (bit0/LED0 = %u)\n",
               gAioServerDigitalState,
               (gAioServerDigitalState & 0x01) ? 1 : 0);

        // Reflect this change back to subscribed clients
        if (gCurrentGattConnHandle >= 0 && gAioServerDigitalCccdHandle > 0) {
            int32_t result = uCxGattServerSendNotification(&gUcxHandle,
                                                           gCurrentGattConnHandle,
                                                           gAioServerDigitalCharHandle,
                                                           &gAioServerDigitalState,
                                                           1);
            if (result == 0) {
                printf("[AIO Digital] Notified subscribers of new state.\n");
            } else {
                printf("[AIO Digital] WARNING: notify failed (code %d)\n", result);
            }
        }

        (void)oldState; // Available for comparison/debounce if needed
        return;
    }
    
    // Check if this is a CCCD write for ESS Temperature
    if (gEnvServerTempCccdHandle > 0 && value_handle == gEnvServerTempCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[ESS Temperature] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gEssServerTempNotificationsEnabled = true;
                
                // Start unified notification thread if not running
                if (gGattNotificationThread == NULL) {
                    gGattNotificationThreadRunning = true;
                    gGattNotificationThread = CreateThread(NULL, 0, gattNotificationThread, NULL, 0, NULL);
                    if (gGattNotificationThread) {
                        printf("[GATT] Unified notification thread started\n");
                    }
                }
            } else {
                printf("\n[ESS Temperature] Client disabled notifications\n");
                gEssServerTempNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for UART TX
    if (gUartServerTxCccdHandle > 0 && value_handle == gUartServerTxCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[UART] Client enabled TX notifications (CCCD handle %d)\n", value_handle);
                gUartServerTxNotificationsEnabled = true;
                printf("[UART] You can now send data using uCxGattServerSendNotification()\n");
            } else {
                printf("\n[UART] Client disabled TX notifications\n");
                gUartServerTxNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // Check if this is a CCCD write for ESS Humidity
    if (gEnvServerHumCccdHandle > 0 && value_handle == gEnvServerHumCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[ESS Humidity] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gEssServerHumNotificationsEnabled = true;
                
                // Start unified notification thread if not running
                if (gGattNotificationThread == NULL) {
                    gGattNotificationThreadRunning = true;
                    gGattNotificationThread = CreateThread(NULL, 0, gattNotificationThread, NULL, 0, NULL);
                    if (gGattNotificationThread) {
                        printf("[GATT] Unified notification thread started\n");
                    }
                }
            } else {
                printf("\n[ESS Humidity] Client disabled notifications\n");
                gEssServerHumNotificationsEnabled = false;
            }
        }
        return;
    }
    
    // Unknown CCCD write - log it for debugging
    if (value->length >= 2) {
        uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Unknown CCCD write to handle %d, value=0x%04X", 
                      value_handle, cccdValue);
        printf("\n[INFO] Client wrote to handle %d (not a known HID CCCD)\n", value_handle);
    }
    
    // Check if this is a CCCD write for the heartbeat characteristic
    if (gHeartbeatCccdHandle > 0 && value_handle == gHeartbeatCccdHandle) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Heartbeat CCCD write detected! (handle=%d, expected=%d)", 
                      value_handle, gHeartbeatCccdHandle);
        
        // CCCD write: 0x0001 = enable notifications, 0x0002 = enable indications, 0x0000 = disable
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "CCCD value: 0x%04X", cccdValue);
            printf("[DEBUG] Heartbeat CCCD write: conn=%d, char_handle=%d, cccd_handle=%d, value=0x%04X\n",
                   conn_handle, gHeartbeatCharHandle, gHeartbeatCccdHandle, cccdValue);
            
            if (cccdValue & 0x0001) {  // Bit 0 = Notifications enabled
                // Client enabled notifications
                printf("\n[Heartbeat] Client enabled notifications (conn=%d stored as gCurrentGattConnHandle)\n", conn_handle);
                gHeartbeatNotificationsEnabled = true;
                
                // Start unified notification thread if not running
                if (gGattNotificationThread == NULL) {
                    gGattNotificationThreadRunning = true;
                    gGattNotificationThread = CreateThread(NULL, 0, gattNotificationThread, NULL, 0, NULL);
                    if (gGattNotificationThread) {
                        printf("[GATT] Unified notification thread started\n");
                    } else {
                        printf("[GATT] ERROR: Failed to start notification thread\n");
                        gGattNotificationThreadRunning = false;
                    }
                }
            } else {
                // Client disabled notifications (cccdValue == 0x0000)
                printf("\n[Heartbeat] Client disabled notifications\n");
                gHeartbeatNotificationsEnabled = false;
            }
        }
    }
    
    // Handle UART RX writes
    if (gUartServerRxHandle > 0 && value_handle == gUartServerRxHandle) {
        printf("\n[UART RX] Data from conn=%d: ", conn_handle);
        for (size_t i = 0; i < value->length; i++) {
            uint8_t b = value->pData[i];
            if (b >= 32 && b <= 126) putchar(b);
            else printf("\\x%02X", b);
        }
        printf("\n");
        return;
    }

    // Handle SPS FIFO CCCD writes
    if (gSpsServerFifoCccdHandle > 0 && value_handle == gSpsServerFifoCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[SPS FIFO] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gSpsServerFifoNotifyEnabled = true;
            } else {
                printf("\n[SPS FIFO] Client disabled notifications\n");
                gSpsServerFifoNotifyEnabled = false;
            }
        }
        return;
    }

    // Handle SPS Credits CCCD writes
    if (gSpsServerCreditsCccdHandle > 0 && value_handle == gSpsServerCreditsCccdHandle) {
        if (value->length >= 2) {
            uint16_t cccdValue = value->pData[0] | (value->pData[1] << 8);
            if (cccdValue & 0x0001) {
                printf("\n[SPS Credits] Client enabled notifications (CCCD handle %d)\n", value_handle);
                gSpsServerCreditsNotifyEnabled = true;
            } else {
                printf("\n[SPS Credits] Client disabled notifications\n");
                gSpsServerCreditsNotifyEnabled = false;
            }
        }
        return;
    }

    // Handle SPS FIFO data writes
    if (gSpsServerFifoHandle > 0 && value_handle == gSpsServerFifoHandle) {
        printf("\n[SPS FIFO RX] Data from conn=%d: ", conn_handle);
        for (size_t i = 0; i < value->length; i++) {
            uint8_t b = value->pData[i];
            if (b >= 32 && b <= 126) putchar(b);
            else printf("\\x%02X", b);
        }
        printf("\n");
        
        // If flow control active, send credits back
        if (gSpsServerFlowControlActive && gSpsServerCreditsNotifyEnabled) {
            int8_t credits = 1;  // Give 1 credit per received message
            int32_t result = uCxGattServerSendNotification(&gUcxHandle, conn_handle,
                                                           gSpsServerCreditsHandle, 
                                                           (uint8_t*)&credits, 1);
            if (result == 0) {
                printf("[SPS] Sent 1 credit to client\n");
            }
        }
        return;
    }

    // Handle SPS Credits writes
    if (gSpsServerCreditsHandle > 0 && value_handle == gSpsServerCreditsHandle) {
        if (value->length >= 1) {
            int8_t credits = (int8_t)value->pData[0];
            
            if (credits == -1) {
                printf("\n[SPS Credits] Client DISCONNECTED flow control (credits=-1)\n");
                gSpsServerFlowControlActive = false;
                gSpsServerRemoteCredits = 0;
            } else if (credits > 0) {
                gSpsServerRemoteCredits += credits;
                printf("\n[SPS Credits] Received %d credits (total: %d)\n", 
                       credits, gSpsServerRemoteCredits);
                
                // First credit received = flow control activated
                if (!gSpsServerFlowControlActive) {
                    printf("[SPS] Flow control ACTIVATED\n");
                    gSpsServerFlowControlActive = true;
                    
                    // Send credits back to establish bidirectional flow control
                    if (gSpsServerCreditsNotifyEnabled) {
                        int8_t responseCredits = 10;
                        int32_t result = uCxGattServerSendNotification(&gUcxHandle, conn_handle,
                                                                       gSpsServerCreditsHandle,
                                                                       (uint8_t*)&responseCredits, 1);
                        if (result == 0) {
                            printf("[SPS] Sent %d credits to client (accepting flow control)\n", responseCredits);
                        }
                    }
                }
            }
        }
        return;
    }
}

// GATT Server URC handler - called when client reads a characteristic
static void gattServerCharReadUrc(struct uCxHandle *puCxHandle, int32_t conn_handle,
                                  int32_t value_handle)
{
    (void)puCxHandle;
    
    printf("[GATT Read] conn=%d, handle=%d", conn_handle, value_handle);
    
    // Try to identify what was read
    const char *name = NULL;
    if (value_handle == gHidInfoHandle)              name = "HID Info";
    else if (value_handle == gHidReportMapHandle)    name = "HID Report Map";
    else if (value_handle == gHidKeyboardInputHandle) name = "HID Keyboard Input";
    else if (value_handle == gHidMediaInputHandle)   name = "HID Media Input";
    else if (value_handle == gHidControlPointHandle) name = "HID Control Point";
    else if (value_handle == gBatteryLevelHandle)    name = "Battery Level";
    else if (value_handle == gGattServiceChangedHandle) name = "Service Changed";
    
    printf(" (");
    printf("%s", name ? name : "Unknown");
    printf(")\n");
}

// Background thread that sends heartbeat notifications every second
static DWORD WINAPI heartbeatThread(LPVOID lpParam)
{
    (void)lpParam;
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Heartbeat thread started");
    printf("[Heartbeat] Thread running (enabled=%d, conn_handle=%d, char_handle=%d)\n",
           gHeartbeatNotificationsEnabled, gCurrentGattConnHandle, gHeartbeatCharHandle);
    
    while (gHeartbeatThreadRunning) {
        // Only send if notifications are enabled and we have a connection
        if (gHeartbeatNotificationsEnabled && gCurrentGattConnHandle >= 0) {
            // Vary the heart rate slightly for realism (58-72 BPM)
            static int direction = 1;
            gHeartbeatCounter = (uint8_t)(gHeartbeatCounter + direction);
            if (gHeartbeatCounter >= 72) direction = -1;
            if (gHeartbeatCounter <= 58) direction = 1;
            
            // Build heart rate measurement value: [Flags, BPM]
            uint8_t heartbeatData[] = {0x00, gHeartbeatCounter};
            
            // Send notification
            int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                           gHeartbeatCharHandle, 
                                                           heartbeatData, sizeof(heartbeatData));
            
            if (result == 0) {
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Heartbeat notification sent: %d BPM", gHeartbeatCounter);
            } else {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to send heartbeat notification (code %d)", result);
            }
        }
        
        // Wait 1 second
        Sleep(1000);
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Heartbeat thread stopped");
    return 0;
}

// Unified GATT server notification thread
// Handles periodic notifications for Battery, CTS, and Heartbeat services
static DWORD WINAPI gattNotificationThread(LPVOID lpParam)
{
    (void)lpParam;
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "GATT notification thread started");
    printf("[GATT] Unified notification thread running\n");
    
    int consecutiveErrors = 0;
    const int MAX_CONSECUTIVE_ERRORS = 3;
    
    while (gGattNotificationThreadRunning) {
        // Only send if we have an active connection
        if (gCurrentGattConnHandle >= 0 && gUcxConnected) {
            
            // 1. Send Heartbeat notification (if enabled)
            if (gHeartbeatNotificationsEnabled && gHeartbeatCharHandle > 0) {
                // Vary the heart rate slightly for realism (58-72 BPM)
                static int direction = 1;
                gHeartbeatCounter = (uint8_t)(gHeartbeatCounter + direction);
                if (gHeartbeatCounter >= 72) direction = -1;
                if (gHeartbeatCounter <= 58) direction = 1;
                
                uint8_t heartbeatData[] = {0x00, gHeartbeatCounter}; // [Flags, BPM]
                
                int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                               gHeartbeatCharHandle, 
                                                               heartbeatData, sizeof(heartbeatData));
                if (result == 0) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Heartbeat: %d BPM", gHeartbeatCounter);
                    consecutiveErrors = 0;  // Reset error counter on success
                } else {
                    consecutiveErrors++;
                    printf("[ERROR] Failed to send heartbeat notification (code %d), errors: %d/%d\n", 
                           result, consecutiveErrors, MAX_CONSECUTIVE_ERRORS);
                    
                    // If we get too many consecutive errors, connection is likely dead
                    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                        printf("[GATT] Too many consecutive errors, stopping notification thread\n");
                        printf("[GATT] Connection appears to be lost (handle %d)\n", gCurrentGattConnHandle);
                        gGattNotificationThreadRunning = false;
                        gCurrentGattConnHandle = -1;
                        break;
                    }
                }
            }
            
            // 2. Send Battery notification (if enabled)
            if (gBatteryNotificationsEnabled && gBatteryLevelHandle > 0) {
                // Slowly decrease battery level (reset at 10%)
                if (gBatteryLevel > 10) {
                    gBatteryLevel--;
                } else {
                    gBatteryLevel = 100;
                }
                
                int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                               gBatteryLevelHandle, 
                                                               &gBatteryLevel, 1);
                if (result == 0) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Battery: %d%%", gBatteryLevel);
                    consecutiveErrors = 0;  // Reset error counter on success
                } else {
                    consecutiveErrors++;
                    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                        printf("[GATT] Too many consecutive errors, stopping notification thread\n");
                        gGattNotificationThreadRunning = false;
                        gCurrentGattConnHandle = -1;
                        break;
                    }
                }
            }
            
            // 3. Send Current Time notification (if enabled)
            if (gCtsServerNotificationsEnabled && gCtsServerTimeValueHandle > 0) {
                uint8_t payload[10];
                ctsBuildTimePayload(payload);
                
                int32_t result = uCxGattServerSendNotification(&gUcxHandle,
                                                               gCurrentGattConnHandle,
                                                               gCtsServerTimeValueHandle,
                                                               payload, 10);
                if (result == 0) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "CTS: %04u-%02u-%02u %02u:%02u:%02u",
                                  payload[0] | (payload[1] << 8),
                                  payload[2], payload[3], payload[4], payload[5], payload[6]);
                    consecutiveErrors = 0;  // Reset error counter on success
                } else {
                    consecutiveErrors++;
                    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                        printf("[GATT] Too many consecutive errors, stopping notification thread\n");
                        gGattNotificationThreadRunning = false;
                        gCurrentGattConnHandle = -1;
                        break;
                    }
                }
            }
            
            // 4. Send ESS Temperature notification (if enabled)
            if (gEssServerTempNotificationsEnabled && gEnvServerTempHandle > 0) {
                // Vary temperature (20.00°C to 30.00°C in 0.25°C increments)
                static int16_t tempDirection = 25;  // 0.25°C steps
                gEssServerTempValue = (int16_t)(gEssServerTempValue + tempDirection);
                if (gEssServerTempValue >= 3000) tempDirection = -25;  // 30.00°C
                if (gEssServerTempValue <= 2000) tempDirection = 25;   // 20.00°C
                
                int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                               gEnvServerTempHandle,
                                                               (uint8_t*)&gEssServerTempValue, 2);
                if (result == 0) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "ESS Temperature: %d.%02d°C", 
                                  gEssServerTempValue / 100, abs(gEssServerTempValue % 100));
                    consecutiveErrors = 0;  // Reset error counter on success
                } else {
                    consecutiveErrors++;
                    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                        printf("[GATT] Too many consecutive errors, stopping notification thread\n");
                        gGattNotificationThreadRunning = false;
                        gCurrentGattConnHandle = -1;
                        break;
                    }
                }
            }
            
            // 5. Send ESS Humidity notification (if enabled)
            if (gEssServerHumNotificationsEnabled && gEnvServerHumHandle > 0) {
                // Vary humidity (30.00% to 70.00% in 1% increments)
                static int16_t humDirection = 100;  // 1% steps
                gEssServerHumValue = (uint16_t)(gEssServerHumValue + humDirection);
                if (gEssServerHumValue >= 7000) humDirection = -100;  // 70.00%
                if (gEssServerHumValue <= 3000) humDirection = 100;   // 30.00%
                
                int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                               gEnvServerHumHandle,
                                                               (uint8_t*)&gEssServerHumValue, 2);
                if (result == 0) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "ESS Humidity: %u.%02u%%", 
                                  gEssServerHumValue / 100, gEssServerHumValue % 100);
                    consecutiveErrors = 0;  // Reset error counter on success
                } else {
                    consecutiveErrors++;
                    if (consecutiveErrors >= MAX_CONSECUTIVE_ERRORS) {
                        printf("[GATT] Too many consecutive errors, stopping notification thread\n");
                        gGattNotificationThreadRunning = false;
                        gCurrentGattConnHandle = -1;
                        break;
                    }
                }
            }
        }
        
        // Wait 1 second before next update
        Sleep(1000);
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "GATT notification thread stopped");
    return 0;
}

// ----------------------------------------------------------------
// GATT Client Notification Handlers
// ----------------------------------------------------------------

// Wrapper to adapt UCX URC signature to our central dispatcher
// Central notification/indication dispatcher - called from UCX URC callbacks
// Handles both GATT notifications and indications from remote GATT servers
static void gattClientNotificationUrc(struct uCxHandle *puCxHandle,
                                      int32_t conn_handle,
                                      int32_t value_handle,
                                      uByteArray_t *hex_data)
{
    (void)puCxHandle;  // Unused
    
    printf("\n[GATT Notify/Indicate] conn=%d handle=0x%04X len=%zu\n",
           conn_handle, value_handle, hex_data->length);

    // HEART RATE ---------------------------------------------------
    if (value_handle == gHeartRateValueHandle) {
        handleHeartRateNotification(conn_handle, hex_data->pData, hex_data->length);
        return;
    }

    // UART RX ------------------------------------------------------
    if (value_handle == gUartTxValueHandle) {
        handleUartRxNotification(conn_handle, hex_data->pData, hex_data->length);
        return;
    }

    // CURRENT TIME SERVICE (CTS) -----------------------------------
    if (value_handle == gCtsClientTimeValueHandle) {
        ctsParseAndPrint(hex_data->pData, hex_data->length);
        return;
    }

    // ENVIRONMENTAL SENSING (ESS) - Temperature --------------------
    if (value_handle == gEssClientTempValueHandle) {
        essParseTemperature(hex_data->pData, hex_data->length);
        return;
    }

    // ENVIRONMENTAL SENSING (ESS) - Humidity -----------------------
    if (value_handle == gEssClientHumValueHandle) {
        essParseHumidity(hex_data->pData, hex_data->length);
        return;
    }

    // LOCATION & NAVIGATION SERVICE (LNS) --------------------------
    if (value_handle == gLnsClientLocValueHandle) {
        lnsParseLocation(hex_data->pData, hex_data->length);
        return;
    }

    // UART SERVICE (NUS) - TX (Notify) -----------------------------
    if (value_handle == gUartClientTxValueHandle) {
        uartParseRxData(hex_data->pData, hex_data->length);
        return;
    }

    // SERIAL PORT SERVICE (SPS) - FIFO (Notify/Indicate) -----------
    if (value_handle == gSpsClientFifoValueHandle) {
        spsParseFifoData(hex_data->pData, hex_data->length);
        return;
    }

    // SERIAL PORT SERVICE (SPS) - Credits (Notify/Indicate) --------
    if (value_handle == gSpsClientCreditsValueHandle) {
        spsParseCredits(hex_data->pData, hex_data->length);
        return;
    }

    // BATTERY SERVICE (BAS) - Battery Level ------------------------
    if (value_handle == gBasClientValueHandle) {
        basParseBatteryLevel(hex_data->pData, hex_data->length);
        return;
    }

    // AUTOMATION IO (AIO) - Digital --------------------------------
    if (value_handle == gAioClientDigitalValueHandle) {
        aioParseDigital(hex_data->pData, hex_data->length);
        return;
    }

    // AUTOMATION IO (AIO) - Analog ---------------------------------
    if (value_handle == gAioClientAnalogValueHandle) {
        aioParseAnalog(hex_data->pData, hex_data->length);
        return;
    }

    // Unknown characteristic ---------------------------------------
    printf("[GATT Notify] Unknown notification source (handle=0x%04X)\n",
           value_handle);

    printf("Data:");
    for (size_t i = 0; i < hex_data->length; i++)
        printf(" %02X", hex_data->pData[i]);
    printf("\n");
}

// Handle Heart Rate Measurement notifications (0x2A37)
static void handleHeartRateNotification(int connHandle, const uint8_t *data, size_t len)
{
    (void)connHandle;
    
    if (len < 2) {
        printf("[Heart Rate] Invalid data length: %zu\n", len);
        return;
    }
    
    uint8_t flags = data[0];
    bool is16bit = (flags & 0x01) != 0;
    
    uint16_t bpm;
    if (is16bit && len >= 3) {
        bpm = data[1] | (data[2] << 8);
    } else {
        bpm = data[1];
    }
    
    printf("[Heart Rate] BPM: %u\n", bpm);
}

// Handle UART RX notifications (data from server TX characteristic)
static void handleUartRxNotification(int connHandle, const uint8_t *data, size_t len)
{
    (void)connHandle;
    
    printf("[UART RX] Received %zu bytes: ", len);
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            printf("%c", data[i]);
        } else {
            printf("[0x%02X]", data[i]);
        }
    }
    printf("\n");
}

// ----------------------------------------------------------------
// CTS (Current Time Service) Client Functions
// ----------------------------------------------------------------

// Parse and display CTS time packet
static void ctsParseAndPrint(const uint8_t *data, size_t len)
{
    if (len < 10) {
        printf("[CTS] Invalid time packet (%zu bytes)\n", len);
        return;
    }

    uint16_t year   = data[0] | (data[1] << 8);
    uint8_t  month  = data[2];
    uint8_t  day    = data[3];
    uint8_t  hour   = data[4];
    uint8_t  minute = data[5];
    uint8_t  second = data[6];
    uint8_t  dow    = data[7];   // 1 = Monday … 7 = Sunday

    printf("[CTS] %04u-%02u-%02u  %02u:%02u:%02u  (DoW=%u)\n",
           year, month, day, hour, minute, second, dow);
}

// Find CTS service and characteristic handles
static bool gattClientFindCtsHandles()
{
    gCtsClientServiceIndex    = -1;
    gCtsClientTimeCharIndex   = -1;
    gCtsClientTimeValueHandle = -1;
    gCtsClientTimeCccdHandle  = -1;

    // 1) Look for service 0x1805
    gCtsClientServiceIndex = findServiceByUuid16(0x1805);
    if (gCtsClientServiceIndex < 0) {
        printf("[CTS] Service 0x1805 not found.\n");
        return false;
    }

    // 2) Look for characteristic 0x2A2B
    gCtsClientTimeCharIndex = findCharByUuid16InService(gCtsClientServiceIndex, 0x2A2B);
    if (gCtsClientTimeCharIndex < 0) {
        printf("[CTS] Characteristic 0x2A2B not found.\n");
        return false;
    }

    gCtsClientTimeValueHandle = gGattCharacteristics[gCtsClientTimeCharIndex].valueHandle;

    // 3) CCCD is valueHandle + 1 (typical)
    gCtsClientTimeCccdHandle = gCtsClientTimeValueHandle + 1;

    printf("[CTS] Found time value handle=0x%04X  CCCD=0x%04X\n",
           gCtsClientTimeValueHandle, gCtsClientTimeCccdHandle);

    return true;
}

// Read current time once
static void gattClientReadCtsTime()
{
    if (gCtsClientTimeValueHandle < 0) {
        printf("[CTS] Value handle not set.\n");
        return;
    }

    uint8_t buf[20];
    uByteArray_t value = { .pData = buf, .length = 0 };

    int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                       gCurrentGattConnHandle,
                                       gCtsClientTimeValueHandle,
                                       &value);

    if (r < 0) {
        printf("[CTS] Read failed (%d)\n", r);
        return;
    }

    printf("[CTS] Read %zu bytes\n", value.length);
    ctsParseAndPrint(value.pData, value.length);

    uCxEnd(&gUcxHandle);
}

// Subscribe to time notifications
static void gattClientSubscribeCts()
{
    if (gCtsClientTimeCccdHandle < 0) {
        printf("[CTS] CCCD handle invalid\n");
        return;
    }

    uint8_t enableNotify[2] = {0x01, 0x00};

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gCtsClientTimeCccdHandle,
                                        enableNotify,
                                        sizeof(enableNotify));
    if (r < 0) {
        printf("[CTS] Failed to write CCCD (%d)\n", r);
        return;
    }

    printf("[CTS] Notifications ENABLED\n\n");
}

// Complete CTS client example (discover, read, subscribe)
static void gattClientCtsExample()
{
    printf("\n--- GATT Client: Current Time Service (CTS) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection.\n");
        return;
    }

    // 1. Discover services + chars
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // 2. Locate CTS
    if (!gattClientFindCtsHandles()) {
        return;
    }

    // 3. Read once
    gattClientReadCtsTime();

    // 4. Subscribe to updates
    gattClientSubscribeCts();

    printf("CTS Client running. Time will update automatically.\n\n");
}

// ----------------------------------------------------------------
// ESS (Environmental Sensing Service) Client Functions
// ----------------------------------------------------------------

// Parse temperature notification (0x2A6E)
static void essParseTemperature(const uint8_t *data, size_t len)
{
    if (len < 2) {
        printf("[ESS] Invalid temperature packet\n");
        return;
    }

    int16_t raw = data[0] | (data[1] << 8);
    float tempC = raw / 100.0f;

    printf("[ESS] Temperature: %.2f °C\n", tempC);
}

// Parse humidity notification (0x2A6F)
static void essParseHumidity(const uint8_t *data, size_t len)
{
    if (len < 2) {
        printf("[ESS] Invalid humidity packet\n");
        return;
    }

    uint16_t raw = data[0] | (data[1] << 8);
    float hum = raw / 100.0f;

    printf("[ESS] Humidity: %.2f %%\n", hum);
}

// Find ESS service and characteristic handles
static bool gattClientFindEssHandles()
{
    gEssClientServiceIndex    = -1;
    gEssClientTempValueHandle = -1;
    gEssClientHumValueHandle  = -1;

    // 1) Service 0x181A
    gEssClientServiceIndex = findServiceByUuid16(0x181A);
    if (gEssClientServiceIndex < 0) {
        printf("[ESS] Service 0x181A not found.\n");
        return false;
    }

    // 2) Temp characteristic 0x2A6E
    gEssClientTempCharIndex = findCharByUuid16InService(gEssClientServiceIndex, 0x2A6E);
    if (gEssClientTempCharIndex >= 0) {
        gEssClientTempValueHandle = gGattCharacteristics[gEssClientTempCharIndex].valueHandle;
        gEssClientTempCccdHandle = gEssClientTempValueHandle + 1;
        printf("[ESS] Temp: handle=0x%04X, CCCD=0x%04X\n",
               gEssClientTempValueHandle, gEssClientTempCccdHandle);
    }

    // 3) Humidity characteristic 0x2A6F
    gEssClientHumCharIndex = findCharByUuid16InService(gEssClientServiceIndex, 0x2A6F);
    if (gEssClientHumCharIndex >= 0) {
        gEssClientHumValueHandle = gGattCharacteristics[gEssClientHumCharIndex].valueHandle;
        gEssClientHumCccdHandle = gEssClientHumValueHandle + 1;
        printf("[ESS] Hum:  handle=0x%04X, CCCD=0x%04X\n",
               gEssClientHumValueHandle, gEssClientHumCccdHandle);
    }

    if (gEssClientTempValueHandle < 0 && gEssClientHumValueHandle < 0) {
        printf("[ESS] No valid characteristics found.\n");
        return false;
    }

    return true;
}

// Read temperature and humidity once
static void gattClientReadEssValues()
{
    uint8_t buf[20];
    uByteArray_t value = { .pData = buf, .length = 0 };

    // Temperature
    if (gEssClientTempValueHandle > 0) {
        int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                           gCurrentGattConnHandle,
                                           gEssClientTempValueHandle,
                                           &value);

        if (r >= 0) {
            essParseTemperature(buf, value.length);
            uCxEnd(&gUcxHandle);
        }
    }

    // Humidity
    if (gEssClientHumValueHandle > 0) {
        value.length = 0;
        int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                           gCurrentGattConnHandle,
                                           gEssClientHumValueHandle,
                                           &value);

        if (r >= 0) {
            essParseHumidity(buf, value.length);
            uCxEnd(&gUcxHandle);
        }
    }
}

// Subscribe to ESS notifications
static void gattClientSubscribeEss()
{
    uint8_t notifyEnable[2] = {0x01, 0x00};

    if (gEssClientTempCccdHandle > 0) {
        uCxGattClientWriteNoRsp(&gUcxHandle,
                                gCurrentGattConnHandle,
                                gEssClientTempCccdHandle,
                                notifyEnable, 2);
        printf("[ESS] Temperature notifications ENABLED\n");
    }

    if (gEssClientHumCccdHandle > 0) {
        uCxGattClientWriteNoRsp(&gUcxHandle,
                                gCurrentGattConnHandle,
                                gEssClientHumCccdHandle,
                                notifyEnable, 2);
        printf("[ESS] Humidity notifications ENABLED\n");
    }
}

// Complete ESS client example
static void gattClientEssExample()
{
    printf("\n--- GATT Client: Environmental Sensing Service (ESS) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection.\n");
        return;
    }

    // Step 1: Discover everything
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // Step 2: Find Temperature/Humidity
    if (!gattClientFindEssHandles())
        return;

    // Step 3: Read once
    gattClientReadEssValues();

    // Step 4: Subscribe to notifications
    gattClientSubscribeEss();

    printf("ESS Client running...\n");
    printf("Live Temperature + Humidity will appear as notifications.\n\n");
}

// ----------------------------------------------------------------
// LNS (Location & Navigation Service) Client Functions
// ----------------------------------------------------------------

// Parse LNS location notification (0x2A67)
static void lnsParseLocation(const uint8_t *data, size_t len)
{
    if (len < 10) {
        printf("[LNS] Invalid packet (%zu bytes)\n", len);
        return;
    }

    uint16_t flags = data[0] | (data[1] << 8);

    int32_t latRaw = data[2] | (data[3] << 8) |
                     (data[4] << 16) | (data[5] << 24);

    int32_t lonRaw = data[6] | (data[7] << 8) |
                     (data[8] << 16) | (data[9] << 24);

    double lat = latRaw / 1e7;
    double lon = lonRaw / 1e7;

    printf("[LNS] Lat=%.7f  Lon=%.7f  (flags=0x%04X)\n", lat, lon, flags);
}

// Find LNS service and characteristic handles
static bool gattClientFindLnsHandles()
{
    gLnsClientServiceIndex   = -1;
    gLnsClientLocCharIndex   = -1;
    gLnsClientLocValueHandle = -1;

    // 1) Find LNS service (0x1819)
    gLnsClientServiceIndex = findServiceByUuid16(0x1819);
    if (gLnsClientServiceIndex < 0) {
        printf("[LNS] Service 0x1819 not found.\n");
        return false;
    }

    // 2) Find Location and Speed (0x2A67)
    gLnsClientLocCharIndex = findCharByUuid16InService(gLnsClientServiceIndex, 0x2A67);
    if (gLnsClientLocCharIndex < 0) {
        printf("[LNS] Characteristic 0x2A67 not found.\n");
        return false;
    }

    gLnsClientLocValueHandle = gGattCharacteristics[gLnsClientLocCharIndex].valueHandle;
    gLnsClientLocCccdHandle = gLnsClientLocValueHandle + 1;

    printf("[LNS] Location+Speed handle=0x%04X  CCCD=0x%04X\n",
           gLnsClientLocValueHandle, gLnsClientLocCccdHandle);

    return true;
}

// Read LNS location once
static void gattClientReadLns()
{
    if (gLnsClientLocValueHandle < 0) {
        printf("[LNS] Value handle not valid.\n");
        return;
    }

    uint8_t buf[32];
    uByteArray_t value = { .pData = buf, .length = 0 };

    int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                       gCurrentGattConnHandle,
                                       gLnsClientLocValueHandle,
                                       &value);

    if (r < 0) {
        printf("[LNS] Read error (%d)\n", r);
        return;
    }

    printf("[LNS] Read %zu bytes\n", value.length);
    lnsParseLocation(value.pData, value.length);

    uCxEnd(&gUcxHandle);
}

// Subscribe to LNS notifications
static void gattClientSubscribeLns()
{
    if (gLnsClientLocCccdHandle < 0) {
        printf("[LNS] CCCD invalid\n");
        return;
    }

    uint8_t enableNotify[2] = {0x01, 0x00};

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gLnsClientLocCccdHandle,
                                        enableNotify,
                                        sizeof(enableNotify));

    if (r < 0) {
        printf("[LNS] Could not enable notifications (%d)\n", r);
        return;
    }

    printf("[LNS] Notifications ENABLED\n\n");
}

// Complete LNS client example
static void gattClientLnsExample()
{
    printf("\n--- GATT Client: Location and Navigation (LNS) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    // 1) Discover services & characteristics
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // 2) Find handles
    if (!gattClientFindLnsHandles())
        return;

    // 3) Read once
    gattClientReadLns();

    // 4) Subscribe
    gattClientSubscribeLns();

    printf("LNS Client running.\n");
    printf("Latitude / Longitude updates will appear automatically.\n\n");
}

// ============================================================================
// GATT CLIENT - AUTOMATION IO (AIO)
// Service UUID: 0x1815
// Characteristics:
//   - Digital (0x2A56): bitfield (each bit = digital channel)
//   - Analog  (0x2A58): typically SINT16
// ============================================================================

// Parse Digital characteristic data
static void aioParseDigital(const uint8_t *data, size_t len)
{
    if (len == 0) {
        printf("[AIO] Digital: (empty)\n");
        return;
    }

    // Print as hex
    printf("[AIO] Digital raw: 0x");
    for (size_t i = 0; i < len; i++) {
        printf("%02X", data[i]);
    }

    // Decode bits into channels (D0, D1, ...)
    printf("  Bits: ");
    size_t bitCount = len * 8;
    for (size_t bit = 0; bit < bitCount; bit++) {
        size_t byteIdx = bit / 8;
        uint8_t mask = (uint8_t)(1u << (bit % 8));
        bool on = (data[byteIdx] & mask) != 0;
        printf("D%zu=%d ", bit, on ? 1 : 0);
    }
    printf("\n");
}

// Parse Analog characteristic data
static void aioParseAnalog(const uint8_t *data, size_t len)
{
    if (len < 2) {
        printf("[AIO] Analog: invalid length (%zu)\n", len);
        return;
    }

    int16_t raw = (int16_t)(data[0] | (data[1] << 8));
    printf("[AIO] Analog value: %d (0x%04X)\n", raw, (uint16_t)raw);
}

// Find Automation IO service and characteristic handles
static bool gattClientFindAioHandles(void)
{
    // Reset
    gAioClientServiceIndex       = -1;
    gAioClientDigitalCharIndex   = -1;
    gAioClientDigitalValueHandle = -1;
    gAioClientDigitalCccdHandle  = -1;
    gAioClientAnalogCharIndex    = -1;
    gAioClientAnalogValueHandle  = -1;
    gAioClientAnalogCccdHandle   = -1;

    // 1) Service 0x1815
    gAioClientServiceIndex = findServiceByUuid16(0x1815);
    if (gAioClientServiceIndex < 0) {
        printf("[AIO] Service 0x1815 (Automation IO) not found.\n");
        return false;
    }

    // 2) Digital characteristic 0x2A56
    gAioClientDigitalCharIndex = findCharByUuid16InService(gAioClientServiceIndex, 0x2A56);
    if (gAioClientDigitalCharIndex >= 0) {
        gAioClientDigitalValueHandle =
            gGattCharacteristics[gAioClientDigitalCharIndex].valueHandle;
        gAioClientDigitalCccdHandle  = gAioClientDigitalValueHandle + 1;

        printf("[AIO] Digital: handle=0x%04X, CCCD=0x%04X\n",
               gAioClientDigitalValueHandle, gAioClientDigitalCccdHandle);
    }

    // 3) Analog characteristic 0x2A58
    gAioClientAnalogCharIndex = findCharByUuid16InService(gAioClientServiceIndex, 0x2A58);
    if (gAioClientAnalogCharIndex >= 0) {
        gAioClientAnalogValueHandle =
            gGattCharacteristics[gAioClientAnalogCharIndex].valueHandle;
        gAioClientAnalogCccdHandle  = gAioClientAnalogValueHandle + 1;

        printf("[AIO] Analog:  handle=0x%04X, CCCD=0x%04X\n",
               gAioClientAnalogValueHandle, gAioClientAnalogCccdHandle);
    }

    if (gAioClientDigitalValueHandle < 0 && gAioClientAnalogValueHandle < 0) {
        printf("[AIO] No valid characteristics (Digital/Analog) found in service 0x1815.\n");
        return false;
    }

    return true;
}

// Read Digital + Analog once
static void gattClientReadAioValues(void)
{
    uint8_t buf[20];
    uByteArray_t value = { .pData = buf, .length = 0 };

    // Digital
    if (gAioClientDigitalValueHandle > 0) {
        value.length = 0;
        int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                           gCurrentGattConnHandle,
                                           gAioClientDigitalValueHandle,
                                           &value);
        if (r >= 0) {
            aioParseDigital(buf, value.length);
            uCxEnd(&gUcxHandle);
        } else {
            printf("[AIO] Digital read failed (%d)\n", r);
        }
    }

    // Analog
    if (gAioClientAnalogValueHandle > 0) {
        value.length = 0;
        int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                           gCurrentGattConnHandle,
                                           gAioClientAnalogValueHandle,
                                           &value);
        if (r >= 0) {
            aioParseAnalog(buf, value.length);
            uCxEnd(&gUcxHandle);
        } else {
            printf("[AIO] Analog read failed (%d)\n", r);
        }
    }
}

// Subscribe to AIO notifications
static void gattClientSubscribeAio(void)
{
    uint8_t notifyEnable[2] = {0x01, 0x00};

    if (gAioClientDigitalCccdHandle > 0) {
        uCxGattClientWriteNoRsp(&gUcxHandle,
                                gCurrentGattConnHandle,
                                gAioClientDigitalCccdHandle,
                                notifyEnable, sizeof(notifyEnable));
        printf("[AIO] Digital notifications ENABLED\n");
    }

    if (gAioClientAnalogCccdHandle > 0) {
        uCxGattClientWriteNoRsp(&gUcxHandle,
                                gCurrentGattConnHandle,
                                gAioClientAnalogCccdHandle,
                                notifyEnable, sizeof(notifyEnable));
        printf("[AIO] Analog notifications ENABLED\n");
    }
}

// Complete Automation IO client example
static void gattClientAioExample(void)
{
    printf("\n--- GATT Client: Automation IO (AIO) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection.\n");
        return;
    }

    // Step 1: Discover everything for this connection
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // Step 2: Find AIO handles
    if (!gattClientFindAioHandles()) {
        return;
    }

    // Step 3: Read initial state
    gattClientReadAioValues();

    // Step 4: Subscribe to notifications
    gattClientSubscribeAio();

    printf("AIO Client running. Digital/Analog changes will show as notifications.\n\n");
}

// ============================================================================
// GATT CLIENT - UART SERVICE (NUS-STYLE)
// ============================================================================

// Parse incoming UART TX (Notify) data from remote server
static void uartParseRxData(const uint8_t *data, size_t len)
{
    printf("[UART RX] %.*s\n", (int)len, data);
}

// Discover UART service and TX/RX characteristic handles
static bool gattClientFindUartHandles(void)
{
    gUartClientServiceIndex = -1;
    gUartClientTxValueHandle = -1;
    gUartClientRxValueHandle = -1;

    // 1) Find the 128-bit NUS service
    gUartClientServiceIndex = findServiceByUuid128(kUartServiceUuid);
    if (gUartClientServiceIndex < 0) {
        printf("[UART] Service not found (UUID: 6E400001-...)\n");
        return false;
    }

    printf("[UART] Found service at index %d\n", gUartClientServiceIndex);

    // 2) Find TX (Notify) characteristic
    for (int i = 0; i < gGattCharacteristicCount; i++) {
        GattCharacteristic_t *ch = &gGattCharacteristics[i];

        if (ch->connHandle != gGattServices[gUartClientServiceIndex].connHandle)
            continue;

        if (ch->uuidLength == 16 &&
            memcmp(ch->uuid, kUartTxCharUuid, 16) == 0)
        {
            gUartClientTxCharIndex   = i;
            gUartClientTxValueHandle = ch->valueHandle;
            gUartClientTxCccdHandle  = ch->valueHandle + 1;

            printf("[UART] TX Notify handle=0x%04X CCCD=0x%04X\n",
                   gUartClientTxValueHandle, gUartClientTxCccdHandle);
        }
    }

    // 3) Find RX (WriteNoRsp) characteristic
    for (int i = 0; i < gGattCharacteristicCount; i++) {
        GattCharacteristic_t *ch = &gGattCharacteristics[i];

        if (ch->connHandle != gGattServices[gUartClientServiceIndex].connHandle)
            continue;

        if (ch->uuidLength == 16 &&
            memcmp(ch->uuid, kUartRxCharUuid, 16) == 0)
        {
            gUartClientRxCharIndex   = i;
            gUartClientRxValueHandle = ch->valueHandle;

            printf("[UART] RX Write handle=0x%04X\n",
                   gUartClientRxValueHandle);
        }
    }

    if (gUartClientTxValueHandle < 0 || gUartClientRxValueHandle < 0) {
        printf("[UART] Missing TX or RX characteristic\n");
        return false;
    }

    return true;
}

// Subscribe to UART TX notifications (enable CCCD)
static void gattClientSubscribeUart(void)
{
    if (gUartClientTxCccdHandle < 0) {
        printf("[UART] No valid TX CCCD handle\n");
        return;
    }

    uint8_t cccd[2] = {0x01, 0x00};  // Enable notifications

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gUartClientTxCccdHandle,
                                        cccd, 2);

    if (r < 0) {
        printf("[UART] Failed to enable notifications (error %d)\n", r);
        return;
    }

    printf("[UART] Notifications ENABLED\n");
}

// Send data to remote UART server (Write Without Response)
static void gattClientUartSend(const char *msg)
{
    if (gUartClientRxValueHandle < 0) {
        printf("[UART] RX handle invalid\n");
        return;
    }

    size_t len = strlen(msg);

    printf("[UART TX] Sending: %s\n", msg);

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gUartClientRxValueHandle,
                                        (const uint8_t *)msg,
                                        (int32_t)len);
    if (r < 0) {
        printf("[UART] Send failed (error %d)\n", r);
        return;
    }
}

// Complete UART client example with interactive send loop
static void gattClientUartExample(void)
{
    printf("\n--- GATT Client: UART Service (NUS-style) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    // 1. Discover services/characteristics
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // 2. Find UART handles
    if (!gattClientFindUartHandles())
        return;

    // 3. Subscribe to TX notifications
    gattClientSubscribeUart();

    printf("[UART] Client ready\n");
    printf("Incoming data will appear as [UART RX] messages\n\n");

    // 4. Interactive send loop
    while (1) {
        char msg[256];
        printf("Enter text to send (or 'exit'): ");
        fflush(stdout);

        if (!fgets(msg, sizeof(msg), stdin))
            break;

        msg[strcspn(msg, "\r\n")] = 0;  // Strip newline

        if (strcmp(msg, "exit") == 0)
            break;

        if (strlen(msg) > 0) {
            gattClientUartSend(msg);
        }
    }

    printf("[UART] Client finished\n");
}

// ============================================================================
// GATT CLIENT - SERIAL PORT SERVICE (SPS - u-blox)
// ============================================================================

// Parse SPS FIFO data notification (similar to UART RX)
static void spsParseFifoData(const uint8_t *data, size_t len)
{
    if (len == 0) {
        return;
    }

    printf("[SPS FIFO RX] ");
    for (size_t i = 0; i < len; i++) {
        if (data[i] >= 32 && data[i] < 127) {
            putchar(data[i]);
        } else {
            printf("\\x%02X", data[i]);
        }
    }
    printf("\n");
}

// Parse SPS credits notification
static void spsParseCredits(const uint8_t *data, size_t len)
{
    if (len < 1) {
        printf("[SPS] Invalid credits packet\n");
        return;
    }

    int8_t credits = (int8_t)data[0];
    
    if (credits == -1) {
        printf("[SPS] Flow control DISCONNECTED (credits=-1)\n");
        gSpsClientFlowControlEnabled = false;
        gSpsClientRemoteCredits = 0;
        return;
    }

    gSpsClientRemoteCredits += credits;
    printf("[SPS] Credits received: %d (total remote credits: %d)\n", 
           credits, gSpsClientRemoteCredits);
}

// Discover SPS service and FIFO/Credits characteristic handles
static bool gattClientFindSpsHandles(void)
{
    gSpsClientServiceIndex = -1;
    gSpsClientFifoValueHandle = -1;
    gSpsClientCreditsValueHandle = -1;

    // 1) Find the 128-bit SPS service (0x2456e1b9...01e9d701)
    gSpsClientServiceIndex = findServiceByUuid128(kSpsServiceUuid);
    if (gSpsClientServiceIndex < 0) {
        printf("[SPS] Service not found (UUID: 2456e1b9-...)\n");
        return false;
    }

    printf("[SPS] Found service at index %d\n", gSpsClientServiceIndex);

    // 2) Find FIFO characteristic (0x2456e1b9...03e9d701)
    for (int i = 0; i < gGattCharacteristicCount; i++) {
        GattCharacteristic_t *ch = &gGattCharacteristics[i];

        if (ch->connHandle != gGattServices[gSpsClientServiceIndex].connHandle)
            continue;

        if (ch->uuidLength == 16 &&
            memcmp(ch->uuid, kSpsFifoCharUuid, 16) == 0)
        {
            gSpsClientFifoCharIndex   = i;
            gSpsClientFifoValueHandle = ch->valueHandle;
            gSpsClientFifoCccdHandle  = ch->valueHandle + 1;

            printf("[SPS] FIFO handle=0x%04X CCCD=0x%04X\n",
                   gSpsClientFifoValueHandle, gSpsClientFifoCccdHandle);
        }
    }

    // 3) Find Credits characteristic (0x2456e1b9...04e9d701) - MANDATORY per spec
    for (int i = 0; i < gGattCharacteristicCount; i++) {
        GattCharacteristic_t *ch = &gGattCharacteristics[i];

        if (ch->connHandle != gGattServices[gSpsClientServiceIndex].connHandle)
            continue;

        if (ch->uuidLength == 16 &&
            memcmp(ch->uuid, kSpsCreditsCharUuid, 16) == 0)
        {
            gSpsClientCreditsCharIndex   = i;
            gSpsClientCreditsValueHandle = ch->valueHandle;
            gSpsClientCreditsCccdHandle  = ch->valueHandle + 1;

            printf("[SPS] Credits handle=0x%04X CCCD=0x%04X\n",
                   gSpsClientCreditsValueHandle, gSpsClientCreditsCccdHandle);
        }
    }

    if (gSpsClientFifoValueHandle < 0) {
        printf("[SPS] Missing FIFO characteristic\n");
        return false;
    }

    if (gSpsClientCreditsValueHandle < 0) {
        printf("[SPS] Missing Credits characteristic (MANDATORY)\n");
        return false;
    }

    return true;
}

// Subscribe to SPS FIFO and Credits notifications
static void gattClientSubscribeSps(bool enableFlowControl)
{
    gSpsClientFlowControlEnabled = enableFlowControl;

    if (gSpsClientFifoCccdHandle < 0 || gSpsClientCreditsCccdHandle < 0) {
        printf("[SPS] Invalid CCCD handles\n");
        return;
    }

    uint8_t cccd[2] = {0x01, 0x00};  // Enable notifications

    // Enable FIFO notifications
    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gSpsClientFifoCccdHandle,
                                        cccd, 2);
    if (r < 0) {
        printf("[SPS] Failed to enable FIFO notifications (error %d)\n", r);
        return;
    }
    printf("[SPS] FIFO notifications ENABLED\n");

    // Enable Credits notifications
    r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                gCurrentGattConnHandle,
                                gSpsClientCreditsCccdHandle,
                                cccd, 2);
    if (r < 0) {
        printf("[SPS] Failed to enable Credits notifications (error %d)\n", r);
        return;
    }
    printf("[SPS] Credits notifications ENABLED\n");

    // If flow control enabled, send initial credits to remote device
    if (enableFlowControl) {
        gSpsClientLocalCredits = 10;
        gSpsClientRemoteCredits = 0;
        printf("[SPS] Flow control ENABLED - sending initial credits\n");
        gattClientSpsSendCredits((int8_t)gSpsClientLocalCredits);
    } else {
        printf("[SPS] Flow control DISABLED\n");
    }
}

// Send data to remote SPS server (Write Without Response to FIFO)
static void gattClientSpsSend(const char *msg)
{
    if (gSpsClientFifoValueHandle < 0) {
        printf("[SPS] FIFO handle invalid\n");
        return;
    }

    size_t len = strlen(msg);

    // Check flow control credits
    if (gSpsClientFlowControlEnabled) {
        if (gSpsClientRemoteCredits <= 0) {
            printf("[SPS] No credits available! Cannot send (credits=%d)\n", 
                   gSpsClientRemoteCredits);
            return;
        }
        gSpsClientRemoteCredits--;
        printf("[SPS TX] Sending (credits remaining: %d): %s\n", 
               gSpsClientRemoteCredits, msg);
    } else {
        printf("[SPS TX] Sending: %s\n", msg);
    }

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gSpsClientFifoValueHandle,
                                        (const uint8_t *)msg,
                                        (int32_t)len);
    if (r < 0) {
        printf("[SPS] Send failed (error %d)\n", r);
        if (gSpsClientFlowControlEnabled) {
            gSpsClientRemoteCredits++;  // Restore credit on failure
        }
        return;
    }
}

// Send credits to remote SPS server
static void gattClientSpsSendCredits(int8_t credits)
{
    if (gSpsClientCreditsValueHandle < 0) {
        printf("[SPS] Credits handle invalid\n");
        return;
    }

    uint8_t creditByte = (uint8_t)credits;
    
    printf("[SPS] Sending %d credits to remote device\n", credits);

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gSpsClientCreditsValueHandle,
                                        &creditByte, 1);
    if (r < 0) {
        printf("[SPS] Failed to send credits (error %d)\n", r);
        return;
    }
}

// Complete SPS client example with interactive send loop
static void gattClientSpsExample(void)
{
    printf("\n--- GATT Client: u-blox Serial Port Service (SPS) ---\n");
    printf("This demonstrates SPS works similarly to NUS (Nordic UART Service)\n");
    printf("Key differences:\n");
    printf("  - SPS uses u-blox UUIDs (2456e1b9-...)\n");
    printf("  - SPS includes mandatory Credits characteristic for flow control\n");
    printf("  - FIFO characteristic handles bidirectional data\n\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    // 1. Discover services/characteristics
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // 2. Find SPS handles
    if (!gattClientFindSpsHandles())
        return;

    // 3. Ask user about flow control
    char flowChoice[16];
    printf("\nEnable credit-based flow control? (y/n): ");
    fflush(stdout);
    if (fgets(flowChoice, sizeof(flowChoice), stdin)) {
        flowChoice[strcspn(flowChoice, "\r\n")] = 0;
    }
    bool enableFlow = (flowChoice[0] == 'y' || flowChoice[0] == 'Y');

    // 4. Subscribe to FIFO and Credits notifications
    gattClientSubscribeSps(enableFlow);

    printf("\n[SPS] Client ready\n");
    printf("Incoming data will appear as [SPS FIFO RX] messages\n");
    if (gSpsClientFlowControlEnabled) {
        printf("Credits will be displayed as [SPS] Credits messages\n");
    }
    printf("\n");

    // 5. Interactive send loop
    while (1) {
        char msg[256];
        
        if (gSpsClientFlowControlEnabled) {
            printf("Enter text to send (or 'exit', 'credits <n>'): ");
        } else {
            printf("Enter text to send (or 'exit'): ");
        }
        fflush(stdout);

        if (!fgets(msg, sizeof(msg), stdin))
            break;

        msg[strcspn(msg, "\r\n")] = 0;  // Strip newline

        if (strcmp(msg, "exit") == 0)
            break;

        // Handle credits command
        if (strncmp(msg, "credits ", 8) == 0) {
            if (gSpsClientFlowControlEnabled) {
                int credits = atoi(msg + 8);
                if (credits > 0 && credits <= 127) {
                    gattClientSpsSendCredits((int8_t)credits);
                } else {
                    printf("[SPS] Invalid credits value (use 1-127)\n");
                }
            } else {
                printf("[SPS] Flow control not enabled\n");
            }
            continue;
        }

        if (strlen(msg) > 0) {
            gattClientSpsSend(msg);
        }
    }

    printf("[SPS] Client finished\n");
}

// ============================================================================
// GATT CLIENT - BATTERY SERVICE (BAS)
// ============================================================================

// Parse battery level notification (1 byte: 0-100%)
static void basParseBatteryLevel(const uint8_t *data, size_t len)
{
    if (len < 1) {
        printf("[BAS] Invalid battery packet\n");
        return;
    }

    int level = data[0];
    if (level > 100) level = 100;

    printf("[BAS] Battery Level: %d %%\n", level);
}

// Discover Battery Service and Battery Level characteristic
static bool gattClientFindBasHandles(void)
{
    gBasClientServiceIndex = -1;
    gBasClientValueHandle  = -1;

    // 1) Battery Service 0x180F
    gBasClientServiceIndex = findServiceByUuid16(0x180F);
    if (gBasClientServiceIndex < 0) {
        printf("[BAS] Service 0x180F not found\n");
        return false;
    }

    printf("[BAS] Found service at index %d\n", gBasClientServiceIndex);

    // 2) Battery Level 0x2A19
    gBasClientCharIndex = findCharByUuid16InService(gBasClientServiceIndex, 0x2A19);
    if (gBasClientCharIndex < 0) {
        printf("[BAS] Characteristic 0x2A19 not found\n");
        return false;
    }

    gBasClientValueHandle = gGattCharacteristics[gBasClientCharIndex].valueHandle;
    gBasClientCccdHandle  = gBasClientValueHandle + 1; // Standard CCCD location

    printf("[BAS] Battery Level handle=0x%04X  CCCD=0x%04X\n",
           gBasClientValueHandle, gBasClientCccdHandle);

    return true;
}

// Read battery level once
static void gattClientReadBattery(void)
{
    if (gBasClientValueHandle < 0) {
        printf("[BAS] Value handle invalid\n");
        return;
    }

    uint8_t buf[8];
    uByteArray_t value = { .pData = buf, .length = 0 };

    int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                       gCurrentGattConnHandle,
                                       gBasClientValueHandle,
                                       &value);
    if (r < 0) {
        printf("[BAS] Read failed (error %d)\n", r);
        return;
    }

    printf("[BAS] Read %zu bytes\n", value.length);

    basParseBatteryLevel(value.pData, value.length);

    uCxEnd(&gUcxHandle);
}

// Subscribe to battery level notifications
static void gattClientSubscribeBattery(void)
{
    if (gBasClientCccdHandle < 0) {
        printf("[BAS] CCCD handle invalid\n");
        return;
    }

    uint8_t cccd[2] = {0x01, 0x00}; // Enable notifications

    int32_t r = uCxGattClientWriteNoRsp(&gUcxHandle,
                                        gCurrentGattConnHandle,
                                        gBasClientCccdHandle,
                                        cccd, 2);

    if (r < 0) {
        printf("[BAS] Failed to enable notifications (error %d)\n", r);
        return;
    }

    printf("[BAS] Notifications ENABLED\n");
}

// Complete Battery Service client example
static void gattClientBasExample(void)
{
    printf("\n--- GATT Client: Battery Service (BAS) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    // 1) Discover services and characteristics
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // 2) Locate Battery Service + Battery Level
    if (!gattClientFindBasHandles())
        return;

    // 3) Read battery once
    gattClientReadBattery();

    // 4) Subscribe to continuous notifications
    gattClientSubscribeBattery();

    printf("BAS Client running.\n");
    printf("Battery %% updates will appear automatically.\n\n");
}

// ============================================================================
// GATT CLIENT - DEVICE INFORMATION SERVICE (DIS)
// ============================================================================

// Discover DIS service and all available characteristics
static bool gattClientFindDisHandles(void)
{
    gDisClientServiceIndex = -1;
    gDisClientCharCount = 0;

    // 1) Find service 0x180A
    gDisClientServiceIndex = findServiceByUuid16(0x180A);
    if (gDisClientServiceIndex < 0) {
        printf("[DIS] Service 0x180A not found\n");
        return false;
    }

    printf("[DIS] Found Device Information Service\n");

    // 2) Look for each characteristic
    for (int i = 0; i < kDisCharUuidCount; i++) {
        uint16_t uuid = kDisCharUuids[i];
        int chIndex = findCharByUuid16InService(gDisClientServiceIndex, uuid);

        if (chIndex >= 0) {
            gDisClientChars[gDisClientCharCount].uuid   = uuid;
            gDisClientChars[gDisClientCharCount].handle =
                gGattCharacteristics[chIndex].valueHandle;

            const char *charName = "Unknown";
            switch (uuid) {
                case 0x2A29: charName = "Manufacturer Name"; break;
                case 0x2A24: charName = "Model Number"; break;
                case 0x2A25: charName = "Serial Number"; break;
                case 0x2A26: charName = "Firmware Revision"; break;
                case 0x2A27: charName = "Hardware Revision"; break;
                case 0x2A28: charName = "Software Revision"; break;
                case 0x2A23: charName = "System ID"; break;
            }

            printf("[DIS]   Found %s (0x%04X) at handle 0x%04X\n",
                   charName, uuid, gDisClientChars[gDisClientCharCount].handle);

            gDisClientCharCount++;
        }
    }

    if (gDisClientCharCount == 0) {
        printf("[DIS] No valid DIS characteristics found\n");
        return false;
    }

    return true;
}

// Read and print a single DIS characteristic
static void disReadAndPrint(uint16_t uuid, int32_t handle)
{
    uint8_t buf[64];
    uByteArray_t value = { .pData = buf, .length = 0 };

    int32_t r = uCxGattClientReadBegin(&gUcxHandle,
                                       gCurrentGattConnHandle,
                                       handle,
                                       &value);

    if (r < 0) {
        printf("[DIS] Read 0x%04X failed (error %d)\n", uuid, r);
        return;
    }

    // Get friendly name
    const char *charName = "Unknown";
    switch (uuid) {
        case 0x2A29: charName = "Manufacturer"; break;
        case 0x2A24: charName = "Model Number"; break;
        case 0x2A25: charName = "Serial Number"; break;
        case 0x2A26: charName = "Firmware Rev"; break;
        case 0x2A27: charName = "Hardware Rev"; break;
        case 0x2A28: charName = "Software Rev"; break;
        case 0x2A23: charName = "System ID"; break;
    }

    printf("[DIS] %-15s: ", charName);

    // Print ASCII for string characteristics
    if (uuid != 0x2A23) {  // System ID is binary
        for (size_t i = 0; i < value.length; i++) {
            uint8_t c = value.pData[i];
            if (c >= 32 && c <= 126)
                putchar(c);
            else
                printf("\\x%02X", c);
        }
    } else {
        // System ID is 8 bytes: [0-4]=Manufacturer, [5-7]=Organizationally Unique
        for (size_t i = 0; i < value.length; i++) {
            printf("%02X", value.pData[i]);
            if (i < value.length - 1) printf(":");
        }
    }

    printf("\n");

    uCxEnd(&gUcxHandle);
}

// Complete DIS client example
static void gattClientDisExample(void)
{
    printf("\n--- GATT Client: Device Information Service (DIS) ---\n");

    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    // Step 1: Discover everything
    gattClientDiscoverServices();
    gattClientDiscoverCharacteristics();

    // Step 2: Find DIS service + chars
    if (!gattClientFindDisHandles())
        return;

    printf("\nReading device information:\n");

    // Step 3: Read each characteristic
    for (int i = 0; i < gDisClientCharCount; i++) {
        disReadAndPrint(gDisClientChars[i].uuid, gDisClientChars[i].handle);
    }

    printf("\nDIS Client finished. All available device info retrieved.\n\n");
}

// ----------------------------------------------------------------
// URC Management Functions
// ----------------------------------------------------------------

static void enableAllUrcs(void)
{
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "*** enableAllUrcs() called ***");
    
    // Wi-Fi link and network events
    uCxWifiRegisterLinkUp(&gUcxHandle, linkUpUrc);
    uCxWifiRegisterLinkDown(&gUcxHandle, linkDownUrc);
    uCxWifiRegisterStationNetworkUp(&gUcxHandle, networkUpUrc);
    uCxWifiRegisterStationNetworkDown(&gUcxHandle, networkDownUrc);
    
    // Wi-Fi Access Point events
    uCxWifiRegisterApNetworkUp(&gUcxHandle, apNetworkUpUrc);
    uCxWifiRegisterApNetworkDown(&gUcxHandle, apNetworkDownUrc);
    uCxWifiRegisterApUp(&gUcxHandle, apUpUrc);
    uCxWifiRegisterApDown(&gUcxHandle, apDownUrc);
    uCxWifiRegisterApStationAssociated(&gUcxHandle, apStationAssociatedUrc);
    uCxWifiRegisterApStationDisassociated(&gUcxHandle, apStationDisassociatedUrc);
    
    // Socket events
    uCxSocketRegisterConnect(&gUcxHandle, sockConnected);
    uCxSocketRegisterDataAvailable(&gUcxHandle, socketDataAvailable);
    
    // SPS events
    uCxSpsRegisterConnect(&gUcxHandle, spsConnected);
    uCxSpsRegisterDisconnect(&gUcxHandle, spsDisconnected);
    uCxSpsRegisterDataAvailable(&gUcxHandle, spsDataAvailable);
    
    // System events (STARTUP now uses callback structure, set during uCxInit)
    
    // Ping/diagnostics events
    uCxDiagnosticsRegisterPingResponse(&gUcxHandle, pingResponseUrc);
    uCxDiagnosticsRegisterPingComplete(&gUcxHandle, pingCompleteUrc);
    uCxDiagnosticsRegisterIperfOutput(&gUcxHandle, iperfOutputUrc);
    
    // MQTT events
    uCxMqttRegisterConnect(&gUcxHandle, mqttConnectedUrc);
    uCxMqttRegisterDataAvailable(&gUcxHandle, mqttDataAvailableUrc);
    
    // Bluetooth events
    uCxBluetoothRegisterConnect(&gUcxHandle, btConnected);
    uCxBluetoothRegisterDisconnect(&gUcxHandle, btDisconnected);
    uCxBluetoothRegisterBondStatus(&gUcxHandle, bluetoothPairUrc);
    uCxBluetoothRegisterUserConfirmation(&gUcxHandle, bluetoothUserConfirmationUrc);
    uCxBluetoothRegisterPasskeyEntry(&gUcxHandle, bluetoothPasskeyDisplayUrc);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Registering PasskeyRequest URC handler at %p", (void*)bluetoothPasskeyRequestUrc);
    uCxBluetoothRegisterPasskeyRequest(&gUcxHandle, bluetoothPasskeyRequestUrc);
    uCxBluetoothRegisterPhyUpdate(&gUcxHandle, bluetoothPhyUpdateUrc);
    
    // GATT Server events
    uCxGattServerRegisterNotification(&gUcxHandle, gattServerCharWriteUrc);
    
    // GATT Client events (notifications and indications)
    uCxGattClientRegisterNotification(&gUcxHandle, gattClientNotificationUrc);
    uCxGattClientRegisterIndication(&gUcxHandle, gattClientNotificationUrc);  // Use same handler
}

static void disableAllUrcs(void)
{
    if (!gUcxConnected) {
        return;
    }
    
    // Deregister all URCs by passing NULL callbacks
    uCxWifiRegisterLinkUp(&gUcxHandle, NULL);
    uCxWifiRegisterLinkDown(&gUcxHandle, NULL);
    uCxWifiRegisterStationNetworkUp(&gUcxHandle, NULL);
    uCxWifiRegisterStationNetworkDown(&gUcxHandle, NULL);
    uCxWifiRegisterApNetworkUp(&gUcxHandle, NULL);
    uCxWifiRegisterApNetworkDown(&gUcxHandle, NULL);
    uCxWifiRegisterApUp(&gUcxHandle, NULL);
    uCxWifiRegisterApDown(&gUcxHandle, NULL);
    uCxWifiRegisterApStationAssociated(&gUcxHandle, NULL);
    uCxWifiRegisterApStationDisassociated(&gUcxHandle, NULL);
    uCxSocketRegisterConnect(&gUcxHandle, NULL);
    uCxSocketRegisterDataAvailable(&gUcxHandle, NULL);
    uCxSpsRegisterConnect(&gUcxHandle, NULL);
    uCxSpsRegisterDisconnect(&gUcxHandle, NULL);
    uCxSpsRegisterDataAvailable(&gUcxHandle, NULL);
    // uCxSystemRegisterStartup(&gUcxHandle, NULL);  // Removed from API
    uCxDiagnosticsRegisterPingResponse(&gUcxHandle, NULL);
    uCxDiagnosticsRegisterPingComplete(&gUcxHandle, NULL);
    uCxMqttRegisterConnect(&gUcxHandle, NULL);
    uCxMqttRegisterDataAvailable(&gUcxHandle, NULL);
    uCxBluetoothRegisterConnect(&gUcxHandle, NULL);
    uCxBluetoothRegisterDisconnect(&gUcxHandle, NULL);
    uCxBluetoothRegisterBondStatus(&gUcxHandle, NULL);
    uCxBluetoothRegisterUserConfirmation(&gUcxHandle, NULL);
    uCxBluetoothRegisterPasskeyEntry(&gUcxHandle, NULL);
    uCxBluetoothRegisterPasskeyRequest(&gUcxHandle, NULL);
    uCxBluetoothRegisterPhyUpdate(&gUcxHandle, NULL);
    uCxGattServerRegisterNotification(&gUcxHandle, NULL);
    uCxGattClientRegisterNotification(&gUcxHandle, NULL);
    uCxGattClientRegisterIndication(&gUcxHandle, NULL);
}

// ============================================================================
// HID OVER GATT (HoG) - Keyboard + Media Control
// ============================================================================

// HID Report Descriptor - Bluetooth SIG Standard Keyboard with LEDs (Report ID 1 only)
// 63 bytes total - Exact reference implementation for maximum compatibility
// Hex: 05010906A101850105071 9E029E715002501750195088102950175088101950675081500256505071900296581000508950575011901290591029501750391 03C0
static const uint8_t gHidReportMapKeyboard[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0xE0,        //   Usage Minimum (Left Control)
    0x29, 0xE7,        //   Usage Maximum (Right GUI)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs) - Modifiers
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs) - Reserved
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Keyboard/Keypad)
    0x19, 0x00,        //   Usage Minimum (0)
    0x29, 0x65,        //   Usage Maximum (101)
    0x81, 0x00,        //   Input (Data,Array,Abs) - Keys
    0x05, 0x08,        //   Usage Page (LEDs)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs) - LEDs
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x03,        //   Output (Const,Var,Abs) - Padding
    0xC0               // End Collection
};


// HID Information value (version 1.11, country code 0, flags 0x03 = normally connectable + remote wake)
static const uint8_t gHidInfo[] = {0x11, 0x01, 0x00, 0x03};

// Setup complete HID over GATT service structure
static void gattServerSetupHidKeyboard(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("HID over GATT (HoG) Setup\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Profile: Keyboard + Media Control\n\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");
    
    // STEP 0: Configure Bluetooth settings BEFORE creating GATT services
    printf("Step 0: Configuring Bluetooth settings...\n");
    
    // Set Bluetooth device name (add timestamp suffix to force cache refresh on phones)
    char deviceName[64];
    time_t now = time(NULL);
    snprintf(deviceName, sizeof(deviceName), "u-blox HID Keyboard %02d", (int)(now % 100));
    printf("  Setting device name to '%s'...\n", deviceName);
    int result = uCxBluetoothSetLocalName(&gUcxHandle, deviceName);
    if (result == 0) {
        printf("  ✓ Device name set\n");
    }
    
    // Set security mode to unauthenticated for "Just Works" pairing
    printf("  Setting security mode to unauthenticated...\n");
    result = uCxBluetoothSetSecurityMode(&gUcxHandle, U_BT_SECURITY_MODE_UNAUTHENTICATED);
    if (result == 0) {
        printf("  ✓ Security mode set\n");
    }
    
    // Set IO capabilities - use KeyboardOnly for HID keyboards to enable passkey entry (MITM protection)
    printf("  Setting IO capabilities to KeyboardOnly...\n");
    result = uCxBluetoothSetIoCapabilities(&gUcxHandle, U_BT_IO_CAP_KEYBOARD_ONLY);
    if (result == 0) {
        printf("  ✓ IO capabilities set\n");
    }
    
    // Set advertising interval to 100-150ms using new API
    // Interval units are in 0.625ms steps: 160 * 0.625 = 100ms, 240 * 0.625 = 150ms
    printf("  Setting advertising interval to 100-150ms...\n");
    result = uCxBluetoothSetLegacyAdvertisementConfig(&gUcxHandle, 160, 240);
    if (result == 0) {
        printf("  ✓ Advertising interval set\n");
    } else {
        printf("  WARNING: Failed to set advertising interval (code %d)\n", result);
    }
    
    // Display Device Information Service (built-in, pre-configured in module)
    printf("  Reading Device Information Service...\n");
    uCxBluetoothListDeviceInfoServiceCharsBegin(&gUcxHandle);
    uCxBtListDeviceInfoServiceChars_t disChar;
    while (uCxBluetoothListDeviceInfoServiceCharsGetNext(&gUcxHandle, &disChar)) {
        const char *charName = "";
        switch (disChar.char_id) {
            case U_BT_CHAR_ID_MANUFACTURER_NAME: charName = "Manufacturer"; break;
            case U_BT_CHAR_ID_MODEL_NAME: charName = "Model"; break;
            case U_BT_CHAR_ID_FIRMWARE_REVISION: charName = "Firmware"; break;
            case U_BT_CHAR_ID_SOFTWARE_REVISION: charName = "Software"; break;
        }
        printf("    %s: %s\n", charName, disChar.char_value);
    }
    uCxEnd(&gUcxHandle);
    printf("  ✓ Device Information Service ready\n\n");
    
    // Step 1: Define HID Service (0x1812)
    printf("Step 1: Creating HID Service (UUID: 0x1812)...\n");
    uint8_t hidServiceUuid[] = {0x18, 0x12};  // 16-bit UUID for HID Service (little-endian: LSB first)
    
    result = uCxGattServerServiceDefine(&gUcxHandle, hidServiceUuid, 2, &gHidServiceHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to create HID Service (code %d)\n", result);
        return;
    }
    
    printf("✓ HID Service defined with handle: %d\n\n", gHidServiceHandle);
    
    // Step 2: Add HID Information characteristic (0x2A4A) - Read (0x02)
    printf("Step 2: Adding HID Information characteristic...\n");
    uint8_t hidInfoUuid[] = {0x2A, 0x4A};  // Little-endian
    uint8_t propRead[] = {0x02};  // Read property
    
    uCxGattServerCharDefine_t infoResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, hidInfoUuid, 2,
                                      propRead, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      (uint8_t*)gHidInfo, sizeof(gHidInfo),
                                      sizeof(gHidInfo),
                                      &infoResponse);
    
    if (result == 0) {
        gHidInfoHandle = infoResponse.value_handle;
        printf("✓ HID Info characteristic created (handle: %d)\n", gHidInfoHandle);
    }
    
    // Step 2a: Add Protocol Mode characteristic (0x2A4E) - Read + Write Without Response
    // CRITICAL: Read = NONE (no encryption) so Windows can read early before bonding
    // Write = UNAUTHENTICATED (encrypted) forces Windows/iOS to pair on first write
    // AT equivalent: AT+UBTGC=2A4E,06,1,2,01,1
    printf("Step 2a: Adding Protocol Mode characteristic...\n");
    uint8_t protoModeUuid[] = {0x2A, 0x4E};  // Little-endian
    uint8_t propReadWriteNoResp[] = {0x06};  // Read (0x02) + Write Without Response (0x04) = 0x06
    uint8_t protoModeValue[] = {0x01};  // 0x01 = Report Protocol (default)
    
    uCxGattServerCharDefine_t protoResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, protoModeUuid, 2,
                                      propReadWriteNoResp, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      protoModeValue, 1,
                                      1,
                                      &protoResponse);
    
    if (result == 0) {
        gHidProtocolModeHandle = protoResponse.value_handle;
        printf("✓ Protocol Mode characteristic created (handle: %d, read=open, write=encrypted)\n", protoResponse.value_handle);
    } else {
        printf("WARNING: Failed to create Protocol Mode (code %d)\n", result);
    }
    
    // Step 3: Add Report Map characteristic (0x2A4B) - Read (0x02)
    printf("Step 3: Adding Report Map characteristic...\n");
    uint8_t reportMapUuid[] = {0x2A, 0x4B};  // Little-endian
    
    uCxGattServerCharDefine_t mapResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, reportMapUuid, 2,
                                      propRead, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      (uint8_t*)gHidReportMapKeyboard, sizeof(gHidReportMapKeyboard),
                                      sizeof(gHidReportMapKeyboard),
                                      &mapResponse);
    
    if (result == 0) {
        gHidReportMapHandle = mapResponse.value_handle;
        printf("✓ Report Map characteristic created (handle: %d)\n", gHidReportMapHandle);
        
        // Add External Report Reference descriptor (0x2907)
        // This links the Report Map to the Battery Level characteristic (UUID 0x2A19)
        // CRITICAL: Must reference the characteristic (0x2A19), NOT the service (0x180F)
        uint8_t extReportRefUuid[] = {0x29, 0x07};  // External Report Reference UUID (little-endian)
        uint8_t batteryLevelCharUuid[] = {0x2A, 0x19};  // Battery Level Characteristic UUID (little-endian)
        
        int32_t extReportRefHandle;
        result = uCxGattServerDescriptorDefine4(&gUcxHandle, extReportRefUuid, 2,
                                                U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                                batteryLevelCharUuid, 2,
                                                &extReportRefHandle);
        
        if (result == 0) {
            printf("  ✓ External Report Reference descriptor added (handle: %d, links to Battery Level char)\n", extReportRefHandle);
        } else {
            printf("  WARNING: Failed to add External Report Reference (code %d)\n", result);
        }
    }
    
    // Step 3a: Add Boot Keyboard Input Report (0x2A22) - Notify (0x10)
    // CRITICAL: Required for Windows/iOS to recognize device as HID Keyboard
    // Security set to UNAUTHENTICATED (Encrypted) as per HID spec
    // Boot reports MUST be exactly 8 bytes (per HID spec)
    printf("Step 3a: Adding Boot Keyboard Input Report (required for OS recognition)...\n");
    uint8_t bootKbdInputUuid[] = {0x2A, 0x22};  // Little-endian
    uint8_t propNotify[] = {0x10};  // Notify only
    uint8_t emptyBootKbdReport[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Standard 8-byte boot report
    
    uCxGattServerCharDefine_t bootKbdResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, bootKbdInputUuid, 2,
                                      propNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      emptyBootKbdReport, 8,
                                      8,
                                      &bootKbdResponse);
    
    if (result == 0) {
        gHidBootKbdInputHandle = bootKbdResponse.value_handle;
        gHidBootKbdCccdHandle = bootKbdResponse.cccd_handle;
        printf("✓ Boot Keyboard Input Report created (handle: %d, CCCD: %d)\n", 
               bootKbdResponse.value_handle, bootKbdResponse.cccd_handle);
        
        // CRITICAL: Add Report Reference descriptor (0x2908) for Boot Keyboard Input
        // This tells Windows: Report ID=1, Report Type=1 (Input)
        // Without this, Windows won't recognize the device as HoG!
        uint8_t reportRefUuid[] = {0x29, 0x08};  // Report Reference UUID (little-endian)
        uint8_t reportRefValue[] = {0x01, 0x01}; // Report ID=1, Report Type=1 (Input)
        
        int32_t reportRefHandle;
        result = uCxGattServerDescriptorDefine4(&gUcxHandle, reportRefUuid, 2,
                                                U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                                reportRefValue, 2,
                                                &reportRefHandle);
        
        if (result == 0) {
            printf("  ✓ Boot Keyboard Report Reference descriptor added (handle: %d)\n", reportRefHandle);
        } else {
            printf("  WARNING: Failed to add Boot Keyboard Report Reference (code %d)\n", result);
        }
    } else {
        printf("WARNING: Failed to create Boot Keyboard Input Report (code %d)\n", result);
    }
    
    // Step 3b: Add Boot Keyboard Output Report (0x2A32) - Write Without Response (0x04)
    // Recommended for complete HID keyboard compliance
    // Security set to UNAUTHENTICATED (Encrypted) as per HID spec
    printf("Step 3b: Adding Boot Keyboard Output Report...\n");
    uint8_t bootKbdOutputUuid[] = {0x2A, 0x32};  // Little-endian
    uint8_t propWriteWoResp[] = {0x04};  // Write Without Response
    uint8_t emptyBootKbdOutput[1] = {0x00};  // LED state (Num/Caps/Scroll Lock)
    
    uCxGattServerCharDefine_t bootKbdOutResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, bootKbdOutputUuid, 2,
                                      propWriteWoResp, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      emptyBootKbdOutput, 1,
                                      1,
                                      &bootKbdOutResponse);
    
    if (result == 0) {
        gHidBootKbdOutputHandle = bootKbdOutResponse.value_handle;
        printf("✓ Boot Keyboard Output Report created (handle: %d)\n", bootKbdOutResponse.value_handle);
        
        // Add Report Reference descriptor (0x2908) for Boot Keyboard Output
        // Value: [Report ID, Report Type] = [0x01, 0x02] (ID=1, Type=Output)
        uint8_t reportRefUuid[] = {0x29, 0x08};  // Little-endian
        uint8_t bootOutReportRefValue[] = {0x01, 0x02};  // Report ID 1, Output Report
        
        int32_t bootOutDescHandle;
        result = uCxGattServerDescriptorDefine4(&gUcxHandle,
                                                reportRefUuid, 2,
                                                U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                                bootOutReportRefValue, 2,
                                                &bootOutDescHandle);
        
        if (result == 0) {
            printf("  ✓ Boot Output Report Reference descriptor added (handle: %d)\n", bootOutDescHandle);
        } else {
            printf("  WARNING: Failed to add Boot Output Report Reference (code %d)\n", result);
        }
    } else {
        printf("WARNING: Failed to create Boot Keyboard Output Report (code %d)\n", result);
    }
    
    // Step 4: Add Keyboard Input Report characteristic (0x2A4D) - Read (0x02) + Notify (0x10) = 0x12
    // Security set to UNAUTHENTICATED (Encrypted) as per HID spec
    printf("Step 4: Adding Keyboard Input Report characteristic...\n");
    uint8_t reportUuid[] = {0x2A, 0x4D};  // Little-endian
    uint8_t propReadNotify[] = {0x12};  // Read + Notify
    uint8_t emptyKbdReport[9] = {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Report ID + 8 bytes
    
    uCxGattServerCharDefine_t kbdResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, reportUuid, 2,
                                      propReadNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      emptyKbdReport, 9,
                                      9,
                                      &kbdResponse);
    
    if (result == 0) {
        gHidKeyboardInputHandle = kbdResponse.value_handle;
        gHidKeyboardCccdHandle = kbdResponse.cccd_handle;
        printf("✓ Keyboard Input Report created (handle: %d)\n", gHidKeyboardInputHandle);
        if (kbdResponse.cccd_handle > 0) {
            printf("  CCCD handle: %d\n", kbdResponse.cccd_handle);
        }
        
        // Add Report Reference descriptor (0x2908) for Keyboard Input Report
        // Value: [Report ID, Report Type] = [0x01, 0x01] (ID=1, Type=Input)
        uint8_t reportRefUuid[] = {0x29, 0x08};  // Little-endian
        uint8_t kbdReportRefValue[] = {0x01, 0x01};  // Report ID 1, Input Report
        
        int32_t kbdDescHandle;
        result = uCxGattServerDescriptorDefine4(&gUcxHandle,
                                                reportRefUuid, 2,
                                                U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                                kbdReportRefValue, 2,
                                                &kbdDescHandle);
        
        if (result == 0) {
            printf("  Report Reference descriptor added (handle: %d)\n", kbdDescHandle);
        } else {
            printf("  WARNING: Failed to add Report Reference descriptor (code %d)\n", result);
        }
    }
    
    // Step 4b: Add Keyboard Output Report characteristic (0x2A4D) - LED indicators
    // CRITICAL: Windows expects an LED Output report to match the Report Map
    // This is for Report mode (complementary to Boot Keyboard Output)
    printf("Step 4b: Adding Keyboard Output Report (LED indicators)...\n");
    uint8_t kbdOutputUuid[] = {0x2A, 0x4D};  // Report characteristic (little-endian)
    uint8_t propReadWriteWWR[] = {0x0E};  // Read (0x02) + Write (0x08) + Write Without Response (0x04) = 0x0E
    uint8_t ledOutputValue[] = {0x00};  // 1 byte: LED bitmap (Num/Caps/Scroll/etc)
    
    uCxGattServerCharDefine_t kbdOutResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, kbdOutputUuid, 2,
                                      propReadWriteWWR, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      ledOutputValue, 1,
                                      1,
                                      &kbdOutResponse);
    
    if (result == 0) {
        gHidKeyboardOutputHandle = kbdOutResponse.value_handle;
        printf("✓ Keyboard Output Report created (handle: %d)\n", gHidKeyboardOutputHandle);
        
        // Add Report Reference descriptor (0x2908) for Keyboard Output Report
        // Value: [Report ID, Report Type] = [0x01, 0x02] (ID=1, Type=Output)
        uint8_t reportRefUuid[] = {0x29, 0x08};  // Little-endian
        uint8_t kbdOutReportRefValue[] = {0x01, 0x02};  // Report ID 1, Output Report
        
        int32_t kbdOutDescHandle;
        result = uCxGattServerDescriptorDefine4(&gUcxHandle,
                                                reportRefUuid, 2,
                                                U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                                kbdOutReportRefValue, 2,
                                                &kbdOutDescHandle);
        
        if (result == 0) {
            printf("  ✓ LED Output Report Reference descriptor added (handle: %d)\n", kbdOutDescHandle);
        } else {
            printf("  WARNING: Failed to add LED Output Report Reference (code %d)\n", result);
        }
    } else {
        printf("WARNING: Failed to create Keyboard Output Report (code %d)\n", result);
    }
    
    // Step 5: Add HID Control Point characteristic (0x2A4C) - Write without response (0x04)
    // AT equivalent: AT+UBTGC=4C2A,04,2,2,00,1
    printf("Step 5: Adding HID Control Point characteristic...\n");
    uint8_t controlPointUuid[] = {0x2A, 0x4C};  // Little-endian
    uint8_t propWriteNoResp[] = {0x04};  // Write without response
    uint8_t cpValue[] = {0x00};
    
    uCxGattServerCharDefine_t cpResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, controlPointUuid, 2,
                                      propWriteNoResp, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      cpValue, 1,
                                      1,
                                      &cpResponse);
    
    if (result == 0) {
        gHidControlPointHandle = cpResponse.value_handle;
        printf("✓ HID Control Point created (handle: %d)\n", gHidControlPointHandle);
    }
    
    // Step 6: Activating HID Service
    printf("\nStep 6: Activating HID Service...\n");
    result = uCxGattServerServiceActivate(&gUcxHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to activate HID service (code %d)\n", result);
        return;
    }
    
    printf("✓ HID Service activated!\n");
    
    // Step 7: Create Battery Service (0x180F)
    printf("Step 7: Creating Battery Service (UUID: 0x180F)...\n");
    uint8_t batteryServiceUuid[] = {0x18, 0x0F};  // Little-endian
    
    result = uCxGattServerServiceDefine(&gUcxHandle, batteryServiceUuid, 2, &gBatteryServiceHandle);
    
    if (result == 0) {
        printf("✓ Battery Service defined (handle: %d)\n", gBatteryServiceHandle);
        
        // Add Battery Level characteristic (0x2A19) - Read + Notify
        uint8_t batteryLevelUuid[] = {0x2A, 0x19};  // Little-endian
        uint8_t batValue[] = {100};  // 100%
        
        uCxGattServerCharDefine_t batResponse;
        result = uCxGattServerCharDefine6(&gUcxHandle, batteryLevelUuid, 2,
                                          propReadNotify, 1,
                                          U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                          batValue, 1,
                                          1,
                                          &batResponse);
        
        if (result == 0) {
            gBatteryLevelHandle = batResponse.value_handle;
            gBatteryCccdHandle = batResponse.cccd_handle;
            printf("✓ Battery Level characteristic created (handle: %d)\n", gBatteryLevelHandle);
            if (batResponse.cccd_handle > 0) {
                printf("  CCCD handle: %d\n", batResponse.cccd_handle);
            }
        }
        
        // Step 8: Activate Battery Service
        printf("\nStep 8: Activating Battery Service...\n");
        result = uCxGattServerServiceActivate(&gUcxHandle);
        
        if (result != 0) {
            printf("ERROR: Failed to activate Battery service (code %d)\n", result);
            return;
        }
        
        printf("✓ Battery Service activated!\n");
    }
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("HID Keyboard + Battery Services Ready!\n");
    printf("─────────────────────────────────────────────────\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    // Bond management - Check existing bonds
    printf("Checking existing bonds...\n");
    uCxBluetoothListBondedDevicesBegin(&gUcxHandle);
    uBtLeAddress_t bondedAddr;
    int bondCount = 0;
    while (uCxBluetoothListBondedDevicesGetNext(&gUcxHandle, &bondedAddr)) {
        bondCount++;
    }
    result = uCxEnd(&gUcxHandle);
    if (result == 0) {
        printf("✓ Found %d bonded device(s)\n", bondCount);
    } else {
        printf("✓ Bond list checked\n");
    }
    
    // Only ask to clear bonds if there are any
    bool clearBonds = false;
    if (bondCount > 0) {
        printf("\nDo you want to clear existing bonds? (y/n): ");
        int clearInput = getchar();
        getchar(); // consume newline
        printf("\n");
        clearBonds = (clearInput == 'y' || clearInput == 'Y');
    } else {
        printf("No existing bonds to clear.\n\n");
    }
    
    if (clearBonds) {
        printf("Clearing all Bluetooth bonds...\n");
        result = uCxBluetoothUnbondAll(&gUcxHandle);
        if (result == 0) {
            printf("✓ All bonds cleared - ready for fresh pairing\n\n");
            
            printf("\n─────────────────────────────────────────────────\n");
            printf("CRITICAL STEP REQUIRED\n");
            printf("─────────────────────────────────────────────────\n\n");
            printf("You MUST also remove this device from your\n");
            printf("iPhone/Windows Bluetooth settings:\n\n");
            printf("iPhone:\n");
            printf("  1. Settings → Bluetooth\n");
            printf("  2. Find 'u-blox ucxclient HID device'\n");
            printf("  3. Tap (i) icon\n");
            printf("  4. Tap 'Forget This Device'\n\n");
            printf("Windows 11:\n");
            printf("  1. Settings → Bluetooth & devices\n");
            printf("  2. Find 'u-blox ucxclient HID device'\n");
            printf("  3. Click ⋮ (three dots)\n");
            printf("  4. Click 'Remove device'\n\n");
            printf("Press ENTER after you have removed the device...");
            getchar();
            printf("\n");
        }
    } else {
        printf("Keeping existing bonds\n\n");
    }
    
    // Configure advertising data for HID keyboard
    printf("Configuring advertising data for HID keyboard...\n");
    
    // Build advertising data packet:
    // Flags (3 bytes): Length=2, Type=0x01, Value=0x06 (LE General Discoverable, BR/EDR not supported)
    // Service UUIDs (6 bytes): Length=5, Type=0x03, UUIDs=0x1812,0x180F (HID, Battery)
    // Appearance (4 bytes): Length=3, Type=0x19, Value=0x03C1 (Keyboard)
    uint8_t advData[] = {
        0x02, 0x01, 0x06,       // Flags
        0x05, 0x03, 0x12, 0x18, 0x0F, 0x18,  // Service UUIDs (HID + Battery)
        0x03, 0x19, 0xC1, 0x03  // Appearance (Keyboard)
    };
    
    result = uCxBluetoothSetLegacyAdvertiseData(&gUcxHandle, advData, sizeof(advData));
    
    if (result == 0) {
        printf("✓ Advertising data configured (Flags + HID/Battery services + Keyboard appearance)\n");
    } else {
        printf("ERROR: Failed to set advertising data (code %d)\n", result);
        return;
    }
    
    // Enable pairing mode
    printf("Enabling pairing mode...\n");
    result = uCxBluetoothSetPairingMode(&gUcxHandle, U_BT_PAIRING_MODE_ENABLE);
    if (result == 0) {
        printf("✓ Pairing mode enabled\n");
    } else {
        printf("WARNING: Failed to enable pairing (code %d)\n", result);
    }
    
    // Enable advertising to make device discoverable using new API
    printf("Enabling Bluetooth legacy advertising...\n");
    result = uCxBluetoothLegacyAdvertisementStart(&gUcxHandle);
    if (result == 0) {
        gBluetoothAdvertising = true;
        printf("✓ Advertising enabled - Device is now discoverable!\n\n");
    } else {
        printf("WARNING: Failed to enable advertising (code %d)\n\n", result);
    }
        printf("\n─────────────────────────────────────────────────\n");
    printf("Device Ready for Pairing!\n");
    printf("─────────────────────────────────────────────────\n\n");
    
    printf("Next steps:\n");
    printf("  1. Scan for Bluetooth devices on your phone/PC\n");
    printf("  2. Look for 'u-blox ucxclient HID device'\n");
    printf("  3. Pair/Connect to the device\n");
    printf("  4. Use menu options to send key presses or media controls\n\n");
    
    printf("Manual control:\n");
    printf("  - Use [8] to send keyboard key presses\n");
    printf("  - Use [9] to send media control commands\n");
    printf("  - Use [a] to send 'Hello World' test\n\n");
    
    gGattServerServiceHandle = gHidServiceHandle;
}

// ============================================================================
// AUTOMATION IO (0x1815) GATT Server
// ============================================================================

// Helper: Send Digital notification
static void automationIoNotifyDigital(uint8_t newState)
{
    gAioServerDigitalState = newState;

    if (gCurrentGattConnHandle < 0 ||
        gAioServerDigitalCharHandle <= 0 ||
        gAioServerDigitalCccdHandle <= 0) {
        return;
    }

    // Send notification with new digital state
    int32_t result = uCxGattServerSendNotification(&gUcxHandle,
                                                   gCurrentGattConnHandle,
                                                   gAioServerDigitalCharHandle,
                                                   &gAioServerDigitalState,
                                                   1);
    if (result == 0) {
        printf("[AIO] Digital Notify: 0x%02X (LED0=%u)\n", 
               gAioServerDigitalState, 
               (gAioServerDigitalState & 0x01) ? 1 : 0);
    } else {
        printf("[AIO] WARNING: Failed to send digital notify (code %d)\n", result);
    }
}

// Helper: Send Analog notification
static void automationIoNotifyAnalog(uint16_t newValue)
{
    gAioServerAnalogValue = newValue;

    if (gCurrentGattConnHandle < 0 ||
        gAioServerAnalogCharHandle <= 0 ||
        gAioServerAnalogCccdHandle <= 0) {
        return;
    }

    // Send notification with new analog value (little-endian)
    uint8_t payload[2];
    payload[0] = (uint8_t)(gAioServerAnalogValue & 0xFF);
    payload[1] = (uint8_t)((gAioServerAnalogValue >> 8) & 0xFF);

    int32_t result = uCxGattServerSendNotification(&gUcxHandle,
                                                   gCurrentGattConnHandle,
                                                   gAioServerAnalogCharHandle,
                                                   payload,
                                                   sizeof(payload));
    if (result == 0) {
        printf("[AIO] Analog Notify: %u\n", gAioServerAnalogValue);
    } else {
        printf("[AIO] WARNING: Failed to send analog notify (code %d)\n", result);
    }
}

// Setup Automation IO service with Digital and Analog characteristics
static void gattServerSetupAutomationIo(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("Automation IO GATT Server Setup\n");
    printf("─────────────────────────────────────────────────\n\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");
    
    // Step 1: Define Automation IO Service (0x1815)
    printf("Step 1: Creating Automation IO Service (UUID: 0x1815)...\n");
    uint8_t aioServiceUuid[] = {0x18, 0x15};  // 16-bit UUID (little-endian)
    
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, aioServiceUuid, 2, &gAioServerServiceHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to create Automation IO Service (code %d)\n", result);
        return;
    }
    
    printf("✓ Automation IO Service defined with handle: %d\n\n", gAioServerServiceHandle);
    
    // Step 2: Add Digital characteristic (0x2A56) - Read + Write + Notify
    printf("Step 2: Adding Digital characteristic (0x2A56)...\n");
    uint8_t digitalUuid[] = {0x2A, 0x56};  // Little-endian
    uint8_t propReadWriteNotify[] = {0x1A};  // Read (0x02) + Write (0x08) + Notify (0x10) = 0x1A
    uint8_t digitalValue[] = {0x00};  // Initial value: all bits off
    
    uCxGattServerCharDefine_t digitalResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, digitalUuid, 2,
                                      propReadWriteNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      digitalValue, 1,
                                      1,
                                      &digitalResponse);
    
    if (result == 0) {
        gAioServerDigitalCharHandle = digitalResponse.value_handle;
        gAioServerDigitalCccdHandle = digitalResponse.cccd_handle;
        printf("✓ Digital characteristic created (handle: %d, CCCD: %d)\n", 
               digitalResponse.value_handle, digitalResponse.cccd_handle);
    } else {
        printf("ERROR: Failed to create Digital characteristic (code %d)\n", result);
        return;
    }
    
    // Step 3: Add Analog characteristic (0x2A58) - Read + Notify
    printf("Step 3: Adding Analog characteristic (0x2A58)...\n");
    uint8_t analogUuid[] = {0x2A, 0x58};  // Little-endian
    uint8_t propReadNotify[] = {0x12};  // Read (0x02) + Notify (0x10) = 0x12
    uint8_t analogValue[2] = {0xF4, 0x01};  // Initial value: 500 (little-endian)
    
    uCxGattServerCharDefine_t analogResponse;
    result = uCxGattServerCharDefine6(&gUcxHandle, analogUuid, 2,
                                      propReadNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE, U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      analogValue, 2,
                                      2,
                                      &analogResponse);
    
    if (result == 0) {
        gAioServerAnalogCharHandle = analogResponse.value_handle;
        gAioServerAnalogCccdHandle = analogResponse.cccd_handle;
        printf("✓ Analog characteristic created (handle: %d, CCCD: %d)\n", 
               analogResponse.value_handle, analogResponse.cccd_handle);
    } else {
        printf("ERROR: Failed to create Analog characteristic (code %d)\n", result);
        return;
    }
    
    // Step 4: Activate Automation IO Service
    printf("\nStep 4: Activating Automation IO Service...\n");
    result = uCxGattServerServiceActivate(&gUcxHandle);
    
    if (result != 0) {
        printf("ERROR: Failed to activate Automation IO service (code %d)\n", result);
        return;
    }
    
    printf("✓ Automation IO Service activated!\n");
    
    printf("─────────────────────────────────────────────────\n");
    printf("Automation IO Service Ready!\n");
    printf("─────────────────────────────────────────────────\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("Service details:\n");
    printf("  Digital characteristic (0x2A56): Read/Write/Notify\n");
    printf("    - bit 0 = LED 0 state (0=off, 1=on)\n");
    printf("    - Current value: 0x%02X\n", gAioServerDigitalState);
    printf("  Analog characteristic (0x2A58): Read/Notify\n");
    printf("    - Range: 0-1000 (example: voltage * 100)\n");
    printf("    - Current value: %u\n\n", gAioServerAnalogValue);
    
    // Enable advertising
    printf("Enabling Bluetooth legacy advertising...\n");
    result = uCxBluetoothLegacyAdvertisementStart(&gUcxHandle);
    if (result == 0) {
        gBluetoothAdvertising = true;
        printf("✓ Advertising enabled - Device is now discoverable!\n\n");
    } else {
        printf("WARNING: Failed to enable advertising (code %d)\n\n", result);
    }
    
    printf("─────────────────────────────────────────────────\n");
    printf("Device Ready for Connection!\n");
    printf("─────────────────────────────────────────────────\n\n");
    
    gGattServerServiceHandle = gAioServerServiceHandle;
}

// Setup Battery Service only (simple example)
static void gattServerSetupBatteryOnly(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: Battery Service Example ---\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    // 1) Define Battery Service 0x180F
    uint8_t svcUuid[] = {0x18, 0x0F};   // 0x180F, big-endian
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, svcUuid, 2, &gBatteryServiceHandle);
    if (result != 0) {
        printf("ERROR: Failed to define Battery Service (code %d)\n", result);
        return;
    }
    printf("✓ Battery Service defined (handle=%d)\n", gBatteryServiceHandle);

    // 2) Add Battery Level characteristic 0x2A19, Read + Notify
    uint8_t charUuid[] = {0x2A, 0x19};  // 0x2A19
    uint8_t props[]    = {0x12};        // Read (0x02) + Notify (0x10)
    uint8_t initial    = 100;

    uCxGattServerCharDefine_t rsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      charUuid, 2,
                                      props, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      &initial, 1,
                                      1,               // add CCCD
                                      &rsp);
    if (result != 0) {
        printf("ERROR: Failed to add Battery Level characteristic (code %d)\n", result);
        return;
    }

    gBatteryLevelHandle = rsp.value_handle;
    gBatteryCccdHandle  = rsp.cccd_handle;

    printf("✓ Battery Level characteristic created (val_handle=%d, cccd_handle=%d)\n",
           gBatteryLevelHandle, gBatteryCccdHandle);

    // 3) Activate service
    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to activate Battery Service (code %d)\n", result);
        return;
    }

    printf("✓ Battery Service activated\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("Auto-Notification System:\n");
    printf("  - Battery level automatically decrements from 100%% to 10%% every second\n");
    printf("  - Shared notification thread handles Battery, Heartbeat, and CTS services\n");
    printf("  - Thread starts automatically when client enables notifications\n");
    printf("  - Updates sent every 1 second while notifications are enabled\n\n");
}

// Setup Environmental Sensing Service (Temperature + Humidity)
static void gattServerSetupEnvSensing(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: Environmental Sensing ---\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    // 1) Define ESS service 0x181A
    uint8_t svcUuid[] = {0x18, 0x1A};   // 0x181A
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, svcUuid, 2, &gEnvServerServiceHandle);
    if (result != 0) {
        printf("ERROR: Failed to define Environmental Sensing service (code %d)\n", result);
        return;
    }
    printf("✓ ESS defined (handle=%d)\n", gEnvServerServiceHandle);

    uint8_t propsReadNotify[] = {0x12}; // Read + Notify

    // 2) Temperature characteristic (0x2A6E), in 0.01°C units, e.g. 23.45°C
    uint8_t tempUuid[] = {0x2A, 0x6E};
    int16_t tempValue  = 2345; // 23.45°C

    uCxGattServerCharDefine_t tempRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      tempUuid, 2,
                                      propsReadNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      (uint8_t *)&tempValue, sizeof(tempValue),
                                      2,  // max_length for int16_t (2 bytes)
                                      &tempRsp);
    if (result != 0) {
        printf("ERROR: Failed to add Temperature characteristic (code %d)\n", result);
        return;
    }

    gEnvServerTempHandle = tempRsp.value_handle;
    gEnvServerTempCccdHandle = tempRsp.cccd_handle;
    printf("✓ Temperature characteristic created (handle=%d, CCCD=%d)\n",
           gEnvServerTempHandle, gEnvServerTempCccdHandle);

    // 3) Humidity characteristic (0x2A6F), in 0.01%% units, e.g. 45.00%%
    uint8_t humUuid[] = {0x2A, 0x6F};
    uint16_t humValue = 4500; // 45.00%

    uCxGattServerCharDefine_t humRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      humUuid, 2,
                                      propsReadNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      (uint8_t *)&humValue, sizeof(humValue),
                                      2,  // max_length for uint16_t (2 bytes)
                                      &humRsp);
    if (result != 0) {
        printf("ERROR: Failed to add Humidity characteristic (code %d)\n", result);
        return;
    }

    gEnvServerHumHandle = humRsp.value_handle;
    gEnvServerHumCccdHandle = humRsp.cccd_handle;
    printf("✓ Humidity characteristic created (handle=%d, CCCD=%d)\n",
           gEnvServerHumHandle, gEnvServerHumCccdHandle);

    // 4) Activate service
    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to activate ESS (code %d)\n", result);
        return;
    }

    printf("✓ ESS activated\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("You can now read Temperature/Humidity or subscribe to notifications.\n\n");
}

// Setup Custom UART Service (NUS-style)
static void gattServerSetupUartService(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: Custom UART Service ---\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    int32_t result = uCxGattServerServiceDefine(&gUcxHandle,
                                                (uint8_t *)kUartServiceUuid, 16,
                                                &gUartServerServiceHandle);
    if (result != 0) {
        printf("ERROR: Failed to define UART service (code %d)\n", result);
        return;
    }
    printf("✓ UART service handle=%d\n", gUartServerServiceHandle);

    // Properties
    uint8_t propsNotify[]   = {0x10}; // Notify
    uint8_t propsWriteNoRsp[] = {0x04}; // WriteWithoutResponse

    // TX characteristic (Notify from server)
    uint8_t initTx[1] = {0x00};
    uCxGattServerCharDefine_t txRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      (uint8_t *)kUartTxCharUuid, 16,
                                      propsNotify, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      initTx, sizeof(initTx),
                                      244,  // max_length for UART data transmission
                                      &txRsp);
    if (result != 0) {
        printf("ERROR: Failed to add UART TX characteristic (code %d)\n", result);
        return;
    }
    gUartServerTxHandle = txRsp.value_handle;
    gUartServerTxCccdHandle = txRsp.cccd_handle;
    printf("✓ UART TX characteristic handle=%d, CCCD=%d\n",
           gUartServerTxHandle, gUartServerTxCccdHandle);

    // RX characteristic (Write Without Response from client)
    uint8_t initRx[1] = {0x00};
    uCxGattServerCharDefine_t rxRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      (uint8_t *)kUartRxCharUuid, 16,
                                      propsWriteNoRsp, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_UNAUTHENTICATED,
                                      initRx, sizeof(initRx),
                                      244,  // max_length for UART data reception
                                      &rxRsp);
    if (result != 0) {
        printf("ERROR: Failed to add UART RX characteristic (code %d)\n", result);
        return;
    }
    gUartServerRxHandle = rxRsp.value_handle;
    printf("✓ UART RX characteristic handle=%d\n", gUartServerRxHandle);

    // Activate
    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to activate UART service (code %d)\n", result);
        return;
    }

    printf("✓ UART service activated\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("Waiting for client to connect and enable notifications...\n");
    printf("When connected, incoming data will be displayed automatically.\n");
    printf("To send data to client, press Enter and type your message.\n\n");
    
    // Wait for client connection and CCCD write
    printf("Press any key when client has connected and enabled notifications...\n");
    getchar();
    
    // Interactive send loop
    if (gUartServerTxNotificationsEnabled && gCurrentGattConnHandle >= 0) {
        printf("\n[UART TX] Ready to send data to client\n");
        printf("Type messages (Enter to send, empty line to exit):\n\n");
        
        char message[244];
        while (1) {
            printf("TX> ");
            if (fgets(message, sizeof(message), stdin) == NULL) {
                break;
            }
            
            // Remove newline
            size_t len = strlen(message);
            if (len > 0 && message[len-1] == '\n') {
                message[len-1] = '\0';
                len--;
            }
            
            // Empty line = exit
            if (len == 0) {
                printf("Exiting UART TX mode.\n");
                break;
            }
            
            // Send notification
            int32_t sendResult = uCxGattServerSendNotification(&gUcxHandle, 
                                                           gCurrentGattConnHandle,
                                                           gUartServerTxHandle,
                                                           (uint8_t*)message, 
                                                           (int32_t)len);
            if (sendResult == 0) {
                printf("[UART TX] Sent %zu bytes\n", len);
            } else {
                printf("[ERROR] Failed to send UART notification (code %d)\n", sendResult);
                if (sendResult == -5) {
                    printf("  Connection may be lost. Press Enter to exit.\n");
                    getchar();
                    break;
                }
            }
        }
    } else {
        printf("\n[WARNING] Client not connected or notifications not enabled.\n");
        printf("Make sure client has:\n");
        printf("  1. Connected to the GATT server\n");
        printf("  2. Written 0x0001 to CCCD handle %d to enable notifications\n", gUartServerTxCccdHandle);
    }
    
    printf("\n");
}

// Setup u-blox Serial Port Service (SPS)
static void gattServerSetupSpsService(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: u-blox Serial Port Service (SPS) ---\n");
    printf("This is similar to NUS but includes optional credit-based flow control\n\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    // Define SPS service (128-bit UUID)
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle,
                                                (uint8_t *)kSpsServiceUuid, 16,
                                                &gSpsServerServiceHandle);
    if (result != 0) {
        printf("ERROR: Failed to define SPS service (code %d)\n", result);
        return;
    }
    printf("✓ SPS service handle=%d\n", gSpsServerServiceHandle);

    // Properties for FIFO: Notify + Indicate + Write + WriteNoRsp
    uint8_t propsFifo[] = {0x3C}; // 0x20 (Indicate) | 0x10 (Notify) | 0x08 (Write) | 0x04 (WriteNoRsp)
    
    // Properties for Credits: Notify + Indicate + Write + WriteNoRsp
    uint8_t propsCredits[] = {0x3C}; // Same as FIFO

    // FIFO characteristic (handles bidirectional data)
    uint8_t initFifo[1] = {0x00};
    uCxGattServerCharDefine_t fifoRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      (uint8_t *)kSpsFifoCharUuid, 16,
                                      propsFifo, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_UNAUTHENTICATED,
                                      initFifo, sizeof(initFifo),
                                      244,  // max_length for SPS data packets
                                      &fifoRsp);
    if (result != 0) {
        printf("ERROR: Failed to add SPS FIFO characteristic (code %d)\n", result);
        return;
    }
    gSpsServerFifoHandle = fifoRsp.value_handle;
    gSpsServerFifoCccdHandle = fifoRsp.cccd_handle;
    printf("✓ SPS FIFO characteristic handle=%d, CCCD=%d\n",
           gSpsServerFifoHandle, gSpsServerFifoCccdHandle);

    // Credits characteristic (MANDATORY for flow control)
    uint8_t initCredits[1] = {0x00};
    uCxGattServerCharDefine_t creditsRsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      (uint8_t *)kSpsCreditsCharUuid, 16,
                                      propsCredits, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_UNAUTHENTICATED,
                                      initCredits, sizeof(initCredits),
                                      4,  // max_length for credit counter (uint32)
                                      &creditsRsp);
    if (result != 0) {
        printf("ERROR: Failed to add SPS Credits characteristic (code %d)\n", result);
        return;
    }
    gSpsServerCreditsHandle = creditsRsp.value_handle;
    gSpsServerCreditsCccdHandle = creditsRsp.cccd_handle;
    printf("✓ SPS Credits characteristic handle=%d, CCCD=%d\n",
           gSpsServerCreditsHandle, gSpsServerCreditsCccdHandle);

    // Activate service
    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to activate SPS service (code %d)\n", result);
        return;
    }

    printf("✓ SPS service activated\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("Service is ready! Waiting for client connection...\n");
    printf("Incoming data will be displayed automatically.\n\n");
    
    printf("When client connects and enables notifications:\n");
    printf("  • Press 's' to enter send mode and type messages\n");
    printf("  • Press '0' to return to menu\n\n");
    printf("Waiting for connection");
    
    // Wait for connection with timeout (60 seconds)
    int timeout = 60;
    while (timeout > 0 && gCurrentGattConnHandle < 0) {
        printf(".");
        fflush(stdout);
        Sleep(1000);
        timeout--;
    }
    printf("\n");
    
    if (gCurrentGattConnHandle < 0) {
        printf("\n[TIMEOUT] No client connected after 60 seconds.\n");
        printf("Service remains active. You can test later or restart the service.\n\n");
        return;
    }
    
    printf("\n[CONNECTED] Client connected (handle %d)\n", gCurrentGattConnHandle);
    printf("Waiting for client to enable FIFO notifications...\n\n");
    
    // Wait for CCCD enable (30 seconds)
    timeout = 30;
    while (timeout > 0 && !gSpsServerFifoNotifyEnabled) {
        Sleep(100);
        timeout--;
        if (timeout % 10 == 0) {
            printf(".");
            fflush(stdout);
        }
    }
    
    if (!gSpsServerFifoNotifyEnabled) {
        printf("\n[TIMEOUT] Client did not enable notifications.\n");
        printf("Service remains active. Client can still connect later.\n\n");
        return;
    }
    
    // Interactive send loop
    if (gSpsServerFifoNotifyEnabled && gCurrentGattConnHandle >= 0) {
        printf("\n\n[SPS] Ready! Client has enabled notifications.\n");
        
        // Show flow control status
        if (gSpsServerFlowControlActive) {
            printf("[SPS] Flow control is ACTIVE (credits available: %d)\n", gSpsServerRemoteCredits);
        } else {
            printf("[SPS] Flow control is INACTIVE (works like NUS)\n");
        }
        
        printf("\nPress 's' + Enter to send messages, or press Enter to continue...\n");
        char choice = getchar();
        
        if (choice == 's' || choice == 'S') {
            printf("\n[SPS TX] Send mode activated\n");
            printf("Type messages (Enter to send, empty line to exit):\n\n");
            
            char message[244];
            while (1) {
                printf("TX> ");
                if (fgets(message, sizeof(message), stdin) == NULL) {
                    break;
                }
                
                // Remove newline
                size_t len = strlen(message);
                if (len > 0 && message[len-1] == '\n') {
                    message[len-1] = '\0';
                    len--;
                }
                
                // Empty line = exit
                if (len == 0) {
                    printf("Exiting SPS TX mode.\n");
                    break;
                }
                
                // Check flow control
                if (gSpsServerFlowControlActive && gSpsServerRemoteCredits <= 0) {
                    printf("[SPS] Waiting for credits from client...\n");
                    // Wait a bit for credits
                    Sleep(500);
                    if (gSpsServerRemoteCredits <= 0) {
                        printf("[SPS] No credits available. Message not sent.\n");
                        printf("      Client needs to send credits or disable flow control (send -1).\n");
                        continue;
                    }
                }
                
                // Send notification
                int32_t sendResult = uCxGattServerSendNotification(&gUcxHandle, 
                                                               gCurrentGattConnHandle,
                                                               gSpsServerFifoHandle,
                                                               (uint8_t*)message, 
                                                               (int32_t)len);
                if (sendResult == 0) {
                    printf("[SPS TX] Sent %zu bytes\n", len);
                    
                    // Consume credit if flow control active
                    if (gSpsServerFlowControlActive && gSpsServerRemoteCredits > 0) {
                        gSpsServerRemoteCredits--;
                        printf("[SPS] Credits remaining: %d\n", gSpsServerRemoteCredits);
                    }
                } else {
                    printf("[ERROR] Failed to send SPS notification (code %d)\n", sendResult);
                    if (sendResult == -5) {
                        printf("  Connection may be lost. Press Enter to exit.\n");
                        getchar();
                        break;
                    }
                }
            }
        }
    } else {
        printf("\n[WARNING] Client not connected or FIFO notifications not enabled.\n");
        printf("Make sure client has:\n");
        printf("  1. Connected to the GATT server\n");
        printf("  2. Written 0x0001 to FIFO CCCD handle %d to enable notifications\n", gSpsServerFifoCccdHandle);
    }
    
    printf("\n");
}

// Setup Location & Navigation Service
static void gattServerSetupLocationService(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: Location & Navigation (LNS) ---\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    // Service 0x1819
    uint8_t svcUuid[] = {0x18, 0x19};
    int32_t result = uCxGattServerServiceDefine(&gUcxHandle, svcUuid, 2, &gLnsServerServiceHandle);
    if (result != 0) {
        printf("ERROR: Failed to define LNS (code %d)\n", result);
        return;
    }
    printf("✓ LNS defined (handle=%d)\n", gLnsServerServiceHandle);

    // Char 0x2A67: Location and Speed (very simplified payload)
    uint8_t charUuid[] = {0x2A, 0x67};
    uint8_t props[]    = {0x12}; // Read + Notify

    // Minimal example flags + latitude/longitude as 1e-7 degrees (like GNSS)
    // Using pragma pack for MSVC compatibility
    #pragma pack(push, 1)
    struct {
        uint16_t flags;
        int32_t  latitude;   // 1e-7 degrees
        int32_t  longitude;  // 1e-7 degrees
    } loc;
    #pragma pack(pop)
    
    loc.flags = 0x0003;           // "position present"
    loc.latitude = 557174228;     // ≈ 55.7174228°
    loc.longitude = 132137795;    // ≈ 13.2137795°

    uCxGattServerCharDefine_t rsp;
    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      charUuid, 2,
                                      props, 1,
                                      U_GATT_SERVER_READ_SECURITY_NONE,
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,
                                      (uint8_t *)&loc, sizeof(loc),
                                      1,
                                      &rsp);
    if (result != 0) {
        printf("ERROR: Failed to add Location and Speed characteristic (code %d)\n", result);
        return;
    }

    gLnsServerLocSpeedHandle = rsp.value_handle;
    printf("✓ Location+Speed characteristic handle=%d, CCCD=%d\n",
           gLnsServerLocSpeedHandle, rsp.cccd_handle);

    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to activate LNS (code %d)\n", result);
        return;
    }

    printf("✓ LNS activated\n");
    
    // Show connection information
    showGattServerConnectionInfo();
}

// GATT Client: Read Current Time Service
static void gattClientReadCurrentTime(void)
{
    if (!gUcxConnected || gCurrentGattConnHandle < 0) {
        printf("ERROR: No active GATT connection\n");
        return;
    }

    printf("\n--- GATT Client: Current Time Service ---\n");

    // 0x1805 / 0x2A2B
    int svcIndex = findServiceByUuid16(0x1805);
    if (svcIndex < 0) {
        printf("CTS service (0x1805) not found. Did you discover services?\n");
        return;
    }

    int chIndex = findCharByUuid16InService(svcIndex, 0x2A2B);
    if (chIndex < 0) {
        printf("Current Time characteristic (0x2A2B) not found.\n");
        return;
    }

    int32_t charHandle = gGattCharacteristics[chIndex].valueHandle;

    uint8_t buf[32];
    uByteArray_t value;
    value.pData = buf;

    int32_t result = uCxGattClientReadBegin(&gUcxHandle,
                                           gCurrentGattConnHandle,
                                           charHandle,
                                           &value);
    if (result < 0) {
        printf("ERROR: Read failed (code %d)\n", result);
        return;
    }

    printf("CTS raw (%d bytes):", (int)value.length);
    for (size_t i = 0; i < value.length; i++) {
        printf(" %02X", value.pData[i]);
    }
    printf("\n");

    uCxEnd(&gUcxHandle);

    if (value.length >= 7) {
        uint16_t year   = value.pData[0] | (value.pData[1] << 8);
        uint8_t  month  = value.pData[2];
        uint8_t  day    = value.pData[3];
        uint8_t  hour   = value.pData[4];
        uint8_t  minute = value.pData[5];
        uint8_t  second = value.pData[6];

        printf("Decoded time: %04u-%02u-%02u %02u:%02u:%02u\n",
               year, month, day, hour, minute, second);
    }
}

// Build CTS time payload from Windows system time
static size_t ctsBuildTimePayload(uint8_t out[10])
{
    SYSTEMTIME t;
    GetLocalTime(&t);

    uint16_t year = (uint16_t)t.wYear;

    out[0] = year & 0xFF;
    out[1] = (year >> 8) & 0xFF;
    out[2] = (uint8_t)t.wMonth;
    out[3] = (uint8_t)t.wDay;
    out[4] = (uint8_t)t.wHour;
    out[5] = (uint8_t)t.wMinute;
    out[6] = (uint8_t)t.wSecond;

    // Day of week: Windows: Sunday=0 → BLE: Monday=1..Sunday=7
    uint8_t dow = (uint8_t)t.wDayOfWeek;
    if (dow == 0) dow = 7;
    out[7] = dow;

    out[8] = 0;    // Fractions256
    out[9] = 1;    // Adjust Reason: 1 = manual time update

    return 10;
}

// Setup CTS GATT Server
static void gattServerSetupCtsService(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }

    printf("\n--- GATT Server: Current Time Service (CTS) ---\n");
    
    // Ensure legacy advertisements are enabled for incoming connections
    if (!ensureLegacyAdvertisementEnabled()) {
        printf("WARNING: Failed to enable advertisements - remote devices may not connect\n");
    }
    printf("\n");

    // Service UUID 0x1805
    uint8_t svcUuid[] = {0x18, 0x05};

    int32_t result =
        uCxGattServerServiceDefine(&gUcxHandle, svcUuid, 2, &gCtsServerServiceHandle);

    if (result != 0) {
        printf("ERROR: Cannot define CTS service (code %d)\n", result);
        return;
    }

    printf("✓ CTS Service defined (handle=%d)\n", gCtsServerServiceHandle);

    // Characteristic UUID 0x2A2B
    uint8_t charUuid[] = {0x2A, 0x2B};
    uint8_t props[]    = {0x12};   // Read + Notify

    uint8_t initPayload[10];
    ctsBuildTimePayload(initPayload);

    uCxGattServerCharDefine_t rsp;

    result = uCxGattServerCharDefine6(&gUcxHandle,
                                      charUuid, 2,          // UUID
                                      props, 1,             // Properties
                                      U_GATT_SERVER_READ_SECURITY_NONE, // Read
                                      U_GATT_SERVER_WRITE_SECURITY_NONE,// Write
                                      initPayload, 10,
                                      1,                    // add CCCD
                                      &rsp);

    if (result != 0) {
        printf("ERROR: Cannot add Current Time characteristic (code %d)\n", result);
        return;
    }

    gCtsServerTimeValueHandle = rsp.value_handle;
    gCtsServerTimeCccdHandle  = rsp.cccd_handle;

    printf("✓ Time characteristic (handle=%d, CCCD=%d)\n",
           gCtsServerTimeValueHandle, gCtsServerTimeCccdHandle);

    // Activate service
    result = uCxGattServerServiceActivate(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Cannot activate CTS service (code %d)\n", result);
        return;
    }

    printf("✓ CTS activated — device now serves accurate time\n");
    
    // Show connection information
    showGattServerConnectionInfo();
    
    printf("Auto-Notification System:\n");
    printf("  - Current time automatically updates every second when notifications enabled\n");
    printf("  - Shared notification thread handles Battery, Heartbeat, and CTS services\n");
    printf("  - Thread starts automatically when client enables notifications\n\n");
}

// Notify CTS time if client has enabled notifications
static void ctsNotifyIfEnabled()
{
    if (gCtsServerTimeValueHandle <= 0 || gCurrentGattConnHandle < 0)
        return;

    // Check if CCCD is enabled
    if (!gCtsServerNotificationsEnabled)
        return;

    uint8_t payload[10];
    ctsBuildTimePayload(payload);

    uCxGattServerSendNotification(&gUcxHandle,
                                  gCurrentGattConnHandle,
                                  gCtsServerTimeValueHandle,
                                  payload, 10);

    // Optional: Show or update dashboard time
    printf("[CTS] Notify: %04u-%02u-%02u %02u:%02u:%02u\n",
           payload[0] | (payload[1] << 8),
           payload[2], payload[3], payload[4], payload[5], payload[6]);
}

// Centralized HID keyboard report sending - protocol-aware (Boot vs Report mode)
static int32_t hidSendKeyReport(uint8_t modifiers,
                                uint8_t key1, uint8_t key2,
                                uint8_t key3, uint8_t key4,
                                uint8_t key5, uint8_t key6)
{
    if ((!gHidBootKbdNotificationsEnabled && !gHidKeyboardNotificationsEnabled) || gCurrentGattConnHandle < 0) {
        printf("[HID] Notifications not enabled or no connection.\n");
        return -1;
    }

    int32_t handle;
    uint8_t report[9];
    size_t len;

    if (gUseBootKeyboard) {
        // Boot mode: 8-byte report, no Report ID
        handle = gHidBootKbdInputHandle;
        report[0] = modifiers;   // modifiers
        report[1] = 0x00;        // reserved
        report[2] = key1;
        report[3] = key2;
        report[4] = key3;
        report[5] = key4;
        report[6] = key5;
        report[7] = key6;
        len = 8;
    } else {
        // Report mode: 9-byte report (Report ID = 1)
        handle = gHidKeyboardInputHandle;
        report[0] = 0x01;        // Report ID
        report[1] = modifiers;
        report[2] = 0x00;        // reserved
        report[3] = key1;
        report[4] = key2;
        report[5] = key3;
        report[6] = key4;
        report[7] = key5;
        report[8] = key6;
        len = 9;
    }

    int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle,
                                                   handle, report, (int32_t)len);
    if (result != 0) {
        printf("[HID] Failed to send key report (code %d)\n", result);
    }
    return result;
}

// Send a keyboard key press via HID
static void gattServerSendKeyPress(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gHidKeyboardInputHandle < 0) {
        printf("ERROR: HID Keyboard service not set up. Use option [6] first.\n");
        return;
    }
    
    if (gCurrentGattConnHandle < 0) {
        printf("ERROR: No GATT client connected\n");
        return;
    }
    
    if (!gHidBootKbdNotificationsEnabled && !gHidKeyboardNotificationsEnabled) {
        printf("\n─────────────────────────────────────────────────\n");
        printf("HID NOTIFICATIONS NOT YET ENABLED\n");
        printf("─────────────────────────────────────────────────\n");
        printf("The client (Windows/phone) has connected but hasn't\n");
        printf("enabled notifications on the HID Input Report yet.\n\n");
        printf("This is normal - the client first enables Service Changed\n");
        printf("notifications, then reads HID characteristics, and finally\n");
        printf("enables HID Input Report notifications.\n\n");
        printf("Please wait a few seconds and try again.\n");
        printf("You'll see '[HID Keyboard] Client enabled notifications'\n");
        printf("or '[HID Boot Keyboard] Client enabled notifications'\n");
        printf("when it's ready.\n");
        printf("─────────────────────────────────────────────────\n");
        return;
    }
    
    printf("\n--- Send Keyboard Key Press ---\n");
    printf("Enter key to send:\n");
    printf("  [a-z] Letters\n");
    printf("  [0-9] Numbers\n");
    printf("  [space] Space bar\n");
    printf("  [enter] Enter key\n");
    printf("  [esc] Escape\n");
    printf("Choice: ");
    
    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    // Remove newline
    input[strcspn(input, "\n")] = '\0';
    
    // HID Keyboard Report: [ReportID][Modifier][Reserved][Key1-Key6]
    // Get keycode for input
    uint8_t keycode = 0;
    const char *keyName = "";
    
    if (strcmp(input, "a") == 0) { keycode = 0x04; keyName = "A"; }
    else if (strcmp(input, "b") == 0) { keycode = 0x05; keyName = "B"; }
    else if (strcmp(input, "c") == 0) { keycode = 0x06; keyName = "C"; }
    else if (strcmp(input, "d") == 0) { keycode = 0x07; keyName = "D"; }
    else if (strcmp(input, "e") == 0) { keycode = 0x08; keyName = "E"; }
    else if (strcmp(input, "f") == 0) { keycode = 0x09; keyName = "F"; }
    else if (strcmp(input, "g") == 0) { keycode = 0x0A; keyName = "G"; }
    else if (strcmp(input, "h") == 0) { keycode = 0x0B; keyName = "H"; }
    else if (strcmp(input, "i") == 0) { keycode = 0x0C; keyName = "I"; }
    else if (strcmp(input, "j") == 0) { keycode = 0x0D; keyName = "J"; }
    else if (strcmp(input, "k") == 0) { keycode = 0x0E; keyName = "K"; }
    else if (strcmp(input, "l") == 0) { keycode = 0x0F; keyName = "L"; }
    else if (strcmp(input, "m") == 0) { keycode = 0x10; keyName = "M"; }
    else if (strcmp(input, "n") == 0) { keycode = 0x11; keyName = "N"; }
    else if (strcmp(input, "o") == 0) { keycode = 0x12; keyName = "O"; }
    else if (strcmp(input, "p") == 0) { keycode = 0x13; keyName = "P"; }
    else if (strcmp(input, "q") == 0) { keycode = 0x14; keyName = "Q"; }
    else if (strcmp(input, "r") == 0) { keycode = 0x15; keyName = "R"; }
    else if (strcmp(input, "s") == 0) { keycode = 0x16; keyName = "S"; }
    else if (strcmp(input, "t") == 0) { keycode = 0x17; keyName = "T"; }
    else if (strcmp(input, "u") == 0) { keycode = 0x18; keyName = "U"; }
    else if (strcmp(input, "v") == 0) { keycode = 0x19; keyName = "V"; }
    else if (strcmp(input, "w") == 0) { keycode = 0x1A; keyName = "W"; }
    else if (strcmp(input, "x") == 0) { keycode = 0x1B; keyName = "X"; }
    else if (strcmp(input, "y") == 0) { keycode = 0x1C; keyName = "Y"; }
    else if (strcmp(input, "z") == 0) { keycode = 0x1D; keyName = "Z"; }
    else if (strcmp(input, "1") == 0) { keycode = 0x1E; keyName = "1"; }
    else if (strcmp(input, "2") == 0) { keycode = 0x1F; keyName = "2"; }
    else if (strcmp(input, "3") == 0) { keycode = 0x20; keyName = "3"; }
    else if (strcmp(input, "4") == 0) { keycode = 0x21; keyName = "4"; }
    else if (strcmp(input, "5") == 0) { keycode = 0x22; keyName = "5"; }
    else if (strcmp(input, "6") == 0) { keycode = 0x23; keyName = "6"; }
    else if (strcmp(input, "7") == 0) { keycode = 0x24; keyName = "7"; }
    else if (strcmp(input, "8") == 0) { keycode = 0x25; keyName = "8"; }
    else if (strcmp(input, "9") == 0) { keycode = 0x26; keyName = "9"; }
    else if (strcmp(input, "0") == 0) { keycode = 0x27; keyName = "0"; }
    else if (strcmp(input, "enter") == 0) { keycode = 0x28; keyName = "Enter"; }
    else if (strcmp(input, "esc") == 0) { keycode = 0x29; keyName = "Escape"; }
    else if (strcmp(input, "space") == 0) { keycode = 0x2C; keyName = "Space"; }
    else {
        printf("Unknown key: %s\n", input);
        return;
    }
    
    printf("Sending key press: %s (code 0x%02X)\n", keyName, keycode);
    
    // Send key down (key pressed) using centralized function
    int32_t result = hidSendKeyReport(0x00, keycode, 0x00, 0x00, 0x00, 0x00, 0x00);
    
    if (result != 0) {
        printf("ERROR: Failed to send key down notification (code %d)\n", result);
        return;
    }
    
    Sleep(50);  // Hold key for 50ms
    
    // Send key up (all zeros = no keys pressed)
    result = hidSendKeyReport(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    
    if (result == 0) {
        printf("✓ Key press sent successfully\n");
    } else {
        printf("ERROR: Failed to send key release (code %d)\n", result);
    }
}

// Send media control command via HID
static void gattServerSendMediaControl(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gHidMediaInputHandle < 0) {
        printf("ERROR: HID service not set up. Use option [6] first.\n");
        return;
    }
    
    if (gCurrentGattConnHandle < 0) {
        printf("ERROR: No GATT client connected\n");
        return;
    }
    
    printf("\n--- Send Media Control ---\n");
    printf("  [1] Play/Pause\n");
    printf("  [2] Next Track\n");
    printf("  [3] Previous Track\n");
    printf("  [4] Volume Up\n");
    printf("  [5] Volume Down\n");
    printf("Choice: ");
    
    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    // Media Report: [ReportID][BitMask]
    // Bits: [VolUp][VolDown][Play][Next][Prev][Pad][Pad][Pad]
    uint8_t report[2] = {0x02, 0x00};  // Report ID 2 for media
    const char *actionName = "";
    
    switch(input[0]) {
        case '1':
            report[1] = 0b00000100;  // Play/Pause bit
            actionName = "Play/Pause";
            break;
        case '2':
            report[1] = 0b00001000;  // Next Track bit
            actionName = "Next Track";
            break;
        case '3':
            report[1] = 0b00010000;  // Previous Track bit
            actionName = "Previous Track";
            break;
        case '4':
            report[1] = 0b00000001;  // Volume Up bit
            actionName = "Volume Up";
            break;
        case '5':
            report[1] = 0b00000010;  // Volume Down bit
            actionName = "Volume Down";
            break;
        default:
            printf("Invalid choice\n");
            return;
    }
    
    printf("Sending: %s\n", actionName);
    
    // Send button press
    int32_t result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle, 
                                         gHidMediaInputHandle, report, 2);
    
    if (result != 0) {
        printf("ERROR: Failed to send media press (code %d)\n", result);
        return;
    }
    
    Sleep(100);  // Hold for 100ms
    
    // Send button release (all zeros)
    uint8_t releaseReport[2] = {0x02, 0x00};
    result = uCxGattServerSendNotification(&gUcxHandle, gCurrentGattConnHandle, 
                                  gHidMediaInputHandle, releaseReport, 2);
    
    if (result == 0) {
        printf("✓ Media control sent successfully\n");
    } else {
        printf("ERROR: Failed to send release (code %d)\n", result);
    }
}

// Send "Hello World" as keyboard key presses
static void gattServerSendHelloWorld(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gHidKeyboardInputHandle < 0) {
        printf("ERROR: HID Keyboard service not set up. Use option [6] first.\n");
        return;
    }
    
    if (gCurrentGattConnHandle < 0) {
        printf("ERROR: No GATT client connected\n");
        return;
    }
    
    if (!gHidBootKbdNotificationsEnabled && !gHidKeyboardNotificationsEnabled) {
        printf("\n─────────────────────────────────────────────────\n");
        printf("HID NOTIFICATIONS NOT YET ENABLED\n");
        printf("─────────────────────────────────────────────────\n");
        printf("The client (Windows/phone) has connected but hasn't\n");
        printf("enabled notifications on the HID Input Report yet.\n\n");
        printf("This is normal - the client first enables Service Changed\n");
        printf("notifications, then reads HID characteristics, and finally\n");
        printf("enables HID Input Report notifications.\n\n");
        printf("Please wait a few seconds and try again.\n");
        printf("You'll see '[HID Keyboard] Client enabled notifications'\n");
        printf("or '[HID Boot Keyboard] Client enabled notifications'\n");
        printf("when it's ready.\n");
        printf("─────────────────────────────────────────────────\n");
        return;
    }
    
    const char *reportType = gUseBootKeyboard ? "Boot Keyboard" : "Regular Keyboard";
    
    printf("\n--- Sending 'Hello World' (via %s) ---\n", reportType);
    
    // Map: H e l l o   W o r l d
    const char *text = "Hello World";
    const uint8_t keycodes[] = {
        0x0B, // H
        0x08, // e
        0x0F, // l
        0x0F, // l
        0x12, // o
        0x2C, // space
        0x1A, // W
        0x12, // o
        0x15, // r
        0x0F, // l
        0x07  // d
    };
    
    for (size_t i = 0; i < strlen(text); i++) {
        // Check if uppercase (need Shift modifier = 0x02)
        uint8_t modifier = 0x00;
        if (text[i] >= 'A' && text[i] <= 'Z') {
            modifier = 0x02;  // Left Shift modifier
        }
        
        printf("  Sending: '%c'\n", text[i]);
        
        // Send key down using centralized function (handles Boot vs Report mode)
        int32_t result = hidSendKeyReport(modifier, keycodes[i], 0x00, 0x00, 0x00, 0x00, 0x00);
        
        if (result != 0) {
            printf("ERROR: Failed to send key (code %d)\n", result);
            return;
        }
        
        Sleep(50);  // Hold key
        
        // Send key up (all zeros)
        hidSendKeyReport(0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
        
        Sleep(50);  // Delay between keys
    }
    
    printf("✓ 'Hello World' sent successfully!\n");
}

// ============================================================================
// BLUETOOTH MANAGEMENT (Advertising, Pairing, Status)
// ============================================================================

// URC handler for bond/pairing status
static void bluetoothPairUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, uBtBondStatus_t bond_status)
{
    (void)puCxHandle;
    
    // Safety check: handle empty URC params
    if (bd_addr == NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Received UEBTB with empty params (ignoring)");
        return;
    }
    
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
             bd_addr->address[3], bd_addr->address[4], bd_addr->address[5]);
    
    if (bond_status == U_BT_BOND_STATUS_BONDING_SUCCEEDED) {
        printf("\n─────────────────────────────────────────────────\n");
        printf("PAIRING SUCCESS\n");
        printf("─────────────────────────────────────────────────\n");
        printf("Device: %s\n", addrStr);
        printf("Type:   %d\n", bd_addr->type);
        printf("─────────────────────────────────────────────────\n");
        strncpy(gBluetoothPairedDevice, addrStr, sizeof(gBluetoothPairedDevice) - 1);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Paired with device: %s", addrStr);
        
        // As a HID keyboard peripheral, we should reconnect to the paired host
        printf("\n[INFO] HID device will advertise for automatic reconnection...\n");
        printf("[INFO] On your phone/PC, tap the device to reconnect\n");
        
        // Note: Some hosts automatically reconnect, others require manual reconnection
        // The advertising is already enabled, so the host can initiate connection

        // printf("[INFO] Restarting advertising for HID reconnect...\n");
        // // Re-enable general advertising so Host can reconnect automatically
        // uCxAtClientExecSimpleCmd(gUcxHandle.pAtClient, "AT+UBTAL");
        // int32_t result = uCxEnd(&gUcxHandle);
        // if (result == 0) {
        //     printf("✓ Advertising restarted for reconnect\n");
        // } else {
        //     printf("WARNING: Could not restart advertising (code %d)\n", result);
        // }
        
    } else {
        printf("\n─────────────────────────────────────────────────\n");
        printf("PAIRING FAILED\n");
        printf("─────────────────────────────────────────────────\n");
        printf("Device: %s\n", addrStr);
        printf("Status: %d\n", bond_status);
        printf("─────────────────────────────────────────────────\n");
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Pairing failed for device: %s, status: %d", addrStr, bond_status);
    }
}

// URC handler for user confirmation during pairing (DisplayYesNo)
static void bluetoothUserConfirmationUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t numeric_value)
{
    (void)puCxHandle;
    
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
             bd_addr->address[3], bd_addr->address[4], bd_addr->address[5]);
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("BLUETOOTH PAIRING REQUEST\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Device:       %s\n", addrStr);
    printf("Pairing Code: %06d\n", numeric_value);
    printf("─────────────────────────────────────────────────\n");
    printf("Verify this code matches on both devices.\n");
    printf("Accept pairing? (y/n): ");
    fflush(stdout);
    
    int response = getchar();
    getchar(); // consume newline
    
    uBtConfirm_t answer = (response == 'y' || response == 'Y') ? U_BT_CONFIRM_YES : U_BT_CONFIRM_NO;
    int32_t result = uCxBluetoothUserConfirmation(&gUcxHandle, bd_addr, answer);
    
    if (result == 0) {
        if (answer == U_BT_CONFIRM_YES) {
            printf("✓ Pairing accepted - waiting for confirmation...\n");
        } else {
            printf("✗ Pairing rejected\n");
        }
    } else {
        printf("ERROR: Failed to send confirmation (code %d)\n", result);
    }
}

// URC handler for passkey display during pairing (DisplayOnly)
static void bluetoothPasskeyDisplayUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, int32_t passkey)
{
    (void)puCxHandle;
    
    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
             bd_addr->address[3], bd_addr->address[4], bd_addr->address[5]);
    
    printf("\n─────────────────────────────────────────────────\n");
    printf("BLUETOOTH PAIRING - ENTER CODE\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Device: %s\n", addrStr);
    printf("\nEnter this code on the remote device:\n");
    printf(">>> %06d <<<\n", passkey);
    printf("─────────────────────────────────────────────────\n");
    printf("Waiting for remote device to confirm...\n");
}

// URC handler for passkey request during pairing (KeyboardOnly)
static void bluetoothPasskeyRequestUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr)
{
    (void)puCxHandle; 

    char addrStr[18];
    snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             bd_addr->address[0], bd_addr->address[1], bd_addr->address[2],
             bd_addr->address[3], bd_addr->address[4], bd_addr->address[5]);

    printf("\n─────────────────────────────────────────────────\n");
    printf("BLUETOOTH PAIRING - PASSKEY ENTRY\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Device: %s\n", addrStr);
    printf("\nThe remote device (Windows) is displaying a 6-digit passkey.\n");
    printf("You have 30 seconds to enter the passkey.\n");
    printf("Passkey (6 digits): ");
    fflush(stdout);
    
    // Non-blocking input with 30 second timeout
    char passkeyStr[16] = {0};
    int idx = 0;
    time_t startTime = time(NULL);
    bool gotInput = false;
    
    while (difftime(time(NULL), startTime) < 30 && idx < 6) {
        if (_kbhit()) {
            int ch = _getch();
            if (ch >= '0' && ch <= '9') {
                passkeyStr[idx++] = (char)ch;
                printf("*"); // Show asterisk for security
                fflush(stdout);
                if (idx == 6) {
                    gotInput = true;
                    break;
                }
            } else if (ch == '\r' || ch == '\n') {
                if (idx > 0) {
                    gotInput = true;
                    break;
                }
            } else if (ch == 8 && idx > 0) { // Backspace
                idx--;
                printf("\b \b"); // Erase character
                fflush(stdout);
            }
        }
        Sleep(50); // Small delay to avoid busy-wait
    }
    
    printf("\n");
    
    if (gotInput && idx >= 6) {
        int32_t passkey = atoi(passkeyStr);
        int32_t result = uCxBluetoothUserPasskeyEntry3(&gUcxHandle, bd_addr, U_BT_CONFIRM_YES, passkey);
        if (result == 0) {
            printf("✓ Passkey sent - waiting for pairing completion...\n");
        } else {
            printf("ERROR: Failed to send passkey (code %d)\n", result);
        }
    } else {
        printf("ERROR: Passkey entry timeout or invalid\n");
        uCxBluetoothUserPasskeyEntry2(&gUcxHandle, bd_addr, U_BT_CONFIRM_NO);
    }
}

// URC handler for PHY update events
static void bluetoothPhyUpdateUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, int32_t phy_status, int32_t tx_phy, int32_t rx_phy)
{
    (void)puCxHandle;
    (void)phy_status;
    
    const char *txPhyStr = (tx_phy == 2) ? "2 Mbps" : (tx_phy == 1) ? "1 Mbps" : "Unknown";
    const char *rxPhyStr = (rx_phy == 2) ? "2 Mbps" : (rx_phy == 1) ? "1 Mbps" : "Unknown";
    
    printf("\n[PHY Update] Connection %d: TX=%s, RX=%s\n", conn_handle, txPhyStr, rxPhyStr);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "PHY updated - conn=%d, TX=%d, RX=%d", conn_handle, tx_phy, rx_phy);
}

// Enable/disable Bluetooth advertising (discoverable mode)
static void bluetoothSetAdvertising(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Advertising Control ---\n");
    printf("Current state: %s\n", gBluetoothAdvertising ? "ENABLED" : "DISABLED");
    printf("\n");
    printf("  [1] Enable advertising (discoverable)\n");
    printf("  [2] Disable advertising\n");
    printf("  [0] Cancel\n");
    printf("Choice: ");
    
    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    int choice = atoi(input);
    
    if (choice == 1) {
        // Enable advertising using new API
        printf("\nEnabling Bluetooth legacy advertising...\n");
        
        int32_t result = uCxBluetoothLegacyAdvertisementStart(&gUcxHandle);
        
        if (result == 0) {
            gBluetoothAdvertising = true;
            printf("✓ Bluetooth advertising ENABLED\n");
            printf("  Device is now discoverable\n");
        } else {
            printf("ERROR: Failed to enable advertising (code %d)\n", result);
        }
    } else if (choice == 2) {
        // Disable advertising using new API
        printf("\nDisabling Bluetooth advertising...\n");
        
        int32_t result = uCxBluetoothLegacyAdvertisementStop(&gUcxHandle);
        
        if (result == 0) {
            gBluetoothAdvertising = false;
            printf("✓ Bluetooth advertising DISABLED\n");
        } else {
            printf("ERROR: Failed to disable advertising (code %d)\n", result);
        }
    }
}

// Configure Bluetooth pairing settings
static void bluetoothSetPairing(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("BLUETOOTH PAIRING CONFIGURATION\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    // Step 1: Pairing Mode
    printf("\n[1/3] Pairing Mode:\n");
    printf("  [1] Enable pairing (allow new devices to pair)\n");
    printf("  [2] Disable pairing (reject new pairing attempts)\n");
    printf("\nChoice: ");
    
    char choice[10];
    if (fgets(choice, sizeof(choice), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    int pairingMode;
    if (choice[0] == '1') {
        pairingMode = U_BT_PAIRING_MODE_ENABLE;
        printf("Selected: Enable pairing\n");
    } else if (choice[0] == '2') {
        pairingMode = U_BT_PAIRING_MODE_DISABLE;
        printf("Selected: Disable pairing\n");
    } else {
        printf("Invalid choice, using default (Enable)\n");
        pairingMode = U_BT_PAIRING_MODE_ENABLE;
    }
    
    // Step 2: Security Mode
    printf("\n[2/3] Security Mode:\n");
    printf("  [1] None (no security - not recommended)\n");
    printf("  [2] Unauthenticated bonding (Just Works)\n");
    printf("  [3] Authenticated bonding (requires PIN/passkey)\n");
    printf("  [4] Authenticated + Secure connection (encrypted link)\n");
    printf("  [5] Authenticated + Secure connection only (strict)\n");
    printf("\nChoice: ");
    
    if (fgets(choice, sizeof(choice), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    int securityMode;
    if (choice[0] == '1') {
        securityMode = U_BT_SECURITY_MODE_NONE;
        printf("Selected: No security\n");
    } else if (choice[0] == '2') {
        securityMode = U_BT_SECURITY_MODE_UNAUTHENTICATED;
        printf("Selected: Unauthenticated bonding\n");
    } else if (choice[0] == '3') {
        securityMode = U_BT_SECURITY_MODE_AUTHENTICATED;
        printf("Selected: Authenticated bonding\n");
    } else if (choice[0] == '4') {
        securityMode = U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION;
        printf("Selected: Authenticated + Secure connection\n");
    } else if (choice[0] == '5') {
        securityMode = U_BT_SECURITY_MODE_AUTHENTICATED_SECURE_CONNECTION_ONLY;
        printf("Selected: Authenticated + Secure connection only\n");
    } else {
        printf("Invalid choice, using default (Unauthenticated)\n");
        securityMode = U_BT_SECURITY_MODE_UNAUTHENTICATED;
    }
    
    // Step 3: IO Capabilities
    printf("\n[3/3] IO Capabilities:\n");
    printf("  [1] Display only (can show PIN)\n");
    printf("  [2] Display + Yes/No buttons (can confirm)\n");
    printf("  [3] Keyboard only (can enter PIN)\n");
    printf("  [4] No input/output (Just Works - automatic)\n");
    printf("  [5] Keyboard + Display (full interaction)\n");
    printf("\nChoice: ");
    
    if (fgets(choice, sizeof(choice), stdin) == NULL) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    
    int ioCapabilities;
    const char *ioCapDesc;
    if (choice[0] == '1') {
        ioCapabilities = U_BT_IO_CAP_DISPLAY_ONLY;
        ioCapDesc = "Display only";
    } else if (choice[0] == '2') {
        ioCapabilities = U_BT_IO_CAP_DISPLAY_YES_NO;
        ioCapDesc = "Display + Yes/No";
    } else if (choice[0] == '3') {
        ioCapabilities = U_BT_IO_CAP_KEYBOARD_ONLY;
        ioCapDesc = "Keyboard only";
    } else if (choice[0] == '4') {
        ioCapabilities = U_BT_IO_CAP_NO_INPUT_NO_OUTPUT;
        ioCapDesc = "No input/output (Just Works)";
    } else if (choice[0] == '5') {
        ioCapabilities = U_BT_IO_CAP_KEYBOARD_DISPLAY;
        ioCapDesc = "Keyboard + Display";
    } else {
        printf("Invalid choice, using default (No input/output)\n");
        ioCapabilities = U_BT_IO_CAP_NO_INPUT_NO_OUTPUT;
        ioCapDesc = "No input/output (Just Works)";
    }
    printf("Selected: %s\n", ioCapDesc);
    
    // Apply settings
    printf("\n");
    printf("\n");
    printf("Applying configuration...\n");
    printf("\n");
    
    // Set pairing mode
    int32_t result = uCxBluetoothSetPairingMode(&gUcxHandle, pairingMode);
    if (result == 0) {
        printf("✓ Pairing mode set\n");
    } else {
        printf("✗ Failed to set pairing mode (code %d)\n", result);
        return;
    }
    
    // Set security mode
    result = uCxBluetoothSetSecurityMode(&gUcxHandle, securityMode);
    if (result == 0) {
        printf("✓ Security mode set\n");
    } else {
        printf("⚠ Failed to set security mode (code %d)\n", result);
    }
    
    // Set IO capabilities
    result = uCxBluetoothSetIoCapabilities(&gUcxHandle, ioCapabilities);
    if (result == 0) {
        printf("✓ IO capabilities set\n");
    } else {
        printf("⚠ Failed to set IO capabilities (code %d)\n", result);
    }
    
    printf("\n✓ Pairing configuration complete!\n");
    printf("  Device is now ready for pairing\n");
}

// List all bonded (paired) devices
static void bluetoothListBondedDevices(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("BONDED DEVICES LIST\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    uCxBluetoothListBondedDevicesBegin(&gUcxHandle);
    uBtLeAddress_t bondedAddr;
    int bondCount = 0;
    
    while (uCxBluetoothListBondedDevicesGetNext(&gUcxHandle, &bondedAddr)) {
        bondCount++;
        char addrStr[18];
        snprintf(addrStr, sizeof(addrStr), "%02X:%02X:%02X:%02X:%02X:%02X",
                 bondedAddr.address[0], bondedAddr.address[1], bondedAddr.address[2],
                 bondedAddr.address[3], bondedAddr.address[4], bondedAddr.address[5]);
        
        const char *typeStr = (bondedAddr.type == 0) ? "Public" : "Random";
        printf("  [%d] %s (%s)\n", bondCount, addrStr, typeStr);
    }
    
    int32_t result = uCxEnd(&gUcxHandle);
    
    if (result == 0 && bondCount > 0) {
        printf("\n✓ Found %d bonded device(s)\n", bondCount);
    } else if (bondCount == 0) {
        printf("  No bonded devices found\n");
    } else {
        printf("\nWARNING: Error reading bond list (code %d)\n", result);
    }
    
    printf("\n");
    printf("Do you want to clear all bonds? (y/n): ");
    int clearBonds = getchar();
    getchar(); // consume newline
    
    if (clearBonds == 'y' || clearBonds == 'Y') {
        printf("\nClearing all Bluetooth bonds...\n");
        result = uCxBluetoothUnbondAll(&gUcxHandle);
        if (result == 0) {
            printf("✓ All bonds cleared\n");
            printf("\n[INFO] Remember to also remove this device from your\n");
            printf("       phone/PC Bluetooth settings for clean re-pairing.\n");
        } else {
            printf("ERROR: Failed to clear bonds (code %d)\n", result);
        }
    }
    
    printf("\n");
}

// Show current Bluetooth status
static void bluetoothShowStatus(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("BLUETOOTH STATUS\n");
    printf("─────────────────────────────────────────────────────────────\n\n");
    
    // Display status based on tracked state
    printf("Advertising:       %s\n", gBluetoothAdvertising ? "ENABLED (Discoverable)" : "DISABLED");
    
    if (gBluetoothPairedDevice[0] != '\0') {
        printf("Last Paired:       %s\n", gBluetoothPairedDevice);
    } else {
        printf("Last Paired:       None\n");
    }
    
    // Show GATT connection status
    if (gCurrentGattConnHandle >= 0) {
        printf("GATT Connection:   CONNECTED (handle %d)\n", gCurrentGattConnHandle);
    } else {
        printf("GATT Connection:   Not connected\n");
    }
    
    // Show active services
    printf("\nActive GATT Services:\n");
    if (gHeartbeatServiceHandle >= 0) {
        printf("  • Heart Rate Service (handle %d)\n", gHeartbeatServiceHandle);
    }
    if (gHidServiceHandle >= 0) {
        printf("  • HID Service (handle %d)\n", gHidServiceHandle);
        printf("  • Battery Service (handle %d)\n", gBatteryServiceHandle);
    }
    if (gHeartbeatServiceHandle < 0 && gHidServiceHandle < 0) {
        printf("  (none)\n");
    }
    
    printf("\n");
}

// ============================================================================
// MQTT OPERATIONS (Publish/Subscribe)
// ============================================================================

#define MQTT_CONFIG_ID 0
#define MQTT_DEFAULT_HOST "broker.emqx.io"
#define MQTT_DEFAULT_PORT 1883

static void mqttConnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- MQTT Connect ---\n");
    
    // Configure MQTT connection parameters
    const char *broker = MQTT_DEFAULT_HOST;
    int32_t port = MQTT_DEFAULT_PORT;
    char clientId[64];
    
    // Generate client ID with random component
    snprintf(clientId, sizeof(clientId), "ucxclient-%d", rand() % 10000);
    
    printf("Broker: %s:%d\n", broker, port);
    printf("Client ID: %s\n", clientId);
    
    // Set connection parameters
    printf("Configuring MQTT connection...\n");
    int32_t result = uCxMqttSetConnectionParams4(&gUcxHandle, MQTT_CONFIG_ID, broker, port, clientId);
    
    if (result != 0) {
        printf("ERROR: Failed to set connection parameters (code %d)\n", result);
        return;
    }
    
    // Set keepalive (60 seconds)
    result = uCxMqttSetKeepAlive(&gUcxHandle, MQTT_CONFIG_ID, 60);
    if (result != 0) {
        printf("WARNING: Failed to set keepalive (code %d)\n", result);
    }
    
    // Connect to broker
    printf("Connecting to MQTT broker...\n");
    result = uCxMqttConnect(&gUcxHandle, MQTT_CONFIG_ID);
    
    if (result == 0) {
        printf("✓ Connected to MQTT broker successfully\n");
        printf("\n");
        printf("You can now:\n");
        printf("  - Subscribe to topics\n");
        printf("  - Publish messages\n");
    } else {
        printf("ERROR: Failed to connect (code %d)\n", result);
        printf("\nTroubleshooting:\n");
        printf("  - Ensure Wi-Fi is connected (use option 8)\n");
        printf("  - Check if broker is accessible\n");
        printf("  - Try again in a few seconds\n");
    }
}

static void mqttDisconnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- MQTT Disconnect ---\n");
    printf("Disconnecting from MQTT broker...\n");
    
    int32_t result = uCxMqttDisconnect(&gUcxHandle, MQTT_CONFIG_ID);
    
    if (result == 0) {
        printf("Disconnected successfully.\n");
    } else {
        printf("ERROR: Failed to disconnect (code %d)\n", result);
    }
}

static void mqttSubscribe(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- MQTT Subscribe ---\n");
    
    char topic[128];
    printf("Enter topic to subscribe (wildcards allowed, e.g., 'test/#'): ");
    if (fgets(topic, sizeof(topic), stdin)) {
        // Remove newline
        char *newline = strchr(topic, '\n');
        if (newline) *newline = '\0';
        
        if (strlen(topic) == 0) {
            printf("ERROR: Topic cannot be empty\n");
            return;
        }
        
        // Ask for QoS
        printf("Select QoS level:\n");
        printf("  [0] At most once (fire and forget)\n");
        printf("  [1] At least once (acknowledged delivery)\n");
        printf("  [2] Exactly once (assured delivery)\n");
        printf("QoS: ");
        
        char qosStr[10];
        if (fgets(qosStr, sizeof(qosStr), stdin)) {
            int qos = atoi(qosStr);
            if (qos < 0 || qos > 2) {
                qos = 0;
            }
            
            printf("Subscribing to '%s' with QoS %d...\n", topic, qos);
            
            int32_t result = uCxMqttSubscribe4(&gUcxHandle, MQTT_CONFIG_ID, 
                                               U_MQTT_SUBSCRIBE_ACTION_SUBSCRIBE, 
                                               topic, (uMqttQos_t)qos);
            
            if (result == 0) {
                printf("✓ Subscribed successfully\n");
                printf("\nYou will receive messages published to this topic.\n");
                printf("Messages appear as URCs in the log (if logging is enabled).\n");
            } else {
                printf("ERROR: Failed to subscribe (code %d)\n", result);
            }
        }
    }
}

static void mqttUnsubscribe(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- MQTT Unsubscribe ---\n");
    
    char topic[128];
    printf("Enter topic to unsubscribe: ");
    if (fgets(topic, sizeof(topic), stdin)) {
        // Remove newline
        char *newline = strchr(topic, '\n');
        if (newline) *newline = '\0';
        
        if (strlen(topic) == 0) {
            printf("ERROR: Topic cannot be empty\n");
            return;
        }
        
        printf("Unsubscribing from '%s'...\n", topic);
        
        int32_t result = uCxMqttSubscribe3(&gUcxHandle, MQTT_CONFIG_ID, 
                                           U_MQTT_SUBSCRIBE_ACTION_UNSUBSCRIBE, 
                                           topic);
        
        if (result == 0) {
            printf("✓ Unsubscribed successfully\n");
        } else {
            printf("ERROR: Failed to unsubscribe (code %d)\n", result);
        }
    }
}

static void mqttPublish(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- MQTT Publish ---\n");
    
    char topic[128];
    char message[256];
    
    printf("Enter topic: ");
    if (!fgets(topic, sizeof(topic), stdin)) return;
    char *newline = strchr(topic, '\n');
    if (newline) *newline = '\0';
    
    if (strlen(topic) == 0) {
        printf("ERROR: Topic cannot be empty\n");
        return;
    }
    
    printf("Enter message: ");
    if (!fgets(message, sizeof(message), stdin)) return;
    newline = strchr(message, '\n');
    if (newline) *newline = '\0';
    
    // Ask for QoS
    printf("Select QoS level:\n");
    printf("  [0] At most once\n");
    printf("  [1] At least once\n");
    printf("  [2] Exactly once\n");
    printf("QoS: ");
    
    char qosStr[10];
    int qos = 0;
    if (fgets(qosStr, sizeof(qosStr), stdin)) {
        qos = atoi(qosStr);
        if (qos < 0 || qos > 2) {
            qos = 0;
        }
    }
    
    // Ask for retain flag
    printf("Retain message? [y/N]: ");
    char retainStr[10];
    int retain = 0;
    if (fgets(retainStr, sizeof(retainStr), stdin)) {
        retain = (tolower(retainStr[0]) == 'y') ? 1 : 0;
    }
    
    printf("\nPublishing to '%s'...\n", topic);
    printf("Message: %s\n", message);
    printf("QoS: %d, Retain: %d\n", qos, retain);
    
    uCxMqttPublish_t publishRsp;
    int32_t result = uCxMqttPublish(&gUcxHandle, MQTT_CONFIG_ID, (uMqttQos_t)qos, 
                                     (uMqttRetain_t)retain, topic, 
                                     (uint8_t*)message, (int32_t)strlen(message), &publishRsp);
    
    if (result == 0) {
        printf("✓ Message published successfully\n");
    } else {
        printf("ERROR: Failed to publish (code %d)\n", result);
    }
}

static void mqttMenu(void)
{
    gMenuState = MENU_MQTT;
}

// ----------------------------------------------------------------
// Wi-Fi Connectivity Check Helper
// ----------------------------------------------------------------

// Check Wi-Fi connection status, IP address, signal strength, and internet connectivity
// Returns: true if all checks pass, false otherwise
static bool checkWiFiConnectivity(bool checkInternet, bool verbose)
{
    int32_t err;
    
    if (verbose) {
        printf("Checking Wi-Fi connectivity...\n");
    }
    
    // Step 1: Check Wi-Fi connection status
    uCxWifiStationStatus_t wifiStatus;
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &wifiStatus)) {
        if (wifiStatus.type == U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT) {
            if (wifiStatus.rsp.StatusIdInt.int_val != 2) {  // 2 = Connected
                printf("ERROR: Wi-Fi is not connected!\n");
                printf("Please connect to Wi-Fi first using the [w] Wi-Fi menu.\n");
                uCxEnd(&gUcxHandle);
                return false;
            }
            if (verbose) {
                printf("✓ Wi-Fi is connected\n");
            }
        } else {
            printf("WARNING: Unexpected Wi-Fi status response type\n");
        }
        uCxEnd(&gUcxHandle);
    } else {
        printf("ERROR: Failed to check Wi-Fi connection status\n");
        return false;
    }
    
    // Step 2: Check RSSI (signal strength)
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &wifiStatus)) {
        if (wifiStatus.type == U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT) {
            int32_t rssi = wifiStatus.rsp.StatusIdInt.int_val;
            if (rssi == -32768) {
                printf("ERROR: Not connected to AP (RSSI unavailable)\n");
                uCxEnd(&gUcxHandle);
                return false;
            }
            
            if (rssi < -90) {
                printf("WARNING: Very weak Wi-Fi signal (RSSI: %d dBm)\n", rssi);
                printf("         Signal quality may be insufficient for reliable operation.\n");
            } else if (verbose) {
                printf("✓ Signal strength: %d dBm", rssi);
                if (rssi >= -50) {
                    printf(" (Excellent)\n");
                } else if (rssi >= -60) {
                    printf(" (Very Good)\n");
                } else if (rssi >= -70) {
                    printf(" (Good)\n");
                } else if (rssi >= -80) {
                    printf(" (Fair)\n");
                } else {
                    printf(" (Weak)\n");
                }
            }
        }
        uCxEnd(&gUcxHandle);
    }
    
    // Step 3: Check IP address using AT+UWSNST (IPv4 only)
    char ipStr[50] = {0};
    uSockIpAddress_t ipAddr;
    
    if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_WIFI_NET_STATUS_ID_IPV4, &ipAddr) == 0) {
        if (ipAddr.type == U_SOCK_ADDRESS_TYPE_V4) {
            uint32_t ipv4 = ipAddr.address.ipv4;
            snprintf(ipStr, sizeof(ipStr), "%d.%d.%d.%d", 
                     (ipv4 >> 24) & 0xFF,
                     (ipv4 >> 16) & 0xFF,
                     (ipv4 >> 8) & 0xFF,
                     ipv4 & 0xFF);
            
            if (ipv4 == 0) {
                printf("ERROR: No valid IP address assigned (0.0.0.0)\n");
                printf("Please ensure Wi-Fi is properly connected with DHCP.\n");
                return false;
            }
            if (verbose) {
                printf("✓ IP address: %s\n", ipStr);
            }
        } else {
            printf("ERROR: Unexpected IP address type (not IPv4)\n");
            return false;
        }
    } else {
        printf("ERROR: Could not read IP address from network status\n");
        printf("Please ensure Wi-Fi is connected with valid IP configuration.\n");
        return false;
    }
    
    // Step 4: Check internet connectivity (optional ping test)
    if (checkInternet) {
        if (verbose) {
            printf("Checking internet connectivity (ping 8.8.8.8)...\n");
        }
        
        // Reset ping counters
        gPingSuccess = 0;
        gPingFailed = 0;
        gPingAvgTime = 0;
        
        // Send ping to Google DNS (1 ping)
        err = uCxDiagnosticsPing2(&gUcxHandle, "8.8.8.8", 1);
        if (err == 0) {
            // Wait for ping complete URC event (max 5 seconds for 1 ping)
            if (waitEvent(URC_FLAG_PING_COMPLETE, 5)) {
                if (gPingSuccess > 0) {
                    if (verbose) {
                        printf("✓ Internet connectivity OK: %d ms\n", gPingAvgTime);
                    }
                } else {
                    printf("WARNING: Internet ping failed (no response from 8.8.8.8)\n");
                    printf("         Local network OK, but internet may not be reachable.\n");
                    // Don't return false - allow operation to continue
                }
            } else {
                printf("WARNING: Internet connectivity test timeout\n");
                printf("         Local network OK, but ping response not received.\n");
                // Don't return false - allow operation to continue
            }
        } else {
            printf("WARNING: Failed to send internet connectivity test (ping 8.8.8.8)\n");
            printf("         Local network OK, but could not initiate ping test.\n");
            printf("         Error code: %d\n", err);
            // Don't return false - allow operation to continue
        }
    }
    
    if (verbose) {
        printf("\n");
    }
    
    return true;
}

// ----------------------------------------------------------------
// HTTP Helper Functions
// ----------------------------------------------------------------

static bool saveResponseToFile(const char *filename, const char *data, int32_t dataLen, bool append)
{
    FILE *fp = fopen(filename, append ? "ab" : "wb");
    if (!fp) {
        printf("ERROR: Failed to open file '%s' for writing\n", filename);
        return false;
    }
    
    size_t written = fwrite(data, 1, dataLen, fp);
    fclose(fp);
    
    if (written != (size_t)dataLen) {
        printf("ERROR: Failed to write complete data to file\n");
        return false;
    }
    
    return true;
}

static int32_t readPostDataFromFile(const char *filename, char *buffer, int32_t maxLen)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        printf("ERROR: Failed to open file '%s' for reading\n", filename);
        return -1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize > maxLen) {
        printf("WARNING: File size (%ld bytes) exceeds buffer limit (%d bytes)\n", fileSize, maxLen);
        printf("         Only first %d bytes will be sent\n", maxLen);
        fileSize = maxLen;
    }
    
    size_t bytesRead = fread(buffer, 1, fileSize, fp);
    fclose(fp);
    
    return (int32_t)bytesRead;
}

// HTTP GET Request Example
static void httpGetExample(void)
{
    char host[256];
    char path[512];
    char filename[256];
    //char response[2048];
    int32_t sessionId = 0;
    int32_t err;
    
    printf("\n");
    printf("=== HTTP GET REQUEST ===\n");
    printf("\n");
    
    // Check Wi-Fi connectivity before proceeding
    if (!checkWiFiConnectivity(true, true)) {  // Include ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Enter hostname (e.g., httpbin.org): ");
    if (!fgets(host, sizeof(host), stdin)) {
        printf("ERROR: Failed to read hostname\n");
        return;
    }
    host[strcspn(host, "\r\n")] = 0;  // Remove newline
    
    printf("Enter path (e.g., /get): ");
    if (!fgets(path, sizeof(path), stdin)) {
        printf("ERROR: Failed to read path\n");
        return;
    }
    path[strcspn(path, "\r\n")] = 0;  // Remove newline
    
    printf("Save response to file (leave empty for display only): ");
    if (!fgets(filename, sizeof(filename), stdin)) {
        printf("ERROR: Failed to read filename\n");
        return;
    }
    filename[strcspn(filename, "\r\n")] = 0;  // Remove newline
    
    bool saveToFile = (strlen(filename) > 0);
    
    printf("\n");
    printf("Configuring HTTP connection...\n");
    
    // Step 1: Set connection parameters
    err = uCxHttpSetConnectionParams2(&gUcxHandle, sessionId, host);
    if (err < 0) {
        printf("ERROR: Failed to set connection parameters (error: %d)\n", err);
        return;
    }
    printf("✓ Connection configured for %s\n", host);
    
    // Step 2: Set request path
    err = uCxHttpSetRequestPath(&gUcxHandle, sessionId, path);
    if (err < 0) {
        printf("ERROR: Failed to set request path (error: %d)\n", err);
        return;
    }
    printf("✓ Request path set to %s\n", path);
    
    // Step 3: Send GET request
    printf("\n");
    printf("Sending GET request to http://%s%s...\n", host, path);
    err = uCxHttpGetRequest(&gUcxHandle, sessionId);
    if (err < 0) {
        printf("ERROR: GET request failed (error: %d)\n", err);
        return;
    }
    printf("✓ GET request sent successfully\n");
    
    // Step 4: Read response headers
    printf("\n");
    printf("Reading response headers...\n");
    uCxHttpGetHeader_t headerResp;
    if (uCxHttpGetHeader1Begin(&gUcxHandle, sessionId, &headerResp)) {
        printf("─────────────────────────────────────────────────\n");
        printf("%.*s", (int)headerResp.byte_array_data.length, headerResp.byte_array_data.pData);
        printf("─────────────────────────────────────────────────\n");
        uCxEnd(&gUcxHandle);
    } else {
        printf("WARNING: Failed to read response headers\n");
    }
    
    // Step 5: Read response body
    printf("\n");
    printf("Reading response body...\n");
    
    if (saveToFile) {
        // Remove existing file
        remove(filename);
    }
    
    int32_t totalBytes = 0;
    int32_t chunkSize = 1000;  // Max 1000 bytes per read
    int32_t moreToRead = 1;
    uint8_t buffer[1000];
    
    while (moreToRead) {
        int32_t bytesRead = uCxHttpGetBody(&gUcxHandle, sessionId, chunkSize, buffer, &moreToRead);
        
        if (bytesRead < 0) {
            printf("\nERROR: Failed to read response body (error: %d)\n", bytesRead);
            break;
        }
        
        if (bytesRead > 0) {
            totalBytes += bytesRead;
            
            if (saveToFile) {
                // Append to file
                FILE *fp = fopen(filename, totalBytes > bytesRead ? "ab" : "wb");
                if (fp) {
                    fwrite(buffer, 1, bytesRead, fp);
                    fclose(fp);
                } else {
                    printf("\nERROR: Failed to open file '%s'\n", filename);
                    break;
                }
            } else {
                // Display to console
                fwrite(buffer, 1, bytesRead, stdout);
            }
            
            if (moreToRead) {
                printf(".");  // Progress indicator
                fflush(stdout);
            }
        }
    }
    
    printf("\n");
    printf("✓ Response received: %d bytes total\n", totalBytes);
    
    if (saveToFile && totalBytes > 0) {
        printf("✓ Response saved to '%s'\n", filename);
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// HTTP POST Request Example
static void httpPostExample(void)
{
    char host[256];
    char path[512];
    char dataSource[256];
    char postData[4096];
    //char response[2048];
    int32_t sessionId = 0;
    int32_t err;
    int32_t dataLen = 0;
    
    printf("\n");
    printf("=== HTTP POST REQUEST ===\n");
    printf("\n");
    
    // Check Wi-Fi connectivity before proceeding
    if (!checkWiFiConnectivity(true, true)) {  // Include ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Enter hostname (e.g., httpbin.org): ");
    if (!fgets(host, sizeof(host), stdin)) {
        printf("ERROR: Failed to read hostname\n");
        return;
    }
    host[strcspn(host, "\r\n")] = 0;  // Remove newline
    
    printf("Enter path (e.g., /post): ");
    if (!fgets(path, sizeof(path), stdin)) {
        printf("ERROR: Failed to read path\n");
        return;
    }
    path[strcspn(path, "\r\n")] = 0;  // Remove newline
    
    printf("\n");
    printf("Data source:\n");
    printf("  [1] Enter text manually\n");
    printf("  [2] Read from file\n");
    printf("Choice: ");
    if (!fgets(dataSource, sizeof(dataSource), stdin)) {
        printf("ERROR: Failed to read choice\n");
        return;
    }
    
    if (dataSource[0] == '1') {
        // Manual text entry
        printf("\n");
        printf("Enter POST data (single line): ");
        if (!fgets(postData, sizeof(postData), stdin)) {
            printf("ERROR: Failed to read POST data\n");
            return;
        }
        postData[strcspn(postData, "\r\n")] = 0;  // Remove newline
        dataLen = (int32_t)strlen(postData);
    } else if (dataSource[0] == '2') {
        // Read from file
        char filename[256];
        printf("\n");
        printf("Enter filename to read: ");
        if (!fgets(filename, sizeof(filename), stdin)) {
            printf("ERROR: Failed to read filename\n");
            return;
        }
        filename[strcspn(filename, "\r\n")] = 0;  // Remove newline
        
        dataLen = readPostDataFromFile(filename, postData, sizeof(postData));
        if (dataLen < 0) {
            return;
        }
        printf("✓ Read %d bytes from '%s'\n", dataLen, filename);
    } else {
        printf("ERROR: Invalid choice\n");
        return;
    }
    
    printf("\n");
    printf("Configuring HTTP connection...\n");
    
    // Step 1: Set connection parameters
    err = uCxHttpSetConnectionParams2(&gUcxHandle, sessionId, host);
    if (err < 0) {
        printf("ERROR: Failed to set connection parameters (error: %d)\n", err);
        return;
    }
    printf("✓ Connection configured for %s\n", host);
    
    // Step 2: Set request path
    err = uCxHttpSetRequestPath(&gUcxHandle, sessionId, path);
    if (err < 0) {
        printf("ERROR: Failed to set request path (error: %d)\n", err);
        return;
    }
    printf("✓ Request path set to %s\n", path);
    
    // Step 3: Send POST request with data
    printf("\n");
    printf("Sending POST request to http://%s%s...\n", host, path);
    printf("POST data: %d bytes\n", dataLen);
    
    err = uCxHttpPostRequest(&gUcxHandle, sessionId, (const uint8_t *)postData, dataLen);
    if (err < 0) {
        printf("ERROR: POST request failed (error: %d)\n", err);
        return;
    }
    printf("✓ POST request sent successfully\n");
    
    // Step 4: Read response headers
    printf("\n");
    printf("Reading response headers...\n");
    uCxHttpGetHeader_t headerResp;
    if (uCxHttpGetHeader1Begin(&gUcxHandle, sessionId, &headerResp)) {
        printf("─────────────────────────────────────────────────\n");
        printf("%.*s", (int)headerResp.byte_array_data.length, headerResp.byte_array_data.pData);
        printf("─────────────────────────────────────────────────\n");
        uCxEnd(&gUcxHandle);
    } else {
        printf("WARNING: Failed to read response headers\n");
    }
    
    // Step 5: Read response body
    printf("\n");
    printf("Reading response body...\n");
    
    int32_t totalBytes = 0;
    int32_t chunkSize = 2048;
    int32_t moreToRead = 1;
    uint8_t buffer[2048];
    
    printf("─────────────────────────────────────────────────\n");
    while (moreToRead) {
        int32_t bytesRead = uCxHttpGetBody(&gUcxHandle, sessionId, chunkSize, buffer, &moreToRead);
        if (bytesRead < 0) {
            printf("\nERROR: Failed to read response body (error: %d)\n", bytesRead);
            break;
        }
        if (bytesRead > 0) {
            totalBytes += bytesRead;
            fwrite(buffer, 1, bytesRead, stdout);
        }
    }
    printf("\n─────────────────────────────────────────────────\n");
    printf("✓ Response received: %d bytes total\n", totalBytes);
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void ipGeolocationExample(void)
{
    int32_t sessionId = 0;
    int32_t err;
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("IP GEOLOCATION\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    // Check Wi-Fi connectivity before proceeding
    if (!checkWiFiConnectivity(true, true)) {  // Include ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Getting IP geolocation information...\n");
    printf("\n");
    
    // API: https://ip-api.com/docs/api:json
    // Endpoint: http://ip-api.com/json/
    // Returns: JSON with country, city, ISP, lat/lon, etc.
    
    const char *host = "ip-api.com";
    const char *path = "/json/";
    
    printf("Configuring HTTP connection...\n");
    
    // Step 1: Set connection parameters
    err = uCxHttpSetConnectionParams2(&gUcxHandle, sessionId, host);
    if (err < 0) {
        printf("ERROR: Failed to set connection parameters (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Connection configured for %s\n", host);
    
    // Step 2: Set request path
    err = uCxHttpSetRequestPath(&gUcxHandle, sessionId, path);
    if (err < 0) {
        printf("ERROR: Failed to set request path (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Request path set to %s\n", path);
    
    // Step 3: Send GET request
    printf("\n");
    printf("Sending GET request to http://%s%s...\n", host, path);
    err = uCxHttpGetRequest(&gUcxHandle, sessionId);
    if (err < 0) {
        printf("ERROR: GET request failed (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ GET request sent successfully\n");
    
    // Step 4: Read response headers
    printf("\n");
    printf("Reading response headers...\n");
    printf("[DEBUG] Before uCxHttpGetHeader1Begin\n");
    fflush(stdout);
    
    uCxHttpGetHeader_t headerResp;
    memset(&headerResp, 0, sizeof(headerResp));
    
    if (uCxHttpGetHeader1Begin(&gUcxHandle, sessionId, &headerResp)) {
        printf("[DEBUG] Header read successful, length=%d\n", (int)headerResp.byte_array_data.length);
        fflush(stdout);
        
        printf("─────────────────────────────────────────────────\n");
        if (headerResp.byte_array_data.length > 0 && headerResp.byte_array_data.pData != NULL) {
            printf("%.*s", (int)headerResp.byte_array_data.length, headerResp.byte_array_data.pData);
        }
        printf("─────────────────────────────────────────────────\n");
        
        printf("[DEBUG] Before uCxEnd\n");
        fflush(stdout);
        uCxEnd(&gUcxHandle);
        printf("[DEBUG] After uCxEnd\n");
        fflush(stdout);
    } else {
        printf("WARNING: Failed to read response headers\n");
    }
    
    printf("[DEBUG] After header reading section\n");
    fflush(stdout);
    
    // Step 5: Read response body (JSON data)
    printf("\n");
    printf("Reading response body...\n");
    
    // Expected JSON response format from ip-api.com:
    // {
    //   "status": "success",
    //   "country": "United States",
    //   "countryCode": "US",
    //   "region": "CA",
    //   "regionName": "California",
    //   "city": "Mountain View",
    //   "zip": "94043",
    //   "lat": 37.4192,
    //   "lon": -122.0574,
    //   "timezone": "America/Los_Angeles",
    //   "isp": "Google LLC",
    //   "org": "Google Cloud",
    //   "as": "AS15169 Google LLC",
    //   "query": "35.192.x.x"
    // }
    
    int32_t totalBytes = 0;
    int32_t chunkSize = 1000;  // Max 1000 bytes per read
    int32_t moreToRead = 1;
    uint8_t buffer[1000];
    
    printf("─────────────────────────────────────────────────\n");
    while (moreToRead) {
        int32_t bytesRead = uCxHttpGetBody(&gUcxHandle, sessionId, chunkSize, buffer, &moreToRead);
        if (bytesRead < 0) {
            printf("\nERROR: Failed to read response body (error: %d)\n", bytesRead);
            break;
        }
        if (bytesRead > 0) {
            totalBytes += bytesRead;
            fwrite(buffer, 1, bytesRead, stdout);
        }
    }
    printf("\n─────────────────────────────────────────────────\n");
    if (totalBytes > 0) {
        printf("✓ Response received: %d bytes total\n", totalBytes);
    }
    printf("\n");
    
    printf("Expected JSON Response Fields:\n");
    printf("  - status:      Success/fail indicator\n");
    printf("  - country:     Country name\n");
    printf("  - countryCode: Two-letter country code (e.g., US, SE, DE)\n");
    printf("  - region:      Region/state code\n");
    printf("  - regionName:  Region/state full name\n");
    printf("  - city:        City name\n");
    printf("  - zip:         Zip/postal code\n");
    printf("  - lat:         Latitude coordinate\n");
    printf("  - lon:         Longitude coordinate\n");
    printf("  - timezone:    Timezone identifier (e.g., America/Los_Angeles)\n");
    printf("  - isp:         Internet Service Provider\n");
    printf("  - org:         Organization name\n");
    printf("  - as:          Autonomous system info\n");
    printf("  - query:       IP address used for lookup\n");
    printf("\n");
    printf("API Documentation: https://ip-api.com/docs/api:json\n");
    printf("\n");
    printf("Other available API formats:\n");
    printf("  - JSON:    http://ip-api.com/json/\n");
    printf("  - XML:     http://ip-api.com/xml/\n");
    printf("  - CSV:     http://ip-api.com/csv/\n");
    printf("  - Newline: http://ip-api.com/line/\n");
    printf("  - PHP:     http://ip-api.com/php/\n");
    printf("\n");
    printf("Note: Free tier is limited to 45 requests per minute\n");
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void externalIpDetectionExample(void)
{
    int32_t sessionId = 0;
    int32_t err;
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("EXTERNAL IP DETECTION\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    // Check Wi-Fi connectivity before proceeding
    if (!checkWiFiConnectivity(true, true)) {  // Include ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Detecting external IP address...\n");
    printf("\n");
    
    // API: https://www.ipify.org/
    // Endpoints:
    //   - Plain text: https://api.ipify.org (or http://api.ipify.org)
    //   - JSON:       https://api.ipify.org?format=json
    //   - JSONP:      https://api.ipify.org?format=jsonp
    //
    // This is a simple, free API that returns your public IP address
    // No rate limiting, no authentication required
    
    const char *host = "api.ipify.org";
    const char *path = "/?format=json";
    
    printf("Configuring HTTP connection...\n");
    
    // Step 1: Set connection parameters
    err = uCxHttpSetConnectionParams2(&gUcxHandle, sessionId, host);
    if (err < 0) {
        printf("ERROR: Failed to set connection parameters (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Connection configured for %s\n", host);
    
    // Step 2: Set request path
    err = uCxHttpSetRequestPath(&gUcxHandle, sessionId, path);
    if (err < 0) {
        printf("ERROR: Failed to set request path (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Request path set to %s\n", path);
    
    // Step 3: Send GET request
    printf("\n");
    printf("Sending GET request to http://%s%s...\n", host, path);
    err = uCxHttpGetRequest(&gUcxHandle, sessionId);
    if (err < 0) {
        printf("ERROR: GET request failed (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ GET request sent successfully\n");
    
    // Step 4: Read response headers
    printf("\n");
    printf("Reading response headers...\n");
    
    uCxHttpGetHeader_t headerResp;
    int32_t contentLength = -1;
    char headerBuffer[2048];
    int32_t headerBufferLen = 0;
    
    // Loop to read all header chunks
    while (true) {
        memset(&headerResp, 0, sizeof(headerResp));
        
        if (uCxHttpGetHeader1Begin(&gUcxHandle, sessionId, &headerResp)) {
            // Collect header data for parsing
            if (headerResp.byte_array_data.length > 0 && 
                headerResp.byte_array_data.pData != NULL &&
                headerBufferLen + headerResp.byte_array_data.length < sizeof(headerBuffer)) {
                memcpy(headerBuffer + headerBufferLen, 
                       headerResp.byte_array_data.pData, 
                       headerResp.byte_array_data.length);
                headerBufferLen += headerResp.byte_array_data.length;
            }
            
            uCxEnd(&gUcxHandle);
            
            // Continue reading if there's more
            if (headerResp.more_to_read == 0) {
                break;  // All headers received
            }
        } else {
            printf("WARNING: Failed to read response headers\n");
            break;
        }
    }
    
    // Parse Content-Length from headers
    if (headerBufferLen > 0) {
        headerBuffer[headerBufferLen] = '\0';  // Null terminate
        char *contentLengthPos = strstr(headerBuffer, "Content-Length: ");
        if (contentLengthPos) {
            contentLength = atoi(contentLengthPos + 16);
            printf("Expected response size: %d bytes\n", contentLength);
        }
    }
    
    // Step 5: Read response body (JSON data)
    printf("\n");
    printf("Reading response body...\n");
    
    // Expected JSON response format from ipify.org:
    // {"ip":"123.45.67.89"}
    //
    // This is a minimal response containing just your external IP address
    
    int32_t totalBytes = 0;
    int32_t chunkSize = 1000;  // Max 1000 bytes per read
    int32_t moreToRead = 1;
    uint8_t buffer[1000];
    
    printf("─────────────────────────────────────────────────\n");
    
    while (moreToRead) {
        int32_t bytesRead = uCxHttpGetBody(&gUcxHandle, sessionId, chunkSize, buffer, &moreToRead);
        
        if (bytesRead < 0) {
            printf("\nERROR: Failed to read response body (error: %d)\n", bytesRead);
            break;
        }
        
        if (bytesRead > 0) {
            totalBytes += bytesRead;
            // Print buffer contents safely
            fwrite(buffer, 1, bytesRead, stdout);
        }
    }
    printf("\n─────────────────────────────────────────────────\n");
    if (totalBytes > 0) {
        printf("✓ Response received: %d bytes total\n", totalBytes);
    }
    printf("\n");
    
    printf("Expected JSON Response Format:\n");
    printf("  {\"ip\":\"123.45.67.89\"}\n");
    printf("\n");
    printf("The 'ip' field contains your public/external IP address\n");
    printf("as seen from the internet.\n");
    printf("\n");
    printf("API Documentation: https://www.ipify.org/\n");
    printf("\n");
    printf("Available formats:\n");
    printf("  - JSON (recommended): http://api.ipify.org?format=json\n");
    printf("  - Plain text:         http://api.ipify.org\n");
    printf("  - JSONP:              http://api.ipify.org?format=jsonp\n");
    printf("\n");
    printf("Note: Free API with no rate limits or authentication required\n");
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// Generate and display Google Maps QR code for a location
static void generateLocationQRCode(double lat, double lng)
{
    char url[256];
    snprintf(url, sizeof(url), "https://www.google.com/maps?q=%.7f,%.7f", lat, lng);
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("Google Maps Link:\n");
    printf("  %s\n", url);
    printf("─────────────────────────────────────────────────\n");
    
    // Generate QR code
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t temp[qrcodegen_BUFFER_LEN_MAX];
    
    bool ok = qrcodegen_encodeText(
        url, temp, qrcode,
        qrcodegen_Ecc_MEDIUM,
        qrcodegen_VERSION_MIN,
        qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO, true
    );
    
    if (!ok) {
        printf("ERROR: Failed to generate QR code\n");
        return;
    }
    
    printf("\n");
    printf("Scan this QR code to open location in Google Maps:\n");
    printf("\n");
    
    int size = qrcodegen_getSize(qrcode);
    int border = 2;
    
    for (int y = -border; y < size + border; y++) {
        printf("  ");  // Left margin
        for (int x = -border; x < size + border; x++) {
            // Use block characters for better visibility
            printf(qrcodegen_getModule(qrcode, x, y) ? "██" : "  ");
        }
        printf("\n");
    }
    
    printf("\n");
    printf("Tip: Open your phone's camera app and point it at the QR code\n");
    printf("     to quickly navigate to this location in Google Maps.\n");
}

static void wifiPositioningExample(void)
{
    int32_t sessionId = 0;
    int32_t err;
    char postData[4096];
    int dataLen = 0;
    
    printf("\n─────────────────────────────────────────────────────────────\n");
    printf("WI-FI POSITIONING (Combain)\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    
    // Check Wi-Fi connectivity before proceeding
    if (!checkWiFiConnectivity(true, true)) {  // Include ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Performing Wi-Fi scan to collect access point data...\n");
    printf("\n");
    
    // Step 1: Perform Wi-Fi scan
    printf("Initiating Wi-Fi scan...\n");
    uCxWifiStationScanDefaultBegin(&gUcxHandle);
    
    // Collect scan results
    uCxWifiStationScanDefault_t scanResult;
    int apCount = 0;
    char macStr[18];
    
    // Build JSON payload for Combain API
    strcpy(postData, "{\n    \"wifiAccessPoints\": [");
    dataLen = (int)strlen(postData);
    
    printf("Collecting access point data...\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    while (uCxWifiStationScanDefaultGetNext(&gUcxHandle, &scanResult)) {
        // Convert MAC address to string format (XX:XX:XX:XX:XX:XX)
        snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
                 scanResult.bssid.address[0], scanResult.bssid.address[1],
                 scanResult.bssid.address[2], scanResult.bssid.address[3],
                 scanResult.bssid.address[4], scanResult.bssid.address[5]);
        
        printf("  AP %2d: %-32s  MAC: %s  RSSI: %3d dBm  CH: %2d\n",
               apCount + 1, scanResult.ssid, macStr, scanResult.rssi, scanResult.channel);
        
        // Add to JSON array (add comma if not first entry)
        if (apCount > 0) {
            dataLen += snprintf(postData + dataLen, sizeof(postData) - dataLen, ",");
        }
        
        // Add access point entry to JSON
        dataLen += snprintf(postData + dataLen, sizeof(postData) - dataLen,
                           "\n        {\n"
                           "            \"macAddress\": \"%s\",\n"
                           "            \"ssid\": \"%s\",\n"
                           "            \"signalStrength\": %d\n"
                           "        }",
                           macStr, scanResult.ssid, scanResult.rssi);
        
        apCount++;
        
        // Prevent buffer overflow
        if (dataLen >= sizeof(postData) - 200) {
            printf("WARNING: Buffer limit reached, truncating AP list\n");
            break;
        }
    }
    
    uCxEnd(&gUcxHandle);
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("✓ Scan complete: %d access points found\n", apCount);
    printf("\n");
    
    if (apCount == 0) {
        printf("ERROR: No access points found. Cannot determine position.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Complete JSON payload
    dataLen += snprintf(postData + dataLen, sizeof(postData) - dataLen,
                       "\n    ],\n"
                       "    \"indoor\": 1\n"
                       "}");
    
    printf("JSON Payload (%d bytes):\n", dataLen);
    printf("─────────────────────────────────────────────────\n");
    printf("%s\n", postData);
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    // Step 2: Check for Combain API key
    // API: https://portal.combain.com/api/#combain-location-api
    // Endpoint: https://apiv2.combain.com?key=YOUR_API_KEY&id=DEVICE_ID
    
    // Check if we have a saved API key
    if (strlen(gCombainApiKey) == 0) {
        printf("─────────────────────────────────────────────────────────────\n");
        printf("COMBAIN API KEY REQUIRED\n");
        printf("─────────────────────────────────────────────────────────────\n");
        printf("\n");
        printf("This example requires a Combain API key to access the\n");
        printf("Wi-Fi positioning service.\n");
        printf("\n");
        printf("Get your free API key at: https://portal.combain.com\n");
        printf("\n");
        printf("Enter your Combain API key (or press Enter to cancel): ");
        
        char apiKeyInput[128];
        if (fgets(apiKeyInput, sizeof(apiKeyInput), stdin) != NULL) {
            // Remove trailing newline
            apiKeyInput[strcspn(apiKeyInput, "\r\n")] = '\0';
            
            if (strlen(apiKeyInput) > 0) {
                strncpy(gCombainApiKey, apiKeyInput, sizeof(gCombainApiKey) - 1);
                gCombainApiKey[sizeof(gCombainApiKey) - 1] = '\0';
                
                // Save to settings file
                saveSettings();
                printf("✓ API key saved to settings (obfuscated)\n");
                printf("\n");
            } else {
                printf("Cancelled. No API key provided.\n");
                printf("\n");
                printf("Press Enter to continue...");
                getchar();
                return;
            }
        } else {
            printf("ERROR: Failed to read API key\n");
            printf("\n");
            printf("Press Enter to continue...");
            getchar();
            return;
        }
    } else {
        printf("✓ Using saved Combain API key from settings\n");
        printf("\n");
    }
    
    // Prepare POST request to Combain API
    const char *host = "apiv2.combain.com";
    char path[256];
    snprintf(path, sizeof(path), "/?key=%s&id=ucxclient_win64", gCombainApiKey);
    
    printf("Configuring HTTP connection to Combain...\n");
    
    // Set connection parameters
    err = uCxHttpSetConnectionParams2(&gUcxHandle, sessionId, host);
    if (err < 0) {
        printf("ERROR: Failed to set connection parameters (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Connection configured for %s\n", host);
    
    // Set request path
    err = uCxHttpSetRequestPath(&gUcxHandle, sessionId, path);
    if (err < 0) {
        printf("ERROR: Failed to set request path (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ Request path set to %s\n", path);
    
    // Note: Header setting functions not yet available
    // err = uCxHttpSetRequestHeader2(&gUcxHandle, sessionId, "Content-Type: application/json");
    // if (err < 0) {
    //     printf("ERROR: Failed to set Content-Type header (error: %d)\n", err);
    //     printf("\n");
    //     printf("Press Enter to continue...");
    //     getchar();
    //     return;
    // }
    // printf("✓ Content-Type header set\n");
    
    // Send POST request with JSON data
    printf("\n");
    printf("Sending POST request to https://%s%s...\n", host, path);
    printf("POST data: %d bytes\n", dataLen);
    
    err = uCxHttpPostRequest(&gUcxHandle, sessionId, (const uint8_t *)postData, dataLen);
    if (err < 0) {
        printf("ERROR: POST request failed (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ POST request sent successfully\n");
    printf("  Data sent: %d bytes\n", dataLen);
    
    // Read response headers
    printf("\n");
    printf("Reading response headers...\n");
    uCxHttpGetHeader_t headerResp;
    if (uCxHttpGetHeader1Begin(&gUcxHandle, sessionId, &headerResp)) {
        printf("─────────────────────────────────────────────────\n");
        printf("%.*s", (int)headerResp.byte_array_data.length, headerResp.byte_array_data.pData);
        printf("─────────────────────────────────────────────────\n");
        uCxEnd(&gUcxHandle);
    } else {
        printf("WARNING: Failed to read response headers\n");
    }
    
    // Read response body (JSON location data)
    printf("\n");
    printf("Reading response body...\n");
    
    // Expected JSON response format from Combain:
    // {
    //     "location": {
    //         "lat": 55.7174228,
    //         "lng": 13.2137795
    //     },
    //     "accuracy": 2,
    //     "state": "...",
    //     "indoor": {
    //         "buildingModelId": 135432394,
    //         "buildingId": 172119,
    //         "building": "Combain Office",
    //         "floorIndex": 13,
    //         "floorLabel": "14",
    //         "roomIndex": 1000000000017996,
    //         "room": "Le Kitchen"
    //     },
    //     "logId": 300000528485206
    // }
    
    int32_t totalBytes = 0;
    int32_t chunkSize = 1000;  // Max 1000 bytes per read
    int32_t moreToRead = 1;
    uint8_t buffer[1000];
    
    printf("Position Data:\n");
    printf("─────────────────────────────────────────────────\n");
    while (moreToRead) {
        int32_t bytesRead = uCxHttpGetBody(&gUcxHandle, sessionId, chunkSize, buffer, &moreToRead);
        if (bytesRead < 0) {
            printf("\nERROR: Failed to read response body (error: %d)\n", bytesRead);
            break;
        }
        if (bytesRead > 0) {
            totalBytes += bytesRead;
            fwrite(buffer, 1, bytesRead, stdout);
        }
    }
    printf("\n─────────────────────────────────────────────────\n");
    printf("✓ Response received: %d bytes total\n", totalBytes);
    
    // TODO: Parse JSON response to extract latitude and longitude
    // Simple parsing example (in production, use a JSON library)
    // Example response: {"location":{"lat":55.7174228,"lng":13.2137795},"accuracy":2}
    // 
    // double lat = 0.0, lng = 0.0;
    // int accuracy = 0;
    // 
    // // Parse JSON properly when needed:
    // // sscanf(jsonBuffer, "...parse lat/lng...", &lat, &lng);
    // 
    // if (lat != 0.0 && lng != 0.0) {
    //     printf("\n");
    //     printf("─────────────────────────────────────────────────\n");
    //     printf("POSITION DETERMINED\n");
    //     printf("─────────────────────────────────────────────────\n");
    //     printf("  Latitude:  %.7f\n", lat);
    //     printf("  Longitude: %.7f\n", lng);
    //     printf("  Accuracy:  %d meters\n", accuracy);
    //     
    //     // Generate Google Maps link and QR code
    //     generateLocationQRCode(lat, lng);
    // }
    printf("\n");
    
    printf("Expected JSON Response Fields:\n");
    printf("  location:\n");
    printf("    - lat:              Latitude coordinate\n");
    printf("    - lng:              Longitude coordinate\n");
    printf("  - accuracy:           Accuracy in meters\n");
    printf("  - state:              State data for subsequent requests\n");
    printf("  indoor (if available):\n");
    printf("    - buildingModelId:  Building model identifier\n");
    printf("    - buildingId:       Building identifier\n");
    printf("    - building:         Building name\n");
    printf("    - floorIndex:       Floor index (0-based)\n");
    printf("    - floorLabel:       Floor label (e.g., \"14\")\n");
    printf("    - roomIndex:        Room identifier\n");
    printf("    - room:             Room name\n");
    printf("  - logId:              Log entry identifier\n");
    printf("\n");
    
    // Show example of what the output will look like when implemented
    printf("Example output when response is received:\n");
    printf("─────────────────────────────────────────────────\n");
    printf("POSITION DETERMINED\n");
    printf("─────────────────────────────────────────────────\n");
    printf("  Latitude:  55.7174228\n");
    printf("  Longitude: 13.2137795\n");
    printf("  Accuracy:  2 meters\n");
    printf("\n");
    printf("  Google Maps link and QR code will be displayed here.\n");
    printf("  You can scan the QR code with your phone to navigate.\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    printf("API Documentation: https://portal.combain.com/api/#combain-location-api\n");
    printf("\n");
    printf("Note: Accuracy depends on Wi-Fi AP database coverage in your area.\n");
    printf("      Send as many APs as possible for best results.\n");
    printf("      Indoor positioning requires building data in Combain's database.\n");
    printf("      Once HTTP body reading is implemented, the QR code will be\n");
    printf("      automatically generated and displayed above.\n");
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// ----------------------------------------------------------------
// Network Diagnostics Functions
// ----------------------------------------------------------------

static void pingExample(void)
{
    char input[256];
    char hostname[128];
    int count = 4;
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("PING TEST (ICMP Echo Request)\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    // Check Wi-Fi connectivity
    if (!checkWiFiConnectivity(false, false)) {
        return;
    }
    
    // Get destination hostname or IP
    printf("Enter destination (hostname or IP address): ");
    if (!fgets(input, sizeof(input), stdin)) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    input[strcspn(input, "\n")] = 0;
    
    if (strlen(input) == 0) {
        strncpy(hostname, "8.8.8.8", sizeof(hostname));
        printf("Using default: %s (Google DNS)\n", hostname);
    } else {
        strncpy(hostname, input, sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }
    
    // Get ping count
    printf("Enter number of pings (1-10, 0 for continuous, default=4): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            count = atoi(input);
            if (count < 0 || count > 10) {
                printf("Invalid count, using default (4)\n");
                count = 4;
            }
        }
    }
    
    printf("\n");
    printf("Pinging %s with %d requests", hostname, count);
    if (count == 0) {
        printf(" (continuous - this feature may not be supported by all modules)");
    }
    printf("...\n");
    printf("\n");
    
    // Reset ping statistics
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    gPingCount = 0;
    memset((void*)gPingTimes, 0, sizeof(gPingTimes));
    
    // Start ping (using uCxDiagnosticsPing2 for custom count)
    int32_t result = uCxDiagnosticsPing2(&gUcxHandle, hostname, count);
    
    if (result != 0) {
        printf("ERROR: Failed to start ping (error code: %d)\n", result);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Wait for ping complete event (timeout after count * 2 seconds + 5)
    int32_t timeoutMs = (count > 0 ? count * 2000 : 10000) + 5000;
    
    printf("Waiting for ping responses");
    if (count == 0) {
        printf(" (press Ctrl+C to stop)");
    }
    printf("...\n");
    printf("\n");
    
    if (waitEvent(URC_FLAG_PING_COMPLETE, timeoutMs / 1000)) {
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("PING RESULTS\n");
        printf("─────────────────────────────────────────────────\n");
        printf("  Destination:  %s\n", hostname);
        printf("  Sent:         %d packets\n", gPingSuccess + gPingFailed);
        printf("  Received:     %d packets\n", gPingSuccess);
        printf("  Lost:         %d packets", gPingFailed);
        if (gPingSuccess + gPingFailed > 0) {
            printf(" (%.1f%% loss)", (float)gPingFailed / (gPingSuccess + gPingFailed) * 100.0f);
        }
        printf("\n");
        
        if (gPingSuccess > 0) {
            printf("  Average RTT:  %d ms\n", gPingAvgTime);
            
            // Show individual ping times
            if (gPingCount > 0) {
                printf("\n");
                printf("Individual Response Times:\n");
                for (int i = 0; i < gPingCount && i < MAX_PING_TIMES; i++) {
                    printf("  Ping #%d: %d ms\n", i + 1, gPingTimes[i]);
                }
            }
        }
        
        printf("─────────────────────────────────────────────────\n");
    } else {
        printf("\n");
        printf("WARNING: Ping timeout - no response received\n");
        printf("─────────────────────────────────────────────────\n");
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void iperfClientExample(void)
{
    char input[256];
    char serverIp[64];
    int port = 5001;
    int protocol = 1;  // TCP
    int duration = 10;
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("iPERF2 CLIENT (Network Throughput Test)\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    if (gIperfRunning) {
        printf("ERROR: An iPerf test is already running!\n");
        printf("Use option [4] to stop it first.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Check Wi-Fi connectivity
    if (!checkWiFiConnectivity(false, false)) {
        return;
    }
    
    // Get server IP
    printf("Enter iPerf server IP address: ");
    if (!fgets(input, sizeof(input), stdin)) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    input[strcspn(input, "\n")] = 0;
    
    if (strlen(input) == 0) {
        printf("ERROR: Server IP is required\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    strncpy(serverIp, input, sizeof(serverIp) - 1);
    serverIp[sizeof(serverIp) - 1] = '\0';
    
    // Get port
    printf("Enter port (default=5001): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            port = atoi(input);
            if (port <= 0 || port > 65535) {
                printf("Invalid port, using default (5001)\n");
                port = 5001;
            }
        }
    }
    
    // Get protocol
    printf("Select protocol:\n");
    printf("  [1] TCP (reliable, default)\n");
    printf("  [2] UDP (faster, supports bandwidth limit)\n");
    printf("Enter choice (default=1): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            protocol = atoi(input);
            if (protocol != 1 && protocol != 2) {
                printf("Invalid protocol, using TCP\n");
                protocol = 1;
            }
        }
    }
    
    // Get duration
    printf("Enter test duration in seconds (default=10): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            duration = atoi(input);
            if (duration <= 0 || duration > 300) {
                printf("Invalid duration, using default (10)\n");
                duration = 10;
            }
        }
    }
    
    printf("\n");
    printf("Starting iPerf2 client test...\n");
    printf("  Server:    %s:%d\n", serverIp, port);
    printf("  Protocol:  %s\n", protocol == 1 ? "TCP" : "UDP");
    printf("  Duration:  %d seconds\n", duration);
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    // Convert IP string to uSockIpAddress_t structure
    uSockIpAddress_t ipAddr;
    if (uCxStringToIpAddress(serverIp, &ipAddr) != 0) {
        printf("ERROR: Invalid IP address format: %s\n", serverIp);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Clear output buffer
    gIperfOutputBuffer[0] = '\0';
    gIperfRunning = true;
    
    // Start iPerf test
    // Using uCxDiagnosticsIperf7: action, protocol, role, port, report_interval, time_boundary, ip_addr
    int32_t result = uCxDiagnosticsIperf7(&gUcxHandle, U_DIAG_IPERF_ACTION_START, 
                                          protocol, U_DIAG_ROLE_CLIENT, 
                                          port, 1, duration, &ipAddr);
    
    if (result != 0) {
        printf("ERROR: Failed to start iPerf test (error code: %d)\n", result);
        gIperfRunning = false;
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("iPerf test started. Output will appear below:\n");
    printf("(Test will run for %d seconds)\n", duration);
    printf("─────────────────────────────────────────────────\n");
    
    // Wait for test to complete (duration + 10 second buffer)
    Sleep((duration + 10) * 1000);
    
    if (gIperfRunning) {
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("Test may still be running. Use [4] to stop if needed.\n");
        printf("─────────────────────────────────────────────────\n");
    } else {
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("Test completed.\n");
        printf("─────────────────────────────────────────────────\n");
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void iperfServerExample(void)
{
    char input[256];
    int port = 5001;
    int protocol = 1;  // TCP
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("iPERF2 SERVER (Listen for Throughput Tests)\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    if (gIperfRunning) {
        printf("ERROR: An iPerf test is already running!\n");
        printf("Use option [4] to stop it first.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Check Wi-Fi connectivity
    if (!checkWiFiConnectivity(false, false)) {
        return;
    }
    
    // Get port
    printf("Enter listen port (default=5001): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            port = atoi(input);
            if (port <= 0 || port > 65535) {
                printf("Invalid port, using default (5001)\n");
                port = 5001;
            }
        }
    }
    
    // Get protocol
    printf("Select protocol:\n");
    printf("  [1] TCP (reliable, default)\n");
    printf("  [2] UDP (faster)\n");
    printf("Enter choice (default=1): ");
    if (fgets(input, sizeof(input), stdin)) {
        input[strcspn(input, "\n")] = 0;
        if (strlen(input) > 0) {
            protocol = atoi(input);
            if (protocol != 1 && protocol != 2) {
                printf("Invalid protocol, using TCP\n");
                protocol = 1;
            }
        }
    }
    
    printf("\n");
    printf("Starting iPerf2 server...\n");
    printf("  Port:      %d\n", port);
    printf("  Protocol:  %s\n", protocol == 1 ? "TCP" : "UDP");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    // Clear output buffer
    gIperfOutputBuffer[0] = '\0';
    gIperfRunning = true;
    
    // Start iPerf server
    // Using uCxDiagnosticsIperf5: action, protocol, role, port, report_interval
    int32_t result = uCxDiagnosticsIperf5(&gUcxHandle, U_DIAG_IPERF_ACTION_START, 
                                          protocol, U_DIAG_ROLE_SERVER, 
                                          port, 1);
    
    if (result != 0) {
        printf("ERROR: Failed to start iPerf server (error code: %d)\n", result);
        gIperfRunning = false;
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("iPerf server is now listening on port %d\n", port);
    printf("Use option [4] to stop the server when done.\n");
    printf("\n");
    printf("You can now run an iPerf client from another device:\n");
    printf("  Example: iperf -c <module-ip> -p %d\n", port);
    printf("─────────────────────────────────────────────────\n");
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void iperfStopExample(void)
{
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("STOP iPERF TEST\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    if (!gIperfRunning) {
        printf("No iPerf test is currently running.\n");
        printf("─────────────────────────────────────────────────\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("Stopping iPerf test...\n");
    
    // Stop iPerf using STOP action
    int32_t result = uCxDiagnosticsIperf2(&gUcxHandle, U_DIAG_IPERF_ACTION_STOP, U_DIAG_PROTOCOL_TYPE_TCP);
    
    if (result == 0) {
        gIperfRunning = false;
        printf("iPerf test stopped successfully.\n");
    } else {
        printf("WARNING: Stop command sent, but may have failed (error: %d)\n", result);
        gIperfRunning = false;  // Reset flag anyway
    }
    
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void dnsLookupExample(void)
{
    char input[256];
    char hostname[128];
    uSockIpAddress_t ipAddress;
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("DNS LOOKUP (Resolve Hostname to IP Address)\n");
    printf("─────────────────────────────────────────────────\n");
    printf("\n");
    
    // Check Wi-Fi connectivity
    if (!checkWiFiConnectivity(false, false)) {
        return;
    }
    
    // Get hostname
    printf("Enter hostname to lookup (e.g., google.com): ");
    if (!fgets(input, sizeof(input), stdin)) {
        printf("ERROR: Failed to read input\n");
        return;
    }
    input[strcspn(input, "\n")] = 0;
    
    if (strlen(input) == 0) {
        strncpy(hostname, "google.com", sizeof(hostname));
        printf("Using default: %s\n", hostname);
    } else {
        strncpy(hostname, input, sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }
    
    printf("\n");
    printf("Looking up %s...\n", hostname);
    
    // Perform DNS lookup using socket API
    int32_t result = uCxSocketGetHostByName(&gUcxHandle, hostname, &ipAddress);
    
    if (result == 0) {
        // Convert IP address to string for display
        char ipStr[64];
        uCxIpAddressToString(&ipAddress, ipStr, sizeof(ipStr));
        
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("DNS LOOKUP SUCCESSFUL\n");
        printf("─────────────────────────────────────────────────\n");
        printf("  Hostname:     %s\n", hostname);
        printf("  IP Address:   %s\n", ipStr);
        printf("  Address Type: %s\n", ipAddress.type == U_SOCK_ADDRESS_TYPE_V4 ? "IPv4" : "IPv6");
        printf("─────────────────────────────────────────────────\n");
    } else {
        printf("\n");
        printf("─────────────────────────────────────────────────\n");
        printf("DNS LOOKUP FAILED\n");
        printf("─────────────────────────────────────────────────\n");
        printf("  Hostname:  %s\n", hostname);
        printf("  Error:     Could not resolve (error code: %d)\n", result);
        printf("\n");
        printf("Possible causes:\n");
        printf("  - Hostname does not exist\n");
        printf("  - DNS server not reachable\n");
        printf("  - No internet connectivity\n");
        printf("  - DNS timeout\n");
        printf("─────────────────────────────────────────────────\n");
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// Wrapper for testConnectivity that retrieves current Wi-Fi information
static void testConnectivityWrapper(void)
{
    char gateway[64] = "";
    char ssid[64] = "Unknown";
    int32_t rssi = 0;
    int32_t channel = 0;
    
    // Check Wi-Fi connectivity
    if (!checkWiFiConnectivity(false, false)) {
        return;
    }
    
    // Try to get SSID
    uCxWifiStationStatus_t wifiStatus;
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_SSID, &wifiStatus)) {
        if (wifiStatus.type == U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_STR) {
            strncpy(ssid, wifiStatus.rsp.StatusIdStr.ssid, sizeof(ssid) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
        }
        uCxEnd(&gUcxHandle);
    }
    
    // Try to get RSSI
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &wifiStatus)) {
        if (wifiStatus.type == U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT) {
            rssi = wifiStatus.rsp.StatusIdInt.int_val;
        }
        uCxEnd(&gUcxHandle);
    }
    
    // Try to get channel
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CHANNEL, &wifiStatus)) {
        if (wifiStatus.type == U_CX_WIFI_STATION_STATUS_RSP_TYPE_STATUS_ID_INT) {
            channel = wifiStatus.rsp.StatusIdInt.int_val;
        }
        uCxEnd(&gUcxHandle);
    }
    
    // Get gateway IP address
    uSockIpAddress_t gatewayAddr;
    if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_WIFI_NET_STATUS_ID_GATE_WAY, &gatewayAddr) == 0) {
        uCxIpAddressToString(&gatewayAddr, gateway, sizeof(gateway));
    }
    
    // If we still don't have a gateway, use a default
    if (strlen(gateway) == 0) {
        strncpy(gateway, "192.168.1.1", sizeof(gateway) - 1);
    }
    
    // Call the actual connectivity test
    testConnectivity(gateway, ssid, rssi, channel);
}

// ----------------------------------------------------------------
// NTP (Network Time Protocol) Helper Functions
// ----------------------------------------------------------------

// Convert Unix timestamp (seconds since Jan 1, 1970) to human-readable format
static void unixTimeToString(uint64_t unixTime, char *buffer, size_t bufferSize)
{
    time_t rawTime = (time_t)unixTime;
    struct tm *timeInfo;
    
    // Convert to local time
    timeInfo = localtime(&rawTime);
    
    if (timeInfo != NULL) {
        // Format: "YYYY-MM-DD HH:MM:SS (Day of Week)"
        strftime(buffer, bufferSize, "%Y-%m-%d %H:%M:%S (%A)", timeInfo);
    } else {
        snprintf(buffer, bufferSize, "Invalid time");
    }
}

// NTP Time Sync Example
static void ntpTimeExample(void)
{
    int32_t err;
    uNtpClientStatus_t ntpEnabled;
    char choice[10];
    char serverAddress[256];
    int32_t serverId = 0;
    
    printf("\n");
    printf("=== NETWORK TIME (NTP) SYNCHRONIZATION ===\n");
    printf("\n");
    
    // Popular NTP servers
    const char *popularServers[] = {
        "time.google.com",       // Google Public NTP
        "time.cloudflare.com",   // Cloudflare NTP
        "pool.ntp.org",          // NTP Pool Project
        "time.nist.gov",         // NIST Internet Time Service
        "time.windows.com",      // Microsoft NTP
        "0.pool.ntp.org",        // Additional pool servers
        "1.pool.ntp.org",
        "2.pool.ntp.org"
    };
    int numServers = sizeof(popularServers) / sizeof(popularServers[0]);
    
    // Check Wi-Fi connectivity (connection, IP, RSSI, internet)
    if (!checkWiFiConnectivity(false, true)) {  // No ping test, verbose output
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Check current NTP status
    printf("\n");
    err = uCxNetworkTimeGetClientEnabled(&gUcxHandle, &ntpEnabled);
    if (err == 0) {
        printf("Current NTP status: %s\n", (ntpEnabled == U_NTP_CLIENT_STATUS_ENABLE_MANUAL || ntpEnabled == U_NTP_CLIENT_STATUS_ENABLE_AUTO) ? "ENABLED" : "DISABLED");
    } else {
        printf("WARNING: Failed to read NTP status (error: %d)\n", err);
    }
    
    printf("\n");
    printf("Popular NTP servers:\n");
    for (int i = 0; i < numServers; i++) {
        printf("  [%d] %s\n", i + 1, popularServers[i]);
    }
    printf("  [9] Enter custom NTP server\n");
    printf("\n");
    printf("Choice (or 0 to cancel): ");
    
    if (!fgets(choice, sizeof(choice), stdin)) {
        printf("ERROR: Failed to read choice\n");
        return;
    }
    
    int selection = atoi(choice);
    
    if (selection == 0) {
        printf("Cancelled.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    } else if (selection >= 1 && selection <= numServers) {
        strncpy(serverAddress, popularServers[selection - 1], sizeof(serverAddress) - 1);
        serverAddress[sizeof(serverAddress) - 1] = '\0';
    } else if (selection == 9) {
        printf("Enter NTP server address: ");
        if (!fgets(serverAddress, sizeof(serverAddress), stdin)) {
            printf("ERROR: Failed to read server address\n");
            return;
        }
        serverAddress[strcspn(serverAddress, "\r\n")] = 0;  // Remove newline
    } else {
        printf("ERROR: Invalid choice\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\n");
    printf("Configuring NTP...\n");
    
    // Step 3: Enable NTP client if not already enabled
    if (ntpEnabled != U_NTP_CLIENT_STATUS_ENABLE_MANUAL && ntpEnabled != U_NTP_CLIENT_STATUS_ENABLE_AUTO) {
        printf("Enabling NTP client...\n");
        err = uCxNetworkTimeSetClientEnabled(&gUcxHandle, U_NTP_CLIENT_STATUS_ENABLE_MANUAL);
        if (err < 0) {
            printf("ERROR: Failed to enable NTP client (error: %d)\n", err);
            printf("\n");
            printf("Press Enter to continue...");
            getchar();
            return;
        }
        printf("✓ NTP client enabled\n");
    }
    
    // Step 4: Configure NTP server
    printf("Setting NTP server to: %s\n", serverAddress);
    err = uCxNetworkTimeSetNtpServer(&gUcxHandle, serverId, serverAddress);
    if (err < 0) {
        printf("ERROR: Failed to set NTP server (error: %d)\n", err);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    printf("✓ NTP server configured\n");
    
    // Step 5: Wait for synchronization with aggressive polling (max 120 seconds)
    // Strategy: Start with 2-second intervals, then back off to 5 seconds after 10 seconds
    printf("\n");
    printf("Waiting for NTP synchronization (max 120 seconds, press Ctrl+C to cancel)...\n");
    
    bool timeSynced = false;
    int totalWaitTime = 0;
    int maxWaitTime = 120;  // 120 seconds max
    int attempt = 0;
    DWORD startTicks = GetTickCount();  // Get start time in milliseconds
    
    while (totalWaitTime < maxWaitTime && !timeSynced) {
        if (attempt == 0) {
            printf("Checking");
        } else {
            // Aggressive 2-second polling for first 10 seconds, then 5-second intervals
            int waitInterval = (totalWaitTime < 10) ? 2 : 5;
            printf("\nWaiting %d more seconds", waitInterval);
            
            // Wait with progress dots
            for (int i = 0; i < waitInterval; i++) {
                printf(".");
                fflush(stdout);
                Sleep(1000);
            }
            totalWaitTime += waitInterval;
        }
        
        // Try to read system time
        uint64_t testTime = 0;
        uByteArray_t testTimeData;
        uint8_t timeBuffer[32];
        testTimeData.pData = timeBuffer;
        testTimeData.length = 0;
        
        bool gotResponse = uCxSystemGetUnixTimeBegin(&gUcxHandle, &testTimeData);
        
        // Always call uCxEnd() to complete the AT command sequence
        int32_t endResult = uCxEnd(&gUcxHandle);
        
        if (gotResponse && endResult == 0) {
            if (testTimeData.length == 4) {
                // Data is 4 bytes in big-endian format (network byte order)
                uint8_t *bytes = testTimeData.pData;
                testTime = ((uint64_t)bytes[0] << 24) |
                          ((uint64_t)bytes[1] << 16) |
                          ((uint64_t)bytes[2] << 8) |
                          ((uint64_t)bytes[3]);
                
                // Distinguish between system uptime and Unix timestamp:
                // - System uptime: < 315360000 (10 years in seconds, would be year 1980)
                // - Unix timestamp: > 1600000000 (Sep 2020, reasonable minimum for NTP sync)
                if (testTime > 1600000000) {
                    timeSynced = true;
                    printf(" ✓\n");
                    
                    // Calculate precise elapsed time in seconds with 0.1s resolution
                    DWORD elapsedMs = GetTickCount() - startTicks;
                    double elapsedSec = elapsedMs / 1000.0;
                    printf("NTP synchronized in %.1f seconds\n", elapsedSec);
                } else if (testTime > 0 && testTime < 315360000) {
                    // This is system uptime, not NTP time yet
                    printf(" (uptime: %llu sec)", testTime);
                }
            } else {
                printf(" (invalid length: %zd)", testTimeData.length);
            }
        } else {
            printf(" (gotResponse=%d or endResult=%d failed)", gotResponse, endResult);
        }
        
        if (!timeSynced && testTime == 0) {
            printf(" (no response)");
        } else if (!timeSynced && testTime >= 315360000 && testTime <= 1600000000) {
            // Time is between 1980 and 2020 - unusual, but not uptime
            printf(" (unusual time: %llu)", testTime);
        }
        
        attempt++;
    }
    
    if (!timeSynced) {
        printf("\n\nWARNING: NTP synchronization timeout after 120 seconds.\n");
        printf("Time may not be synchronized. Possible reasons:\n");
        printf("  - NTP server is unreachable\n");
        printf("  - Firewall blocking NTP traffic (UDP port 123)\n");
        printf("  - Internet connectivity issues\n");
        printf("\nAttempting to read system time anyway...\n");
    }
    
    // Step 6: Read the synchronized time
    printf("\n");
    printf("Reading system time...\n");
    uByteArray_t unixTimeData;
    uint8_t timeBuffer2[32];
    unixTimeData.pData = timeBuffer2;
    unixTimeData.length = 0;
    
    bool gotTime = uCxSystemGetUnixTimeBegin(&gUcxHandle, &unixTimeData);
    int32_t endResult2 = uCxEnd(&gUcxHandle);
    
    if (gotTime && endResult2 == 0) {
        // The module returns time as 4 bytes in big-endian format
        uint64_t unixTime = 0;
        
        if (unixTimeData.length == 4) {
            // Convert 4 bytes (big-endian) to uint64_t
            uint8_t *bytes = unixTimeData.pData;
            unixTime = ((uint64_t)bytes[0] << 24) |
                      ((uint64_t)bytes[1] << 16) |
                      ((uint64_t)bytes[2] << 8) |
                      ((uint64_t)bytes[3]);
            
            char timeString[128];
            unixTimeToString(unixTime, timeString, sizeof(timeString));
            
            printf("─────────────────────────────────────────────────\n");
            printf("NTP Server:    %s\n", serverAddress);
            printf("Unix Time:     %llu seconds\n", unixTime);
            printf("Local Time:    %s\n", timeString);
            printf("─────────────────────────────────────────────────\n");
            
            // Also show UTC time
            time_t rawTime = (time_t)unixTime;
            struct tm *utcTime = gmtime(&rawTime);
            if (utcTime != NULL) {
                char utcString[128];
                strftime(utcString, sizeof(utcString), "%Y-%m-%d %H:%M:%S UTC", utcTime);
                printf("UTC Time:      %s\n", utcString);
                printf("─────────────────────────────────────────────────\n");
            }
        } else {
            printf("WARNING: Unexpected time data length: %zu bytes\n", unixTimeData.length);
            printf("Raw data: ");
            for (int i = 0; i < unixTimeData.length; i++) {
                printf("%02X ", unixTimeData.pData[i]);
            }
            printf("\n");
        }
        
    } else {
        printf("ERROR: Failed to read system time\n");
        printf("NOTE: Make sure Wi-Fi is connected and NTP server is reachable\n");
    }
    // Always call uCxEnd() to complete the command sequence
    uCxEnd(&gUcxHandle);
    
    // Step 7: Show configured NTP servers
    printf("\n");
    printf("Configured NTP servers:\n");
    uCxNtpGetNtpServer_t ntpServerInfo;
    if (uCxNetworkTimeGetNtpServerBegin(&gUcxHandle, &ntpServerInfo)) {
        printf("  Server %d: %s", ntpServerInfo.ntp_server_id, ntpServerInfo.ntp_server_address);
        if (ntpServerInfo.reachable) {
            printf(" (REACHABLE ✓)");
        } else {
            printf(" (NOT REACHABLE ✗)");
        }
        printf("\n");
    } else {
        printf("  (No servers configured or query failed)\n");
    }
    // Always call uCxEnd() to complete the command sequence
    uCxEnd(&gUcxHandle);
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void httpMenu(void)
{
    gMenuState = MENU_HTTP;
}

// ============================================================================
// SECURITY / TLS FUNCTIONS
// ============================================================================

static void tlsSetVersion(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("                   SET TLS VERSION\n");
    printf("\n");
    printf("\n");
    printf("NOTE: TLS version configuration is managed through server\n");
    printf("      certificate selection and connection parameters.\n");
    printf("\n");
    printf("The module supports TLS 1.2 and will negotiate the best\n");
    printf("available version with the server automatically.\n");
    printf("\n");
    printf("For TLS configuration, use:\n");
    printf("  - Upload CA certificates (option [5])\n");
    printf("  - Configure certificate validation (AT+USECVAL)\n");
    printf("  - Enable/disable TLS extensions (AT+USECEXT)\n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void tlsShowConfig(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("                TLS CONFIGURATION STATUS\n");
    printf("\n");
    printf("\n");
    
    printf("TLS Version: TLS 1.2 (auto-negotiated)\n");
    printf("\n");
    
    // Get TLS Server Name Indication status
    uEnabled_t sniEnabled;
    int32_t err = uCxSecurityGetTlsServerNameIndication(&gUcxHandle, &sniEnabled);
    
    if (err == 0) {
        printf("Server Name Indication (SNI): %s\n", 
               (sniEnabled == U_TLS_EXT_ENABLED) ? "Enabled" : "Disabled");
    } else {
        printf("Server Name Indication (SNI): ERROR (failed to query)\n");
    }
    
    // Get TLS Handshake Fragmentation status
    uEnabled_t fragEnabled;
    err = uCxSecurityGetTlsHandshakeFrag(&gUcxHandle, &fragEnabled);
    
    if (err == 0) {
        printf("Handshake Fragmentation: %s\n",
               (fragEnabled == U_TLS_EXT_ENABLED) ? "Enabled" : "Disabled");
    } else {
        printf("Handshake Fragmentation: ERROR (failed to query)\n");
    }
    
    printf("\n");
    printf("NOTE: Certificate validation and other TLS settings are\n");
    printf("      configured per connection. Use certificate management\n");
    printf("      options to upload and configure certificates.\n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void tlsListCertificates(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("                  CERTIFICATE LIST\n");
    printf("\n");
    printf("\n");
    
    // List certificates using AT+USECL command
    printf("Querying installed certificates...\n");
    printf("\n");
    
    uCxSecurityListCertificatesBegin(&gUcxHandle);
    
    uCxSecListCertificates_t certInfo;
    int certCount = 0;
    
    while (uCxSecurityListCertificatesGetNext(&gUcxHandle, &certInfo)) {
        certCount++;
        printf("[%d] Certificate:\n", certCount);
        printf("    Name: %s\n", certInfo.name);
        
        // Certificate type
        printf("    Type: ");
        switch (certInfo.cert_type) {
            case U_SEC_CERT_TYPE_ROOT:
                printf("Root/CA Certificate\n");
                break;
            case U_SEC_CERT_TYPE_CLIENT:
                printf("Client Certificate\n");
                break;
            case U_SEC_CERT_TYPE_KEY:
                printf("Client Private Key\n");
                break;
            default:
                printf("Unknown (%d)\n", certInfo.cert_type);
                break;
        }
        
        printf("\n");
    }
    
    uCxEnd(&gUcxHandle);
    
    if (certCount == 0) {
        printf("No certificates installed.\n");
        printf("\n");
        printf("Use option [5] to upload certificates (CA, Client, or Key).\n");
    } else {
        printf("Total: %d certificate(s)\n", certCount);
    }
    
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void tlsShowCertificateDetails(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("              CERTIFICATE DETAILS\n");
    printf("\n");
    printf("\n");
    
    char certName[128];
    printf("Enter certificate name: ");
    if (!fgets(certName, sizeof(certName), stdin)) {
        return;
    }
    certName[strcspn(certName, "\r\n")] = 0;  // Remove newline
    
    if (strlen(certName) == 0) {
        printf("ERROR: Certificate name cannot be empty\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\n");
    printf("Querying certificate details for: %s\n", certName);
    printf("\n");
    
    // Note: The UCX API might not provide detailed certificate info retrieval
    // This is a placeholder for future implementation
    printf("NOTE: Detailed certificate information retrieval is not fully\n");
    printf("      supported by the current UCX API.\n");
    printf("\n");
    printf("Available actions:\n");
    printf("  - View certificate list with [3]\n");
    printf("  - Delete certificate with [6]\n");
    printf("  - Upload new certificate with [5]\n");
    printf("\n");
    printf("For detailed certificate inspection, use OpenSSL tools:\n");
    printf("  openssl x509 -in cert.pem -text -noout\n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void tlsUploadCertificate(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("                 UPLOAD CERTIFICATE\n");
    printf("\n");
    printf("\n");
    printf("Select certificate type:\n");
    printf("  [1] CA Certificate (root or intermediate)\n");
    printf("  [2] Client Certificate (for mutual TLS)\n");
    printf("  [3] Client Private Key (for mutual TLS)\n");
    printf("  [0] Cancel\n");
    printf("\n");
    printf("Choice: ");
    
    char input[256];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    int choice = atoi(input);
    uSecCertType_t certType;
    const char *typeStr;
    
    switch (choice) {
        case 1:
            certType = U_SEC_CERT_TYPE_ROOT;
            typeStr = "CA/Root Certificate";
            break;
        case 2:
            certType = U_SEC_CERT_TYPE_CLIENT;
            typeStr = "Client Certificate";
            break;
        case 3:
            certType = U_SEC_CERT_TYPE_KEY;
            typeStr = "Client Private Key";
            break;
        case 0:
            printf("Cancelled.\n");
            printf("\n");
            printf("Press Enter to continue...");
            getchar();
            return;
        default:
            printf("ERROR: Invalid choice\n");
            printf("\n");
            printf("Press Enter to continue...");
            getchar();
            return;
    }
    
    printf("\n");
    printf("Selected: %s\n", typeStr);
    printf("\n");
    
    // Get certificate name
    char certName[128];
    printf("Enter certificate name (internal reference): ");
    if (!fgets(certName, sizeof(certName), stdin)) {
        return;
    }
    certName[strcspn(certName, "\r\n")] = 0;  // Remove newline
    
    if (strlen(certName) == 0) {
        printf("ERROR: Certificate name cannot be empty\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Get file path
    char filePath[512];
    printf("Enter certificate file path (PEM format): ");
    if (!fgets(filePath, sizeof(filePath), stdin)) {
        return;
    }
    filePath[strcspn(filePath, "\r\n")] = 0;  // Remove newline
    
    if (strlen(filePath) == 0) {
        printf("ERROR: File path cannot be empty\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Read certificate file
    FILE *fp = fopen(filePath, "rb");
    if (!fp) {
        printf("ERROR: Failed to open file: %s\n", filePath);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    if (fileSize <= 0 || fileSize > 8192) {
        printf("ERROR: Invalid file size (%ld bytes). Must be 1-8192 bytes.\n", fileSize);
        fclose(fp);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Allocate buffer and read file
    char *certData = (char *)malloc(fileSize + 1);
    if (!certData) {
        printf("ERROR: Memory allocation failed\n");
        fclose(fp);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    size_t bytesRead = fread(certData, 1, fileSize, fp);
    fclose(fp);
    
    if (bytesRead != (size_t)fileSize) {
        printf("ERROR: Failed to read complete file\n");
        free(certData);
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    certData[fileSize] = '\0';  // Null terminate
    
    printf("\n");
    printf("Uploading certificate...\n");
    printf("  Name: %s\n", certName);
    printf("  Type: %s\n", typeStr);
    printf("  Size: %ld bytes\n", fileSize);
    printf("\n");
    
    // Upload certificate using UCX API
    int32_t err = uCxSecurityCertificateUpload(&gUcxHandle, certType, certName, (const uint8_t *)certData, (int32_t)fileSize);
    
    free(certData);
    
    if (err == 0) {
        printf("✓ Certificate uploaded successfully!\n");
        printf("\n");
        printf("The certificate is now available for TLS connections.\n");
        printf("Use option [3] to verify it appears in the certificate list.\n");
    } else {
        printf("ERROR: Failed to upload certificate (error: %d)\n", err);
        printf("\n");
        printf("Common issues:\n");
        printf("  - Invalid PEM format (must start with -----BEGIN CERTIFICATE-----)\n");
        printf("  - File encoding issues (use UTF-8 or ASCII)\n");
        printf("  - Certificate too large (max 8KB)\n");
        printf("  - Certificate name already exists\n");
    }
    
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void tlsDeleteCertificate(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("                DELETE CERTIFICATE\n");
    printf("\n");
    printf("\n");
    
    char certName[128];
    printf("Enter certificate name to delete: ");
    if (!fgets(certName, sizeof(certName), stdin)) {
        return;
    }
    certName[strcspn(certName, "\r\n")] = 0;  // Remove newline
    
    if (strlen(certName) == 0) {
        printf("ERROR: Certificate name cannot be empty\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Confirm deletion
    printf("\n");
    printf("WARNING: This will permanently delete the certificate!\n");
    printf("Are you sure you want to delete '%s'? (y/N): ", certName);
    
    char confirm[10];
    if (!fgets(confirm, sizeof(confirm), stdin)) {
        return;
    }
    
    if (tolower(confirm[0]) != 'y') {
        printf("Cancelled.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    printf("\n");
    printf("Deleting certificate: %s\n", certName);
    
    // Note: UCX API requires cert type for deletion
    // We'll try all three types since we don't know which one it is
    int32_t err = -1;
    
    // Try as Root cert first
    err = uCxSecurityCertificateRemove(&gUcxHandle, U_SEC_CERT_TYPE_ROOT, certName);
    if (err != 0) {
        // Try as Client cert
        err = uCxSecurityCertificateRemove(&gUcxHandle, U_SEC_CERT_TYPE_CLIENT, certName);
    }
    if (err != 0) {
        // Try as Key
        err = uCxSecurityCertificateRemove(&gUcxHandle, U_SEC_CERT_TYPE_KEY, certName);
    }
    
    if (err == 0) {
        printf("✓ Certificate deleted successfully\n");
    } else {
        printf("ERROR: Failed to delete certificate (error: %d)\n", err);
        printf("\n");
        printf("Possible reasons:\n");
        printf("  - Certificate name not found\n");
        printf("  - Certificate is currently in use by an active connection\n");
    }
    
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// ----------------------------------------------------------------
// Main Function
// ----------------------------------------------------------------

// Helper function to get executable directory
static void getExecutableDirectory(char *buffer, size_t bufferSize)
{
    GetModuleFileNameA(NULL, buffer, (DWORD)bufferSize);
    // Remove the executable filename, keep only the directory
    char *lastSlash = strrchr(buffer, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';  // Keep the trailing backslash
    }
}

// ============================================================================
// MAIN APPLICATION ENTRY POINT
// ============================================================================

int main(int argc, char *argv[])
{
    // Set console to UTF-8 to properly display Unicode box drawing characters
    SetConsoleOutputCP(CP_UTF8);
    
    // Initialize settings file path (in project root directory)
    // Get executable directory (e.g., build\Release\ or build\Debug\)
    getExecutableDirectory(gSettingsFilePath, sizeof(gSettingsFilePath));
    
    // Navigate up to project root (from build\Release\ to root)
    // Remove "Release\" or "Debug\"
    char *lastSlash = strrchr(gSettingsFilePath, '\\');
    if (lastSlash && lastSlash > gSettingsFilePath) {
        *lastSlash = '\0';  // Remove trailing slash and config dir
        lastSlash = strrchr(gSettingsFilePath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';  // Keep trailing slash, now at build\
            // Remove "build\"
            *lastSlash = '\0';
            lastSlash = strrchr(gSettingsFilePath, '\\');
            if (lastSlash) {
                *(lastSlash + 1) = '\0';  // Keep trailing slash, now at root
            }
        }
    }
    
    strncat(gSettingsFilePath, SETTINGS_FILENAME, sizeof(gSettingsFilePath) - strlen(gSettingsFilePath) - 1);
    
    // Load settings from file
    loadSettings();
    
    // Check for COM port argument
    if (argc > 1) {
        strncpy(gComPort, argv[1], sizeof(gComPort) - 1);
        gComPort[sizeof(gComPort) - 1] = '\0';
    } else {
        // No argument provided - show available ports and let user choose
        char recommendedPort[32];
        char recommendedDevice[64];
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No COM port specified. Available ports:");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
        listAvailableComPorts(recommendedPort, sizeof(recommendedPort),
                             recommendedDevice, sizeof(recommendedDevice));
        
        // Auto-connect if saved port matches recommended port AND device model matches
        if (recommendedPort[0] != '\0' && 
            strcmp(gComPort, recommendedPort) == 0 &&
            recommendedDevice[0] != '\0' &&
            gLastDeviceModel[0] != '\0' &&
            strcmp(gLastDeviceModel, recommendedDevice) == 0) {
            // Same port and same device - auto-connect without asking
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Auto-connecting to saved %s on %s...", 
                         recommendedDevice, recommendedPort);
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
        } else {
            // Port changed, device changed, or no saved device - ask user
            char *selectedPort = selectComPortFromList(recommendedPort);
            if (selectedPort) {
                strncpy(gComPort, selectedPort, sizeof(gComPort) - 1);
                gComPort[sizeof(gComPort) - 1] = '\0';
                free(selectedPort);
            } else {
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "No port selected. Using last saved port: %s", gComPort);
            }
        }
    }
    
    printHeader();
    
    // Enable UCX logging to see AT commands and responses
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Enabling UCX logging (AT commands, responses, debug info)...");
    uCxLogEnable();
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Windows Console App started");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "UCX logging is now active - you'll see detailed AT traffic below");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    
    // Try to auto-connect
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Attempting to connect to %s...", gComPort);
    if (ucxclientConnect(gComPort)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Connected successfully!");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
        saveSettings();  // Save successful port
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Failed to connect. You can try again from the menu.");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
        
        // Show welcome guide for first-time users
        printWelcomeGuide();
    }
    
    // Main menu loop
    while (gMenuState != MENU_EXIT) {
        // Periodic tasks: CTS time notifications
        ULONGLONG now = GetTickCount64();
        if (now - gCtsServerLastTick >= 1000) {
            gCtsServerLastTick = now;
            ctsNotifyIfEnabled();
        }
        
        printMenu();
        handleUserInput();
    }
    
    // Cleanup
    if (gUcxConnected) {
        ucxclientDisconnect();
    }
    
    // Free API commands if loaded
    freeApiCommands();
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Goodbye!");
    return 0;
}

// ============================================================================
// UI & MENU SYSTEM
// ============================================================================

static void printHeader(void)
{
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "─────────────────────────────────────────────────────────────");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "u-connectXpress ucxclient App v%s", APP_VERSION);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "─────────────────────────────────────────────────────────────");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Simple C application for NORA-B26 and NORA-W36");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "NOTE: UCX Logging is %s", uCxLogIsEnabled() ? "ENABLED" : "DISABLED");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "      AT commands/responses will appear in this console");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "      Use menu option [9] to toggle logging on/off");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
}

static void printWelcomeGuide(void)
{
    printf("\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("WELCOME - Getting Started Guide\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    printf("This is a simple example application to help you get\n");
    printf("started with u-connectXpress modules (NORA-B26/NORA-W36).\n");
    printf("\n");
    printf("QUICK START:\n");
    printf("  1. Connect your module via USB\n");
    printf("  2. Use menu option [c] to connect to the device\n");
    printf("  3. Try AT command test to verify communication\n");
    printf("  4. Try device info to see model and firmware version\n");
    printf("\n");
    printf("BLUETOOTH OPERATIONS (NORA-B26 and NORA-W36):\n");
    printf("  - [b] Bluetooth - Scan, connect, pair, status\n");
    printf("  - [s] SPS - Serial Port Service data transfer\n");
    printf("  - [u] Bluetooth Functions - GATT Client/Server operations\n");
    printf("  - [e] GATT Server Examples - 9 profiles\n");
    printf("  - [g] GATT Client Examples - 8 demos\n");
    printf("\n");
    printf("WI-FI OPERATIONS (NORA-W36 only):\n");
    printf("  - [w] Wi-Fi Station - Scan, connect, disconnect, status\n");
    printf("  - [o] Wi-Fi Access Point - Create hotspot with QR code\n");
    printf("  - [n] Network - Sockets, MQTT, HTTP\n");
    printf("  - [x] Security - TLS/SSL certificates and configuration\n");
    printf("  - [p] HTTP Examples - GET/POST requests\n");
    printf("  - [y] NTP Examples - Time synchronization\n");
    printf("\n");
    printf("TIPS:\n");
    printf("  - Type [h] anytime for help\n");
    printf("  - Type [0] to return to main menu\n");
    printf("  - Settings are saved automatically after successful operations\n");
    printf("  - Use [l] to toggle AT command logging on/off\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void printHelp(void)
{
    printf("\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("HELP & TIPS\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("\n");
    printf("QUICK ACTIONS:\n");
    printf("  [c] Connect      - Select and connect to your UCX device\n");
    printf("  [d] Disconnect   - Close connection to device\n");
    printf("  [i] Device info  - Show model and firmware version\n");
    printf("  [r] Reboot       - Restart the module\n");
    printf("  [a] AT terminal  - Send custom AT commands\n");
    printf("  AT test          - Test basic communication\n");
    printf("\n");
    printf("BLUETOOTH FEATURES (NORA-B26 and NORA-W36):\n");
    printf("  [b] Bluetooth - Scan, connect, pair, disconnect, status\n");
    printf("      - Scan for nearby Bluetooth devices\n");
    printf("      - Connect/disconnect devices\n");
    printf("      - Configure pairing settings (security, IO capabilities)\n");
    printf("      - Manage bonded devices\n");
    printf("  [s] Serial Port Service (SPS)\n");
    printf("      - Enable SPS for wireless serial data transfer\n");
    printf("      - Send/receive data over Bluetooth\n");
    printf("  [u] Bluetooth Functions\n");
    printf("      - GATT Client: Discover and interact with remote services\n");
    printf("      - GATT Server: Host custom services and characteristics\n");
    printf("  [e] GATT Server Examples - 9 profiles\n");
    printf("      - Heart Rate, HID, Automation IO, Battery, ESS, NUS, SPS, LNS, CTS\n");
    printf("  [g] GATT Client Examples - 8 demos\n");
    printf("      - Discover and interact with remote GATT servers\n");
    printf("      - CTS, ESS, LNS, NUS, SPS, BAS, DIS, AIO clients\n");
    printf("\n");
    printf("Wi-Fi FEATURES (NORA-W36 only):\n");
    printf("  [w] Wi-Fi Station - Scan, connect, disconnect, status\n");
    printf("      - Scan for Wi-Fi networks with RSSI and security info\n");
    printf("      - Connect to Wi-Fi (SSID and password are saved)\n");
    printf("      - Show connection status and IP address\n");
    printf("  [o] Wi-Fi Access Point - Create wireless hotspot\n");
    printf("      - Enable/disable Access Point mode\n");
    printf("      - Configure SSID, password, and security (WPA2)\n");
    printf("      - Generate QR code for easy mobile connection\n");
    printf("      - View connected stations and AP status\n");
    printf("  [n] Network - Sockets, MQTT, HTTP\n");
    printf("      - TCP/UDP sockets for client/server communication\n");
    printf("      - MQTT publish/subscribe with QoS\n");
    printf("      - HTTP client (GET/POST/PUT/DELETE)\n");
    printf("  [x] Security - TLS/SSL configuration and certificates\n");
    printf("      - Configure TLS version (1.2 / 1.3)\n");
    printf("      - Upload CA certificates for server validation\n");
    printf("      - Manage client certificates for mutual TLS\n");
    printf("      - View certificate details and validation settings\n");
    printf("\n");
    printf("FIRMWARE:\n");
    printf("  [f] Firmware update - Update module firmware via XMODEM\n");
    printf("      - Upload new firmware files\n");
    printf("      - Automatic XMODEM transfer protocol\n");
    printf("      - Verify firmware after update\n");
    printf("\n");
    printf("EXAMPLES:\n");
    printf("  [e] GATT Examples - Server & Client demonstrations\n");
    printf("      - GATT Server: 9 profiles (Heart Rate, HID, AIO, Battery, ESS, NUS, SPS, LNS, CTS)\n");
    printf("      - GATT Client: 8 examples (CTS, ESS, LNS, NUS, SPS, BAS, DIS, AIO)\n");
    printf("  [p] HTTP Examples - REST API operations\n");
    printf("      - HTTP GET request with optional file save\n");
    printf("      - HTTP POST request with text or file data\n");
    printf("      - Automatic Wi-Fi connectivity validation\n");
    printf("  [y] NTP Examples - Network time synchronization\n");
    printf("      - Sync time with popular NTP servers\n");
    printf("      - Configure custom NTP server\n");
    printf("      - Display synchronized system time\n");
    printf("\n");
    printf("TOOLS & SETTINGS:\n");
    printf("  [l] Toggle logging - Show/hide AT command traffic\n");
    printf("  [t] Toggle timestamps - Add timing info to logs\n");
    printf("  [m] List API commands - Show all available UCX APIs\n");
    printf("  [h] Help - Show this help screen\n");
    printf("  [0] Main menu - Return to main menu from submenus\n");
    printf("\n");
    printf("SAVED SETTINGS:\n");
    printf("  The app remembers:\n");
    printf("    - Last COM port used\n");
    printf("    - Last device model (NORA-B26/NORA-W36)\n");
    printf("    - Last Wi-Fi SSID and password\n");
    printf("    - Last remote server address\n");
    printf("  Settings saved in: %s\n", gSettingsFilePath);
    printf("\n");
    printf("TROUBLESHOOTING:\n");
    printf("  - Can't connect? Check COM port with Device Manager\n");
    printf("  - Wi-Fi not working? Use [w] to check status and signal\n");
    printf("  - HTTP/NTP errors? Ensure Wi-Fi is connected with valid IP\n");
    printf("  - Module not responding? Try [r] to reboot it\n");
    printf("  - Weak signal? Check RSSI in Wi-Fi status (> -90 dBm needed)\n");
    printf("\n");
    printf("CONNECTIVITY VALIDATION:\n");
    printf("  HTTP and NTP examples automatically check:\n");
    printf("    ✓ Wi-Fi connection status\n");
    printf("    ✓ Signal strength (RSSI)\n");
    printf("    ✓ IP address assignment\n");
    printf("    ✓ Internet connectivity (ping 8.8.8.8)\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void printMenu(void)
{
    printf("\n");
    
    switch (gMenuState) {
        case MENU_MAIN:
            printf("\n");
            printf("─────────────────────────────────────────────────────────────\n");
            printf("u-blox UCX Client - Main Menu\n");
            printf("─────────────────────────────────────────────────────────────\n");
            
            // === STATUS DASHBOARD ===
            if (gUcxConnected) {
                printf("\n✓ CONNECTED");
                if (gDeviceModel[0] != '\0') {
                    printf("  |  %s", gDeviceModel);
                    if (gDeviceFirmware[0] != '\0') {
                        printf(" (FW: %s)", gDeviceFirmware);
                    }
                }
                printf("  |  Port: %s\n", gComPort);
                
                // Show active connections summary
                bool hasActivity = false;
                if (gBtConnectionCount > 0) {
                    printf("  Bluetooth: %d connection%s", gBtConnectionCount, gBtConnectionCount > 1 ? "s" : "");
                    hasActivity = true;
                }
                // TODO: Add Wi-Fi status when available
                if (hasActivity) {
                    printf("\n");
                }
            } else {
                printf("\n○ NOT CONNECTED");
                if (gComPort[0] != '\0') {
                    printf("  |  Last: %s", gComPort);
                    if (gLastDeviceModel[0] != '\0') {
                        printf(" (%s)", gLastDeviceModel);
                    }
                }
                printf("\n");
            }
            
            printf("─────────────────────────────────────────────────────────────\n");
            printf("\n");
            
            // === QUICK ACTIONS (always visible) ===
            printf("QUICK ACTIONS\n");
            if (!gUcxConnected) {
                if (gComPort[0] != '\0') {
                    printf("  [ENTER] Quick connect to %s\n", gComPort);
                    printf("  [1]     Connect to different device\n");
                } else {
                    printf("  [1]     Connect to UCX device\n");
                }
                printf("  [2]     Disconnect device\n");
            } else {
                printf("  [a]     AT command test\n");
                printf("  [i]     Device information (ATI9)\n");
                printf("  [r]     Reboot module\n");
                printf("  [t]     AT Terminal (interactive)\n");
            }
            printf("\n");
            
            if (gUcxConnected) {
                // === BLUETOOTH FEATURES (only when connected) ===
                printf("BLUETOOTH FEATURES\n");
                
                // Bluetooth
                printf("  [b]     Bluetooth - Scan, connect, pair\n");
                if (gBtConnectionCount > 0) {
                    printf("          └─ %d active connection%s\n", 
                           gBtConnectionCount, gBtConnectionCount > 1 ? "s" : "");
                }
                printf("  [s]     Serial Port Service (SPS)\n");
                printf("  [u]     Bluetooth Functions (SPS, GATT Client, GATT Server)\n");
                printf("  [e]     GATT Server Examples (9 profiles)\n");
                printf("  [g]     GATT Client Examples (8 demos)\n");
                printf("\n");
                
                // === Wi-Fi FEATURES (only for W3x modules) ===
                bool hasWiFi = gDeviceModel[0] != '\0' && strstr(gDeviceModel, "W3") != NULL;
                if (hasWiFi) {
                    printf("Wi-Fi FEATURES\n");
                    printf("  [w]     Wi-Fi Station - Scan, connect, status\n");
                    printf("  [o]     Wi-Fi Access Point - Enable, QR code\n");
                    printf("  [n]     Network - Sockets, MQTT, HTTP\n");
                    printf("  [d]     Network Diagnostics - Ping, iPerf, DNS lookup\n");
                    printf("  [x]     Security - TLS and certificates\n");
                } else {
                    printf("Wi-Fi FEATURES\n");
                    printf("  [w]     Wi-Fi - Not available on this device\n");
                }
                printf("\n");
                
                // === FIRMWARE ===
                printf("FIRMWARE\n");
                printf("  [f]     Firmware update (XMODEM)\n");
                printf("\n");
                
                // === EXAMPLES ===
                printf("EXAMPLES\n");
                printf("  [e]     GATT Server Examples (9 profiles)\n");
                printf("  [g]     GATT Client Examples (8 demos)\n");
                printf("  [p]     HTTP Examples (GET, POST)\n");
                printf("  [y]     NTP Examples (Time Sync)\n");
                printf("  [k]     Location Examples (IP Geolocation, Wi-Fi Positioning)\n");
                printf("\n");
            }
            
            // === TOOLS & SETTINGS (always available) ===
            printf("TOOLS & SETTINGS\n");
            printf("  [l]     Toggle logging: %s\n", 
                   uCxLogIsEnabled() ? "ON (disable for cleaner output)" : "OFF (enable to see AT commands)");
            printf("  [m]     Toggle timestamps: %s\n",
                   uCxLogTimestampIsEnabled() ? "ON" : "OFF");
            printf("  [c]     List all UCX API commands\n");
            printf("  [h]     Help & getting started\n");
            printf("\n");
            
            printf("  [q]     Quit application\n");
            printf("\n");
            
            // Contextual hint
            if (!gUcxConnected) {
                printf("Tip: Connect a device to unlock all features\n");
            } else {
                printf("Tip: Press [h] for help, [t] for AT terminal\n");
            }
            break;
            
        case MENU_BLUETOOTH:
            printf("\n");
            printf("                      BLUETOOTH MENU\n");
            printf("\n");
            printf("\n");
            if (gBtConnectionCount > 0) {
                printf("STATUS: %d active connection(s)\n", gBtConnectionCount);
            } else {
                printf("STATUS: No active connections\n");
            }
            printf("\n");
            printf("  [1] Show BT status\n");
            printf("  [2] Scan for devices\n");
            printf("  [3] Connect to device\n");
            printf("  [4] Disconnect from device\n");
            printf("  [5] List active connections\n");
            printf("  [6] Enable/Disable advertising (discoverable)\n");
            printf("  [7] Configure pairing settings\n");
            printf("  [8] List bonded devices\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_WIFI:
            printf("\n");
            printf("                  WI-FI STATION MENU\n");
            printf("\n");
            printf("\n");
            if (gWifiProfileCount > 0) {
                printf("SAVED PROFILES: %d\n", gWifiProfileCount);
            } else {
                printf("SAVED PROFILES: None\n");
            }
            printf("\n");
            printf("  [1] Show Wi-Fi status\n");
            printf("  [2] Regulatory domain (World)\n");
            printf("  [3] Scan networks\n");
            printf("  [4] Connect to network\n");
            printf("  [5] Disconnect from network\n");
            printf("  [6] Manage Wi-Fi profiles\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_WIFI_AP:
            printf("\n");
            printf("                  WI-FI ACCESS POINT MENU\n");
            printf("\n");
            printf("\n");
            printf("  [1] Enable Access Point\n");
            printf("  [2] Disable Access Point\n");
            printf("  [3] Show AP status\n");
            printf("  [4] Generate QR code for easy connection\n");
            printf("  [5] Configure AP settings (SSID, password, security)\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SOCKET:
            printf("\n");
            printf("                    SOCKET MENU (TCP/UDP)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  [1] Create TCP socket\n");
            printf("  [2] Create UDP socket\n");
            printf("  [3] Connect socket\n");
            printf("  [4] Send data\n");
            printf("  [5] Read data\n");
            printf("  [6] Close socket (current session)\n");
            printf("  [7] List sockets\n");
            printf("  [8] Close socket by handle (any socket)\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SPS:
            printf("\n");
            printf("           SPS MENU (Bluetooth Serial Port Service)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Bluetooth connection\n");
            printf("\n");
            printf("  [1] Enable SPS service\n");
            printf("  [2] Connect SPS on BT connection\n");
            printf("  [3] Send data\n");
            printf("  [4] Read data\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_MQTT:
            printf("\n");
            printf("                 MQTT MENU (Publish/Subscribe)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  Broker: %s:%d\n", MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT);
            printf("\n");
            printf("  [1] Connect to MQTT broker\n");
            printf("  [2] Disconnect from broker\n");
            printf("  [3] Subscribe to topic\n");
            printf("  [4] Unsubscribe from topic\n");
            printf("  [5] Publish message\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_HTTP:
            printf("\n");
            printf("                  HTTP CLIENT MENU (REST API)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  [1] HTTP GET request (with optional file save)\n");
            printf("  [2] HTTP POST request (with text or file data)\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_LOCATION:
            printf("\n");
            printf("              LOCATION EXAMPLES (IP & Wi-Fi Based)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  [1] External IP Detection (find your public IP)\n");
            printf("  [2] IP Geolocation (find country from IP address)\n");
            printf("  [3] Wi-Fi Positioning (locate using Wi-Fi scan)\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_DIAGNOSTICS:
            printf("\n");
            printf("              NETWORK DIAGNOSTICS (Testing & Troubleshooting)\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  [1] Ping Test (ICMP echo to host/IP)\n");
            printf("  [2] iPerf2 Client (throughput test to server)\n");
            printf("  [3] iPerf2 Server (listen for incoming tests)\n");
            printf("  [4] Stop iPerf Test (abort running test)\n");
            printf("  [5] DNS Lookup (resolve hostname to IP)\n");
            printf("  [6] Connectivity Test (gateway + internet check)\n");
            printf("\n");
            if (gIperfRunning) {
                printf("STATUS: iPerf test is currently running\n");
                printf("\n");
            }
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SECURITY_TLS:
            printf("\n");
            printf("           SECURITY/TLS MENU (Certificates & Encryption)\n");
            printf("\n");
            printf("\n");
            printf("TLS CONFIGURATION\n");
            printf("  [1] Set TLS version (1.2 / 1.3)\n");
            printf("  [2] Show TLS configuration\n");
            printf("\n");
            printf("CERTIFICATE MANAGEMENT\n");
            printf("  [3] List all certificates\n");
            printf("  [4] Show certificate details\n");
            printf("  [5] Upload certificate (CA, Client, or Private Key)\n");
            printf("  [6] Delete certificate\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_BLUETOOTH_FUNCTIONS:
            printf("\n");
            printf("                   BLUETOOTH FUNCTIONS\n");
            printf("\n");
            printf("\n");
            printf("  [1] SPS (Serial Port Service)\n");
            printf("  [2] GATT Client\n");
            printf("  [3] GATT Server\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_GATT_EXAMPLES:
            printf("┌─────────────────────────────────────────────────────────────┐\n");
            printf("│  GATT SERVER EXAMPLES (9 Profiles)                          │\n");
            printf("│  This device provides services to remote BLE clients        │\n");
            printf("└─────────────────────────────────────────────────────────────┘\n");
            printf("\n");
            printf("  [h] Heart Rate Service - Heartbeat notifications\n");
            printf("  [k] HID Keyboard + Media + Battery - Full HID device\n");
            printf("  [b] Battery Service only - Simple battery reporting\n");
            printf("  [e] Environmental Sensing - Temperature + Humidity\n");
            printf("  [u] UART Service (NUS) - Bidirectional text data\n");
            printf("  [s] Serial Port Service (SPS) - u-blox, like NUS + credits\n");
            printf("  [l] Location & Navigation (LNS) - GPS coordinates\n");
            printf("  [c] Current Time Service (CTS) - Broadcast PC time\n");
            printf("  [a] Automation IO Service - Digital + Analog I/O\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_WIFI_FUNCTIONS:
            printf("\n");
            printf("                     WI-FI FUNCTIONS\n");
            printf("\n");
            printf("\n");
            printf("NOTE: Requires active Wi-Fi connection\n");
            printf("\n");
            printf("  [1] Socket menu (TCP/UDP)\n");
            printf("  [2] MQTT (publish/subscribe)\n");
            printf("  [3] HTTP Client (GET/POST/PUT)\n");
            printf("  [4] Security/TLS (certificates)\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_GATT_CLIENT:
            printf("\n");
            printf("         GATT CLIENT (Connect to Remote GATT Servers)\n");
            printf("\n");
            printf("\n");
            if (gCurrentGattConnHandle != -1) {
                printf("CONNECTION: Handle %d\n", gCurrentGattConnHandle);
            } else {
                printf("CONNECTION: No active connection\n");
                printf("NOTE: Use Bluetooth menu to connect first\n");
            }
            printf("\n");
            printf("GENERIC OPERATIONS\n");
            printf("  [1] Discover services\n");
            printf("  [2] Discover characteristics\n");
            printf("  [3] Read characteristic\n");
            printf("  [4] Write characteristic\n");
            printf("  [5] Subscribe to notifications\n");
            printf("\n");
            printf("SERVICE-SPECIFIC CLIENT EXAMPLES\n");
            printf("  [c] Current Time Service (CTS) - read + subscribe to time\n");
            printf("  [e] Environmental Sensing (ESS) - temp + humidity\n");
            printf("  [l] Location & Navigation (LNS) - GPS coordinates\n");
            printf("  [u] UART Service (NUS) - bidirectional text data\n");
            printf("  [s] Serial Port Service (SPS) - u-blox, like NUS + credits\n");
            printf("  [b] Battery Service (BAS) - battery percentage\n");
            printf("  [d] Device Information Service (DIS) - device details\n");
            printf("  [a] Automation IO (AIO) - digital + analog I/O\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_GATT_SERVER:
            // Sync GATT connection state before showing menu (no discovery for server!)
            syncGattConnectionOnly();
            
            printf("\n");
            printf("         GATT SERVER (Provide Services to Remote Clients)\n");
            printf("\n");
            printf("\n");
            if (gCurrentGattConnHandle != -1) {
                printf("CONNECTION: Handle %d\n", gCurrentGattConnHandle);
            } else {
                printf("CONNECTION: No active connection\n");
            }
            printf("\n");
            printf("GENERIC OPERATIONS\n");
            printf("  [1] Add service\n");
            printf("  [2] Add characteristic\n");
            printf("  [3] Set characteristic value\n");
            printf("  [4] Send notification\n");
            printf("\n");
            printf("SERVICE-SPECIFIC SERVER EXAMPLES\n");
            printf("  See main menu option [9] for complete examples:\n");
            printf("  - Heart Rate Service (Heartbeat)\n");
            printf("  - HID Keyboard + Media + Battery\n");
            printf("  - Environmental Sensing (Temp + Humidity)\n");
            printf("  - UART Service (NUS)\n");
            printf("  - Serial Port Service (SPS - u-blox)\n");
            printf("  - Location & Navigation Service (LNS)\n");
            printf("  - Current Time Service (CTS)\n");
            printf("\n");
            printf("  [0] Back to Bluetooth Functions  [q] Quit\n");
            break;
            
        case MENU_HID:
            // Sync GATT connection state
            syncGattConnectionOnly();
            
            printf("\n");
            printf("                    HID OVER GATT (HOGP)\n");
            printf("\n");
            printf("\n");
            if (gCurrentGattConnHandle != -1) {
                printf("CONNECTION: Handle %d\n", gCurrentGattConnHandle);
            } else {
                printf("CONNECTION: No active connection\n");
            }
            printf("\n");
            printf("SERVICE SETUP\n");
            printf("  [1] Enable HID Keyboard service\n");
            printf("  [2] Enable HID Media Control service\n");
            printf("  [3] Disable HID (reboot device)\n");
            printf("\n");
            printf("KEYBOARD ACTIONS\n");
            printf("  [4] Send keyboard key press\n");
            printf("  [5] Send 'Hello World' text\n");
            printf("\n");
            printf("MEDIA CONTROL\n");
            printf("  [6] Send media control command\n");
            printf("\n");
            printf("STATUS\n");
            printf("  [s] Show Bluetooth connection status\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_FIRMWARE_UPDATE:
            printf("\n");
            printf("                  FIRMWARE UPDATE (XMODEM)\n");
            printf("\n");
            printf("\n");
            printf("This will update the module firmware via XMODEM protocol.\n");
            printf("The module will reboot after a successful update.\n");
            printf("\n");
            printf("CURRENT DEVICE\n");
            printf("  Port          : %s\n", gComPort);
            if (gDeviceModel[0] != '\0') {
                printf("  Model         : %s\n", gDeviceModel);
                if (gDeviceFirmware[0] != '\0') {
                    printf("  Firmware      : %s\n", gDeviceFirmware);
                }
            }
            printf("\n");
            printf("  [1] Select firmware file and start update\n");
            printf("  [2] Download latest firmware from GitHub\n");
            printf("\n");
            printf("  [0] Back to main menu\n");
            break;
            
        case MENU_API_LIST:
            listAllApiCommands();
            gMenuState = MENU_MAIN;
            return;
            
        default:
            break;
    }
    
    printf("\nChoice: ");
}

static void handleUserInput(void)
{
    char input[128];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    // Remove newline
    input[strcspn(input, "\n")] = 0;
    
    // Handle ENTER key (empty input) for quick connect
    if (strlen(input) == 0 && gMenuState == MENU_MAIN && !gUcxConnected && gComPort[0] != '\0') {
        quickConnectToLastDevice();
        return;
    }
    
    // Parse choice
    int choice = atoi(input);
    
    // Handle letter inputs (convert to numbers)
    if (choice == 0 && strlen(input) > 0) {
        char firstChar = (char)tolower(input[0]);
        
        // Handle special commands available in all menus
        if (firstChar == 'q') {
            gMenuState = MENU_EXIT;
            return;
        }
        
        if (firstChar == 'h' && gMenuState == MENU_MAIN) {
            printHelp();
            return;
        }
        
        // Handle 'm' for timestamp toggle (main menu only)
        if (firstChar == 'm' && gMenuState == MENU_MAIN) {
            if (uCxLogTimestampIsEnabled()) {
                uCxLogTimestampDisable();
                printf("✓ Log timestamps DISABLED (cleaner output)\n");
            } else {
                uCxLogTimestampEnable();
                printf("✓ Log timestamps ENABLED (shows [HH:MM:SS.mmm] timing)\n");
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Timestamps enabled from menu");
            }
            return;
        }
        
        // Handle specific letter commands for main menu
        if (gMenuState == MENU_MAIN) {
            if (firstChar == 'w') {
                choice = 23;  // Wi-Fi Station menu
            } else if (firstChar == 'o') {
                choice = 60;  // Wi-Fi Access Point menu
            } else if (firstChar == 'n') {
                choice = 24;  // Network functions (sockets, MQTT, HTTP)
            } else if (firstChar == 'x') {
                choice = 61;  // Security/TLS menu
            } else if (firstChar == 'b') {
                choice = 7;   // Bluetooth menu
            } else if (firstChar == 's') {
                choice = 50;  // SPS (Serial Port Service)
            } else if (firstChar == 'u') {
                choice = 8;   // Bluetooth Functions (GATT Client/Server)
            } else if (firstChar == 'g') {
                choice = 51;  // GATT Client Examples
            } else if (firstChar == 't') {
                choice = 52;  // AT Terminal (interactive)
            } else if (firstChar == 'l') {
                choice = 21;  // Toggle UCX logging
            } else if (firstChar == 'c') {
                choice = 12;  // List API commands
            } else if (firstChar == 'd') {
                choice = 13;  // Network Diagnostics
            } else if (firstChar == 'f') {
                choice = 16;  // Firmware update
            } else if (firstChar == 'e') {
                choice = 9;   // Examples (Heartbeat, HID)
            } else if (firstChar == 'p') {
                choice = 15;  // HTTP Examples (GET, POST)
            } else if (firstChar == 'y') {
                choice = 17;  // NTP Examples (Time Sync)
            } else if (firstChar == 'k') {
                choice = 19;  // Location Examples
            } else if (firstChar == 'a') {
                choice = 3;   // AT command test
            } else if (firstChar == 'i') {
                choice = 4;   // Device information (ATI9)
            } else if (firstChar == 'r') {
                choice = 5;   // Reboot module
            }
        }
        
        // Generic letter conversion (for other contexts)
        if (choice == 0 && firstChar >= 'a' && firstChar <= 'z') {
            // Convert letter to number: a=10, b=11, c=12, ... z=35
            choice = 10 + (firstChar - 'a');
        }
    }
    
    switch (gMenuState) {
        case MENU_MAIN:
            switch (choice) {
                case 1: {
                    // If we have saved settings, offer quick connect
                    if (gComPort[0] != '\0' && !gUcxConnected) {
                        printf("Quick connect to last device (%s", gComPort);
                        if (gLastDeviceModel[0] != '\0') {
                            printf(" - %s", gLastDeviceModel);
                        }
                        printf(")? (Y/n): ");
                        if (fgets(input, sizeof(input), stdin)) {
                            input[strcspn(input, "\n")] = 0;
                            // Default to Yes if Enter pressed or 'y' typed
                            if (strlen(input) == 0 || tolower(input[0]) == 'y') {
                                quickConnectToLastDevice();
                                break;
                            }
                        }
                    }
                    
                    // Manual port entry
                    printf("Enter COM port (e.g., COM31): ");
                    if (fgets(input, sizeof(input), stdin)) {
                        input[strcspn(input, "\n")] = 0;
                        if (strlen(input) > 0) {
                            // Basic COM port validation
                            if (strncmp(input, "COM", 3) == 0 || strncmp(input, "com", 3) == 0) {
                                // Convert to uppercase
                                for (int i = 0; i < 3; i++) {
                                    input[i] = (char)toupper(input[i]);
                                }
                                strncpy(gComPort, input, sizeof(gComPort) - 1);
                                if (ucxclientConnect(gComPort)) {
                                    saveSettings();
                                }
                            } else {
                                printf("ERROR: Invalid COM port format. Use format like 'COM31'\n");
                            }
                        }
                    }
                    break;
                }
                case 2:
                    ucxclientDisconnect();
                    break;
                case 3:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeAtTest();
                    }
                    break;
                case 4:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeAti9();
                    }
                    break;
                case 5:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeModuleReboot();
                    }
                    break;
                case 6:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        showLegacyAdvertisementStatus();
                    }
                    break;
                case 7:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_BLUETOOTH;
                    }
                    break;
                case 8:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_BLUETOOTH_FUNCTIONS;
                    }
                    break;
                case 9:
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_GATT_EXAMPLES;
                    }
                    break;
                case 15:  // HTTP Examples (GET, POST)
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_HTTP;
                    }
                    break;
                case 17:  // NTP Examples (Time Sync)
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        ntpTimeExample();
                    }
                    break;
                case 19:  // Location Examples
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_LOCATION;
                    }
                    break;
                case 23:  // Also accept 'w' or 'W' - Wi-Fi (scan, connect, disconnect, status)
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_WIFI;
                    }
                    break;
                case 24:  // Also accept 'x' or 'X' - Wi-Fi functions
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_WIFI_FUNCTIONS;
                    }
                    break;
                case 13:  // Also accept 'd' or 'D' - Network Diagnostics
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_DIAGNOSTICS;
                    }
                    break;
                case 12:  // Also accept 'c' or 'C' - List API commands
                    gMenuState = MENU_API_LIST;
                    break;
                case 21:  // Also accept 'l' or 'L' - Toggle UCX logging
                    if (uCxLogIsEnabled()) {
                        uCxLogDisable();
                        printf("UCX logging DISABLED\n");
                    } else {
                        uCxLogEnable();
                        printf("UCX logging ENABLED\n");
                        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Logging re-enabled from menu");
                    }
                    break;
                case 16:  // Also accept 'f' or 'F' - Firmware update
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_FIRMWARE_UPDATE;
                    }
                    break;
                case 50:  // Also accept 's' or 'S' - SPS (Serial Port Service)
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_SPS;
                    }
                    break;
                case 51:  // Also accept 'g' or 'G' - GATT Client Examples
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        bluetoothSyncConnections();  // Sync BT connections first
                        syncGattConnection();        // Then sync GATT connection handle
                        gMenuState = MENU_GATT_CLIENT;
                    }
                    break;
                case 52:  // Also accept 't' or 'T' - AT Terminal
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeAtTerminal();
                    }
                    break;
                case 60:  // Also accept 'o' or 'O' - Wi-Fi Access Point
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_WIFI_AP;
                    }
                    break;
                case 61:  // Also accept 'x' or 'X' - Security/TLS
                    if (!gUcxConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_SECURITY_TLS;
                    }
                    break;
                case 18:  // Also accept 'h' or 'H' - Help (handled above but keep for consistency)
                    printHelp();
                    break;
                case 0:
                    // Don't exit on Enter/0 in main menu - only 'q' should quit
                    // This prevents accidental exits
                    if (strlen(input) > 0) {
                        printf("Invalid choice! Use [q] to quit.\n");
                    }
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_BLUETOOTH:
            // Sync connection list from module before processing menu choice
            if (choice >= 1 && choice <= 5) {
                bluetoothSyncConnections();
            }
            
            switch (choice) {
                case 1:
                    showBluetoothStatus();
                    break;
                case 2:
                    bluetoothScan();
                    break;
                case 3:
                    bluetoothConnect();
                    break;
                case 4:
                    bluetoothDisconnect();
                    break;
                case 5:
                    showBluetoothStatus();  // Shows connections
                    break;
                case 6:
                    bluetoothSetAdvertising();
                    break;
                case 7:
                    bluetoothSetPairing();
                    break;
                case 8:
                    bluetoothListBondedDevices();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_WIFI:
            switch (choice) {
                case 1:
                    showWifiStatus();
                    break;
                case 2:
                    printf("\n--- Regulatory Domain Configuration ---\n");
                    printf("This feature will be available in the next release.\n");
                    printf("It will allow setting the country/region for compliance\n");
                    printf("with local regulations (affects available channels).\n");
                    printf("Current setting: World (all channels enabled)\n");
                    break;
                case 3:
                    wifiScan();
                    break;
                case 4:
                    wifiConnect();
                    break;
                case 5:
                    wifiDisconnect();
                    break;
                case 6:
                    wifiManageProfiles();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_WIFI_AP:
            switch (choice) {
                case 1:
                    wifiApEnable();
                    break;
                case 2:
                    wifiApDisable();
                    break;
                case 3:
                    wifiApShowStatus();
                    break;
                case 4:
                    wifiApGenerateQrCode();
                    break;
                case 5:
                    wifiApConfigure();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_SOCKET:
            switch (choice) {
                case 1:
                    socketCreateTcp();
                    break;
                case 2:
                    socketCreateUdp();
                    break;
                case 3:
                    socketConnect();
                    break;
                case 4:
                    socketSendData();
                    break;
                case 5:
                    socketReadData();
                    break;
                case 6:
                    socketClose();
                    break;
                case 7:
                    socketListStatus();
                    break;
                case 8:
                    socketCloseByHandle();
                    break;
                case 0:
                    gMenuState = MENU_WIFI_FUNCTIONS;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_SPS:
            switch (choice) {
                case 1:
                    spsEnableService();
                    break;
                case 2:
                    spsConnect();
                    break;
                case 3:
                    spsSendData();
                    break;
                case 4:
                    spsReadData();
                    break;
                case 0:
                    gMenuState = MENU_BLUETOOTH_FUNCTIONS;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_MQTT:
            switch (choice) {
                case 0:
                    gMenuState = MENU_WIFI_FUNCTIONS;
                    break;
                case 1:
                    mqttConnect();
                    break;
                case 2:
                    mqttDisconnect();
                    break;
                case 3:
                    mqttSubscribe();
                    break;
                case 4:
                    mqttUnsubscribe();
                    break;
                case 5:
                    mqttPublish();
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_HTTP:
            switch (choice) {
                case 1:
                    httpGetExample();
                    break;
                case 2:
                    httpPostExample();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_LOCATION:
            switch (choice) {
                case 1:
                    externalIpDetectionExample();
                    break;
                case 2:
                    ipGeolocationExample();
                    break;
                case 3:
                    wifiPositioningExample();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_DIAGNOSTICS:
            switch (choice) {
                case 1:
                    pingExample();
                    break;
                case 2:
                    iperfClientExample();
                    break;
                case 3:
                    iperfServerExample();
                    break;
                case 4:
                    iperfStopExample();
                    break;
                case 5:
                    dnsLookupExample();
                    break;
                case 6:
                    testConnectivityWrapper();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_SECURITY_TLS:
            switch (choice) {
                case 1:
                    tlsSetVersion();
                    break;
                case 2:
                    tlsShowConfig();
                    break;
                case 3:
                    tlsListCertificates();
                    break;
                case 4:
                    tlsShowCertificateDetails();
                    break;
                case 5:
                    tlsUploadCertificate();
                    break;
                case 6:
                    tlsDeleteCertificate();
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_BLUETOOTH_FUNCTIONS:
            switch (choice) {
                case 1:
                    gMenuState = MENU_SPS;
                    break;
                case 2:
                    bluetoothSyncConnections();  // Sync BT connections first
                    syncGattConnection();        // Then sync GATT connection handle
                    gMenuState = MENU_GATT_CLIENT;
                    break;
                case 3:
                    gMenuState = MENU_GATT_SERVER;
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_GATT_EXAMPLES:
            switch (choice) {
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    // Handle letter commands
                    if (strlen(input) > 0) {
                        char firstChar = (char)tolower(input[0]);
                        if (firstChar == 'h') {
                            gattServerSetupHeartbeat();  // Heart Rate Service
                            break;
                        }
                        if (firstChar == 'k') {
                            gattServerSetupHidKeyboard();  // HID Keyboard + Media + Battery
                            break;
                        }
                        if (firstChar == 'a') {
                            gattServerSetupAutomationIo();  // Automation IO Service
                            break;
                        }
                        if (firstChar == 'b') {
                            gattServerSetupBatteryOnly();  // Battery Service
                            break;
                        }
                        if (firstChar == 'e') {
                            gattServerSetupEnvSensing();  // Environmental Sensing
                            break;
                        }
                        if (firstChar == 'u') {
                            gattServerSetupUartService();  // UART Service
                            break;
                        }
                        if (firstChar == 's') {
                            gattServerSetupSpsService();  // Serial Port Service (SPS)
                            break;
                        }
                        if (firstChar == 'l') {
                            gattServerSetupLocationService();  // Location & Navigation
                            break;
                        }
                        if (firstChar == 'c') {
                            gattServerSetupCtsService();  // Current Time Service (Server)
                            break;
                        }
                        if (firstChar == 't') {
                            gattClientReadCurrentTime();  // Read Current Time (Client)
                            break;
                        }
                    }
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_WIFI_FUNCTIONS:
            switch (choice) {
                case 1:
                    gMenuState = MENU_SOCKET;
                    break;
                case 2:
                    gMenuState = MENU_MQTT;
                    break;
                case 3:
                    gMenuState = MENU_HTTP;
                    break;
                case 4:
                    gMenuState = MENU_SECURITY_TLS;
                    break;
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_GATT_CLIENT:
            switch (choice) {
                case 1:
                    gattClientDiscoverServices();
                    break;
                case 2:
                    gattClientDiscoverCharacteristics();
                    break;
                case 3:
                    gattClientReadCharacteristic();
                    break;
                case 4:
                    gattClientWriteCharacteristic();
                    break;
                case 5:
                    printf("Subscribe to notifications - not yet implemented\n");
                    break;
                case 0:
                    gMenuState = MENU_BLUETOOTH_FUNCTIONS;
                    break;
                default:
                    // Handle letter commands
                    if (strlen(input) > 0) {
                        char firstChar = (char)tolower(input[0]);
                        if (firstChar == 'c') {
                            gattClientCtsExample();
                            break;
                        }
                        if (firstChar == 'e') {
                            gattClientEssExample();
                            break;
                        }
                        if (firstChar == 'l') {
                            gattClientLnsExample();
                            break;
                        }
                        if (firstChar == 'u') {
                            gattClientUartExample();
                            break;
                        }
                        if (firstChar == 's') {
                            gattClientSpsExample();
                            break;
                        }
                        if (firstChar == 'b') {
                            gattClientBasExample();
                            break;
                        }
                        if (firstChar == 'd') {
                            gattClientDisExample();
                            break;
                        }
                        if (firstChar == 'a') {
                            gattClientAioExample();
                            break;
                        }
                    }
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_GATT_SERVER:
            // No special character handling needed for basic GATT Server menu
            
            switch (choice) {
                case 1:
                    gattServerAddService();
                    break;
                case 2:
                    gattServerAddCharacteristic();
                    break;
                case 3:
                    gattServerSetCharacteristic();
                    break;
                case 4:
                    gattServerSendNotification();
                    break;
                case 0:
                    gMenuState = MENU_BLUETOOTH_FUNCTIONS;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_HID:
            // Check for 's' (status) command
            if (input[0] == 's' || input[0] == 'S') {
                bluetoothShowStatus();
                break;
            }
            
            switch (choice) {
                case 1:
                    gattServerSetupHidKeyboard();
                    printf("\nNOTE: HID Keyboard service includes both keyboard and media control\n");
                    break;
                case 2:
                    gattServerSetupHidKeyboard();
                    printf("\nNOTE: Media control is included in the HID Keyboard service\n");
                    break;
                case 3:
                    // Disable HID by rebooting
                    printf("\nRebooting device to reset GATT services...\n");
                    uCxSystemReboot(&gUcxHandle);
                    gUcxConnected = false;
                    gCurrentGattConnHandle = -1;
                    gHeartbeatServiceHandle = -1;
                    gHidServiceHandle = -1;
                    gHidKeyboardInputHandle = -1;
                    gHidMediaInputHandle = -1;
                    printf("Device rebooted. Please reconnect.\n");
                    gMenuState = MENU_MAIN;
                    break;
                case 4:
                    gattServerSendKeyPress();
                    break;
                case 5:
                    gattServerSendHelloWorld();
                    break;
                case 6:
                    gattServerSendMediaControl();
                    break;
                case 0:
                    gMenuState = MENU_GATT_EXAMPLES;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_FIRMWARE_UPDATE:
            switch (choice) {
                case 1: {
                    // Select firmware file and update
                    char firmwarePath[256];
                    printf("Enter firmware file path: ");
                    if (fgets(firmwarePath, sizeof(firmwarePath), stdin) == NULL) {
                        printf("ERROR: Failed to read input\n");
                        break;
                    }
                    // Remove newline
                    firmwarePath[strcspn(firmwarePath, "\r\n")] = '\0';
                    
                    // Check if file exists
                    FILE *testFile = fopen(firmwarePath, "rb");
                    if (testFile == NULL) {
                        printf("ERROR: Cannot open file: %s\n", firmwarePath);
                        break;
                    }
                    fclose(testFile);
                    
                    // Check if device is connected
                    if (!gUcxConnected) {
                        printf("ERROR: Device not connected. Please connect first.\n");
                        break;
                    }
                    
                    printf("\nStarting firmware update...\n");
                    printf("This will take several minutes. Please wait...\n\n");
                    printf("NOTE: The connection will be closed and reopened for XMODEM transfer.\n");
                    printf("      The device will reboot after successful update.\n\n");
                    
                    // Perform firmware update with progress callback
                    int32_t result = uCxFirmwareUpdate(
                        &gUcxHandle,
                        firmwarePath,
                        gComPort,
                        115200,
                        false,  // No flow control
                        true,   // Use 1K blocks
                        firmwareUpdateProgress,
                        NULL
                    );
                    
                    if (result == 0) {
                        printf("\n\nFirmware update completed successfully!\n");
                        printf("The module is rebooting...\n");
                        printf("Waiting for +STARTUP URC");
                        fflush(stdout);
                        
                        // Wait up to 10 seconds for the +STARTUP URC
                        // The URC will be processed by the RX thread and signal the event
                        bool startupReceived = waitEvent(URC_FLAG_STARTUP, 10);
                        
                        if (startupReceived) {
                            printf(" Received!\n");
                        } else {
                            printf(" Timeout! Continuing anyway...\n");
                        }
                        fflush(stdout);
                        
                        // Now the module has sent +STARTUP and is ready for commands
                        // Disable echo again (module reboot resets this to default ON)
                        printf("Disabling AT echo...\n");
                        result = uCxSystemSetEchoOff(&gUcxHandle);
                        if (result != 0) {
                            printf("Warning: Failed to disable echo (error %d), continuing...\n", result);
                        }
                        
                        // Re-query device information to get new firmware version
                        printf("Querying new firmware version...\n");
                        
                        // Clear old device info
                        gDeviceModel[0] = '\0';
                        gDeviceFirmware[0] = '\0';
                        
                        // AT+GMM - Model identification
                        const char *model = NULL;
                        if (uCxGeneralGetDeviceModelIdentificationBegin(&gUcxHandle, &model) && model != NULL) {
                            strncpy(gDeviceModel, model, sizeof(gDeviceModel) - 1);
                            gDeviceModel[sizeof(gDeviceModel) - 1] = '\0';
                            strncpy(gLastDeviceModel, model, sizeof(gLastDeviceModel) - 1);
                            gLastDeviceModel[sizeof(gLastDeviceModel) - 1] = '\0';
                            uCxEnd(&gUcxHandle);
                        } else {
                            uCxEnd(&gUcxHandle);
                        }
                        
                        // AT+GMR - Firmware version
                        const char *fwVersion = NULL;
                        if (uCxGeneralGetSoftwareVersionBegin(&gUcxHandle, &fwVersion) && fwVersion != NULL) {
                            strncpy(gDeviceFirmware, fwVersion, sizeof(gDeviceFirmware) - 1);
                            gDeviceFirmware[sizeof(gDeviceFirmware) - 1] = '\0';
                            uCxEnd(&gUcxHandle);
                        } else {
                            uCxEnd(&gUcxHandle);
                        }
                        
                        // Connection is still active
                        gUcxConnected = true;
                        
                        printf("\nFirmware update complete!\n");
                        if (gDeviceModel[0] != '\0' && gDeviceFirmware[0] != '\0') {
                            printf("Device: %s\n", gDeviceModel);
                            printf("New firmware version: %s\n", gDeviceFirmware);
                            printf("\nThe device is ready to use!\n");
                        } else {
                            printf("Note: Could not read new firmware version. You may need to reconnect.\n");
                        }
                        
                        saveSettings();
                    } else {
                        printf("\n\nERROR: Firmware update failed with code %d\n", result);
                        printf("The connection may still be active. Try using the device or reconnect if needed.\n");
                    }
                    
                    break;
                }
                case 2: {
                    // Download firmware from GitHub and update
                    
                    // Check if device is connected
                    if (!gUcxConnected) {
                        printf("ERROR: Device not connected. Please connect first.\n");
                        break;
                    }
                    
                    char firmwarePath[512];
                    if (!downloadFirmwareFromGitHubInteractive(firmwarePath, sizeof(firmwarePath))) {
                        printf("\nFirmware download cancelled or failed.\n");
                        break;
                    }
                    
                    printf("\nFirmware downloaded successfully!\n");
                    printf("Path: %s\n", firmwarePath);
                    printf("\nStarting firmware update...\n");
                    printf("This will take several minutes. Please wait...\n\n");
                    printf("NOTE: The connection will be closed and reopened for XMODEM transfer.\n");
                    printf("      The device will reboot after successful update.\n\n");
                    
                    // Perform firmware update with progress callback
                    int32_t result = uCxFirmwareUpdate(
                        &gUcxHandle,
                        firmwarePath,
                        gComPort,
                        115200,
                        false,  // No flow control
                        true,   // Use 1K blocks
                        firmwareUpdateProgress,
                        NULL
                    );
                    
                    if (result == 0) {
                        printf("\n\nFirmware update completed successfully!\n");
                        printf("The module is rebooting...\n");
                        printf("Waiting for +STARTUP URC");
                        fflush(stdout);
                        
                        // Wait up to 10 seconds for the +STARTUP URC
                        bool startupReceived = waitEvent(URC_FLAG_STARTUP, 10);
                        
                        if (startupReceived) {
                            printf(" Received!\n");
                        } else {
                            printf(" Timeout! Continuing anyway...\n");
                        }
                        fflush(stdout);
                        
                        // Disable echo again
                        printf("Disabling AT echo...\n");
                        result = uCxSystemSetEchoOff(&gUcxHandle);
                        if (result != 0) {
                            printf("Warning: Failed to disable echo (error %d), continuing...\n", result);
                        }
                        
                        // Re-query device information
                        printf("Querying new firmware version...\n");
                        
                        gDeviceModel[0] = '\0';
                        gDeviceFirmware[0] = '\0';
                        
                        const char *model = NULL;
                        if (uCxGeneralGetDeviceModelIdentificationBegin(&gUcxHandle, &model) && model != NULL) {
                            strncpy(gDeviceModel, model, sizeof(gDeviceModel) - 1);
                            gDeviceModel[sizeof(gDeviceModel) - 1] = '\0';
                            strncpy(gLastDeviceModel, model, sizeof(gLastDeviceModel) - 1);
                            gLastDeviceModel[sizeof(gLastDeviceModel) - 1] = '\0';
                            uCxEnd(&gUcxHandle);
                        } else {
                            uCxEnd(&gUcxHandle);
                        }
                        
                        const char *fwVersion = NULL;
                        if (uCxGeneralGetSoftwareVersionBegin(&gUcxHandle, &fwVersion) && fwVersion != NULL) {
                            strncpy(gDeviceFirmware, fwVersion, sizeof(gDeviceFirmware) - 1);
                            gDeviceFirmware[sizeof(gDeviceFirmware) - 1] = '\0';
                            uCxEnd(&gUcxHandle);
                        } else {
                            uCxEnd(&gUcxHandle);
                        }
                        
                        gUcxConnected = true;
                        
                        printf("\nFirmware update complete!\n");
                        if (gDeviceModel[0] != '\0' && gDeviceFirmware[0] != '\0') {
                            printf("Device: %s\n", gDeviceModel);
                            printf("New firmware version: %s\n", gDeviceFirmware);
                            printf("\nThe device is ready to use!\n");
                        } else {
                            printf("Note: Could not read new firmware version.\n");
                        }
                        
                        saveSettings();
                    } else {
                        printf("\n\nERROR: Firmware update failed with code %d\n", result);
                        printf("The connection may still be active. Try using the device or reconnect if needed.\n");
                    }
                    
                    break;
                }
                case 0:
                    gMenuState = MENU_MAIN;
                    break;
                default:
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        default:
            break;
    }
}

static void firmwareUpdateProgress(size_t totalBytes, size_t bytesTransferred, 
                                   int32_t percentComplete, void *pUserData)
{
    (void)pUserData;  // Unused
    
    // Show progress bar
    printf("\rFirmware update: [");
    int barWidth = 40;
    int pos = (barWidth * percentComplete) / 100;
    for (int i = 0; i < barWidth; i++) {
        if (i < pos) printf("=");
        else if (i == pos) printf(">");
        else printf(" ");
    }
    printf("] %d%% (%zu/%zu bytes)", percentComplete, bytesTransferred, totalBytes);
    fflush(stdout);
    
    if (percentComplete == 100) {
        printf("\n");
    }
}

// ============================================================================
// DEVICE CONNECTION & MANAGEMENT
// ============================================================================

// Common module initialization function
// Called after initial connection and after +STARTUP events
static void moduleStartupInit(void)
{
    int32_t result;
    
    // Configure module settings
    printf("Disabling echo...\n");
    result = uCxSystemSetEchoOff(&gUcxHandle);
    if (result == 0) {
        printf("Echo disabled.\n");
    } else {
        printf("Warning: Failed to disable echo (error %d)\n", result);
    }
    
    printf("Enabling extended error codes...\n");
    result = uCxSystemSetExtendedError(&gUcxHandle, U_SYS_EXTENDED_ERRORS_ON);
    if (result == 0) {
        printf("Extended errors enabled.\n");
    } else {
        printf("Warning: Failed to enable extended errors (error %d)\n", result);
    }
    
    // Read device information
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Device Information:");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "-------------------");
    
    // AT+GMI - Manufacturer identification
    const char *manufacturer = NULL;
    if (uCxGeneralGetManufacturerIdentificationBegin(&gUcxHandle, &manufacturer) && manufacturer != NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Manufacturer:     %s", manufacturer);
        uCxEnd(&gUcxHandle);
    } else {
        uCxEnd(&gUcxHandle);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Manufacturer:     (not available)");
    }
    
    // AT+GMM - Model identification
    const char *model = NULL;
    if (uCxGeneralGetDeviceModelIdentificationBegin(&gUcxHandle, &model) && model != NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Model:            %s", model);
        // Save model for menu display and settings
        strncpy(gDeviceModel, model, sizeof(gDeviceModel) - 1);
        gDeviceModel[sizeof(gDeviceModel) - 1] = '\0';
        strncpy(gLastDeviceModel, model, sizeof(gLastDeviceModel) - 1);
        gLastDeviceModel[sizeof(gLastDeviceModel) - 1] = '\0';
        uCxEnd(&gUcxHandle);
    } else {
        uCxEnd(&gUcxHandle);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Model:            (not available)");
        gDeviceModel[0] = '\0';
        gLastDeviceModel[0] = '\0';
    }
    
    // AT+GMR - Software version
    const char *fwVersion = NULL;
    if (uCxGeneralGetSoftwareVersionBegin(&gUcxHandle, &fwVersion) && fwVersion != NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Firmware Version: %s", fwVersion);
        // Save firmware version for menu display
        strncpy(gDeviceFirmware, fwVersion, sizeof(gDeviceFirmware) - 1);
        gDeviceFirmware[sizeof(gDeviceFirmware) - 1] = '\0';
        uCxEnd(&gUcxHandle);
    } else {
        uCxEnd(&gUcxHandle);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Firmware Version: (not available)");
        gDeviceFirmware[0] = '\0';
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "-------------------");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
}

static bool ucxclientConnect(const char *comPort)
{
    if (gUcxConnected) {
        printf("Already connected. Disconnect first.\n");
        return false;
    }
    
    printf("Connecting to %s...\n", comPort);
    
    // Initialize AT client (like windows_basic.c)
    uPortAtInit(&gUcxAtClient);
    
    // Open COM port
    if (!uPortAtOpen(&gUcxAtClient, comPort, 115200, false)) {
        printf("ERROR: Failed to open %s\n", comPort);
        return false;
    }
    
    printf("COM port opened successfully\n");
    
    
    // Initialize UCX handle
    uCxInit(&gUcxAtClient, &gUcxHandle);
    
    // Set startup callback (new API uses callback structure)
    gUcxHandle.callbacks.STARTUP = startupUrc;
    
    // Create mutex for URC event handling
    U_CX_MUTEX_CREATE(gUrcMutex);
    
    // Register all URC handlers
    enableAllUrcs();
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "UCX initialized successfully");

    // Perform common module initialization (echo, extended errors, device info)
    moduleStartupInit();
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Connected successfully!");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    
    gUcxConnected = true;
    return true;
}

static void ucxclientDisconnect(void)
{
    if (!gUcxConnected) {
        printf("Not connected.\n");
        return;
    }
    
    printf("Disconnecting...\n");
    
    // Unregister all URC handlers (set callbacks to NULL)
    // This is important for proper cleanup and prevents memory leaks
    
    // Unregister Wi-Fi link and network event handlers
    uCxWifiRegisterLinkUp(&gUcxHandle, NULL);
    uCxWifiRegisterLinkDown(&gUcxHandle, NULL);
    uCxWifiRegisterStationNetworkUp(&gUcxHandle, NULL);
    uCxWifiRegisterStationNetworkDown(&gUcxHandle, NULL);
    
    // Unregister Wi-Fi Access Point event handlers
    uCxWifiRegisterApNetworkUp(&gUcxHandle, NULL);
    uCxWifiRegisterApNetworkDown(&gUcxHandle, NULL);
    uCxWifiRegisterApUp(&gUcxHandle, NULL);
    uCxWifiRegisterApDown(&gUcxHandle, NULL);
    uCxWifiRegisterApStationAssociated(&gUcxHandle, NULL);
    uCxWifiRegisterApStationDisassociated(&gUcxHandle, NULL);
    
    // Unregister socket event handlers
    uCxSocketRegisterConnect(&gUcxHandle, NULL);
    uCxSocketRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister SPS event handlers
    uCxSpsRegisterConnect(&gUcxHandle, NULL);
    uCxSpsRegisterDisconnect(&gUcxHandle, NULL);
    uCxSpsRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister system event handlers
    // uCxSystemRegisterStartup(&gUcxHandle, NULL);  // Removed from API
    
    // Unregister diagnostics/ping event handlers
    uCxDiagnosticsRegisterPingResponse(&gUcxHandle, NULL);
    uCxDiagnosticsRegisterPingComplete(&gUcxHandle, NULL);
    
    // Unregister MQTT event handlers
    uCxMqttRegisterConnect(&gUcxHandle, NULL);
    uCxMqttRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister Bluetooth event handlers
    uCxBluetoothRegisterConnect(&gUcxHandle, NULL);
    uCxBluetoothRegisterDisconnect(&gUcxHandle, NULL);
    
    // Unregister GATT Server event handlers
    uCxGattServerRegisterNotification(&gUcxHandle, NULL);
    
    printf("All URC handlers unregistered.\n");
    
    // Stop heartbeat thread if running
    if (gHeartbeatThread) {
        gHeartbeatThreadRunning = false;
        WaitForSingleObject(gHeartbeatThread, 2000);
        CloseHandle(gHeartbeatThread);
        gHeartbeatThread = NULL;
    }
    
    // Delete mutex
    U_CX_MUTEX_DELETE(gUrcMutex);

    // Deinitialize UCX handle
    uCxAtClientDeinit(&gUcxAtClient);
    
    // Close COM port
    uPortAtClose(&gUcxAtClient);
    
    // Clear device info
    gDeviceModel[0] = '\0';
    gDeviceFirmware[0] = '\0';
    
    // Clear Bluetooth connection tracking
    gBtConnectionCount = 0;
    memset(gBtConnections, 0, sizeof(gBtConnections));
    
    gUcxConnected = false;
    printf("Disconnected.\n");
}

static bool quickConnectToLastDevice(void)
{
    if (gComPort[0] == '\0') {
        printf("No saved connection information.\n");
        return false;
    }
    
    printf("\n--- Quick Connect ---\n");
    printf("Attempting to reconnect to last device...\n");
    printf("Port: %s\n", gComPort);
    if (gLastDeviceModel[0] != '\0') {
        printf("Device: %s\n", gLastDeviceModel);
    }
    printf("\n");
    
    if (ucxclientConnect(gComPort)) {
        printf("\nQuick connect successful!\n");
        
        // If Wi-Fi profiles saved and this is a W3x device, offer to connect
        if (gWifiProfileCount > 0 && gDeviceModel[0] != '\0' && strstr(gDeviceModel, "W3") != NULL) {
            printf("\n%d Wi-Fi profile(s) available\n", gWifiProfileCount);
            printf("Connect to Wi-Fi? (y/n): ");
            char response[10];
            if (fgets(response, sizeof(response), stdin)) {
                if (tolower(response[0]) == 'y') {
                    wifiConnect();
                }
            }
        }
        
        return true;
    } else {
        printf("\nQuick connect failed. Please use [1] to connect manually.\n");
        return false;
    }
}

// ----------------------------------------------------------------
// Dynamic Product Firmware Path Functions
// ----------------------------------------------------------------

// Extract product name from firmware filename
// Examples:
//   "NORA-W36-3.1.0.zip" -> "NORA-W36"
//   "NORA-B26X-SW-3.0.1.bin" -> "NORA-B26"
//   "C:\path\to\NORA-W36-something.bin" -> "NORA-W36"
static char* extractProductFromFilename(const char *filename)
{
    static char product[64];
    product[0] = '\0';
    
    // Extract just the filename (remove path)
    const char *filenameOnly = strrchr(filename, '\\');
    if (!filenameOnly) filenameOnly = strrchr(filename, '/');
    if (filenameOnly) {
        filenameOnly++; // Skip the slash
    } else {
        filenameOnly = filename;
    }
    
    // Look for product name pattern: uppercase letters and hyphens followed by hyphen or 'X'
    // Examples: "NORA-W36", "NORA-B26", etc.
    // The pattern is typically: <NAME>-<MODEL><number>[-<more>]
    // We want to extract up to and including the first number sequence
    
    int i = 0;
    int lastHyphen = -1;
    bool foundDigit = false;
    
    while (filenameOnly[i] && i < 63) {
        char c = filenameOnly[i];
        
        // Stop at common delimiters
        if (c == '.' || c == '_') {
            // If we've found at least one digit, this might be a good stopping point
            if (foundDigit && lastHyphen > 0 && lastHyphen < 64) {
                // Include up to the last hyphen before the digit section
                size_t copyLen = (lastHyphen < 63) ? lastHyphen : 63;
                strncpy(product, filenameOnly, copyLen);
                product[copyLen] = '\0';
                return product;
            }
            break;
        }
        
        // Track hyphens (but skip 'X' suffix like in NORA-W36X)
        if (c == '-') {
            lastHyphen = i;
        }
        
        // Track if we've seen digits
        if (isdigit(c)) {
            foundDigit = true;
        }
        
        // If we hit a hyphen after finding digits, and it's followed by more content,
        // we've likely found the product name (e.g., "NORA-W36-")
        if (c == '-' && foundDigit && filenameOnly[i+1]) {
            char next = filenameOnly[i+1];
            // If next char is digit or lowercase letter, this is likely version info
            if (isdigit(next) || (next >= 'a' && next <= 'z') || next == 'S') {
                // Product name ends before this hyphen - ensure we don't overflow
                if (i < 64) {
                    size_t copyLen = (i < 63) ? i : 63;
                    strncpy(product, filenameOnly, copyLen);
                    product[copyLen] = '\0';
                    
                    // Trim trailing 'X' if present (NORA-W36X -> NORA-W36)
                    size_t len = strlen(product);
                    if (len > 0 && product[len-1] == 'X') {
                        product[len-1] = '\0';
                    }
                    
                    return product;
                }
            }
        }
        
        product[i] = c;
        i++;
    }
    
    product[i] = '\0';
    
    // Trim trailing 'X' if present
    size_t len = strlen(product);
    if (len > 0 && product[len-1] == 'X') {
        product[len-1] = '\0';
    }
    
    return product;
}

// Get the last used firmware path for a product
static const char* getProductFirmwarePath(const char *productName)
{
    for (int i = 0; i < gProductFirmwarePathCount; i++) {
        if (strcmp(gProductFirmwarePaths[i].productName, productName) == 0) {
            return gProductFirmwarePaths[i].lastFirmwarePath;
        }
    }
    return ""; // Not found
}

// Set/update the firmware path for a product
static void setProductFirmwarePath(const char *productName, const char *firmwarePath)
{
    // Check if product already exists
    for (int i = 0; i < gProductFirmwarePathCount; i++) {
        if (strcmp(gProductFirmwarePaths[i].productName, productName) == 0) {
            // Update existing entry
            strncpy(gProductFirmwarePaths[i].lastFirmwarePath, firmwarePath, 
                    sizeof(gProductFirmwarePaths[i].lastFirmwarePath) - 1);
            gProductFirmwarePaths[i].lastFirmwarePath[sizeof(gProductFirmwarePaths[i].lastFirmwarePath) - 1] = '\0';
            return;
        }
    }
    
    // Add new entry if we have space
    if (gProductFirmwarePathCount < MAX_PRODUCT_PATHS) {
        strncpy(gProductFirmwarePaths[gProductFirmwarePathCount].productName, productName,
                sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].productName) - 1);
        gProductFirmwarePaths[gProductFirmwarePathCount].productName[sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].productName) - 1] = '\0';
        
        strncpy(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath, firmwarePath,
                sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath) - 1);
        gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath[sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath) - 1] = '\0';
        
        gProductFirmwarePathCount++;
    }
}

// Load settings from file
static void loadSettings(void)
{
    // Reset profile data
    gWifiProfileCount = 0;
    gActiveProfileIndex = -1;
    
    FILE *f = fopen(gSettingsFilePath, "r");
    if (f) {
        char line[256];
        int tempProfileCount = 0;
        
        while (fgets(line, sizeof(line), f)) {
            // Remove trailing newline/whitespace
            char *end = strchr(line, '\n');
            if (end) *end = '\0';
            end = strchr(line, '\r');
            if (end) *end = '\0';
            
            if (strncmp(line, "last_port=", 10) == 0) {
                strncpy(gComPort, line + 10, sizeof(gComPort) - 1);
                gComPort[sizeof(gComPort) - 1] = '\0';
                printf("Loaded last port from settings: %s\n", gComPort);
            }
            else if (strncmp(line, "last_device=", 12) == 0) {
                strncpy(gLastDeviceModel, line + 12, sizeof(gLastDeviceModel) - 1);
                gLastDeviceModel[sizeof(gLastDeviceModel) - 1] = '\0';
                if (strlen(gLastDeviceModel) > 0) {
                    printf("Loaded last device from settings: %s\n", gLastDeviceModel);
                }
            }
            else if (strncmp(line, "wifi_profile_count=", 19) == 0) {
                tempProfileCount = atoi(line + 19);
                if (tempProfileCount > MAX_WIFI_PROFILES) {
                    tempProfileCount = MAX_WIFI_PROFILES;
                }
            }
            else if (strncmp(line, "wifi_active_profile=", 20) == 0) {
                gActiveProfileIndex = atoi(line + 20);
            }
            else if (strncmp(line, "wifi_profile_", 13) == 0) {
                // Parse profile field: wifi_profile_N_field=value
                int profileIdx;
                char field[32];
                if (sscanf(line + 13, "%d_%31[^=]", &profileIdx, field) == 2) {
                    if (profileIdx >= 0 && profileIdx < MAX_WIFI_PROFILES) {
                        char *value = strchr(line, '=');
                        if (value) {
                            value++;  // Skip '='
                            
                            if (strcmp(field, "name") == 0) {
                                strncpy(gWifiProfiles[profileIdx].name, value, sizeof(gWifiProfiles[profileIdx].name) - 1);
                                gWifiProfiles[profileIdx].name[sizeof(gWifiProfiles[profileIdx].name) - 1] = '\0';
                            }
                            else if (strcmp(field, "ssid") == 0) {
                                strncpy(gWifiProfiles[profileIdx].ssid, value, sizeof(gWifiProfiles[profileIdx].ssid) - 1);
                                gWifiProfiles[profileIdx].ssid[sizeof(gWifiProfiles[profileIdx].ssid) - 1] = '\0';
                            }
                            else if (strcmp(field, "password") == 0) {
                                // Deobfuscate password
                                deobfuscatePassword(value, gWifiProfiles[profileIdx].password, sizeof(gWifiProfiles[profileIdx].password));
                            }
                            else if (strcmp(field, "ip_prefix") == 0) {
                                strncpy(gWifiProfiles[profileIdx].ipPrefix, value, sizeof(gWifiProfiles[profileIdx].ipPrefix) - 1);
                                gWifiProfiles[profileIdx].ipPrefix[sizeof(gWifiProfiles[profileIdx].ipPrefix) - 1] = '\0';
                            }
                        }
                    }
                }
            }
            else if (strncmp(line, "remote_address=", 15) == 0) {
                strncpy(gRemoteAddress, line + 15, sizeof(gRemoteAddress) - 1);
                gRemoteAddress[sizeof(gRemoteAddress) - 1] = '\0';
            }
            else if (strncmp(line, "combain_api_key=", 16) == 0) {
                // Deobfuscate Combain API key
                deobfuscatePassword(line + 16, gCombainApiKey, sizeof(gCombainApiKey));
                if (strlen(gCombainApiKey) > 0) {
                    printf("Loaded Combain API key from settings\n");
                }
            }
            else if (strncmp(line, "firmware_path_", 14) == 0) {
                // Dynamic firmware path: firmware_path_<PRODUCT>=<path>
                // e.g., "firmware_path_NORA-W36=/path/to/firmware.bin"
                char *equals = strchr(line + 14, '=');
                if (equals) {
                    // Extract product name (between "firmware_path_" and "=")
                    size_t productNameLen = equals - (line + 14);
                    if (productNameLen > 0 && productNameLen < 64) {
                        char productName[64];
                        strncpy(productName, line + 14, productNameLen);
                        productName[productNameLen] = '\0';
                        
                        // Convert underscores back to hyphens (NORA_W36 -> NORA-W36)
                        for (size_t i = 0; i < productNameLen; i++) {
                            if (productName[i] == '_') productName[i] = '-';
                        }
                        
                        // Store the firmware path for this product
                        setProductFirmwarePath(productName, equals + 1);
                    }
                }
            }
        }
        
        // Set profile count after loading all profiles
        gWifiProfileCount = tempProfileCount;
        if (gWifiProfileCount > 0) {
            printf("Loaded %d Wi-Fi profile(s)\n", gWifiProfileCount);
        }
        
        fclose(f);
    }
}

// Save settings to file
static void saveSettings(void)
{
    FILE *f = fopen(gSettingsFilePath, "w");
    if (f) {
        fprintf(f, "last_port=%s\n", gComPort);
        fprintf(f, "last_device=%s\n", gLastDeviceModel);
        fprintf(f, "remote_address=%s\n", gRemoteAddress);
        
        // Save Combain API key (obfuscated)
        if (strlen(gCombainApiKey) > 0) {
            char obfuscatedApiKey[256];
            obfuscatePassword(gCombainApiKey, obfuscatedApiKey, sizeof(obfuscatedApiKey));
            fprintf(f, "combain_api_key=%s\n", obfuscatedApiKey);
        }
        
        // Save WiFi profiles (up to 10)
        fprintf(f, "wifi_profile_count=%d\n", gWifiProfileCount);
        fprintf(f, "wifi_active_profile=%d\n", gActiveProfileIndex);
        for (int i = 0; i < gWifiProfileCount; i++) {
            fprintf(f, "wifi_profile_%d_name=%s\n", i, gWifiProfiles[i].name);
            fprintf(f, "wifi_profile_%d_ssid=%s\n", i, gWifiProfiles[i].ssid);
            
            // Obfuscate profile password
            char obfuscatedProfilePassword[128];
            obfuscatePassword(gWifiProfiles[i].password, obfuscatedProfilePassword, sizeof(obfuscatedProfilePassword));
            fprintf(f, "wifi_profile_%d_password=%s\n", i, obfuscatedProfilePassword);
            
            fprintf(f, "wifi_profile_%d_ip_prefix=%s\n", i, gWifiProfiles[i].ipPrefix);
        }
        
        // Save dynamic per-product firmware paths
        for (int i = 0; i < gProductFirmwarePathCount; i++) {
            if (gProductFirmwarePaths[i].productName[0] != '\0' && 
                gProductFirmwarePaths[i].lastFirmwarePath[0] != '\0') {
                // Convert hyphens to underscores for INI file compatibility
                // (NORA-W36 -> NORA_W36)
                char productKey[64];
                strncpy(productKey, gProductFirmwarePaths[i].productName, sizeof(productKey) - 1);
                productKey[sizeof(productKey) - 1] = '\0';
                for (size_t j = 0; j < strlen(productKey); j++) {
                    if (productKey[j] == '-') productKey[j] = '_';
                }
                fprintf(f, "firmware_path_%s=%s\n", productKey, gProductFirmwarePaths[i].lastFirmwarePath);
            }
        }
        
        fclose(f);
    }
}

// Initialize FTD2XX library (dynamic loading)
// Resource ID for embedded FTDI DLL (must match ucxclient_win64.rc)
#define IDR_FTD2XX_DLL 101

static bool initFtd2xxLibrary(void)
{
    if (gFtd2xxModule != NULL) {
        return true;  // Already loaded
    }
    
    char dllPath[MAX_PATH];
    bool dllExtracted = false;
    
    // Step 1: Try to extract embedded DLL from resources
    HRSRC hResource = FindResource(NULL, MAKEINTRESOURCE(IDR_FTD2XX_DLL), RT_RCDATA);
    if (hResource) {
        HGLOBAL hLoadedResource = LoadResource(NULL, hResource);
        if (hLoadedResource) {
            LPVOID pResourceData = LockResource(hLoadedResource);
            DWORD dwResourceSize = SizeofResource(NULL, hResource);
            
            if (pResourceData && dwResourceSize > 0) {
                // Extract to temp directory
                GetTempPath(MAX_PATH, dllPath);
                strcat(dllPath, "ftd2xx64_embedded.dll");
                
                // Write DLL to temp file
                HANDLE hFile = CreateFile(dllPath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
                if (hFile != INVALID_HANDLE_VALUE) {
                    DWORD dwBytesWritten;
                    if (WriteFile(hFile, pResourceData, dwResourceSize, &dwBytesWritten, NULL)) {
                        if (dwBytesWritten == dwResourceSize) {
                            dllExtracted = true;
                            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Extracted embedded FTDI DLL to: %s", dllPath);
                        }
                    }
                    CloseHandle(hFile);
                }
            }
        }
    }
    
    // Step 2: Try to load the extracted DLL
    if (dllExtracted) {
        gFtd2xxModule = LoadLibrary(dllPath);
        if (gFtd2xxModule != NULL) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Loaded embedded FTDI DLL successfully");
        }
    }
    
    // Step 3: Fallback - try external DLL file
    if (gFtd2xxModule == NULL) {
        // Try to load ftd2xx64.dll from executable directory
        GetModuleFileName(NULL, dllPath, sizeof(dllPath));
        char *lastSlash = strrchr(dllPath, '\\');
        if (lastSlash) {
            *(lastSlash + 1) = '\0';
            strcat(dllPath, "ftd2xx64.dll");
        }
        
        gFtd2xxModule = LoadLibrary(dllPath);
        if (gFtd2xxModule == NULL) {
            // Try current directory
            gFtd2xxModule = LoadLibrary("ftd2xx64.dll");
        }
        
        if (gFtd2xxModule != NULL) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Loaded external FTDI DLL");
        }
    }
    
    if (gFtd2xxModule == NULL) {
        return false;
    }
    
    // Load function pointers
    gpFT_ListDevices = (PFN_FT_ListDevices)GetProcAddress(gFtd2xxModule, "FT_ListDevices");
    gpFT_Open = (PFN_FT_Open)GetProcAddress(gFtd2xxModule, "FT_Open");
    gpFT_OpenEx = (PFN_FT_OpenEx)GetProcAddress(gFtd2xxModule, "FT_OpenEx");
    gpFT_GetComPortNumber = (PFN_FT_GetComPortNumber)GetProcAddress(gFtd2xxModule, "FT_GetComPortNumber");
    gpFT_Close = (PFN_FT_Close)GetProcAddress(gFtd2xxModule, "FT_Close");
    
    if (!gpFT_ListDevices || !gpFT_Open || !gpFT_GetComPortNumber || !gpFT_Close) {
        FreeLibrary(gFtd2xxModule);
        gFtd2xxModule = NULL;
        return false;
    }
    
    return true;
}

// Helper function to get FTDI device info using FTD2XX library
static bool getFtdiDeviceInfo(const char *portName, char *deviceDesc, size_t deviceDescSize, char *portLabel, size_t portLabelSize)
{
    DWORD numDevs = 0;
    FT_STATUS status;
    
    // Initialize outputs
    deviceDesc[0] = '\0';
    portLabel[0] = '\0';
    
    // Initialize library if not already done
    if (!initFtd2xxLibrary()) {
        return false;
    }
    
    // Get number of devices
    status = gpFT_ListDevices(&numDevs, NULL, FT_LIST_NUMBER_ONLY);
    if (status != FT_OK || numDevs == 0) {
        return false;
    }
    
    // Enumerate devices and find the one with matching COM port
    for (DWORD i = 0; i < numDevs; i++) {
        char description[256];
        
        // Get device description by index
        status = gpFT_ListDevices((PVOID)(DWORD_PTR)i, description, FT_LIST_BY_INDEX | FT_OPEN_BY_DESCRIPTION);
        if (status != FT_OK) {
            continue;
        }
        
        // Try to open the device to get its COM port number
        FT_HANDLE ftHandle;
        status = gpFT_Open(i, &ftHandle);
        if (status == FT_OK) {
            LONG comPortNumber = -1;
            status = gpFT_GetComPortNumber(ftHandle, &comPortNumber);
            
            gpFT_Close(ftHandle);
            
            if (status == FT_OK && comPortNumber > 0) {
                char comPortStr[16];
                snprintf(comPortStr, sizeof(comPortStr), "COM%ld", comPortNumber);
                
                if (strcmp(comPortStr, portName) == 0) {
                    // Found it!
                    strncpy(deviceDesc, description, deviceDescSize - 1);
                    deviceDesc[deviceDescSize - 1] = '\0';
                    
                    // Extract label from description
                    // Common formats: 
                    // "EVK-NORA-W36 A" -> A = AT command port
                    // "EVK-NORA-W36 B" -> B = (not used)
                    // "EVK-NORA-W36 C" -> C = LOG port
                    // "EVK-NORA-W36 D" -> D = (not used)
                    
                    // Check for single letter suffix (A, B, C, D)
                    size_t len = strlen(description);
                    if (len >= 2 && description[len-2] == ' ') {
                        char suffix = description[len-1];
                        if (suffix == 'A') {
                            strncpy(portLabel, "AT", portLabelSize - 1);
                            portLabel[portLabelSize - 1] = '\0';
                        } else if (suffix == 'C') {
                            strncpy(portLabel, "LOG", portLabelSize - 1);
                            portLabel[portLabelSize - 1] = '\0';
                        }
                    } else {
                        // Try to extract from " - XXX" format
                        char *dashPos = strrchr(description, '-');
                        if (dashPos) {
                            dashPos++;
                            while (*dashPos == ' ') dashPos++;
                            
                            if (strncmp(dashPos, "AT", 2) == 0 || strncmp(dashPos, "LOG", 3) == 0) {
                                strncpy(portLabel, dashPos, portLabelSize - 1);
                                portLabel[portLabelSize - 1] = '\0';
                                
                                // Remove any trailing spaces
                                char *end = portLabel + strlen(portLabel) - 1;
                                while (end > portLabel && *end == ' ') {
                                    *end = '\0';
                                    end--;
                                }
                            }
                        }
                    }
                    
                    return true;
                }
            }
        }
    }
    
    return false;
}

// Helper function to get device info using Windows SetupAPI
// This works without admin rights and for all COM port devices
static bool getDeviceInfoFromSetupAPI(const char *portName, char *deviceDesc, size_t deviceDescSize, char *portLabel, size_t portLabelSize)
{
    HDEVINFO deviceInfoSet;
    SP_DEVINFO_DATA deviceInfoData;
    DWORD i;
    bool found = false;
    
    // Initialize outputs
    deviceDesc[0] = '\0';
    portLabel[0] = '\0';
    
    // Get the device information set for all COM ports
    deviceInfoSet = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL, DIGCF_PRESENT);
    if (deviceInfoSet == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    // Enumerate through all devices
    deviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    for (i = 0; SetupDiEnumDeviceInfo(deviceInfoSet, i, &deviceInfoData); i++) {
        HKEY hDeviceRegistryKey;
        char portNameFromReg[256];
        DWORD dataType;
        DWORD dataSize;
        
        // Open the device registry key
        hDeviceRegistryKey = SetupDiOpenDevRegKey(deviceInfoSet, &deviceInfoData,
                                                   DICS_FLAG_GLOBAL, 0,
                                                   DIREG_DEV, KEY_QUERY_VALUE);
        
        if (hDeviceRegistryKey == INVALID_HANDLE_VALUE) {
            continue;
        }
        
        // Get the port name from registry
        dataSize = sizeof(portNameFromReg);
        if (RegQueryValueEx(hDeviceRegistryKey, "PortName", NULL, &dataType,
                           (LPBYTE)portNameFromReg, &dataSize) == ERROR_SUCCESS) {
            
            // Check if this is the port we're looking for
            if (strcmp(portNameFromReg, portName) == 0) {
                // Found it! Get the friendly name
                char friendlyName[256];
                if (SetupDiGetDeviceRegistryProperty(deviceInfoSet, &deviceInfoData,
                                                     SPDRP_FRIENDLYNAME,
                                                     &dataType,
                                                     (PBYTE)friendlyName,
                                                     sizeof(friendlyName),
                                                     &dataSize)) {
                    strncpy(deviceDesc, friendlyName, deviceDescSize - 1);
                    deviceDesc[deviceDescSize - 1] = '\0';
                    
                    // Try to extract port label from friendly name
                    // Format can be: "USB Serial Port (COM25) - AT" or "EVK NORA-W36 - AT (COM25)"
                    char *dashPos = strrchr(friendlyName, '-');
                    if (dashPos) {
                        // Skip whitespace after dash
                        dashPos++;
                        while (*dashPos == ' ') dashPos++;
                        
                        // Extract label
                        char *comPos = strstr(dashPos, " (COM");
                        if (comPos) {
                            size_t len = comPos - dashPos;
                            if (len >= portLabelSize) len = portLabelSize - 1;
                            strncpy(portLabel, dashPos, len);
                            portLabel[len] = '\0';
                        } else {
                            // Check for space or end of string
                            char *spacePos = strchr(dashPos, ' ');
                            if (spacePos) {
                                size_t len = spacePos - dashPos;
                                if (len >= portLabelSize) len = portLabelSize - 1;
                                strncpy(portLabel, dashPos, len);
                                portLabel[len] = '\0';
                            } else {
                                strncpy(portLabel, dashPos, portLabelSize - 1);
                                portLabel[portLabelSize - 1] = '\0';
                            }
                        }
                        
                    }
                    
                    found = true;
                }
            }
        }
        
        RegCloseKey(hDeviceRegistryKey);
        
        if (found) break;
    }
    
    SetupDiDestroyDeviceInfoList(deviceInfoSet);
    return found;
}

// List available COM ports
// Helper function to get friendly name for a COM port
static bool getComPortFriendlyName(const char *portName, char *friendlyName, size_t friendlyNameSize, char *portLabel, size_t portLabelSize)
{
    bool found = false;
    
    // Initialize output
    friendlyName[0] = '\0';
    portLabel[0] = '\0';
    
    // Search through multiple device paths
    const char *devicePaths[] = {
        "SYSTEM\\CurrentControlSet\\Enum\\FTDIBUS",
        "SYSTEM\\CurrentControlSet\\Enum\\USB",
        "SYSTEM\\CurrentControlSet\\Enum\\BTHENUM"
    };
    
    for (int pathIdx = 0; pathIdx < 3 && !found; pathIdx++) {
        HKEY hDeviceKey;
        
        LONG result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, devicePaths[pathIdx], 0, KEY_READ, &hDeviceKey);
        if (result != ERROR_SUCCESS) continue;
        
        // Enumerate device IDs
        DWORD deviceIndex = 0;
        char deviceId[256];
        while (RegEnumKey(hDeviceKey, deviceIndex++, deviceId, sizeof(deviceId)) == ERROR_SUCCESS) {
            // Enumerate instances under this device
            char instancePath[512];
            snprintf(instancePath, sizeof(instancePath), "%s\\%s", devicePaths[pathIdx], deviceId);
            
            HKEY hInstancesKey;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, instancePath, 0, KEY_READ, &hInstancesKey) == ERROR_SUCCESS) {
                DWORD instanceIndex = 0;
                char instanceId[256];
                
                while (RegEnumKey(hInstancesKey, instanceIndex++, instanceId, sizeof(instanceId)) == ERROR_SUCCESS) {
                    char fullPath[512];
                    snprintf(fullPath, sizeof(fullPath), "%s\\%s\\Device Parameters", instancePath, instanceId);
                    
                    HKEY hParamsKey;
                    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, fullPath, 0, KEY_READ, &hParamsKey) == ERROR_SUCCESS) {
                        BYTE portData[256];
                        DWORD portDataSize = sizeof(portData);
                        DWORD portType;
                        
                        // Check if this device has our COM port
                        if (RegQueryValueEx(hParamsKey, "PortName", NULL, &portType, portData, &portDataSize) == ERROR_SUCCESS) {
                            if (strcmp((char*)portData, portName) == 0) {
                                // Found our port! Get the friendly name and parent device info
                                char parentPath[512];
                                snprintf(parentPath, sizeof(parentPath), "%s\\%s", instancePath, instanceId);
                                
                                HKEY hDevKey;
                                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, parentPath, 0, KEY_READ, &hDevKey) == ERROR_SUCCESS) {
                                    BYTE nameData[256];
                                    DWORD nameDataSize = sizeof(nameData);
                                    DWORD nameType;
                                    
                                    // Get friendly name
                                    if (RegQueryValueEx(hDevKey, "FriendlyName", NULL, &nameType, nameData, &nameDataSize) == ERROR_SUCCESS) {
                                        strncpy(friendlyName, (char*)nameData, friendlyNameSize - 1);
                                        friendlyName[friendlyNameSize - 1] = '\0';
                                        
                                        // Try to extract port label from friendly name
                                        // Format can be: "USB Serial Port (COM25) - AT" or "EVK NORA-W36 - AT (COM25)"
                                        char *dashPos = strrchr(friendlyName, '-');
                                        if (dashPos) {
                                            // Skip whitespace after dash
                                            dashPos++;
                                            while (*dashPos == ' ') dashPos++;
                                            
                                            // Extract label (might have COM port after it)
                                            char *comPos = strstr(dashPos, " (COM");
                                            if (comPos) {
                                                size_t len = comPos - dashPos;
                                                if (len >= portLabelSize) len = portLabelSize - 1;
                                                strncpy(portLabel, dashPos, len);
                                                portLabel[len] = '\0';
                                            } else {
                                                strncpy(portLabel, dashPos, portLabelSize - 1);
                                                portLabel[portLabelSize - 1] = '\0';
                                            }
                                        }
                                        
                                        found = true;
                                    }
                                    
                                    // Also try to get device description (for parent device name)
                                    if (!found || friendlyName[0] == '\0') {
                                        nameDataSize = sizeof(nameData);
                                        if (RegQueryValueEx(hDevKey, "DeviceDesc", NULL, &nameType, nameData, &nameDataSize) == ERROR_SUCCESS) {
                                            strncpy(friendlyName, (char*)nameData, friendlyNameSize - 1);
                                            friendlyName[friendlyNameSize - 1] = '\0';
                                            found = true;
                                        }
                                    }
                                    
                                    RegCloseKey(hDevKey);
                                }
                                
                                // Try to get parent device name (e.g., "EVK NORA-W36")
                                if (found) {
                                    // Go up one level to get parent device
                                    char parentDevicePath[512];
                                    snprintf(parentDevicePath, sizeof(parentDevicePath), "%s", instancePath);
                                    
                                    // For FTDI devices, the parent might have the EVK name
                                    char *lastSlash = strrchr(parentDevicePath, '\\');
                                    if (lastSlash) {
                                        *lastSlash = '\0';  // Go up one level
                                        
                                        HKEY hParentKey;
                                        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, parentDevicePath, 0, KEY_READ, &hParentKey) == ERROR_SUCCESS) {
                                            // Enumerate parent's instances
                                            DWORD parentInstIndex = 0;
                                            char parentInstId[256];
                                            
                                            while (RegEnumKey(hParentKey, parentInstIndex++, parentInstId, sizeof(parentInstId)) == ERROR_SUCCESS) {
                                                char parentFullPath[512];
                                                snprintf(parentFullPath, sizeof(parentFullPath), "%s\\%s", parentDevicePath, parentInstId);
                                                
                                                HKEY hPDevKey;
                                                if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, parentFullPath, 0, KEY_READ, &hPDevKey) == ERROR_SUCCESS) {
                                                    BYTE pNameData[256];
                                                    DWORD pNameDataSize = sizeof(pNameData);
                                                    
                                                    if (RegQueryValueEx(hPDevKey, "FriendlyName", NULL, NULL, pNameData, &pNameDataSize) == ERROR_SUCCESS) {
                                                        // Check if this looks like the EVK name (e.g., "EVK NORA-W36")
                                                        char *parentName = (char*)pNameData;
                                                        if (strstr(parentName, "EVK") || strstr(parentName, "NORA")) {
                                                            // Prepend parent name to our friendly name
                                                            char temp[256];
                                                            strncpy(temp, friendlyName, sizeof(temp) - 1);
                                                            snprintf(friendlyName, friendlyNameSize, "%s", parentName);
                                                        }
                                                    }
                                                    
                                                    RegCloseKey(hPDevKey);
                                                    break;  // Found parent, no need to continue
                                                }
                                            }
                                            
                                            RegCloseKey(hParentKey);
                                        }
                                    }
                                }
                            }
                        }
                        
                        RegCloseKey(hParamsKey);
                    }
                    
                    if (found) break;
                }
                
                RegCloseKey(hInstancesKey);
            }
            
            if (found) break;
        }
        
        RegCloseKey(hDeviceKey);
    }
    
    return found;
}

// List available COM ports
static void listAvailableComPorts(char *recommendedPort, size_t recommendedPortSize, 
                                   char *recommendedDevice, size_t recommendedDeviceSize)
{
    HKEY hKey;
    LONG result;
    
    // Initialize recommended port and device
    if (recommendedPort && recommendedPortSize > 0) {
        recommendedPort[0] = '\0';
    }
    if (recommendedDevice && recommendedDeviceSize > 0) {
        recommendedDevice[0] = '\0';
    }
    
    // Open the registry key where COM ports are listed
    result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         "HARDWARE\\DEVICEMAP\\SERIALCOMM",
                         0,
                         KEY_READ,
                         &hKey);
    
    if (result == ERROR_SUCCESS) {
        DWORD index = 0;
        char valueName[256];
        DWORD valueNameSize;
        BYTE data[256];
        DWORD dataSize;
        DWORD type;
        int count = 0;
        
        printf("Available COM ports:\n");
        printf("%-8s %-12s %-40s %s\n", "Port", "Status", "Device", "Function");
        printf("%-8s %-12s %-40s %s\n", "--------", "------------", "----------------------------------------", "--------");
        
        // Enumerate all values in the registry key
        while (1) {
            valueNameSize = sizeof(valueName);
            dataSize = sizeof(data);
            
            result = RegEnumValue(hKey, index, valueName, &valueNameSize,
                                NULL, &type, data, &dataSize);
            
            if (result == ERROR_NO_MORE_ITEMS) {
                break;
            }
            
            if (result == ERROR_SUCCESS && type == REG_SZ) {
                count++;
                char *portName = (char*)data;
                
                // Try to open to see if it's available (not in use)
                char fullName[32];
                snprintf(fullName, sizeof(fullName), "\\\\.\\%s", portName);
                HANDLE hPort = CreateFile(fullName, GENERIC_READ | GENERIC_WRITE,
                                        0, NULL, OPEN_EXISTING, 0, NULL);
                
                const char *status;
                if (hPort != INVALID_HANDLE_VALUE) {
                    CloseHandle(hPort);
                    status = "Available";
                } else {
                    status = "In use";
                }
                
                // Try to get device info from FTDI library first, then SetupAPI
                char deviceDesc[256];
                char portLabel[64];
                bool gotInfo = false;
                
                // First try: FTDI library (for FTDI devices with detailed info)
                if (getFtdiDeviceInfo(portName, deviceDesc, sizeof(deviceDesc), 
                                     portLabel, sizeof(portLabel))) {
                    gotInfo = true;
                } else {
                    // Second try: Windows SetupAPI (for all devices)
                    gotInfo = getDeviceInfoFromSetupAPI(portName, deviceDesc, sizeof(deviceDesc), 
                                                         portLabel, sizeof(portLabel));
                }
                
                if (gotInfo) {
                    // Extract device name (before the COM port part)
                    char deviceName[256];
                    char *comPart = strstr(deviceDesc, " (COM");
                    if (comPart) {
                        size_t len = comPart - deviceDesc;
                        if (len >= sizeof(deviceName)) len = sizeof(deviceName) - 1;
                        strncpy(deviceName, deviceDesc, len);
                        deviceName[len] = '\0';
                    } else {
                        strncpy(deviceName, deviceDesc, sizeof(deviceName) - 1);
                        deviceName[sizeof(deviceName) - 1] = '\0';
                    }
                    
                    // Check if this is a NORA device and should be recommended
                    if (recommendedPort && recommendedPortSize > 0 && recommendedPort[0] == '\0') {
                        // Check if it's available and contains NORA-W36 or NORA-B26
                        const char *noraModel = NULL;
                        if (strstr(deviceDesc, "NORA-W36")) {
                            noraModel = "NORA-W36";
                        } else if (strstr(deviceDesc, "NORA-B26")) {
                            noraModel = "NORA-B26";
                        }
                        
                        if (hPort != INVALID_HANDLE_VALUE && noraModel) {
                            // Prefer AT port if available, otherwise any NORA port
                            if (portLabel[0] == '\0' || strcmp(portLabel, "AT") == 0) {
                                strncpy(recommendedPort, portName, recommendedPortSize - 1);
                                recommendedPort[recommendedPortSize - 1] = '\0';
                                
                                // Save recommended device model
                                if (recommendedDevice && recommendedDeviceSize > 0) {
                                    strncpy(recommendedDevice, noraModel, recommendedDeviceSize - 1);
                                    recommendedDevice[recommendedDeviceSize - 1] = '\0';
                                }
                            }
                        }
                    }
                    
                    printf("%-8s %-12s %-40s %s\n", 
                           portName, status, deviceName,
                           portLabel[0] ? portLabel : "-");
                } else {
                    printf("%-8s %-12s %-40s %s\n", 
                           portName, status, valueName, "-");
                }
            }
            
            index++;
        }
        
        RegCloseKey(hKey);
        
        if (count == 0) {
            printf("\nNo COM ports found in registry.\n");
        }
    } else {
        printf("ERROR: Could not access registry to enumerate COM ports.\n");
        printf("Falling back to simple scan...\n\n");
        
        // Fallback: try common port numbers
        int count = 0;
        for (int i = 1; i <= 256; i++) {
            char portName[32];
            snprintf(portName, sizeof(portName), "\\\\.\\COM%d", i);
            
            // Try to open the port to see if it exists
            HANDLE hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE,
                                    0, NULL, OPEN_EXISTING, 0, NULL);
            if (hPort != INVALID_HANDLE_VALUE) {
                CloseHandle(hPort);
                printf("  COM%d\n", i);
                count++;
            }
        }
        
        if (count == 0) {
            printf("  No COM ports found.\n");
        }
    }
}

// Select COM port from list
static char* selectComPortFromList(const char *recommendedPort)
{
    char input[64];
    char currentRecommendedPort[32] = "";
    char currentRecommendedDevice[64] = "";
    
    // Copy initial recommendation
    if (recommendedPort && recommendedPort[0] != '\0') {
        strncpy(currentRecommendedPort, recommendedPort, sizeof(currentRecommendedPort) - 1);
        currentRecommendedPort[sizeof(currentRecommendedPort) - 1] = '\0';
    }
    
    while (1) {
        if (currentRecommendedPort[0] != '\0') {
            printf("\nEnter COM port name, 'r' to refresh, or press Enter to use recommended [%s]: ", currentRecommendedPort);
        } else {
            printf("\nEnter COM port name, 'r' to refresh, or press Enter to use last saved port: ");
        }
        
        if (fgets(input, sizeof(input), stdin)) {
            // Remove trailing newline
            char *end = strchr(input, '\n');
            if (end) *end = '\0';
            
            // Check for refresh command
            if (strlen(input) == 1 && (input[0] == 'r' || input[0] == 'R')) {
                printf("\nRefreshing COM port list...\n\n");
                // Re-scan and update recommendation
                listAvailableComPorts(currentRecommendedPort, sizeof(currentRecommendedPort),
                                     currentRecommendedDevice, sizeof(currentRecommendedDevice));
                continue;  // Ask again with updated recommendation
            }
            
            // If user entered something, use it
            if (strlen(input) > 0) {
                char *result = (char*)malloc(strlen(input) + 1);
                if (result) {
                    strcpy(result, input);
                    return result;
                } else {
                    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate memory for COM port string");
                }
            }
            
            // User pressed Enter without input - use recommended port if available
            if (currentRecommendedPort[0] != '\0') {
                char *result = (char*)malloc(strlen(currentRecommendedPort) + 1);
                if (result) {
                    strcpy(result, currentRecommendedPort);
                    return result;
                } else {
                    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate memory for COM port string");
                }
            }
            
            // No recommendation and user pressed Enter - return NULL
            return NULL;
        }
        
        // fgets failed
        return NULL;
    }
}

// ============================================================================
// API COMMANDS (GITHUB INTEGRATION)
// ============================================================================

static void listAllApiCommands(void)
{
    printf("\n=============== UCX API Command Reference ===============\n\n");
    
    // Ask user for product
    printf("Select product:\n");
    printf("  [1] NORA-W36\n");
    printf("  [2] NORA-B26\n");
    printf("  [3] Use static list (offline)\n");
    printf("\nChoice: ");
    
    char input[128];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    int choice = atoi(input);
    const char *product = NULL;
    
    switch (choice) {
        case 1:
            product = "NORA-W36";
            break;
        case 2:
            product = "NORA-B26";
            break;
        case 3:
            // Fall through to static list
            break;
        default:
            printf("Invalid choice\n");
            return;
    }
    
    if (product) {
        // Ask for version or use latest
        printf("\nEnter version (e.g., 3.1.0) or press Enter to use latest: ");
        char version[32];
        if (fgets(version, sizeof(version), stdin)) {
            char *end = strchr(version, '\n');
            if (end) *end = '\0';
            
            char *versionToUse = NULL;
            
            if (strlen(version) == 0) {
                printf("Fetching latest version for %s...\n", product);
                versionToUse = fetchLatestVersion(product);
                if (versionToUse) {
                    printf("Latest version: %s\n", versionToUse);
                } else {
                    printf("ERROR: Failed to fetch latest version\n");
                    return;
                }
            } else {
                versionToUse = version;
            }
            
            // Fetch commands from GitHub
            if (fetchApiCommandsFromGitHub(product, versionToUse)) {
                printf("\n========== API Commands (%s %s) ==========\n\n", product, versionToUse);
                
                // Display commands with chapter markers
                char currentChapter[128] = "";
                int commandsInSection = 0;
                
                for (int i = 0; i < gApiCommandCount; i++) {
                    // Check if we've entered a new chapter
                    if (gApiCommands[i].chapter[0] != '\0' && 
                        strcmp(currentChapter, gApiCommands[i].chapter) != 0) {
                        
                        // Show total for previous chapter
                        if (currentChapter[0] != '\0' && commandsInSection > 0) {
                            printf("  (%d commands in this section)\n", commandsInSection);
                            commandsInSection = 0;
                        }
                        
                        // Print new chapter header (using UTF-8 box drawing)
                        printf("\n");
                        printf("\xE2\x95\x94");  // ╔ (UTF-8: E2 95 94)
                        for (int j = 0; j < 60; j++) printf("\xE2\x95\x90");  // ═ (UTF-8: E2 95 90)
                        printf("\xE2\x95\x97\n");  // ╗ (UTF-8: E2 95 97)
                        printf("\xE2\x95\x91  %-56s  \xE2\x95\x91\n", gApiCommands[i].chapter);  // ║ (UTF-8: E2 95 91)
                        printf("\xE2\x95\x9A");  // ╚ (UTF-8: E2 95 9A)
                        for (int j = 0; j < 60; j++) printf("\xE2\x95\x90");  // ═ (UTF-8: E2 95 90)
                        printf("\xE2\x95\x9D\n");  // ╝ (UTF-8: E2 95 9D)
                        printf("\n");
                        
                        strncpy(currentChapter, gApiCommands[i].chapter, sizeof(currentChapter) - 1);
                        currentChapter[sizeof(currentChapter) - 1] = '\0';
                    }
                    
                    // Display command
                    printf("  [%d] ", i + 1);
                    if (gApiCommands[i].ucxApi[0] != '\0') {
                        printf("%s\n", gApiCommands[i].ucxApi);
                    } else if (gApiCommands[i].atCommand[0] != '\0') {
                        printf("%s\n", gApiCommands[i].atCommand);
                    } else {
                        printf("(unnamed command)\n");
                    }
                    
                    if (gApiCommands[i].atCommand[0] != '\0') {
                        printf("      AT: %s\n", gApiCommands[i].atCommand);
                    }
                    if (gApiCommands[i].description[0] != '\0') {
                        printf("      %s\n", gApiCommands[i].description);
                    }
                    printf("\n");
                    
                    commandsInSection++;
                    
                    // Pause every 20 commands
                    if ((i + 1) % 20 == 0 && i + 1 < gApiCommandCount) {
                        printf("--- Press Enter for more commands ---");
                        getchar();
                    }
                }
                
                // Show total for last chapter
                if (commandsInSection > 0) {
                    printf("  (%d commands in this section)\n", commandsInSection);
                }
                
                printf("\n========================================\n");
                printf("Total: %d commands\n", gApiCommandCount);
            }
            
            if (versionToUse != version && versionToUse) {
                free(versionToUse);
            }
        }
    } else {
        // Static fallback list
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "GENERAL API (u_cx_general.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxGeneralGetManufacturerIdentificationBegin()  - Get manufacturer ID\n");
        printf("  uCxGeneralGetDeviceModelIdentificationBegin()   - Get device model\n");
        printf("  uCxGeneralGetSoftwareVersionBegin()             - Get software version\n");
        printf("  uCxGeneralGetIdentInfoBegin()                   - Get identification info\n");
        printf("  uCxGeneralGetSerialNumberBegin()                - Get device serial number\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "SYSTEM API (u_cx_system.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxSystemStoreConfiguration()                   - Store current config to flash\n");
        printf("  uCxSystemDefaultSettings()                      - Reset to factory defaults\n");
        printf("  uCxSystemReboot()                               - Reboot the module\n");
        printf("  uCxSystemGetLocalAddressBegin()                 - Get local MAC addresses\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "BLUETOOTH API (u_cx_bluetooth.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxBluetoothSetMode()                           - Set BT mode (off/classic/LE)\n");
        printf("  uCxBluetoothGetMode()                           - Get current BT mode\n");
        printf("  uCxBluetoothListConnectionsBegin()              - List active BT connections\n");
        printf("  uCxBluetoothDiscoverBegin()                     - Start device discovery\n");
        printf("  uCxBluetoothDiscoverGetNext()                   - Get next discovered device\n");
        printf("  uCxBluetoothConnect()                           - Connect to remote device\n");
        printf("  uCxBluetoothDisconnect()                        - Disconnect from device\n");
        printf("  uCxBluetoothGetBondingStatusBegin()             - Get bonded devices\n");
        printf("  uCxBluetoothSetPin()                            - Set PIN code\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "WI-FI API (u_cx_wifi.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxWifiStationSetConnectionParamsBegin()        - Set Wi-Fi connection params\n");
        printf("  uCxWifiStationConnectBegin()                    - Connect to Wi-Fi network\n");
        printf("  uCxWifiStationDisconnectBegin()                 - Disconnect from Wi-Fi\n");
        printf("  uCxWifiStationStatusBegin()                     - Get Wi-Fi connection status\n");
        printf("  uCxWifiStationScanDefaultBegin()                - Scan for Wi-Fi networks\n");
        printf("  uCxWifiStationScanDefaultGetNext()              - Get next scan result\n");
        printf("  uCxWifiApSetConnectionParamsBegin()             - Set AP mode params\n");
        printf("  uCxWifiApStartBegin()                           - Start AP mode\n");
        printf("  uCxWifiApStopBegin()                            - Stop AP mode\n");
        printf("  uCxWifiApGetStationListBegin()                  - List connected stations\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "SOCKET API (u_cx_socket.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxSocketCreate()                               - Create TCP/UDP socket\n");
        printf("  uCxSocketConnect()                              - Connect socket to remote\n");
        printf("  uCxSocketListen()                               - Listen for connections\n");
        printf("  uCxSocketAccept()                               - Accept incoming connection\n");
        printf("  uCxSocketClose()                                - Close socket\n");
        printf("  uCxSocketWrite()                                - Write data to socket\n");
        printf("  uCxSocketRead()                                 - Read data from socket\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "MQTT API (u_cx_mqtt.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxMqttConnectBegin()                           - Connect to MQTT broker\n");
        printf("  uCxMqttDisconnect()                             - Disconnect from broker\n");
        printf("  uCxMqttPublishBegin()                           - Publish message to topic\n");
        printf("  uCxMqttSubscribeBegin()                         - Subscribe to topic\n");
        printf("  uCxMqttUnsubscribeBegin()                       - Unsubscribe from topic\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "SECURITY API (u_cx_security.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxSecurityTlsCertificateStoreBegin()           - Store TLS certificate\n");
        printf("  uCxSecurityTlsCertificateRemove()               - Remove certificate\n");
        printf("  uCxSecurityTlsCertificateListBegin()            - List stored certificates\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "GATT CLIENT API (u_cx_gatt_client.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxGattClientDiscoverAllPrimaryServicesBegin()  - Discover GATT services\n");
        printf("  uCxGattClientDiscoverCharacteristicsBegin()     - Discover characteristics\n");
        printf("  uCxGattClientWriteCharacteristicBegin()         - Write to characteristic\n");
        printf("  uCxGattClientReadCharacteristicBegin()          - Read from characteristic\n");
        printf("  uCxGattClientSubscribeBegin()                   - Subscribe to notifications\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "GATT SERVER API (u_cx_gatt_server.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxGattServerAddServiceBegin()                  - Add GATT service\n");
        printf("  uCxGattServerAddCharacteristicBegin()           - Add characteristic\n");
        printf("  uCxGattServerSetCharacteristicValueBegin()      - Set characteristic value\n");
        printf("\n");
        
        printf("\n");
        printf("╔════════════════════════════════════════════════════════════╗\n");
        printf("║  %-56s  ║\n", "SPS API (u_cx_sps.h)");
        printf("╚════════════════════════════════════════════════════════════╝\n");
        printf("\n");
        printf("  uCxSpsConnect()                                 - Connect SPS channel\n");
        printf("  uCxSpsDisconnect()                              - Disconnect SPS channel\n");
        printf("  uCxSpsWrite()                                   - Write SPS data\n");
        printf("  uCxSpsRead()                                    - Read SPS data\n");
        printf("\n");
        
        printf("\n");
    }
}

// ============================================================================
// AT COMMANDS & DIAGNOSTICS
// ============================================================================

static void executeAtTest(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- AT Test ---\n");
    if (uCxLogIsEnabled()) {
        printf(">>> WATCH BELOW FOR AT TRAFFIC <<<\n");
        printf("\n");
        // Test if logging works at all
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Starting AT test - logging is enabled");
    }
    
    // Simple AT command using UCX API
    int32_t result = uCxGeneralAttention(&gUcxHandle);
    
    if (uCxLogIsEnabled()) {
        printf("\n");
    }
    
    if (result == 0) {
        printf("Result: OK - Device responded successfully\n");
    } else {
        printf("Result: ERROR (code %d)\n", result);
    }
}

static void executeAtTerminal(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    // Disable all URCs for clean terminal output
    printf("\nPreparing AT Terminal session...\n");
    disableAllUrcs();
    
    printf("\n");
    printf("\n");
    printf("                    AT COMMAND TERMINAL\n");
    printf("\n");
    printf("\n");
    printf("Interactive AT command terminal using UCX API.\n");
    printf("Commands are sent via uCxAtClientExecSimpleCmd() API.\n");
    printf("\n");
    printf("Note: You'll see [AT TX] and [AT RX] logging below.\n");
    printf("      This is normal and shows the AT command traffic.\n");
    printf("\n");
    printf("Commands:\n");
    printf("  Type complete AT command (e.g., 'ATI9' or 'AT+GMI')\n");
    printf("  [ENTER] sends 'AT' (module health check)\n");
    printf("  Type 'quit', 'exit', or 'q' to exit terminal\n");
    printf("  Type 'help' for more info, 'history' to see past commands\n");
    printf("\n");
    printf("\n");
    
    // Command history (simple circular buffer)
    #define MAX_HISTORY 20
    #define MAX_CMD_LEN 256
    char history[MAX_HISTORY][MAX_CMD_LEN];
    int historyCount = 0;
    
    while (true) {
        printf("\nAT> ");
        fflush(stdout);
        
        char input[MAX_CMD_LEN];
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        
        // Remove newline
        input[strcspn(input, "\n")] = 0;
        
        // Exit commands
        if (strcmp(input, "quit") == 0 || strcmp(input, "exit") == 0 || strcmp(input, "q") == 0) {
            printf("\nExiting AT Terminal...\n");
            break;
        }
        
        // Empty input = send AT (health check)
        if (strlen(input) == 0) {
            strcpy(input, "AT");
        }
        
        // Add to history (simple implementation - circular buffer)
        if (historyCount < MAX_HISTORY) {
            strncpy(history[historyCount], input, MAX_CMD_LEN - 1);
            history[historyCount][MAX_CMD_LEN - 1] = '\0';
            historyCount++;
        } else {
            // Shift history
            for (int i = 0; i < MAX_HISTORY - 1; i++) {
                strncpy(history[i], history[i + 1], MAX_CMD_LEN - 1);
            }
            strncpy(history[MAX_HISTORY - 1], input, MAX_CMD_LEN - 1);
            history[MAX_HISTORY - 1][MAX_CMD_LEN - 1] = '\0';
        }
        
        // Special commands
        if (strcmp(input, "help") == 0 || strcmp(input, "?") == 0) {
            printf("\nAT Terminal Help:\n");
            printf("  - Type complete AT commands (e.g., 'ATI9', 'AT+GMI')\n");
            printf("  - Can also use shortcuts: 'I9', '+GMI' (AT prefix added automatically)\n");
            printf("  - Press ENTER to send 'AT' (health check)\n");
            printf("  - Type 'quit', 'exit', or 'q' to exit terminal\n");
            printf("  - Type 'history' to see command history\n");
            continue;
        }
        
        if (strcmp(input, "history") == 0) {
            printf("\nCommand History:\n");
            for (int i = 0; i < historyCount; i++) {
                printf("  %2d: %s\n", i + 1, history[i]);
            }
            if (historyCount == 0) {
                printf("  (empty)\n");
            }
            continue;
        }
        
        // Determine if user typed full AT command or shorthand
        const char *atCommand;
        char fullCommand[MAX_CMD_LEN + 3];
        
        if (strncmp(input, "AT", 2) == 0 || strncmp(input, "at", 2) == 0) {
            // User typed full command with AT prefix
            atCommand = input;
        } else {
            // Add AT prefix automatically for convenience
            snprintf(fullCommand, sizeof(fullCommand), "AT%s", input);
            atCommand = fullCommand;
        }
        
        // Send command using UCX AT Client API
        printf("\nSending: %s\n", atCommand);
        
        int32_t result = uCxAtClientExecSimpleCmd(gUcxHandle.pAtClient, atCommand);
        
        if (result == 0) {
            printf("✓ OK\n");
        } else {
            printf("✗ ERROR (code %d)\n", result);
        }
    }
    
    // Re-enable URCs when exiting terminal
    printf("\nRestoring normal operation...\n");
    enableAllUrcs();
    printf("✓ Terminal session ended\n");
}

static void executeAti9(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- ATI9 Device Information ---\n");
    if (uCxLogIsEnabled()) {
        printf("\n");
    }
    
    // Use uCxGeneralGetIdentInfoBegin to get device info
    uCxGeneralGetIdentInfo_t info;
    
    if (uCxGeneralGetIdentInfoBegin(&gUcxHandle, &info)) {
        if (uCxLogIsEnabled()) {
            printf("\n");
        }
        printf("Application Version: %s\n", info.application_version);
        printf("Unique Identifier:   %s\n", info.unique_identifier);
        
        // Call uCxEnd to complete the command
        uCxEnd(&gUcxHandle);
    } else {
        if (uCxLogIsEnabled()) {
            printf("\n");
        }
        printf("ERROR: Failed to get device information\n");
    }
}

static void executeModuleReboot(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Module Reboot/Switch Off ---\n");
    
    // Clear any pending STARTUP flag and timestamp from previous operations
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags &= ~URC_FLAG_STARTUP;
    gStartupTimestamp = 0;
    U_CX_MUTEX_UNLOCK(gUrcMutex);
    
    // WORKAROUND for NORA-W36 firmware bug: Enable echo before AT+CPWROFF
    // Bug: When echo is OFF, module doesn't send OK before rebooting
    printf("Enabling echo (workaround for firmware bug)...\n");
    int32_t result = uCxSystemSetEchoOn(&gUcxHandle);
    if (result != 0) {
        printf("Warning: Failed to enable echo (error %d)\n", result);
    }
    
    printf("Sending AT+CPWROFF...\n");
    
    // Start timing immediately before sending the command
    ULONGLONG startTime = GetTickCount64();
    
    // Note: AT+CPWROFF sends OK response then immediately reboots
    result = uCxSystemReboot(&gUcxHandle);
    
    // AT+CPWROFF sends OK then immediately reboots
    // NOTE: NORA-W36 firmware bug - when echo is OFF, no OK is sent before reboot!
    if (result == 0) {
        printf("Module reboot initiated (OK received).\n");
        printf("Waiting for module to reboot");
        fflush(stdout);
        
        // Wait for +STARTUP URC
        if (waitEvent(URC_FLAG_STARTUP, 5)) {
            // Use the timestamp from when STARTUP URC was actually received
            ULONGLONG elapsedMs = gStartupTimestamp - startTime;
            
            printf(" done!\n");
            printf("Module has rebooted successfully.\n");
            printf("Reboot time: %llu ms (%.2f seconds)\n", elapsedMs, elapsedMs / 1000.0);
            
            // Reconfigure module after reboot using common initialization
            moduleStartupInit();
        } else {
            printf(" timeout!\n");
            printf("Module may have shut down completely (no +STARTUP received).\n");
        }
    } else if (result == -65536) {  // -65536 is timeout (shouldn't happen with echo ON)
        printf("Module reboot initiated (timeout - echo workaround may have failed).\n");
        printf("Waiting for module to reboot");
        fflush(stdout);
        
        // Wait for +STARTUP URC (should already be received during the timeout above)
        // Allow up to 5 additional seconds in case reboot is slower than expected
        if (waitEvent(URC_FLAG_STARTUP, 5)) {
            // Use the timestamp from when STARTUP URC was actually received
            ULONGLONG elapsedMs = gStartupTimestamp - startTime;
            
            printf(" done!\n");
            printf("Module has rebooted successfully.\n");
            printf("Reboot time: %llu ms (%.2f seconds)\n", elapsedMs, elapsedMs / 1000.0);
            
            // Reconfigure module after reboot using common initialization
            moduleStartupInit();
        } else {
            printf(" timeout!\n");
            printf("Module may have shut down completely (no +STARTUP received).\n");
        }
    } else {
        printf("ERROR: Failed to send AT+CPWROFF (error %d)\n", result);
    }
}

static void showLegacyAdvertisementStatus(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Legacy Advertisement Status ---\n");
    
    // Get advertisement information (enabled/disabled state)
    uCxBtGetAdvertiseInformation_t advInfo;
    int32_t result = uCxBluetoothGetAdvertiseInformation(&gUcxHandle, &advInfo);
    
    if (result != 0) {
        printf("ERROR: Failed to get advertisement information (error %d)\n", result);
        return;
    }
    
    printf("\nAdvertisement State:\n");
    printf("  Legacy Advertisement: %s\n", 
           advInfo.legacy_adv ? "ENABLED" : "DISABLED");
    printf("  Directed Advertisement: %s\n", 
           advInfo.directed_adv ? "ENABLED" : "DISABLED");
    
    // Display extended advertisements if any
    if (advInfo.extended_adv_list.length > 0) {
        printf("  Extended Advertisements: ");
        for (size_t i = 0; i < advInfo.extended_adv_list.length; i++) {
            printf("%d", advInfo.extended_adv_list.pIntValues[i]);
            if (i < advInfo.extended_adv_list.length - 1) {
                printf(", ");
            }
        }
        printf("\n");
    } else {
        printf("  Extended Advertisements: None\n");
    }
    
    // Get legacy advertisement configuration (interval settings)
    uCxBtGetLegacyAdvertisementConfig_t legacyConfig;
    result = uCxBluetoothGetLegacyAdvertisementConfig(&gUcxHandle, &legacyConfig);
    
    if (result != 0) {
        printf("ERROR: Failed to get legacy advertisement configuration (error %d)\n", result);
        return;
    }
    
    printf("\nLegacy Advertisement Configuration:\n");
    printf("  Interval Minimum: %d units (%.2f ms)\n", 
           legacyConfig.advertisement_interval_minimum,
           legacyConfig.advertisement_interval_minimum * 0.625);
    printf("  Interval Maximum: %d units (%.2f ms)\n", 
           legacyConfig.advertisement_interval_maximum,
           legacyConfig.advertisement_interval_maximum * 0.625);
    
    // Get and display advertisement data
    printf("\nAdvertisement Data:\n");
    uint8_t advDataBuffer[256];
    uByteArray_t advData;
    advData.pData = advDataBuffer;
    
    if (uCxBluetoothGetLegacyAdvertiseDataBegin(&gUcxHandle, &advData)) {
        if (advData.length > 0) {
            printf("  Raw data (%zu bytes): ", advData.length);
            for (size_t i = 0; i < advData.length; i++) {
                printf("%02X ", advData.pData[i]);
                if ((i + 1) % 16 == 0 && i < advData.length - 1) {
                    printf("\n                         ");
                }
            }
            printf("\n");
            
            // Parse and display AD structures
            printf("\n  Parsed AD Structures:\n");
            size_t pos = 0;
            while (pos < advData.length) {
                uint8_t len = advData.pData[pos];
                if (len == 0 || pos + len >= advData.length) {
                    break;
                }
                
                uint8_t type = advData.pData[pos + 1];
                printf("    [Type 0x%02X, Len %d] ", type, len - 1);
                
                // Decode common AD types
                switch (type) {
                    case 0x01: printf("Flags: 0x%02X", advData.pData[pos + 2]); break;
                    case 0x02: printf("Incomplete 16-bit UUIDs"); break;
                    case 0x03: printf("Complete 16-bit UUIDs"); break;
                    case 0x06: printf("Incomplete 128-bit UUIDs"); break;
                    case 0x07: printf("Complete 128-bit UUIDs"); break;
                    case 0x08: printf("Shortened Local Name: "); 
                              for (size_t i = 0; i < len - 1; i++) {
                                  printf("%c", advData.pData[pos + 2 + i]);
                              }
                              break;
                    case 0x09: printf("Complete Local Name: ");
                              for (size_t i = 0; i < len - 1; i++) {
                                  printf("%c", advData.pData[pos + 2 + i]);
                              }
                              break;
                    case 0x0A: printf("TX Power Level: %d dBm", (int8_t)advData.pData[pos + 2]); break;
                    case 0x16: printf("Service Data - 16-bit UUID"); break;
                    case 0xFF: printf("Manufacturer Specific Data"); break;
                    default: printf("Unknown Type"); break;
                }
                printf("\n");
                
                pos += len + 1;
            }
        } else {
            printf("  No advertisement data configured\n");
        }
        uCxEnd(&gUcxHandle);
    } else {
        printf("  ERROR: Failed to read advertisement data\n");
    }
    
    printf("\nNote: Use menu option [6] in Bluetooth menu to enable/disable advertising\n");
    printf("      Use menu options [8] for GATT examples (Heartbeat, HID over GATT)\n");
}

// Helper function to ensure legacy advertisements are enabled for GATT server
// Returns true if enabled successfully or already enabled, false on error
static bool ensureLegacyAdvertisementEnabled(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return false;
    }
    
    // Check current advertisement status
    uCxBtGetAdvertiseInformation_t advInfo;
    int32_t result = uCxBluetoothGetAdvertiseInformation(&gUcxHandle, &advInfo);
    
    if (result != 0) {
        printf("ERROR: Failed to get advertisement information (error %d)\n", result);
        return false;
    }
    
    // If already enabled, return success
    if (advInfo.legacy_adv) {
        printf("✓ Legacy advertisement already enabled\n");
        return true;
    }
    
    // Need to enable legacy advertisements
    printf("Legacy advertisement is currently DISABLED\n");
    printf("Enabling legacy advertisement for GATT server connections...\n");
    
    // Configure legacy advertisement intervals if needed
    // Use default intervals: 160 units (100ms) to 240 units (150ms)
    result = uCxBluetoothSetLegacyAdvertisementConfig(&gUcxHandle, 160, 240);
    if (result != 0) {
        printf("WARNING: Failed to configure advertisement intervals (error %d)\n", result);
        // Continue anyway - may already be configured
    }
    
    // Start legacy advertisements
    result = uCxBluetoothLegacyAdvertisementStart(&gUcxHandle);
    if (result != 0) {
        printf("ERROR: Failed to start legacy advertisement (error %d)\n", result);
        printf("       Remote devices will not be able to connect!\n");
        return false;
    }
    
    gBluetoothAdvertising = true;
    printf("✓ Legacy advertisement enabled - Device is now connectable\n");
    return true;
}

// Helper function to display GATT server connection information
static void showGattServerConnectionInfo(void)
{
    if (!gUcxConnected) {
        return;
    }
    
    // Get local Bluetooth name
    const char *pDeviceName = NULL;
    char deviceNameCopy[64] = {0};
    
    if (uCxBluetoothGetLocalNameBegin(&gUcxHandle, &pDeviceName)) {
        // Copy the name before calling uCxEnd (pointer may become invalid)
        if (pDeviceName) {
            strncpy(deviceNameCopy, pDeviceName, sizeof(deviceNameCopy) - 1);
        }
        // Must call uCxEnd before making another AT command
        uCxEnd(&gUcxHandle);
        
        // Get local Bluetooth address
        uMacAddress_t btAddr;
        if (uCxSystemGetLocalAddress(&gUcxHandle, U_INTERFACE_ID_BLUETOOTH, &btAddr) == 0) {
            printf("\n");
            printf("┌─────────────────────────────────────────────────────────────┐\n");
            printf("│  GATT Server Ready - Connect from GATT Client              │\n");
            printf("└─────────────────────────────────────────────────────────────┘\n");
            printf("\n");
            printf("  Device Name: %s\n", deviceNameCopy[0] ? deviceNameCopy : "Unknown");
            printf("  BT Address:  %02X:%02X:%02X:%02X:%02X:%02X\n",
                   btAddr.address[0], btAddr.address[1], btAddr.address[2],
                   btAddr.address[3], btAddr.address[4], btAddr.address[5]);
            printf("\n");
            printf("  Scan for this device from your BLE client app or device.\n");
            printf("\n");
        }
    }
}

// ============================================================================
// BLUETOOTH OPERATIONS
// ============================================================================

static void showBluetoothStatus(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("─────────────────────────────────────────────────\n");
    printf("BLUETOOTH STATUS\n");
    printf("─────────────────────────────────────────────────\n");
    
    // Get Bluetooth mode
    uBtMode_t btMode;
    int32_t result = uCxBluetoothGetMode(&gUcxHandle, &btMode);
    
    if (result == 0) {
        printf("Mode: ");
        switch (btMode) {
            case 0: printf("Disabled\n"); break;
            case 1: printf("Enabled (Central only)\n"); break;
            case 2: printf("Enabled (Peripheral only)\n"); break;
            case 3: printf("Enabled (Central + Peripheral)\n"); break;
            default: printf("Unknown (%d)\n", btMode); break;
        }
        
        if (btMode != 0) {
            // Get advertising information
            uCxBtGetAdvertiseInformation_t advInfo;
            if (uCxBluetoothGetAdvertiseInformation(&gUcxHandle, &advInfo) == 0) {
                printf("\nAdvertising Status:\n");
                printf("  Legacy Advertising:   %s\n", 
                       advInfo.legacy_adv ? "Enabled" : "Disabled");
                printf("  Directed Advertising: %s\n", 
                       advInfo.directed_adv ? "Enabled" : "Disabled");
                
                // Show legacy advertising configuration if enabled
                if (advInfo.legacy_adv) {
                    uCxBtGetLegacyAdvertisementConfig_t legacyConfig;
                    if (uCxBluetoothGetLegacyAdvertisementConfig(&gUcxHandle, &legacyConfig) == 0) {
                        printf("  Legacy Adv Interval:  %d-%d ms (%.1f-%.1f ms)\n",
                               legacyConfig.advertisement_interval_minimum,
                               legacyConfig.advertisement_interval_maximum,
                               legacyConfig.advertisement_interval_minimum * 0.625,
                               legacyConfig.advertisement_interval_maximum * 0.625);
                    }
                }
            }
            
            // List active connections
            printf("\nActive Connections:\n");
            
            uCxBluetoothListConnectionsBegin(&gUcxHandle);
            
            uCxBtListConnections_t conn;
            int connCount = 0;
            
            while (uCxBluetoothListConnectionsGetNext(&gUcxHandle, &conn)) {
                connCount++;
                printf("  [%d] %02X:%02X:%02X:%02X:%02X:%02X (%s)\n",
                       conn.conn_handle,
                       conn.bd_addr.address[0],
                       conn.bd_addr.address[1],
                       conn.bd_addr.address[2],
                       conn.bd_addr.address[3],
                       conn.bd_addr.address[4],
                       conn.bd_addr.address[5],
                       conn.bd_addr.type == U_BD_ADDRESS_TYPE_PUBLIC ? "Public" : "Random");
            }
            
            if (connCount == 0) {
                printf("  No active connections\n");
            } else {
                printf("\nTotal: %d connection%s\n", connCount, connCount > 1 ? "s" : "");
            }
            
            uCxEnd(&gUcxHandle);
        }
        
        printf("─────────────────────────────────────────────────\n");
    } else {
        printf("ERROR: Failed to get Bluetooth mode (code %d)\n", result);
        printf("─────────────────────────────────────────────────\n");
    }
}

// ============================================================================
// WIFI OPERATIONS
// ============================================================================

static void showWifiStatus(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Status ---\n");
    
    // Get connection status
    uCxWifiStationStatus_t status;
    
    // Check if connected
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &status)) {
        int32_t connState = status.rsp.StatusIdInt.int_val;
        uCxEnd(&gUcxHandle);
        
        if (connState == 2) {
            printf("Status: Connected\n");
            
            // Get SSID
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_SSID, &status)) {
                printf("SSID: %s\n", status.rsp.StatusIdStr.ssid);
                uCxEnd(&gUcxHandle);
            }
            
            // Get RSSI
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &status)) {
                int32_t rssi = status.rsp.StatusIdInt.int_val;
                if (rssi != -32768) {
                    printf("RSSI: %d dBm\n", rssi);
                }
                uCxEnd(&gUcxHandle);
            }
        } else {
            printf("Status: Not connected\n");
        }
    } else {
        printf("ERROR: Failed to get Wi-Fi status\n");
    }
}

// Decode Bluetooth advertising data based on Bluetooth SIG assigned numbers
// Reference: https://bitbucket.org/bluetooth-SIG/public/src/main/assigned_numbers/
static void decodeAdvertisingData(const uint8_t *data, size_t dataLen)
{
    if (!data || dataLen == 0) {
        return;
    }
    
    printf("  Advertising Data:\n");
    
    size_t offset = 0;
    while (offset < dataLen) {
        // Each AD structure: [length][type][data...]
        uint8_t length = data[offset];
        if (length == 0 || offset + length >= dataLen) {
            break; // Invalid or padding
        }
        
        uint8_t type = data[offset + 1];
        const uint8_t *adData = &data[offset + 2];
        size_t adDataLen = length - 1;
        
        // Decode based on AD type (Bluetooth SIG assigned numbers)
        switch (type) {
            case 0x01: // Flags
                printf("    Flags: 0x%02X", adData[0]);
                if (adData[0] & 0x01) printf(" [LE Limited Discoverable]");
                if (adData[0] & 0x02) printf(" [LE General Discoverable]");
                if (adData[0] & 0x04) printf(" [BR/EDR Not Supported]");
                if (adData[0] & 0x08) printf(" [Simultaneous LE and BR/EDR Controller]");
                if (adData[0] & 0x10) printf(" [Simultaneous LE and BR/EDR Host]");
                printf("\n");
                break;
                
            case 0x02: // Incomplete List of 16-bit Service UUIDs
            case 0x03: // Complete List of 16-bit Service UUIDs
                printf("    %s16-bit Service UUIDs:\n", type == 0x03 ? "Complete " : "Incomplete ");
                for (size_t i = 0; i < adDataLen; i += 2) {
                    if (i + 1 < adDataLen) {
                        uint16_t uuid = adData[i] | (adData[i + 1] << 8);
                        const char *serviceName = btGetServiceName(uuid);
                        printf("      0x%04X", uuid);
                        if (serviceName) {
                            printf(" (%s)", serviceName);
                        }
                        printf("\n");
                    }
                }
                break;
                
            case 0x04: // Incomplete List of 32-bit Service UUIDs
            case 0x05: // Complete List of 32-bit Service UUIDs
                printf("    %s32-bit Service UUIDs: ", type == 0x05 ? "Complete " : "Incomplete ");
                for (size_t i = 0; i < adDataLen; i += 4) {
                    if (i + 3 < adDataLen) {
                        uint32_t uuid = adData[i] | (adData[i + 1] << 8) | 
                                       (adData[i + 2] << 16) | (adData[i + 3] << 24);
                        printf("0x%08X ", uuid);
                    }
                }
                printf("\n");
                break;
                
            case 0x06: // Incomplete List of 128-bit Service UUIDs
            case 0x07: // Complete List of 128-bit Service UUIDs
                printf("    %s128-bit Service UUIDs:\n", type == 0x07 ? "Complete " : "Incomplete ");
                for (size_t i = 0; i < adDataLen; i += 16) {
                    if (i + 15 < adDataLen) {
                        printf("      ");
                        for (int j = 15; j >= 0; j--) {
                            printf("%02X", adData[i + j]);
                            if (j == 12 || j == 10 || j == 8 || j == 6) printf("-");
                        }
                        printf("\n");
                    }
                }
                break;
                
            case 0x08: // Shortened Local Name
            case 0x09: // Complete Local Name
                printf("    %sName: %.*s\n", 
                       type == 0x09 ? "Complete " : "Shortened ", 
                       (int)adDataLen, (const char*)adData);
                break;
                
            case 0x0A: // Tx Power Level
                printf("    TX Power: %d dBm\n", (int8_t)adData[0]);
                break;
                
            case 0x14: // List of 16-bit Service Solicitation UUIDs
            case 0x15: // List of 16-bit Service Solicitation UUIDs (complete)
                printf("    Service Solicitation UUIDs (16-bit): ");
                for (size_t i = 0; i < adDataLen; i += 2) {
                    if (i + 1 < adDataLen) {
                        uint16_t uuid = adData[i] | (adData[i + 1] << 8);
                        printf("0x%04X ", uuid);
                    }
                }
                printf("\n");
                break;
                
            case 0x1F: // List of 32-bit Service Solicitation UUIDs
                printf("    Service Solicitation UUIDs (32-bit): ");
                for (size_t i = 0; i < adDataLen; i += 4) {
                    if (i + 3 < adDataLen) {
                        uint32_t uuid = adData[i] | (adData[i + 1] << 8) | 
                                       (adData[i + 2] << 16) | (adData[i + 3] << 24);
                        printf("0x%08X ", uuid);
                    }
                }
                printf("\n");
                break;
                
            case 0x1C: // List of 128-bit Service Solicitation UUIDs
                printf("    Service Solicitation UUIDs (128-bit):\n");
                for (size_t i = 0; i < adDataLen; i += 16) {
                    if (i + 15 < adDataLen) {
                        printf("      ");
                        for (int j = 15; j >= 0; j--) {
                            printf("%02X", adData[i + j]);
                            if (j == 12 || j == 10 || j == 8 || j == 6) printf("-");
                        }
                        printf("\n");
                    }
                }
                break;
                
            case 0x16: // Service Data - 16-bit UUID
                if (adDataLen >= 2) {
                    uint16_t uuid = adData[0] | (adData[1] << 8);
                    const char *serviceName = btGetServiceName(uuid);
                    printf("    Service Data (UUID 0x%04X", uuid);
                    if (serviceName) {
                        printf(" - %s", serviceName);
                    }
                    printf("): ");
                    for (size_t i = 2; i < adDataLen; i++) {
                        printf("%02X ", adData[i]);
                    }
                    printf("\n");
                }
                break;
                
            case 0x20: // Service Data - 32-bit UUID
                if (adDataLen >= 4) {
                    uint32_t uuid = adData[0] | (adData[1] << 8) | 
                                   (adData[2] << 16) | (adData[3] << 24);
                    printf("    Service Data (UUID 0x%08X): ", uuid);
                    for (size_t i = 4; i < adDataLen; i++) {
                        printf("%02X ", adData[i]);
                    }
                    printf("\n");
                }
                break;
                
            case 0x21: // Service Data - 128-bit UUID
                if (adDataLen >= 16) {
                    printf("    Service Data (UUID: ");
                    for (int j = 15; j >= 0; j--) {
                        printf("%02X", adData[j]);
                        if (j == 12 || j == 10 || j == 8 || j == 6) printf("-");
                    }
                    printf("): ");
                    for (size_t i = 16; i < adDataLen && i < 32; i++) {
                        printf("%02X ", adData[i]);
                    }
                    if (adDataLen > 32) printf("...");
                    printf("\n");
                }
                break;
                
            case 0x24: // URI (Eddystone, Physical Web, etc.)
                printf("    URI: ");
                if (adDataLen > 0) {
                    // First byte is URI scheme prefix
                    const char *scheme = "";
                    switch (adData[0]) {
                        case 0x00: scheme = "aaa:"; break;
                        case 0x01: scheme = "aaas:"; break;
                        case 0x02: scheme = "about:"; break;
                        case 0x03: scheme = "acap:"; break;
                        case 0x04: scheme = "acct:"; break;
                        case 0x05: scheme = "cap:"; break;
                        case 0x06: scheme = "cid:"; break;
                        case 0x07: scheme = "coap:"; break;
                        case 0x08: scheme = "coaps:"; break;
                        case 0x09: scheme = "crid:"; break;
                        case 0x0A: scheme = "data:"; break;
                        case 0x0B: scheme = "dav:"; break;
                        case 0x0C: scheme = "dict:"; break;
                        case 0x0D: scheme = "dns:"; break;
                        case 0x0E: scheme = "file:"; break;
                        case 0x0F: scheme = "ftp:"; break;
                        case 0x10: scheme = "geo:"; break;
                        case 0x11: scheme = "go:"; break;
                        case 0x12: scheme = "gopher:"; break;
                        case 0x13: scheme = "h323:"; break;
                        case 0x14: scheme = "http:"; break;
                        case 0x15: scheme = "https:"; break;
                        case 0x16: scheme = "iax:"; break;
                        case 0x17: scheme = "icap:"; break;
                        case 0x18: scheme = "im:"; break;
                        case 0x19: scheme = "imap:"; break;
                        case 0x1A: scheme = "info:"; break;
                        case 0x1B: scheme = "ipp:"; break;
                        case 0x1C: scheme = "ipps:"; break;
                        case 0x1D: scheme = "iris:"; break;
                        case 0x1E: scheme = "iris.beep:"; break;
                        case 0x1F: scheme = "iris.xpc:"; break;
                        case 0x20: scheme = "iris.xpcs:"; break;
                        case 0x21: scheme = "iris.lwz:"; break;
                        case 0x22: scheme = "jabber:"; break;
                        case 0x23: scheme = "ldap:"; break;
                        case 0x24: scheme = "mailto:"; break;
                        case 0x25: scheme = "mid:"; break;
                        case 0x26: scheme = "msrp:"; break;
                        case 0x27: scheme = "msrps:"; break;
                        case 0x28: scheme = "mtqp:"; break;
                        case 0x29: scheme = "mupdate:"; break;
                        case 0x2A: scheme = "news:"; break;
                        case 0x2B: scheme = "nfs:"; break;
                        case 0x2C: scheme = "ni:"; break;
                        case 0x2D: scheme = "nih:"; break;
                        case 0x2E: scheme = "nntp:"; break;
                        case 0x2F: scheme = "opaquelocktoken:"; break;
                        case 0x30: scheme = "pop:"; break;
                        case 0x31: scheme = "pres:"; break;
                        case 0x32: scheme = "reload:"; break;
                        case 0x33: scheme = "rtsp:"; break;
                        case 0x34: scheme = "rtsps:"; break;
                        case 0x35: scheme = "rtspu:"; break;
                        case 0x36: scheme = "service:"; break;
                        case 0x37: scheme = "session:"; break;
                        case 0x38: scheme = "shttp:"; break;
                        case 0x39: scheme = "sieve:"; break;
                        case 0x3A: scheme = "sip:"; break;
                        case 0x3B: scheme = "sips:"; break;
                        case 0x3C: scheme = "sms:"; break;
                        case 0x3D: scheme = "snmp:"; break;
                        case 0x3E: scheme = "soap.beep:"; break;
                        case 0x3F: scheme = "soap.beeps:"; break;
                        case 0x40: scheme = "stun:"; break;
                        case 0x41: scheme = "stuns:"; break;
                        case 0x42: scheme = "tag:"; break;
                        case 0x43: scheme = "tel:"; break;
                        case 0x44: scheme = "telnet:"; break;
                        case 0x45: scheme = "tftp:"; break;
                        case 0x46: scheme = "thismessage:"; break;
                        case 0x47: scheme = "tn3270:"; break;
                        case 0x48: scheme = "tip:"; break;
                        case 0x49: scheme = "turn:"; break;
                        case 0x4A: scheme = "turns:"; break;
                        case 0x4B: scheme = "tv:"; break;
                        case 0x4C: scheme = "urn:"; break;
                        case 0x4D: scheme = "vemmi:"; break;
                        case 0x4E: scheme = "ws:"; break;
                        case 0x4F: scheme = "wss:"; break;
                        case 0x50: scheme = "xcon:"; break;
                        case 0x51: scheme = "xcon-userid:"; break;
                        case 0x52: scheme = "xmlrpc.beep:"; break;
                        case 0x53: scheme = "xmlrpc.beeps:"; break;
                        case 0x54: scheme = "xmpp:"; break;
                        case 0x55: scheme = "z39.50r:"; break;
                        case 0x56: scheme = "z39.50s:"; break;
                        default: scheme = "[Unknown scheme]"; break;
                    }
                    printf("%s", scheme);
                    // Rest is the URI body
                    for (size_t i = 1; i < adDataLen; i++) {
                        printf("%c", adData[i]);
                    }
                }
                printf("\n");
                break;
                
            case 0x19: // Appearance
                if (adDataLen >= 2) {
                    uint16_t appearance = adData[0] | (adData[1] << 8);
                    const char *appearanceName = btGetAppearanceName(appearance);
                    printf("    Appearance: 0x%04X", appearance);
                    if (appearanceName) {
                        printf(" (%s)", appearanceName);
                    }
                    printf("\n");
                }
                break;
                
            case 0xFF: // Manufacturer Specific Data
                if (adDataLen >= 2) {
                    uint16_t companyId = adData[0] | (adData[1] << 8);
                    const char *companyName = btGetCompanyName(companyId);
                    printf("    Manufacturer Data (Company ID: 0x%04X", companyId);
                    if (companyName) {
                        printf(" - %s", companyName);
                    }
                    printf("): ");
                    for (size_t i = 2; i < adDataLen && i < 22; i++) { // Limit output
                        printf("%02X ", adData[i]);
                    }
                    if (adDataLen > 22) printf("...");
                    printf("\n");
                }
                break;
                
            default:
                printf("    Type 0x%02X (%d bytes): ", type, (int)adDataLen);
                for (size_t i = 0; i < adDataLen && i < 16; i++) { // Limit output
                    printf("%02X ", adData[i]);
                }
                if (adDataLen > 16) printf("...");
                printf("\n");
                break;
        }
        
        offset += length + 1;
    }
}

static void bluetoothScan(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Device Scan ---\n");
    printf("Scanning for devices using default parameters...\n\n");
    
    // Set 30 second timeout for scan command
    uCxAtClientSetCommandTimeout(gUcxHandle.pAtClient, 30000, false);
    
    // Start discovery using default parameters (AT+UBTD with no parameters)
    uCxBluetoothDiscoveryDefaultBegin(&gUcxHandle);
    
    // Store unique devices (deduplicate by MAC address)
    #define MAX_BT_DEVICES 100
    #define MAX_ADV_DATA 256
    typedef struct {
        uBtLeAddress_t addr;
        char name[64];
        int8_t rssi;
        uint8_t advData[MAX_ADV_DATA];
        size_t advDataLen;
    } BtDevice_t;
    
    BtDevice_t devices[MAX_BT_DEVICES];
    memset(devices, 0, sizeof(devices));  // Clear array before use
    int deviceCount = 0;
    uCxBtDiscoveryDefault_t device;
    
    // Get discovered devices and deduplicate
    bool gotResponse;
    do {
        gotResponse = uCxBluetoothDiscoveryDefaultGetNext(&gUcxHandle, &device);
        if (gotResponse) {
            // Check if device already exists (compare MAC address)
            bool found = false;
            for (int i = 0; i < deviceCount; i++) {
                if (memcmp(devices[i].addr.address, device.bd_addr.address, 6) == 0 &&
                    devices[i].addr.type == device.bd_addr.type) {
                    // Device already exists - update RSSI if higher
                    if (device.rssi > devices[i].rssi) {
                        devices[i].rssi = (int8_t)device.rssi;
                    }
                    // Update name if we have a new name and stored name is empty, OR new name is longer
                    if (device.device_name && device.device_name[0] != '\0') {
                        if (devices[i].name[0] == '\0' || strlen(device.device_name) > strlen(devices[i].name)) {
                            strncpy(devices[i].name, device.device_name, sizeof(devices[i].name) - 1);
                            devices[i].name[sizeof(devices[i].name) - 1] = '\0';
                        }
                    }
                    // Store advertising data if available and not already stored
                    if (device.data.pData && device.data.length > 0 && devices[i].advDataLen == 0) {
                        size_t copyLen = device.data.length < MAX_ADV_DATA ? device.data.length : MAX_ADV_DATA;
                        memcpy(devices[i].advData, device.data.pData, copyLen);
                        devices[i].advDataLen = copyLen;
                    }
                    found = true;
                    break;
                }
            }
            
            // New device - add to list
            if (!found && deviceCount < MAX_BT_DEVICES) {
                memcpy(devices[deviceCount].addr.address, device.bd_addr.address, 6);
                devices[deviceCount].addr.type = device.bd_addr.type;
                devices[deviceCount].rssi = (int8_t)device.rssi;
                if (device.device_name && device.device_name[0] != '\0') {
                    strncpy(devices[deviceCount].name, device.device_name, sizeof(devices[deviceCount].name) - 1);
                    devices[deviceCount].name[sizeof(devices[deviceCount].name) - 1] = '\0';
                } else {
                    devices[deviceCount].name[0] = '\0';
                }
                // Store advertising data
                if (device.data.pData && device.data.length > 0) {
                    size_t copyLen = device.data.length < MAX_ADV_DATA ? device.data.length : MAX_ADV_DATA;
                    memcpy(devices[deviceCount].advData, device.data.pData, copyLen);
                    devices[deviceCount].advDataLen = copyLen;
                } else {
                    devices[deviceCount].advDataLen = 0;
                }
                deviceCount++;
            }
        }
    } while (gotResponse);
    
    uCxEnd(&gUcxHandle);
    
    // Sort devices by RSSI (strongest first, i.e., highest/least negative value)
    for (int i = 0; i < deviceCount - 1; i++) {
        for (int j = i + 1; j < deviceCount; j++) {
            // Sort by RSSI: higher is better (closer to 0 = stronger signal)
            if (devices[j].rssi > devices[i].rssi) {
                BtDevice_t temp = devices[i];
                devices[i] = devices[j];
                devices[j] = temp;
            }
        }
    }
    
    // Display unique devices
    if (deviceCount == 0) {
        printf("No devices found.\n");
    } else {
        printf("Found %d unique device(s):\n\n", deviceCount);
        for (int i = 0; i < deviceCount; i++) {
            printf("Device %d:\n", i + 1);
            printf("  Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
                   devices[i].addr.address[0], devices[i].addr.address[1],
                   devices[i].addr.address[2], devices[i].addr.address[3],
                   devices[i].addr.address[4], devices[i].addr.address[5]);
            
            if (devices[i].name[0] != '\0') {
                printf("  Name: %s\n", devices[i].name);
            }
            
            printf("  RSSI: %d dBm\n", devices[i].rssi);
            
            // Decode and display advertising data if available
            if (devices[i].advDataLen > 0) {
                decodeAdvertisingData(devices[i].advData, devices[i].advDataLen);
            }
            
            printf("\n");
        }
    }
}

static void bluetoothConnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Connect ---\n");
    
    // Show last connected device if available
    if (gRemoteAddress[0] != '\0') {
        // Extract just the MAC address for display (before comma if present)
        char displayAddr[32];
        strncpy(displayAddr, gRemoteAddress, sizeof(displayAddr) - 1);
        displayAddr[sizeof(displayAddr) - 1] = '\0';
        char *comma = strchr(displayAddr, ',');
        if (comma) *comma = '\0';
        
        printf("Last connected: %s\n", displayAddr);
        printf("Enter Bluetooth address (or press Enter to use last): ");
    } else {
        printf("Enter Bluetooth address (format: XX:XX:XX:XX:XX:XX): ");
    }
    
    char addrStr[64];
    if (fgets(addrStr, sizeof(addrStr), stdin)) {
        // Remove trailing newline
        char *end = strchr(addrStr, '\n');
        if (end) *end = '\0';
        
        // If user pressed Enter without input, use last address
        if (strlen(addrStr) == 0 && gRemoteAddress[0] != '\0') {
            strncpy(addrStr, gRemoteAddress, sizeof(addrStr) - 1);
            addrStr[sizeof(addrStr) - 1] = '\0';
        }
        
        // Parse the address
        uBtLeAddress_t addr;
        if (parseBluetoothAddress(addrStr, &addr)) {
            printf("Connecting to device...\n");
            
            // uCxBluetoothConnect returns conn handle on success, negative on error
            int32_t connHandle = uCxBluetoothConnect(&gUcxHandle, &addr);
            if (connHandle >= 0) {
                printf("Connected successfully! Connection handle: %d\n", connHandle);
                printf("Wait for +UEBTC URC to confirm connection...\n");
            } else {
                printf("ERROR: Failed to connect to device (error: %d)\n", connHandle);
            }
        } else {
            printf("ERROR: Invalid MAC address format\n");
            printf("Expected format: XX:XX:XX:XX:XX:XX or XX:XX:XX:XX:XX:XX,type\n");
        }
    }
}

static void bluetoothDisconnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gBtConnectionCount == 0) {
        printf("\n--- Bluetooth Disconnect ---\n");
        printf("No active Bluetooth connections.\n");
        return;
    }
    
    printf("\n--- Bluetooth Disconnect ---\n");
    printf("Active connections:\n");
    
    // List active connections
    for (int i = 0; i < gBtConnectionCount; i++) {
        printf("  [%d] Handle: %d, Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               i + 1,
               gBtConnections[i].handle,
               gBtConnections[i].address.address[0],
               gBtConnections[i].address.address[1],
               gBtConnections[i].address.address[2],
               gBtConnections[i].address.address[3],
               gBtConnections[i].address.address[4],
               gBtConnections[i].address.address[5]);
    }
    
    // Suggest default if only one connection
    if (gBtConnectionCount == 1) {
        printf("\nEnter connection number to disconnect [1]: ");
    } else {
        printf("\nEnter connection number to disconnect (1-%d): ", gBtConnectionCount);
    }
    
    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        // Trim newline
        input[strcspn(input, "\n")] = 0;
        
        int32_t index;
        
        // If empty input and only one connection, use it
        if (strlen(input) == 0 && gBtConnectionCount == 1) {
            index = 1;
            printf("Using connection 1\n");
        } else {
            index = atoi(input);
        }
        
        // Verify the index is valid
        if (index < 1 || index > gBtConnectionCount) {
            printf("ERROR: Invalid connection number %d (must be 1-%d)\n", index, gBtConnectionCount);
            return;
        }
        
        // Get the handle from the index (subtract 1 for 0-based array)
        int32_t handle = gBtConnections[index - 1].handle;
        
        printf("Disconnecting connection %d (handle %d)...\n", index, handle);
        
        // uCxBluetoothDisconnect returns 0 on success
        int32_t result = uCxBluetoothDisconnect(&gUcxHandle, handle);
        if (result == 0) {
            printf("Disconnect command sent successfully.\n");
            printf("Wait for +UEBTDC URC to confirm disconnection...\n");
        } else {
            printf("ERROR: Failed to disconnect (error: %d)\n", result);
        }
    }
}

static void bluetoothSyncConnections(void)
{
    if (!gUcxConnected) {
        return;
    }
    
    // Clear existing tracking
    gBtConnectionCount = 0;
    memset(gBtConnections, 0, sizeof(gBtConnections));
    
    // Query active connections from module
    uCxBluetoothListConnectionsBegin(&gUcxHandle);
    
    uCxBtListConnections_t conn;
    while (uCxBluetoothListConnectionsGetNext(&gUcxHandle, &conn)) {
        if (gBtConnectionCount < MAX_BT_CONNECTIONS) {
            gBtConnections[gBtConnectionCount].handle = conn.conn_handle;
            memcpy(&gBtConnections[gBtConnectionCount].address, &conn.bd_addr, sizeof(uBtLeAddress_t));
            gBtConnections[gBtConnectionCount].active = true;
            
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Synced BT connection: handle=%d, addr=%02X:%02X:%02X:%02X:%02X:%02X",
                         conn.conn_handle,
                         conn.bd_addr.address[0], conn.bd_addr.address[1], conn.bd_addr.address[2],
                         conn.bd_addr.address[3], conn.bd_addr.address[4], conn.bd_addr.address[5]);
            
            gBtConnectionCount++;
        }
    }
    
    uCxEnd(&gUcxHandle);
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Bluetooth sync complete: %d connection(s) tracked", gBtConnectionCount);
}

static void wifiScan(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Network Scan ---\n");
    printf("Scanning for networks... (this may take several seconds)\n\n");
    
    // Set 60 second timeout for scan command (scan can take 30-40 seconds for many networks)
    uCxAtClientSetCommandTimeout(gUcxHandle.pAtClient, 60000, false);
    
    // Start WiFi scan
    uCxWifiStationScanDefaultBegin(&gUcxHandle);
    
    int networkCount = 0;
    // Track both 2.4 GHz (channels 1-14) and 5 GHz channels (up to 165)
    int channelCount2_4[15] = {0}; // 2.4 GHz: channels 1-14
    int channelCount5[166] = {0};   // 5 GHz: channels up to 165
    uCxWifiStationScanDefault_t network;
    
    // Get scan results
    // Process responses until GetNext() returns false (no more responses or timeout/OK)
    // Note: Some firmware versions may send malformed URCs, so we continue even after errors
    bool gotResponse;
    do {
        gotResponse = uCxWifiStationScanDefaultGetNext(&gUcxHandle, &network);
        if (gotResponse) {
            networkCount++;
            printf("Network %d:\n", networkCount);
            // Display SSID (or "Hidden Network" if empty)
            if (network.ssid && network.ssid[0] != '\0') {
                printf("  SSID: %s\n", network.ssid);
            } else {
                printf("  SSID: <Hidden Network>\n");
            }
        printf("  BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
               network.bssid.address[0], network.bssid.address[1],
               network.bssid.address[2], network.bssid.address[3],
               network.bssid.address[4], network.bssid.address[5]);
        printf("  Channel: %d", network.channel);
        // Indicate band
        if (network.channel >= 1 && network.channel <= 14) {
            printf(" (2.4 GHz)\n");
        } else if (network.channel >= 36) {
            printf(" (5 GHz)\n");
        } else {
            printf("\n");
        }
        printf("  RSSI: %d dBm\n", network.rssi);
        
        // Track channel usage
        if (network.channel >= 1 && network.channel <= 14) {
            channelCount2_4[network.channel]++;
        } else if (network.channel >= 36 && network.channel <= 165) {
            channelCount5[network.channel]++;
        }
        
        // Print security type based on authentication suites
        printf("  Security: ");
        if (network.authentication_suites == 0) {
            printf("Open\n");
        } else {
            if (network.authentication_suites & (1 << 5)) printf("WPA3 ");
            if (network.authentication_suites & (1 << 4)) printf("WPA2 ");
            if (network.authentication_suites & (1 << 3)) printf("WPA ");
            if (network.authentication_suites & (1 << 1)) printf("PSK ");
        }
        printf("\n");
        }
    } while (gotResponse);
    
    uCxEnd(&gUcxHandle);
    
    if (networkCount == 0) {
        printf("No networks found.\n");
    } else {
        printf("Found %d network(s).\n", networkCount);
        
        // Display channel usage summary
        printf("\n==============================================================\n");
        printf("                   CHANNEL USAGE SUMMARY\n");
        printf("\n");
        
        // 2.4 GHz Band
        printf("\n2.4 GHz Band (Channels 1-14):\n");
        int maxNetworks2_4 = 0;
        int hasAny2_4 = 0;
        for (int i = 1; i <= 14; i++) {
            if (channelCount2_4[i] > 0) {
                hasAny2_4 = 1;
                if (channelCount2_4[i] > maxNetworks2_4) {
                    maxNetworks2_4 = channelCount2_4[i];
                }
            }
        }
        
        if (hasAny2_4) {
            for (int i = 1; i <= 14; i++) {
                if (channelCount2_4[i] > 0) {
                    printf("Channel %2d: %2d network(s) ", i, channelCount2_4[i]);
                    
                    // Visual bar graph
                    printf("[");
                    int barLen = (channelCount2_4[i] * 30) / (maxNetworks2_4 > 0 ? maxNetworks2_4 : 1);
                    for (int j = 0; j < barLen; j++) {
                        printf("=");
                    }
                    for (int j = barLen; j < 30; j++) {
                        printf(" ");
                    }
                    printf("]");
                    
                    // Congestion indicator
                    if (channelCount2_4[i] >= 5) {
                        printf(" - CONGESTED");
                    } else if (channelCount2_4[i] >= 3) {
                        printf(" - Busy");
                    } else if (channelCount2_4[i] == 1) {
                        printf(" - Clear");
                    }
                    printf("\n");
                }
            }
            
            // Recommend best 2.4 GHz channels (1, 6, 11 are non-overlapping)
            printf("\n** Best channel for creating a new 2.4 GHz access point **\n");
            printf("(Non-overlapping channels: 1, 6, 11)\n");
            int bestChannel = 0;
            int minCount = 999;
            int recommendedChannels[] = {1, 6, 11};
            for (int i = 0; i < 3; i++) {
                int ch = recommendedChannels[i];
                if (channelCount2_4[ch] < minCount) {
                    minCount = channelCount2_4[ch];
                    bestChannel = ch;
                }
            }
            if (bestChannel > 0) {
                printf("Least congested: Channel %d (%d existing network(s))\n", bestChannel, minCount);
            }
        } else {
            printf("No 2.4 GHz networks detected\n");
        }
        
        // 5 GHz Band
        printf("\n5 GHz Band:\n");
        int maxNetworks5 = 0;
        int hasAny5 = 0;
        for (int i = 36; i <= 165; i++) {
            if (channelCount5[i] > 0) {
                hasAny5 = 1;
                if (channelCount5[i] > maxNetworks5) {
                    maxNetworks5 = channelCount5[i];
                }
            }
        }
        
        if (hasAny5) {
            for (int i = 36; i <= 165; i++) {
                if (channelCount5[i] > 0) {
                    printf("Channel %3d: %2d network(s) ", i, channelCount5[i]);
                    
                    // Visual bar graph
                    printf("[");
                    int barLen = (channelCount5[i] * 30) / (maxNetworks5 > 0 ? maxNetworks5 : 1);
                    for (int j = 0; j < barLen; j++) {
                        printf("=");
                    }
                    for (int j = barLen; j < 30; j++) {
                        printf(" ");
                    }
                    printf("]");
                    
                    // Congestion indicator
                    if (channelCount5[i] >= 5) {
                        printf(" - CONGESTED");
                    } else if (channelCount5[i] >= 3) {
                        printf(" - Busy");
                    } else if (channelCount5[i] == 1) {
                        printf(" - Clear");
                    }
                    printf("\n");
                }
            }
            
            // Find best 5 GHz channel (only from valid channels that were detected)
            printf("\n** Best channel for creating a new 5 GHz access point **\n");
            printf("(5 GHz offers less interference and more bandwidth)\n");
            int best5GHz = 0;
            int min5Count = 999;
            
            // Valid 5 GHz channels (most common)
            int valid5GHzChannels[] = {36, 40, 44, 48, 52, 56, 60, 64, 
                                       100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
                                       149, 153, 157, 161, 165};
            int numValid5GHz = sizeof(valid5GHzChannels) / sizeof(valid5GHzChannels[0]);
            
            for (int i = 0; i < numValid5GHz; i++) {
                int ch = valid5GHzChannels[i];
                if (channelCount5[ch] < min5Count) {
                    min5Count = channelCount5[ch];
                    best5GHz = ch;
                }
            }
            
            if (best5GHz > 0 && min5Count < 999) {
                printf("Least congested: Channel %d (%d existing network(s))\n", best5GHz, min5Count);
            }
        } else {
            printf("No 5 GHz networks detected\n");
        }
        
        printf("\n");
    }
}

static void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel)
{
    printf("\n--- Testing Network Connectivity ---\n");
    
    // Variables to store results for summary
    int32_t localPingAvg = 0;
    int32_t internetPingAvg = 0;
    bool localSuccess = false;
    bool internetSuccess = false;
    
    // Warm-up: Single ping to gateway (helps wake up Wi-Fi from power save mode)
    printf("\nPerforming warm-up ping to gateway...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    if (uCxDiagnosticsPing2(&gUcxHandle, gateway, 1) == 0) {
        waitEvent(URC_FLAG_PING_COMPLETE, 10);
        printf("   Warm-up complete (%d ms)\n", gPingAvgTime);
    }
    
    // Test 1: Ping gateway (local network) - 4 pings
    printf("\n1. Testing local network (gateway: %s)...\n", gateway);
    printf("   Sending 4 pings...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    if (uCxDiagnosticsPing2(&gUcxHandle, gateway, 4) == 0) {
        // Wait for ping complete URC event (max 15 seconds for 4 pings)
        if (waitEvent(URC_FLAG_PING_COMPLETE, 15)) {
            if (gPingSuccess > 0) {
                localPingAvg = gPingAvgTime;
                localSuccess = true;
                printf("   ✓ Local network OK: %d/%d packets, avg %d ms\n", 
                       gPingSuccess, gPingSuccess + gPingFailed, gPingAvgTime);
            } else {
                printf("   ✗ Local network FAILED: No response from gateway\n");
            }
        } else {
            printf("   ✗ Local network test TIMEOUT\n");
        }
    } else {
        printf("   ✗ Failed to start ping test\n");
    }
    
    // Reset counters for second test
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    // Test 2: Ping Google DNS (internet connectivity) - 4 pings
    printf("\n2. Testing internet connectivity (8.8.8.8)...\n");
    printf("   Sending 4 pings...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    if (uCxDiagnosticsPing2(&gUcxHandle, "8.8.8.8", 4) == 0) {
        // Wait for ping complete URC event (max 15 seconds for 4 pings)
        if (waitEvent(URC_FLAG_PING_COMPLETE, 15)) {
            if (gPingSuccess > 0) {
                internetPingAvg = gPingAvgTime;
                internetSuccess = true;
                printf("   ✓ Internet access OK: %d/%d packets, avg %d ms\n", 
                       gPingSuccess, gPingSuccess + gPingFailed, gPingAvgTime);
            } else {
                printf("   ✗ Internet access FAILED: No response\n");
            }
        } else {
            printf("   ✗ Internet test TIMEOUT\n");
        }
    } else {
        printf("   ✗ Failed to start ping test\n");
    }
    
    // Display connection summary
    printf("\n");
    printf("\n");
    printf("                   CONNECTION SUMMARY\n");
    printf("\n");
    printf("Network:           %s\n", ssid);
    printf("Channel:           %d\n", channel);
    
    // Signal strength assessment
    const char *signalQuality;
    if (rssi >= -50) {
        signalQuality = "Excellent";
    } else if (rssi >= -60) {
        signalQuality = "Very Good";
    } else if (rssi >= -70) {
        signalQuality = "Good";
    } else if (rssi >= -80) {
        signalQuality = "Fair";
    } else {
        signalQuality = "Poor";
    }
    printf("Signal Strength:   %d dBm (%s)\n", rssi, signalQuality);
    
    // Local network ping
    if (localSuccess) {
        const char *localQuality = localPingAvg < 10 ? "Excellent" : 
                                   localPingAvg < 50 ? "Good" : "Fair";
        printf("Local Ping:        %d ms (%s)\n", localPingAvg, localQuality);
    } else {
        printf("Local Ping:        FAILED\n");
    }
    
    // Internet ping
    if (internetSuccess) {
        const char *internetQuality = internetPingAvg < 20 ? "Excellent" : 
                                      internetPingAvg < 50 ? "Very Good" :
                                      internetPingAvg < 100 ? "Good" : "Fair";
        printf("Internet Ping:     %d ms (%s)\n", internetPingAvg, internetQuality);
    } else {
        printf("Internet Ping:     FAILED\n");
    }
    
    // Overall assessment
    printf("\n");
    const char *overall;
    if (localSuccess && internetSuccess && rssi >= -70 && localPingAvg < 50 && internetPingAvg < 100) {
        overall = "EXCELLENT - Ready for all applications";
    } else if (localSuccess && internetSuccess && rssi >= -80) {
        overall = "GOOD - Suitable for most applications";
    } else if (localSuccess && rssi >= -80) {
        overall = "FAIR - Local network OK, check internet";
    } else if (localSuccess) {
        overall = "MARGINAL - Weak signal or connectivity issues";
    } else {
        overall = "POOR - Connection not stable";
    }
    printf("Overall:           %s\n", overall);
    printf("\n");
}

static void wifiConnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Connect ---\n");
    
    // Check if already connected and disconnect if necessary
    uCxWifiStationStatus_t connStatus;
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &connStatus)) {
        int32_t connState = connStatus.rsp.StatusIdInt.int_val;
        uCxEnd(&gUcxHandle);
        
        if (connState == 2) {  // 2 = Connected
            printf("Already connected to Wi-Fi. Disconnecting first...\n");
            
            if (uCxWifiStationDisconnect(&gUcxHandle) == 0) {
                printf("Disconnect command sent successfully.\n");
                Sleep(1000);  // Give module time to disconnect
            } else {
                printf("Warning: Disconnect command failed, attempting to connect anyway...\n");
            }
        }
    }
    
    char ssid[64] = "";
    char password[64] = "";
    bool useProfile = false;
    
    // Check for saved profiles and offer auto-suggestion
    if (gWifiProfileCount > 0) {
        // Try to suggest profile based on PC IP
        int suggestedIdx = wifiSuggestProfile();
        
        if (suggestedIdx >= 0) {
            char currentIP[40];
            getCurrentPCIPAddress(currentIP, sizeof(currentIP));
            printf("Auto-detected: Profile '%s' matches your network (%s)\n", 
                   gWifiProfiles[suggestedIdx].name, currentIP);
            printf("Use profile '%s' (SSID: %s)? (Y/n): ", 
                   gWifiProfiles[suggestedIdx].name, gWifiProfiles[suggestedIdx].ssid);
            
            char input[16];
            if (fgets(input, sizeof(input), stdin)) {
                if (strlen(input) == 0 || tolower(input[0]) == 'y' || input[0] == '\n') {
                    strncpy(ssid, gWifiProfiles[suggestedIdx].ssid, sizeof(ssid) - 1);
                    strncpy(password, gWifiProfiles[suggestedIdx].password, sizeof(password) - 1);
                    gActiveProfileIndex = suggestedIdx;
                    useProfile = true;
                    printf("Using profile '%s'\n", gWifiProfiles[suggestedIdx].name);
                }
            }
        }
        
        // If no auto-suggestion or user declined, offer profile selection
        if (!useProfile) {
            if (gActiveProfileIndex >= 0 && gActiveProfileIndex < gWifiProfileCount) {
                printf("Active profile: '%s' (SSID: %s)\n", 
                       gWifiProfiles[gActiveProfileIndex].name, 
                       gWifiProfiles[gActiveProfileIndex].ssid);
                printf("Use this profile? (Y/n/l=list): ");
            } else {
                printf("Saved profiles: %d\n", gWifiProfileCount);
                printf("Use a profile? (y/N/l=list): ");
            }
            
            char input[16];
            if (fgets(input, sizeof(input), stdin)) {
                input[strcspn(input, "\n")] = '\0';
                
                if (tolower(input[0]) == 'l') {
                    // List profiles and let user choose
                    wifiListProfiles();
                    printf("Select profile (1-%d, 0=manual): ", gWifiProfileCount);
                    if (fgets(input, sizeof(input), stdin)) {
                        int profileIdx = atoi(input) - 1;
                        if (profileIdx >= 0 && profileIdx < gWifiProfileCount) {
                            strncpy(ssid, gWifiProfiles[profileIdx].ssid, sizeof(ssid) - 1);
                            strncpy(password, gWifiProfiles[profileIdx].password, sizeof(password) - 1);
                            gActiveProfileIndex = profileIdx;
                            useProfile = true;
                            printf("Using profile '%s'\n", gWifiProfiles[profileIdx].name);
                        }
                    }
                } else if ((gActiveProfileIndex >= 0 && (strlen(input) == 0 || tolower(input[0]) == 'y')) ||
                           (gActiveProfileIndex < 0 && tolower(input[0]) == 'y')) {
                    // Use active profile or prompt for selection
                    if (gActiveProfileIndex >= 0) {
                        strncpy(ssid, gWifiProfiles[gActiveProfileIndex].ssid, sizeof(ssid) - 1);
                        strncpy(password, gWifiProfiles[gActiveProfileIndex].password, sizeof(password) - 1);
                        useProfile = true;
                        printf("Using profile '%s'\n", gWifiProfiles[gActiveProfileIndex].name);
                    } else {
                        wifiListProfiles();
                        printf("Select profile (1-%d): ", gWifiProfileCount);
                        if (fgets(input, sizeof(input), stdin)) {
                            int profileIdx = atoi(input) - 1;
                            if (profileIdx >= 0 && profileIdx < gWifiProfileCount) {
                                strncpy(ssid, gWifiProfiles[profileIdx].ssid, sizeof(ssid) - 1);
                                strncpy(password, gWifiProfiles[profileIdx].password, sizeof(password) - 1);
                                gActiveProfileIndex = profileIdx;
                                useProfile = true;
                                printf("Using profile '%s'\n", gWifiProfiles[profileIdx].name);
                            }
                        }
                    }
                }
            }
        }
    }
    
    // Manual entry if no profile used
    if (!useProfile) {
        printf("Enter SSID: ");
        if (fgets(ssid, sizeof(ssid), stdin)) {
            // Remove trailing newline
            char *end = strchr(ssid, '\n');
            if (end) *end = '\0';
            
            if (strlen(ssid) == 0) {
                printf("ERROR: SSID cannot be empty.\n");
                return;
            }
            
            printf("Enter password (or press Enter for open network): ");
            if (fgets(password, sizeof(password), stdin)) {
                // Remove trailing newline
                end = strchr(password, '\n');
                if (end) *end = '\0';
            }
        } else {
            printf("ERROR: Failed to read SSID.\n");
            return;
        }
    }
    
    // At this point we have ssid and password (either from profile or manual entry)
    if (strlen(ssid) == 0) {
        printf("ERROR: No SSID provided.\n");
        return;
    }
    
    printf("Connecting to '%s'...\n", ssid);
    
    // Set connection parameters (wlan_handle = 0, default)
    if (uCxWifiStationSetConnectionParams(&gUcxHandle, 0, ssid) != 0) {
        printf("ERROR: Failed to set connection parameters\n");
        return;
    }
    
    // Set security based on password
    if (strlen(password) > 0) {
        // WPA2/WPA3 with password (threshold = WPA2 or higher)
        printf("Setting WPA2/WPA3 security...\n");
        if (uCxWifiStationSetSecurityWpa(&gUcxHandle, 0, password, U_WIFI_WPA_THRESHOLD_WPA2) != 0) {
            printf("ERROR: Failed to set WPA security\n");
            return;
        }
    } else {
        // Open network (no password)
        printf("Setting open security (no password)...\n");
        if (uCxWifiStationSetSecurityOpen(&gUcxHandle, 0) != 0) {
            printf("ERROR: Failed to set open security\n");
            return;
        }
    }
    
    // Clear any pending network event flags before connecting
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags &= ~(URC_FLAG_NETWORK_UP | URC_FLAG_NETWORK_DOWN);
    U_CX_MUTEX_UNLOCK(gUrcMutex);
    
    // Initiate connection
    printf("Initiating connection...\n");
    if (uCxWifiStationConnect(&gUcxHandle, 0) != 0) {
        printf("ERROR: Failed to initiate connection\n");
        return;
    }
    
    // Wait for network up event (using URC handler)
    printf("Waiting for network up event...\n");
    if (waitEvent(URC_FLAG_NETWORK_UP, 20)) {
        printf("Successfully connected to '%s'\n", ssid);
        
        // Get RSSI
        int32_t rssi = -100;  // Default value
        uCxWifiStationStatus_t rssiStatus;
        if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &rssiStatus)) {
            rssi = rssiStatus.rsp.StatusIdInt.int_val;
            if (rssi != -32768) {
                printf("Signal strength: %d dBm\n", rssi);
            }
            uCxEnd(&gUcxHandle);
        }
        
        // Get IP address using WiFi Station Network Status (AT+UWSNST)
        uSockIpAddress_t ipAddr;
        char ipStr[40];  // Allow for IPv6 - stores the actual IP address
        char subnetStr[40] = "";  // Store subnet mask
        char gatewayStr[40] = "";  // Store gateway for ping test
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_WIFI_NET_STATUS_ID_IPV4, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                printf("IP address: %s\n", ipStr);
            }
        }
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_WIFI_NET_STATUS_ID_SUBNET, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, subnetStr, sizeof(subnetStr)) > 0) {
                printf("Subnet mask: %s\n", subnetStr);
            }
        }
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_WIFI_NET_STATUS_ID_GATE_WAY, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, gatewayStr, sizeof(gatewayStr)) > 0) {
                printf("Gateway: %s\n", gatewayStr);
            }
        }
        
        // Get channel number
        int32_t channel = 0;
        uCxWifiStationStatus_t channelStatus;
        if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CHANNEL, &channelStatus)) {
            channel = channelStatus.rsp.StatusIdInt.int_val;
            uCxEnd(&gUcxHandle);
        }
        
        // Test connectivity (ping gateway and internet) with connection summary
        if (strlen(gatewayStr) > 0) {
            testConnectivity(gatewayStr, ssid, rssi, channel);
        }
        
        // After connection summary, offer to save as profile
        if (!useProfile && gWifiProfileCount < MAX_WIFI_PROFILES) {
            printf("\nSave this network as a profile? (y/N): ");
            char saveInput[16];
            if (fgets(saveInput, sizeof(saveInput), stdin)) {
                if (tolower(saveInput[0]) == 'y') {
                    char profileName[32];
                    printf("Profile name (e.g., Office, Home, Lab): ");
                    if (fgets(profileName, sizeof(profileName), stdin)) {
                        profileName[strcspn(profileName, "\n")] = '\0';
                        if (strlen(profileName) > 0) {
                            // Extract IP prefix from connected IP (first 3 octets)
                            char ipPrefix[16] = "";
                            char *lastDot = strrchr(ipStr, '.');
                            if (lastDot) {
                                size_t prefixLen = lastDot - ipStr;
                                if (prefixLen < sizeof(ipPrefix)) {
                                    strncpy(ipPrefix, ipStr, prefixLen);
                                    ipPrefix[prefixLen] = '\0';
                                }
                            }
                            
                            wifiSaveProfile(profileName, ssid, password, ipPrefix);
                        }
                    }
                }
            }
        }
    } else {
        printf("Connection failed - timeout waiting for network up event (IP configuration)\n");
        printf("Wi-Fi link is established but network layer failed to initialize.\n");
    }
}

static void wifiDisconnect(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Disconnect ---\n");
    printf("Disconnecting from Wi-Fi...\n");
    
    // Clear any pending disconnect event flags
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags &= ~(URC_FLAG_NETWORK_DOWN | URC_FLAG_WIFI_LINK_DOWN);
    U_CX_MUTEX_UNLOCK(gUrcMutex);
    
    if (uCxWifiStationDisconnect(&gUcxHandle) == 0) {
        // Wait for Wi-Fi link down URC event (max 3 seconds)
        if (waitEvent(URC_FLAG_WIFI_LINK_DOWN, 3)) {
            printf("Disconnected successfully.\n");
        } else {
            printf("Disconnect command sent (waiting for confirmation timed out).\n");
        }
    } else {
        printf("ERROR: Failed to disconnect\n");
    }
}

// ============================================================================
// WI-FI ACCESS POINT FUNCTIONS
// ============================================================================

static void wifiApEnable(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    char ssid[64];
    char password[64];
    int32_t err;
    
    printf("\n");
    printf("\n");
    printf("                  ENABLE WI-FI ACCESS POINT\n");
    printf("\n");
    printf("\n");
    
    // Get SSID
    printf("Enter AP SSID (default: ucxclient Access Point): ");
    if (!fgets(ssid, sizeof(ssid), stdin)) {
        printf("ERROR: Failed to read SSID\n");
        return;
    }
    ssid[strcspn(ssid, "\r\n")] = 0;  // Remove newline
    
    if (strlen(ssid) == 0) {
        strcpy(ssid, "ucxclient Access Point");
        printf("Using default SSID: %s\n", ssid);
    }
    
    // Get password
    printf("Enter WPA2 password (default: 12345678): ");
    if (!fgets(password, sizeof(password), stdin)) {
        printf("ERROR: Failed to read password\n");
        return;
    }
    password[strcspn(password, "\r\n")] = 0;  // Remove newline
    
    if (strlen(password) == 0) {
        strcpy(password, "12345678");
        printf("Using default password: %s\n", password);
    } else if (strlen(password) < 8) {
        printf("ERROR: Password must be at least 8 characters\n");
        return;
    }
    
    printf("\n");
    printf("Configuring Access Point...\n");
    
    // Step 1: Set AP SSID and channel
    err = uCxWifiApSetConnectionParams2(&gUcxHandle, ssid, 6);
    if (err != 0) {
        printf("ERROR: Failed to set AP SSID and channel (error: %d)\n", err);
        return;
    }
    printf("✓ SSID set: %s\n", ssid);
    printf("✓ Channel set: 6\n");
    
    // Step 2: Set AP passphrase (WPA2)
    err = uCxWifiApSetSecurityWpa2(&gUcxHandle, password, U_WIFI_WPA_VERSION_WPA2);
    if (err != 0) {
        printf("ERROR: Failed to set AP passphrase (error: %d)\n", err);
        return;
    }
    printf("✓ WPA2 passphrase configured\n");
    printf("✓ Security mode: WPA2\n");
    
    // Step 3: Activate the Access Point
    printf("\n");
    printf("Activating Access Point...\n");
    err = uCxWifiApActivate(&gUcxHandle);
    if (err != 0) {
        printf("ERROR: Failed to activate AP (error: %d)\n", err);
        printf("Make sure Wi-Fi Station is disconnected before enabling AP.\n");
        return;
    }
    
    printf("\n");
    printf("✓ Access Point ENABLED successfully!\n");
    printf("\n");
    printf("\n");
    printf("  SSID:     %s\n", ssid);
    printf("  Password: %s\n", password);
    printf("  Security: WPA2\n");
    printf("\n");
    printf("\n");
    
    // Always show QR code after creating AP
    // Generate Wi-Fi QR code string
    char qrCodeString[256];
    snprintf(qrCodeString, sizeof(qrCodeString),
             "WIFI:T:WPA;S:%s;P:%s;;", ssid, password);
    
    printf("Generating QR Code...\n");
    printf("\n");
    
    // Generate QR code using qrcodegen library
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    
    bool ok = qrcodegen_encodeText(qrCodeString, tempBuffer, qrcode,
        qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO, true);
    
    if (ok) {
        int size = qrcodegen_getSize(qrcode);
        int border = 2;
        
        printf("\n");
        printf("                SCAN TO CONNECT TO WI-FI AP\n");
        printf("\n");
        printf("\n");
        
        // Print QR code to terminal using block characters
        for (int y = -border; y < size + border; y++) {
            for (int x = -border; x < size + border; x++) {
                bool module = qrcodegen_getModule(qrcode, x, y);
                printf(module ? "██" : "  ");
            }
            printf("\n");
        }
        
        printf("\n");
        printf("Scan this QR code with your smartphone to automatically\n");
        printf("connect to the Wi-Fi Access Point!\n");
        printf("\n");
    } else {
        printf("ERROR: Failed to generate QR code\n");
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void wifiApDisable(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Disable Wi-Fi Access Point ---\n");
    printf("Deactivating Access Point...\n");
    
    int32_t err = uCxWifiApDeactivate(&gUcxHandle);
    if (err == 0) {
        printf("✓ Access Point disabled successfully\n");
    } else {
        printf("ERROR: Failed to disable AP (error: %d)\n", err);
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void wifiApShowStatus(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("              WI-FI ACCESS POINT STATUS\n");
    printf("\n");
    printf("\n");
    
    // Get AP connection parameters (SSID and channel)
    uCxWifiApGetConnectionParams_t apParams;
    if (uCxWifiApGetConnectionParamsBegin(&gUcxHandle, &apParams)) {
        printf("SSID: %s\n", apParams.ssid);
        printf("Channel: %d\n", apParams.channel);
        uCxEnd(&gUcxHandle);
    } else {
        printf("Status: ERROR (failed to query AP parameters)\n");
    }
    
    // Get security settings
    uCxWifiApGetSecurity_t secInfo;
    if (uCxWifiApGetSecurity(&gUcxHandle, &secInfo) == 0) {
        printf("Security Mode: ");
        if (secInfo.type == U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE_WPA_VERSION) {
            printf("WPA (version: %d, mode: %d)\n", 
                   secInfo.rsp.SecurityModeWpaVersion.wpa_version,
                   secInfo.rsp.SecurityModeWpaVersion.security_mode);
        } else if (secInfo.type == U_CX_WIFI_AP_GET_SECURITY_RSP_TYPE_SECURITY_MODE) {
            printf("Mode: %d\n", secInfo.rsp.SecurityMode.security_mode);
        }
    }
    
    // Get network status (IP addresses)
    printf("\n");
    printf("Network Status:\n");
    uCxWifiApListNetworkStatusBegin(&gUcxHandle);
    uCxWifiApListNetworkStatus_t netStatus;
    while (uCxWifiApListNetworkStatusGetNext(&gUcxHandle, &netStatus)) {
        uSockIpAddress_t *ip = &netStatus.net_status_val;
        if (ip->type == U_SOCK_ADDRESS_TYPE_V4) {
            char ipStr[50];
            uint32_t ipv4 = ip->address.ipv4;
            sprintf(ipStr, "%d.%d.%d.%d",
                   (ipv4 & 0xFF), ((ipv4 >> 8) & 0xFF),
                   ((ipv4 >> 16) & 0xFF), ((ipv4 >> 24) & 0xFF));
            
            // Status ID meanings: 0=IPv4, 1=Subnet, 2=Gateway, 3=DNS
            const char *statusDesc[] = {"IPv4 Address", "Subnet Mask", "Gateway", "DNS Server"};
            if (netStatus.net_status_id >= 0 && netStatus.net_status_id <= 3) {
                printf("  %s: %s\n", statusDesc[netStatus.net_status_id], ipStr);
            } else {
                printf("  Status %d: %s\n", netStatus.net_status_id, ipStr);
            }
        }
    }
    uCxEnd(&gUcxHandle);
    
    // Get connected stations
    printf("\n");
    printf("Connected Stations:\n");
    uCxWifiApListStationsBegin(&gUcxHandle);
    uMacAddress_t staMac;
    int stationCount = 0;
    while (uCxWifiApListStationsGetNext(&gUcxHandle, &staMac)) {
        stationCount++;
        printf("  [%d] MAC: %02X:%02X:%02X:%02X:%02X:%02X\n",
               stationCount,
               staMac.address[0], staMac.address[1], staMac.address[2],
               staMac.address[3], staMac.address[4], staMac.address[5]);
    }
    uCxEnd(&gUcxHandle);
    
    if (stationCount == 0) {
        printf("  (No stations connected)\n");
    }
    
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void wifiApGenerateQrCode(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("           WI-FI ACCESS POINT - QR CODE GENERATOR\n");
    printf("\n");
    printf("\n");
    
    // Get SSID from AP connection parameters
    char ssid[64] = {0};
    uCxWifiApGetConnectionParams_t apParams;
    if (uCxWifiApGetConnectionParamsBegin(&gUcxHandle, &apParams)) {
        strncpy(ssid, apParams.ssid, sizeof(ssid) - 1);
        uCxEnd(&gUcxHandle);
    }
    
    if (strlen(ssid) == 0) {
        printf("ERROR: No SSID configured. Please enable Access Point first.\n");
        printf("\n");
        printf("Press Enter to continue...");
        getchar();
        return;
    }
    
    // Ask for password (can't retrieve it from module)
    char password[64];
    printf("Enter AP password: ");
    if (!fgets(password, sizeof(password), stdin)) {
        printf("ERROR: Failed to read password\n");
        return;
    }
    password[strcspn(password, "\r\n")] = 0;  // Remove newline
    
    // Generate Wi-Fi QR code string format:
    // WIFI:T:WPA;S:<SSID>;P:<password>;H:<hidden>;;
    // Example: WIFI:T:WPA;S:MyNetwork;P:MyPassword;;
    char qrCodeString[256];
    snprintf(qrCodeString, sizeof(qrCodeString),
             "WIFI:T:WPA;S:%s;P:%s;;", ssid, password);
    
    printf("\n");
    printf("Generating QR Code...\n");
    printf("\n");
    
    // Generate QR code using qrcodegen library
    uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
    uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
    
    bool ok = qrcodegen_encodeText(qrCodeString, tempBuffer, qrcode,
        qrcodegen_Ecc_LOW, qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX,
        qrcodegen_Mask_AUTO, true);
    
    if (!ok) {
        printf("ERROR: Failed to generate QR code\n");
        return;
    }
    
    int size = qrcodegen_getSize(qrcode);
    int border = 2;
    
    printf("\n");
    printf("                SCAN TO CONNECT TO WI-FI AP\n");
    printf("\n");
    printf("\n");
    printf("SSID: %s\n", ssid);
    printf("\n");
    
    // Print QR code to terminal using block characters
    // Using █ (full block) for dark modules and space for light modules
    for (int y = -border; y < size + border; y++) {
        for (int x = -border; x < size + border; x++) {
            // Print two characters per module for better aspect ratio
            bool module = qrcodegen_getModule(qrcode, x, y);
            printf(module ? "██" : "  ");
        }
        printf("\n");
    }
    
    printf("\n");
    printf("QR Code String: %s\n", qrCodeString);
    printf("\n");
    printf("Scan this QR code with your smartphone to automatically\n");
    printf("connect to the Wi-Fi Access Point!\n");
    printf("\n");
    printf("\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void wifiApConfigure(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n");
    printf("\n");
    printf("            CONFIGURE WI-FI ACCESS POINT SETTINGS\n");
    printf("\n");
    printf("\n");
    printf("Advanced configuration options:\n");
    printf("\n");
    printf("  [1] Set channel (1-13)\n");
    printf("  [2] Change SSID\n");
    printf("  [3] Change security (WPA2/Open)\n");
    printf("  [0] Cancel\n");
    printf("\n");
    printf("Choice: ");
    
    char input[64];
    if (!fgets(input, sizeof(input), stdin)) {
        return;
    }
    
    int choice = atoi(input);
    int32_t err;
    
    switch (choice) {
        case 1: {
            printf("Enter SSID: ");
            char ssid[64];
            if (fgets(ssid, sizeof(ssid), stdin)) {
                ssid[strcspn(ssid, "\r\n")] = 0;
                printf("Enter channel (1-13): ");
                char chInput[10];
                if (fgets(chInput, sizeof(chInput), stdin)) {
                    int channel = atoi(chInput);
                    if (channel >= 1 && channel <= 13) {
                        err = uCxWifiApSetConnectionParams2(&gUcxHandle, ssid, channel);
                        if (err == 0) {
                            printf("✓ SSID and channel updated\n");
                        } else {
                            printf("ERROR: Failed to set parameters (error: %d)\n", err);
                        }
                    } else {
                        printf("ERROR: Invalid channel. Must be 1-13.\n");
                    }
                }
            }
            break;
        }
        case 2: {
            printf("Enter new SSID: ");
            char ssid[64];
            if (fgets(ssid, sizeof(ssid), stdin)) {
                ssid[strcspn(ssid, "\r\n")] = 0;
                if (strlen(ssid) > 0) {
                    err = uCxWifiApSetConnectionParams1(&gUcxHandle, ssid);
                    if (err == 0) {
                        printf("✓ SSID updated to: %s\n", ssid);
                    } else {
                        printf("ERROR: Failed to set SSID (error: %d)\n", err);
                    }
                } else {
                    printf("ERROR: SSID cannot be empty\n");
                }
            }
            break;
        }
        case 3: {
            printf("\n");
            printf("Select security type:\n");
            printf("  [1] WPA2 (with password)\n");
            printf("  [2] Open (no password)\n");
            printf("Choice: ");
            char secChoice[10];
            if (fgets(secChoice, sizeof(secChoice), stdin)) {
                if (secChoice[0] == '1') {
                    printf("Enter WPA2 password (min 8 characters): ");
                    char password[64];
                    if (fgets(password, sizeof(password), stdin)) {
                        password[strcspn(password, "\r\n")] = 0;
                        if (strlen(password) >= 8) {
                            err = uCxWifiApSetSecurityWpa2(&gUcxHandle, password, U_WIFI_WPA_VERSION_WPA2);
                            if (err == 0) {
                                printf("✓ Security set to WPA2\n");
                            } else {
                                printf("ERROR: Failed to set WPA2 security (error: %d)\n", err);
                            }
                        } else {
                            printf("ERROR: Password must be at least 8 characters\n");
                        }
                    }
                } else if (secChoice[0] == '2') {
                    err = uCxWifiApSetSecurityOpen(&gUcxHandle);
                    if (err == 0) {
                        printf("✓ Security set to Open (no password)\n");
                        printf("WARNING: Open networks are not secure!\n");
                    } else {
                        printf("ERROR: Failed to set Open security (error: %d)\n", err);
                    }
                }
            }
            break;
        }
        default:
            printf("Cancelled.\n");
            break;
    }
    
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

// ============================================================================
// WIFI PROFILE MANAGEMENT (Save up to 10 network configurations)
// ============================================================================

// Get current PC's IP address to suggest profile
static void getCurrentPCIPAddress(char *ipBuffer, size_t bufferSize)
{
    ipBuffer[0] = '\0';
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }
    
    char hostname[256];
    if (gethostname(hostname, sizeof(hostname)) == 0) {
        struct addrinfo hints, *result;
        ZeroMemory(&hints, sizeof(hints));
        hints.ai_family = AF_INET;  // IPv4
        hints.ai_socktype = SOCK_STREAM;
        
        if (getaddrinfo(hostname, NULL, &hints, &result) == 0) {
            for (struct addrinfo *ptr = result; ptr != NULL; ptr = ptr->ai_next) {
                struct sockaddr_in *sockaddr_ipv4 = (struct sockaddr_in *)ptr->ai_addr;
                char *ip = inet_ntoa(sockaddr_ipv4->sin_addr);
                
                // Skip loopback addresses
                if (ip && strncmp(ip, "127.", 4) != 0) {
                    strncpy(ipBuffer, ip, bufferSize - 1);
                    ipBuffer[bufferSize - 1] = '\0';
                    break;
                }
            }
            freeaddrinfo(result);
        }
    }
    
    WSACleanup();
}

// Suggest WiFi profile based on current PC IP address
static int wifiSuggestProfile(void)
{
    char currentIP[40];
    getCurrentPCIPAddress(currentIP, sizeof(currentIP));
    
    if (currentIP[0] == '\0') {
        return -1;  // Couldn't get IP
    }
    
    // Extract first 3 octets as subnet (e.g., "192.168.1.x" -> "192.168.1")
    char subnet[16] = "";
    char *lastDot = strrchr(currentIP, '.');
    if (lastDot) {
        size_t subnetLen = lastDot - currentIP;
        if (subnetLen < sizeof(subnet)) {
            strncpy(subnet, currentIP, subnetLen);
            subnet[subnetLen] = '\0';
        }
    }
    
    // Find matching profile
    for (int i = 0; i < gWifiProfileCount; i++) {
        if (gWifiProfiles[i].ipPrefix[0] != '\0') {
            if (strcmp(gWifiProfiles[i].ipPrefix, subnet) == 0) {
                return i;  // Found matching profile
            }
        }
    }
    
    return -1;  // No matching profile
}

// List all WiFi profiles
static void wifiListProfiles(void)
{
    printf("\n=== Saved Wi-Fi Profiles ===\n");
    
    if (gWifiProfileCount == 0) {
        printf("No saved profiles.\n");
        return;
    }
    
    for (int i = 0; i < gWifiProfileCount; i++) {
        printf("[%d] %s\n", i + 1, gWifiProfiles[i].name);
        printf("    SSID: %s\n", gWifiProfiles[i].ssid);
        if (gWifiProfiles[i].ipPrefix[0] != '\0') {
            printf("    Expected IP prefix: %s.x\n", gWifiProfiles[i].ipPrefix);
        }
        if (i == gActiveProfileIndex) {
            printf("    [ACTIVE]\n");
        }
    }
    printf("\n");
}

// Save a WiFi profile
static void wifiSaveProfile(const char *name, const char *ssid, const char *password, const char *ipPrefix)
{
    if (gWifiProfileCount >= MAX_WIFI_PROFILES) {
        printf("ERROR: Maximum number of profiles (%d) reached.\n", MAX_WIFI_PROFILES);
        printf("Please delete a profile before adding a new one.\n");
        return;
    }
    
    // Check if profile with same name exists
    for (int i = 0; i < gWifiProfileCount; i++) {
        if (strcmp(gWifiProfiles[i].name, name) == 0) {
            printf("Profile '%s' already exists. Updating...\n", name);
            strncpy(gWifiProfiles[i].ssid, ssid, sizeof(gWifiProfiles[i].ssid) - 1);
            gWifiProfiles[i].ssid[sizeof(gWifiProfiles[i].ssid) - 1] = '\0';
            
            strncpy(gWifiProfiles[i].password, password, sizeof(gWifiProfiles[i].password) - 1);
            gWifiProfiles[i].password[sizeof(gWifiProfiles[i].password) - 1] = '\0';
            
            strncpy(gWifiProfiles[i].ipPrefix, ipPrefix, sizeof(gWifiProfiles[i].ipPrefix) - 1);
            gWifiProfiles[i].ipPrefix[sizeof(gWifiProfiles[i].ipPrefix) - 1] = '\0';
            
            saveSettings();
            printf("Profile '%s' updated successfully.\n", name);
            return;
        }
    }
    
    // Add new profile
    WiFiProfile_t *profile = &gWifiProfiles[gWifiProfileCount];
    
    strncpy(profile->name, name, sizeof(profile->name) - 1);
    profile->name[sizeof(profile->name) - 1] = '\0';
    
    strncpy(profile->ssid, ssid, sizeof(profile->ssid) - 1);
    profile->ssid[sizeof(profile->ssid) - 1] = '\0';
    
    strncpy(profile->password, password, sizeof(profile->password) - 1);
    profile->password[sizeof(profile->password) - 1] = '\0';
    
    strncpy(profile->ipPrefix, ipPrefix, sizeof(profile->ipPrefix) - 1);
    profile->ipPrefix[sizeof(profile->ipPrefix) - 1] = '\0';
    
    gWifiProfileCount++;
    saveSettings();
    
    printf("Profile '%s' added successfully. (Total: %d/%d)\n", name, gWifiProfileCount, MAX_WIFI_PROFILES);
}

// Manage WiFi profiles (add, edit, delete, select)
static void wifiManageProfiles(void)
{
    if (!gUcxConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    char input[128];
    
    while (1) {
        printf("\n=== Wi-Fi Profile Management ===\n");
        printf("[1] List all profiles\n");
        printf("[2] Add new profile\n");
        printf("[3] Edit profile\n");
        printf("[4] Delete profile\n");
        printf("[5] Select profile for connection\n");
        printf("[6] Auto-suggest profile (based on PC IP)\n");
        printf("[0] Back to Wi-Fi menu\n");
        printf("Choice: ");
        
        if (!fgets(input, sizeof(input), stdin)) {
            continue;
        }
        
        int choice = atoi(input);
        
        switch (choice) {
            case 0:
                return;
                
            case 1:  // List profiles
                wifiListProfiles();
                break;
                
            case 2: {  // Add new profile
                char name[32], ssid[64], password[64], ipPrefix[16];
                
                printf("\nAdd New Wi-Fi Profile\n");
                printf("Profile name (e.g., Office, Home, Lab): ");
                if (!fgets(name, sizeof(name), stdin)) break;
                name[strcspn(name, "\n")] = '\0';
                if (strlen(name) == 0) {
                    printf("ERROR: Profile name cannot be empty.\n");
                    break;
                }
                
                printf("SSID: ");
                if (!fgets(ssid, sizeof(ssid), stdin)) break;
                ssid[strcspn(ssid, "\n")] = '\0';
                if (strlen(ssid) == 0) {
                    printf("ERROR: SSID cannot be empty.\n");
                    break;
                }
                
                printf("Password (or press Enter for open network): ");
                if (!fgets(password, sizeof(password), stdin)) break;
                password[strcspn(password, "\n")] = '\0';
                
                printf("Expected IP prefix - first 3 octets (e.g., 10.12.4 or 192.168.1) [optional]: ");
                if (!fgets(ipPrefix, sizeof(ipPrefix), stdin)) break;
                ipPrefix[strcspn(ipPrefix, "\n")] = '\0';
                
                wifiSaveProfile(name, ssid, password, ipPrefix);
                break;
            }
                
            case 3: {  // Edit profile
                wifiListProfiles();
                if (gWifiProfileCount == 0) break;
                
                printf("Select profile to edit (1-%d): ", gWifiProfileCount);
                if (!fgets(input, sizeof(input), stdin)) break;
                int profileIdx = atoi(input) - 1;
                
                if (profileIdx < 0 || profileIdx >= gWifiProfileCount) {
                    printf("ERROR: Invalid profile number.\n");
                    break;
                }
                
                WiFiProfile_t *profile = &gWifiProfiles[profileIdx];
                char newValue[64];
                
                printf("\nEditing profile: %s\n", profile->name);
                printf("Current SSID: %s\n", profile->ssid);
                printf("New SSID (or press Enter to keep): ");
                if (fgets(newValue, sizeof(newValue), stdin)) {
                    newValue[strcspn(newValue, "\n")] = '\0';
                    if (strlen(newValue) > 0) {
                        strncpy(profile->ssid, newValue, sizeof(profile->ssid) - 1);
                        profile->ssid[sizeof(profile->ssid) - 1] = '\0';
                    }
                }
                
                printf("Current password: %s\n", strlen(profile->password) > 0 ? "****" : "(open)");
                printf("New password (or press Enter to keep): ");
                if (fgets(newValue, sizeof(newValue), stdin)) {
                    newValue[strcspn(newValue, "\n")] = '\0';
                    if (strlen(newValue) > 0) {
                        strncpy(profile->password, newValue, sizeof(profile->password) - 1);
                        profile->password[sizeof(profile->password) - 1] = '\0';
                    }
                }
                
                printf("Current IP prefix: %s\n", profile->ipPrefix[0] != '\0' ? profile->ipPrefix : "(none)");
                printf("New IP prefix (or press Enter to keep): ");
                if (fgets(newValue, sizeof(newValue), stdin)) {
                    newValue[strcspn(newValue, "\n")] = '\0';
                    if (strlen(newValue) > 0) {
                        strncpy(profile->ipPrefix, newValue, sizeof(profile->ipPrefix) - 1);
                        profile->ipPrefix[sizeof(profile->ipPrefix) - 1] = '\0';
                    }
                }
                
                saveSettings();
                printf("Profile '%s' updated successfully.\n", profile->name);
                break;
            }
                
            case 4: {  // Delete profile
                wifiListProfiles();
                if (gWifiProfileCount == 0) break;
                
                printf("Select profile to delete (1-%d): ", gWifiProfileCount);
                if (!fgets(input, sizeof(input), stdin)) break;
                int profileIdx = atoi(input) - 1;
                
                if (profileIdx < 0 || profileIdx >= gWifiProfileCount) {
                    printf("ERROR: Invalid profile number.\n");
                    break;
                }
                
                printf("Delete profile '%s'? (y/N): ", gWifiProfiles[profileIdx].name);
                if (fgets(input, sizeof(input), stdin)) {
                    if (tolower(input[0]) == 'y') {
                        // Shift remaining profiles down
                        for (int i = profileIdx; i < gWifiProfileCount - 1; i++) {
                            gWifiProfiles[i] = gWifiProfiles[i + 1];
                        }
                        gWifiProfileCount--;
                        
                        if (gActiveProfileIndex == profileIdx) {
                            gActiveProfileIndex = -1;
                        } else if (gActiveProfileIndex > profileIdx) {
                            gActiveProfileIndex--;
                        }
                        
                        saveSettings();
                        printf("Profile deleted successfully.\n");
                    }
                }
                break;
            }
                
            case 5: {  // Select profile
                wifiListProfiles();
                if (gWifiProfileCount == 0) break;
                
                printf("Select profile (1-%d, 0=manual): ", gWifiProfileCount);
                if (!fgets(input, sizeof(input), stdin)) break;
                int profileIdx = atoi(input) - 1;
                
                if (profileIdx == -1) {
                    gActiveProfileIndex = -1;
                    printf("Manual mode selected.\n");
                } else if (profileIdx >= 0 && profileIdx < gWifiProfileCount) {
                    gActiveProfileIndex = profileIdx;
                    printf("Profile '%s' selected.\n", gWifiProfiles[profileIdx].name);
                } else {
                    printf("ERROR: Invalid profile number.\n");
                }
                break;
            }
                
            case 6: {  // Auto-suggest
                char currentIP[40];
                getCurrentPCIPAddress(currentIP, sizeof(currentIP));
                
                if (currentIP[0] == '\0') {
                    printf("ERROR: Could not determine your PC's IP address.\n");
                    break;
                }
                
                printf("Your PC's IP address: %s\n", currentIP);
                
                int suggestedIdx = wifiSuggestProfile();
                if (suggestedIdx >= 0) {
                    printf("Suggested profile: [%d] %s\n", suggestedIdx + 1, gWifiProfiles[suggestedIdx].name);
                    printf("SSID: %s\n", gWifiProfiles[suggestedIdx].ssid);
                    printf("IP prefix: %s.x\n", gWifiProfiles[suggestedIdx].ipPrefix);
                    printf("\nUse this profile? (Y/n): ");
                    
                    if (fgets(input, sizeof(input), stdin)) {
                        if (strlen(input) == 0 || tolower(input[0]) == 'y' || input[0] == '\n') {
                            gActiveProfileIndex = suggestedIdx;
                            printf("Profile '%s' selected.\n", gWifiProfiles[suggestedIdx].name);
                        }
                    }
                } else {
                    printf("No matching profile found for your current network.\n");
                    printf("Suggestion: Create a profile with subnet matching your IP.\n");
                }
                break;
            }
                
            default:
                printf("Invalid choice.\n");
                break;
        }
    }
}

