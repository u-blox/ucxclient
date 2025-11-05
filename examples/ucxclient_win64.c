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
#include "u_cx_security.h"
#include "u_cx_diagnostics.h"
#include "u_cx_gatt_client.h"
#include "u_cx_gatt_server.h"
#include "u_cx_sps.h"
#include "u_cx_firmware_update.h"

// Port layer
#include "port/u_port.h"

// Bluetooth SIG name databases
#include "bluetooth-sig/bt_company_identifiers.h"
#include "bluetooth-sig/bt_service_uuids.h"
#include "bluetooth-sig/bt_appearance_values.h"

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
static bool gConnected = false;

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

// Settings (saved to file)
static char gComPort[16] = "COM31";           // Default COM port
static char gLastDeviceModel[64] = "";        // Last connected device model
static char gRemoteAddress[128] = "";         // Last remote address/hostname

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

// Ping test results
static volatile int32_t gPingSuccess = 0;
static volatile int32_t gPingFailed = 0;
static volatile int32_t gPingAvgTime = 0;
#define MAX_PING_TIMES 10
static volatile int32_t gPingTimes[MAX_PING_TIMES];
static volatile int32_t gPingCount = 0;

// Reboot timing
static volatile ULONGLONG gStartupTimestamp = 0;

// Menu state
typedef enum {
    MENU_MAIN,
    MENU_BLUETOOTH,
    MENU_BLUETOOTH_FUNCTIONS,
    MENU_WIFI,
    MENU_WIFI_FUNCTIONS,
    MENU_SOCKET,
    MENU_SPS,
    MENU_GATT_CLIENT,
    MENU_GATT_SERVER,
    MENU_MQTT,
    MENU_HTTP,
    MENU_SECURITY_TLS,
    MENU_FIRMWARE_UPDATE,
    MENU_API_LIST,
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
//   - connectDevice()                  Connect to UCX device via COM port
//   - quickConnectToLastDevice()       Auto-connect to last device
//   - disconnectDevice()               Disconnect and cleanup
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
static bool connectDevice(const char *comPort);
static bool quickConnectToLastDevice(void);
static void disconnectDevice(void);
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
static void executeAtTest(void);
static void executeAti9(void);
static void executeModuleReboot(void);
static void showBluetoothStatus(void);
static void showWifiStatus(void);
static void bluetoothMenu(void);
static void bluetoothScan(void);
static void bluetoothConnect(void);
static void bluetoothDisconnect(void);
static void bluetoothSyncConnections(void);
static void decodeAdvertisingData(const uint8_t *data, size_t dataLen);
static void wifiMenu(void);
static void wifiScan(void);
static void wifiConnect(void);
static void wifiDisconnect(void);
static void wifiManageProfiles(void);
static void wifiSaveProfile(const char *name, const char *ssid, const char *password, const char *ipPrefix);
static int wifiSuggestProfile(void);
static void wifiListProfiles(void);
static void getCurrentPCIPAddress(char *ipBuffer, size_t bufferSize);
static void loadSettings(void);
static void saveSettings(void);
static void obfuscatePassword(const char *input, char *output, size_t outputSize);
static void deobfuscatePassword(const char *input, char *output, size_t outputSize);
static void socketCreateTcp(void);
static void socketCreateUdp(void);
static void socketConnect(void);
static void socketSendData(void);
static void socketReadData(void);
static void socketClose(void);
static void socketListStatus(void);
static void spsEnableService(void);
static void spsConnect(void);
static void spsSendData(void);
static void spsReadData(void);
static void gattClientMenu(void);
static void gattClientDiscoverServices(void);
static void gattClientReadCharacteristic(void);
static void gattClientWriteCharacteristic(void);
static void gattServerMenu(void);
static void gattServerAddService(void);
static void gattServerSetCharacteristic(void);
static void bluetoothFunctionsMenu(void);
static void wifiFunctionsMenu(void);
static void socketMenu(void);
static void mqttMenu(void);
static void httpMenu(void);
static void securityTlsMenu(void);
static void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel);

// URC handlers for ping
static void pingResponseUrc(struct uCxHandle *puCxHandle, uPingResponse_t ping_response, int32_t response_time);
static void pingCompleteUrc(struct uCxHandle *puCxHandle, int32_t transmitted_packets, 
                           int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time);

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
    printf("\n==============================================================\n");
    printf("           Download Firmware from GitHub\n");
    printf("==============================================================\n\n");
    
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
    gStartupTimestamp = GetTickCount64();
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** Module STARTUP detected ***");
    
    // Module has restarted - need to reconfigure echo and error codes
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Reconfiguring module after restart...");
    
    // Turn off echo
    int32_t result = uCxSystemSetEchoOff(puCxHandle);
    if (result != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, puCxHandle->pAtClient->instance, 
                       "Warning: Failed to disable echo after restart (error %d)", result);
    }
    
    // Enable extended error codes
    result = uCxSystemSetExtendedError(puCxHandle, U_EXTENDED_ERRORS_ON);
    if (result != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, puCxHandle->pAtClient->instance, 
                       "Warning: Failed to enable extended errors after restart (error %d)", result);
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Module reconfiguration complete");
    
    signalEvent(URC_FLAG_STARTUP);
}

static void pingResponseUrc(struct uCxHandle *puCxHandle, uPingResponse_t ping_response, int32_t response_time)
{
    (void)puCxHandle;
    if (ping_response == U_PING_RESPONSE_TRUE) {
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

static void mqttConnectedUrc(struct uCxHandle *puCxHandle, int32_t mqtt_client_id)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "MQTT connected: client %d", mqtt_client_id);
    
    printf("\n*** MQTT Connection Established ***\n");
    printf("Client ID: %d\n", mqtt_client_id);
    printf("Status: Connected to broker\n");
    printf("***********************************\n\n");
    
    signalEvent(URC_FLAG_MQTT_CONNECTED);
}

static void mqttDataAvailableUrc(struct uCxHandle *puCxHandle, int32_t mqtt_client_id, int32_t number_bytes)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "MQTT data received: %d bytes on client %d", number_bytes, mqtt_client_id);
    
    // Read the MQTT data immediately
    printf("\n*** MQTT Message Received ***\n");
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
    
    printf("*****************************\n\n");
    
    signalEvent(URC_FLAG_MQTT_DATA);
}

static void btConnected(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Bluetooth connected: handle %d", conn_handle);
    
    printf("\n*** Bluetooth Connection Established ***\n");
    printf("Connection handle: %d\n", conn_handle);
    if (bd_addr) {
        printf("Device address: %02X:%02X:%02X:%02X:%02X:%02X\n",
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
    
    printf("****************************************\n\n");
    
    signalEvent(URC_FLAG_BT_CONNECTED);
}

static void btDisconnected(struct uCxHandle *puCxHandle, int32_t conn_handle)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Bluetooth disconnected: handle %d", conn_handle);
    
    printf("\n*** Bluetooth Disconnected ***\n");
    printf("Connection handle: %d\n", conn_handle);
    
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
    
    printf("******************************\n\n");
    
    signalEvent(URC_FLAG_BT_DISCONNECTED);
}

// ----------------------------------------------------------------
// ============================================================================
// SOCKET OPERATIONS (TCP/UDP)
// ============================================================================

static void socketCreateTcp(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Create TCP Socket ---");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_PROTOCOL_TCP, &socketHandle);
    
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
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Create UDP Socket ---");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_PROTOCOL_UDP, &socketHandle);
    
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
    if (!gConnected) {
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
    if (!gConnected) {
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
        
        int32_t result = uCxSocketWrite(&gUcxHandle, gCurrentSocket, (uint8_t*)data, len);
        
        if (result >= 0) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully sent %d bytes", result);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to send data (code %d)", result);
        }
    }
}

static void socketReadData(void)
{
    if (!gConnected) {
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
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    if (gCurrentSocket < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "No socket to close");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Close Socket ---");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Closing socket %d...", gCurrentSocket);
    
    int32_t result = uCxSocketClose(&gUcxHandle, gCurrentSocket);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully closed socket");
        gCurrentSocket = -1;
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to close socket (code %d)", result);
    }
}

static void socketListStatus(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Socket Status ---");
    
    uCxSocketListStatusBegin(&gUcxHandle);
    
    uCxSocketListStatus_t status;
    int count = 0;
    
    while (uCxSocketListStatusGetNext(&gUcxHandle, &status)) {
        count++;
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Socket %d: Protocol=%s, Status=%s",
               status.socket_handle,
               status.protocol == U_PROTOCOL_TCP ? "TCP" : "UDP",
               status.socket_status == 0 ? "Not Connected" :
               status.socket_status == 1 ? "Listening" : "Connected");
    }
    
    uCxEnd(&gUcxHandle);
    
    if (count == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "  No sockets");
    }
    
    if (gCurrentSocket >= 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Current socket: %d", gCurrentSocket);
    }
}

// ----------------------------------------------------------------
// ============================================================================
// SPS (SERIAL PORT SERVICE)
// ============================================================================

static void spsEnableService(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- Enable SPS Service ---");
    
    int32_t result = uCxSpsSetServiceEnable(&gUcxHandle, U_SPS_SERVICE_OPTION_ENABLE_SPS_SERVICE);
    
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully enabled SPS service");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "NOTE: SPS will be active after reboot");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to enable SPS (code %d)", result);
    }
}

static void spsConnect(void)
{
    if (!gConnected) {
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
    if (!gConnected) {
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
        
        int32_t result = uCxSpsWrite(&gUcxHandle, connHandle, (uint8_t*)data, len);
        
        if (result >= 0) {
            U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Successfully sent %d bytes", result);
        } else {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to send data (code %d)", result);
        }
    }
}

static void spsReadData(void)
{
    if (!gConnected) {
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

static void gattClientDiscoverServices(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Discover Services ---");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Starting service discovery...");
    
    // Call GATT service discovery command
    uCxGattClientDiscoverPrimaryServicesBegin(&gUcxHandle, connHandle);
    
    // Get services
    uCxGattClientDiscoverPrimaryServices_t service;
    int serviceCount = 0;
    while (uCxGattClientDiscoverPrimaryServicesGetNext(&gUcxHandle, &service)) {
        serviceCount++;
        printf("  Service %d: start=0x%04X, end=0x%04X, UUID=", 
               serviceCount, service.start_handle, service.end_handle);
        for (int i = 0; i < service.uuid.length; i++) {
            printf("%02X", service.uuid.pData[i]);
        }
        printf("\n");
    }
    
    int32_t result = uCxEnd(&gUcxHandle);
    if (result == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Service discovery complete. Found %d services.", serviceCount);
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Service discovery failed (code %d)", result);
    }
}

static void gattClientReadCharacteristic(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Read Characteristic ---");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Enter characteristic handle: ");
    int charHandle;
    scanf("%d", &charHandle);
    getchar(); // consume newline
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Reading characteristic...");
    
    // Call GATT read characteristic command
    uByteArray_t data;
    uint8_t buffer[512];
    data.pData = buffer;
    data.length = 0;
    
    bool success = uCxGattClientReadBegin(&gUcxHandle, connHandle, charHandle, &data);
    int32_t result = uCxEnd(&gUcxHandle);
    
    if (success && result == 0) {
        printf("  Read %zu bytes: ", data.length);
        for (int i = 0; i < data.length; i++) {
            printf("%02X", data.pData[i]);
        }
        printf("\n");
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Read successful.");
    } else {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to read characteristic (code %d)", result);
    }
}

static void gattClientWriteCharacteristic(void)
{
    if (!gConnected) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Not connected to device");
        return;
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "--- GATT Client: Write Characteristic ---");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Enter characteristic handle: ");
    int charHandle;
    scanf("%d", &charHandle);
    getchar(); // consume newline
    
    printf("Enter data to write (hex format, e.g., 01020304): ");
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
    if (!gConnected) {
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
    if (!gConnected) {
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

// ============================================================================
// MQTT OPERATIONS (Publish/Subscribe)
// ============================================================================

#define MQTT_CONFIG_ID 0
#define MQTT_DEFAULT_HOST "broker.emqx.io"
#define MQTT_DEFAULT_PORT 1883

static void mqttConnect(void)
{
    if (!gConnected) {
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
    if (!gConnected) {
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
    if (!gConnected) {
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
                                               U_SUBSCRIBE_ACTION_SUBSCRIBE, 
                                               topic, (uQos_t)qos);
            
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
    if (!gConnected) {
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
                                           U_SUBSCRIBE_ACTION_UNSUBSCRIBE, 
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
    if (!gConnected) {
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
    
    int32_t result = uCxMqttPublish(&gUcxHandle, MQTT_CONFIG_ID, (uQos_t)qos, 
                                     (uRetain_t)retain, topic, 
                                     (uint8_t*)message, strlen(message));
    
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

static void httpMenu(void)
{
    gMenuState = MENU_HTTP;
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
    if (connectDevice(gComPort)) {
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
        printMenu();
        handleUserInput();
    }
    
    // Cleanup
    if (gConnected) {
        disconnectDevice();
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
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "========================================");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "  u-connectXpress ucxclient App v%s", APP_VERSION);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "========================================");
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
    printf("=========================================================\n");
    printf("            WELCOME - Getting Started Guide             \n");
    printf("=========================================================\n");
    printf("\n");
    printf("This is a simple example application to help you get\n");
    printf("started with u-connectXpress modules (NORA-B26/NORA-W36).\n");
    printf("\n");
    printf("QUICK START:\n");
    printf("  1. Connect your module via USB\n");
    printf("  2. Use menu option [1] to connect to the device\n");
    printf("  3. Try [3] AT test to verify communication\n");
    printf("  4. Try [4] ATI9 to see device information\n");
    printf("\n");
    printf("COMMON OPERATIONS:\n");
    printf("  - Wi-Fi: Use [8] Wi-Fi (scan, connect, disconnect, status)\n");
    printf("  - Bluetooth: Use [6] Bluetooth (scan, connect, disconnect, status)\n");
    printf("  - Sockets: Use [9] Wi-Fi functions for TCP/UDP (requires Wi-Fi)\n");
    printf("  - SPS: Use [7] Bluetooth functions for Serial Port Service\n");
    printf("\n");
    printf("TIPS:\n");
    printf("  - Type [h] anytime for help\n");
    printf("  - Type [q] to quit from any menu\n");
    printf("  - Settings are saved automatically after successful operations\n");
    printf("  - Use [l] to toggle AT command logging on/off\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void printHelp(void)
{
    printf("\n");
    printf("=========================================================\n");
    printf("                    HELP & TIPS                          \n");
    printf("=========================================================\n");
    printf("\n");
    printf("CONNECTION:\n");
    printf("  [1] Connect      - Select and connect to your UCX device\n");
    printf("  [2] Disconnect   - Close connection to device\n");
    printf("  [q] Quit         - Exit from any menu\n");
    printf("\n");
    printf("BASIC OPERATIONS:\n");
    printf("  [3] AT test      - Test basic communication with device\n");
    printf("  [4] ATI9         - Show device model and firmware version\n");
    printf("  [5] Reboot       - Restart the module\n");
    printf("\n");
    printf("BLUETOOTH OPERATIONS:\n");
    printf("  [6] Bluetooth (scan, connect, disconnect, status)\n");
    printf("      - Scan for nearby Bluetooth devices\n");
    printf("      - Connect to Bluetooth devices\n");
    printf("      - Disconnect active connections\n");
    printf("      - Show connection status\n");
    printf("  [7] Bluetooth functions (SPS, GATT)\n");
    printf("      - Serial Port Service for data transfer\n");
    printf("      - GATT Client/Server operations\n");
    printf("  NOTE: NORA-B26 is Bluetooth only, NORA-W36 has BT+Wi-Fi\n");
    printf("\n");
    printf("WI-FI OPERATIONS:\n");
    printf("  [8] Wi-Fi (scan, connect, disconnect, status)\n");
    printf("      - Scan for Wi-Fi networks\n");
    printf("      - Connect to Wi-Fi (SSID and password saved)\n");
    printf("      - Disconnect from networks\n");
    printf("      - Show connection status\n");
    printf("  [9] Wi-Fi functions (Sockets, MQTT, HTTP, TLS)\n");
    printf("      - TCP/UDP sockets\n");
    printf("      - MQTT publish/subscribe\n");
    printf("      - HTTP client operations\n");
    printf("      - Security and TLS certificates\n");
    printf("  NOTE: Only available on NORA-W36 modules\n");
    printf("\n");
    printf("OTHER OPTIONS:\n");
    printf("  [l] Toggle logging - Show/hide AT command traffic\n");
    printf("  [t] Toggle timestamps - Add timing info to logs\n");
    printf("  [c] List API commands - Show all available UCX APIs\n");
    printf("  [f] Firmware update - Update module firmware via XMODEM\n");
    printf("\n");
    printf("  [b] SPS menu - Bluetooth Serial Port Service\n");
    printf("      - Enable SPS service\n");
    printf("      - Connect to SPS over Bluetooth\n");
    printf("      - Send and receive serial data\n");
    printf("  REQUIRES: Bluetooth connection first!\n");
    printf("\n");
    printf("  [c] MQTT menu - Message Queue Telemetry Transport\n");
    printf("      - Connect to MQTT brokers\n");
    printf("      - Publish and subscribe to topics\n");
    printf("      - QoS configuration\n");
    printf("  REQUIRES: Active Wi-Fi connection first!\n");
    printf("  STATUS: [IN PROGRESS]\n");
    printf("\n");
    printf("  [d] HTTP Client menu - REST API operations\n");
    printf("      - HTTP GET/POST/PUT/DELETE requests\n");
    printf("      - Custom headers and data\n");
    printf("      - HTTPS/TLS support\n");
    printf("  REQUIRES: Active Wi-Fi connection first!\n");
    printf("  STATUS: [IN PROGRESS]\n");
    printf("\n");
    printf("SECURITY:\n");
    printf("  [e] Security/TLS menu - Certificate management\n");
    printf("      - Upload CA and client certificates\n");
    printf("      - Manage private keys\n");
    printf("      - Configure TLS settings\n");
    printf("  STATUS: [IN PROGRESS]\n");
    printf("\n");
    printf("ADVANCED:\n");
    printf("  [3] List APIs    - Show all available UCX API commands\n");
    printf("  [f] Firmware     - Update module firmware via XMODEM\n");
    printf("  [9] Toggle log   - Show/hide AT command traffic\n");
    printf("\n");
    printf("SAVED SETTINGS:\n");
    printf("  The app remembers:\n");
    printf("    - Last COM port used\n");
    printf("    - Last Wi-Fi SSID and password\n");
    printf("    - Last remote server address\n");
    printf("  Settings saved in: %s\n", gSettingsFilePath);
    printf("\n");
    printf("TROUBLESHOOTING:\n");
    printf("  - Can't connect? Check COM port with Device Manager\n");
    printf("  - Wi-Fi not working? Use [8] -> [1] to check status\n");
    printf("  - Socket errors? Ensure Wi-Fi is connected first\n");
    printf("  - Module not responding? Try [6] to reboot it\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void printMenu(void)
{
    printf("\n");
    
    switch (gMenuState) {
        case MENU_MAIN:
            printf("--- Main Menu ---\n");
            if (gConnected) {
                printf("  Device:      %s", gComPort);
                if (gDeviceModel[0] != '\0') {
                    printf(" (%s", gDeviceModel);
                    if (gDeviceFirmware[0] != '\0') {
                        printf(" %s", gDeviceFirmware);
                    }
                    printf(")");
                }
                printf("\n");
                
                // Show connection status for WiFi/Bluetooth if available
                if (gDeviceModel[0] != '\0') {
                    // Check if this is a Wi-Fi-capable device (NORA-W36)
                    if (strstr(gDeviceModel, "W3") != NULL) {
                        printf("  Wi-Fi:        Available (use [8] to connect)\n");
                    }
                    // All devices have Bluetooth
                    printf("  Bluetooth:   Available (use [6] for operations)\n");
                }
            } else {
                printf("  Status:      Not connected\n");
                if (gComPort[0] != '\0') {
                    printf("  Last port:   %s\n", gComPort);
                }
            }
            printf("  UCX Logging: %s\n", uCxLogIsEnabled() ? "ENABLED" : "DISABLED");
            printf("\n");
            printf("  === CONNECTION ===\n");
            printf("  [1] Connect to UCX device\n");
            printf("  [2] Disconnect from device\n");
            printf("\n");
            printf("  === DEVICE OPERATIONS ===\n");
            printf("  [3] AT test (basic communication)%s\n", gConnected ? "" : " (requires connection)");
            printf("  [4] ATI9 (device info)%s\n", gConnected ? "" : " (requires connection)");
            printf("  [5] Module reboot/switch off%s\n", gConnected ? "" : " (requires connection)");
            printf("\n");
            printf("  === BLUETOOTH ===\n");
            printf("  [6] Bluetooth (scan, connect, disconnect, status)%s\n", gConnected ? "" : " (requires connection)");
            printf("  [7] Bluetooth functions (SPS, GATT)%s\n", gConnected ? "" : " (requires connection)");
            printf("\n");
            printf("  === WI-FI ===\n");
            printf("  [8] Wi-Fi (scan, connect, disconnect, status)%s\n", gConnected ? "" : " (requires connection)");
            printf("  [9] Wi-Fi functions (Sockets, MQTT, HTTP, TLS)%s\n", gConnected ? "" : " (requires connection)");
            printf("\n");
            printf("  === UTILITIES ===\n");
            printf("  [l] Toggle UCX logging (AT traffic)\n");
            printf("  [t] Toggle timestamps in logs\n");
            printf("  [c] List UCX API commands\n");
            printf("  [f] Firmware update (XMODEM)%s\n", gConnected ? "" : " (requires connection)");
            printf("  [h] Help - Getting started guide\n");
            printf("  [q] Quit application\n");
            break;
            
        case MENU_BLUETOOTH:
            printf("--- Bluetooth Menu ---\n");
            printf("  [1] Show BT status\n");
            printf("  [2] Scan for devices\n");
            printf("  [3] Connect to device\n");
            printf("  [4] Disconnect from device");
            if (gBtConnectionCount > 0) {
                printf(" (%d active)", gBtConnectionCount);
            }
            printf("\n");
            printf("  [5] List active connections\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_WIFI:
            printf("--- Wi-Fi Menu ---\n");
            printf("  [1] Show Wi-Fi status\n");
            printf("  [2] Regulatory Domain (World)\n");
            printf("  [3] Scan networks\n");
            printf("  [4] Connect to network\n");
            printf("  [5] Disconnect from network\n");
            printf("  [6] Manage Wi-Fi profiles");
            if (gWifiProfileCount > 0) {
                printf(" (%d saved)", gWifiProfileCount);
            }
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SOCKET:
            printf("--- Socket Menu (TCP/UDP) ---\n");
            printf("  NOTE: Requires Active Wi-Fi connection!\n");
            printf("  [1] Create TCP socket\n");
            printf("  [2] Create UDP socket\n");
            printf("  [3] Connect socket\n");
            printf("  [4] Send data\n");
            printf("  [5] Read data\n");
            printf("  [6] Close socket\n");
            printf("  [7] List sockets\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SPS:
            printf("--- SPS Menu (Bluetooth Serial Port Service) ---\n");
            printf("  NOTE: Requires active Bluetooth connection!\n");
            printf("  [1] Enable SPS service\n");
            printf("  [2] Connect SPS on BT connection\n");
            printf("  [3] Send data\n");
            printf("  [4] Read data\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_MQTT:
            printf("--- MQTT Menu (Publish/Subscribe) ---\n");
            printf("  NOTE: Requires Active Wi-Fi connection!\n");
            printf("  Broker: %s:%d\n", MQTT_DEFAULT_HOST, MQTT_DEFAULT_PORT);
            printf("\n");
            printf("  [1] Connect to MQTT broker\n");
            printf("  [2] Disconnect from broker\n");
            printf("  [3] Subscribe to topic\n");
            printf("  [4] Unsubscribe from topic\n");
            printf("  [5] Publish message\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_HTTP:
            printf("--- HTTP Client Menu (REST API) ---\n");
            printf("  NOTE: Requires Active Wi-Fi connection!\n");
            printf("  [IN PROGRESS] - Feature under development\n");
            printf("\n");
            printf("  Planned features:\n");
            printf("  - HTTP GET requests\n");
            printf("  - HTTP POST with data\n");
            printf("  - HTTP PUT/DELETE methods\n");
            printf("  - Custom headers\n");
            printf("  - Response parsing\n");
            printf("  - HTTPS/TLS support\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_SECURITY_TLS:
            printf("--- Security/TLS Menu (Certificates & Encryption) ---\n");
            printf("  [IN PROGRESS] - Feature under development\n");
            printf("\n");
            printf("  Planned features:\n");
            printf("  - Upload CA certificates\n");
            printf("  - Upload client certificates\n");
            printf("  - Manage private keys\n");
            printf("  - Configure TLS settings\n");
            printf("  - Certificate validation options\n");
            printf("\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_BLUETOOTH_FUNCTIONS:
            printf("--- Bluetooth Functions ---\n");
            printf("  [1] SPS (Serial Port Service)\n");
            printf("  [2] GATT Client\n");
            printf("  [3] GATT Server\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_WIFI_FUNCTIONS:
            printf("--- Wi-Fi Functions ---\n");
            printf("  NOTE: Requires Active Wi-Fi connection!\n");
            printf("  [1] Socket menu (TCP/UDP)\n");
            printf("  [2] MQTT (publish/subscribe)\n");
            printf("  [3] HTTP Client (GET/POST/PUT)\n");
            printf("  [4] Security/TLS (certificates)\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_GATT_CLIENT:
            printf("--- GATT Client Menu ---\n");
            printf("  NOTE: Requires active Bluetooth connection!\n");
            printf("  [1] Discover services\n");
            printf("  [2] Discover characteristics\n");
            printf("  [3] Read characteristic\n");
            printf("  [4] Write characteristic\n");
            printf("  [5] Subscribe to notifications\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_GATT_SERVER:
            printf("--- GATT Server Menu ---\n");
            printf("  [1] Add service\n");
            printf("  [2] Add characteristic\n");
            printf("  [3] Set characteristic value\n");
            printf("  [4] Send notification\n");
            printf("  [0] Back to main menu  [q] Quit\n");
            break;
            
        case MENU_FIRMWARE_UPDATE:
            printf("--- Firmware Update (XMODEM) ---\n");
            printf("  This will update the module firmware via XMODEM protocol.\n");
            printf("  The module will reboot after a successful update.\n");
            printf("\n");
            printf("  Current device: %s", gComPort);
            if (gDeviceModel[0] != '\0') {
                printf(" (%s", gDeviceModel);
                if (gDeviceFirmware[0] != '\0') {
                    printf(" v%s", gDeviceFirmware);
                }
                printf(")");
            }
            printf("\n\n");
            printf("  [1] Select firmware file and start update\n");
            printf("  [2] Download latest firmware from GitHub\n");
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
        
        // Handle 't' for timestamp toggle (main menu only)
        if (firstChar == 't' && gMenuState == MENU_MAIN) {
            if (uCxLogTimestampIsEnabled()) {
                uCxLogTimestampDisable();
                printf("Log timestamps DISABLED (cleaner output)\n");
            } else {
                uCxLogTimestampEnable();
                printf("Log timestamps ENABLED (shows [HH:MM:SS.mmm] timing)\n");
                U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Timestamps enabled from menu");
            }
            return;
        }
        
        // Handle specific letter commands for main menu
        if (gMenuState == MENU_MAIN) {
            if (firstChar == 'l') {
                choice = 12;  // Toggle UCX logging
            } else if (firstChar == 'c') {
                choice = 13;  // List API commands
            } else if (firstChar == 'f') {
                choice = 16;  // Firmware update
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
                    if (gComPort[0] != '\0' && !gConnected) {
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
                                if (connectDevice(gComPort)) {
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
                    disconnectDevice();
                    break;
                case 3:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeAtTest();
                    }
                    break;
                case 4:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeAti9();
                    }
                    break;
                case 5:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        executeModuleReboot();
                    }
                    break;
                case 6:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_BLUETOOTH;
                    }
                    break;
                case 7:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_BLUETOOTH_FUNCTIONS;
                    }
                    break;
                case 8:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_WIFI;
                    }
                    break;
                case 9:
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_WIFI_FUNCTIONS;
                    }
                    break;
                case 12:  // Also accept 'l' or 'L' - Toggle UCX logging
                    if (uCxLogIsEnabled()) {
                        uCxLogDisable();
                        printf("UCX logging DISABLED\n");
                    } else {
                        uCxLogEnable();
                        printf("UCX logging ENABLED\n");
                        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Logging re-enabled from menu");
                    }
                    break;
                case 13:  // Also accept 'c' or 'C' - List API commands
                    gMenuState = MENU_API_LIST;
                    break;
                case 16:  // Also accept 'f' or 'F' - Firmware update
                    if (!gConnected) {
                        printf("ERROR: Not connected to device. Use [1] to connect first.\n");
                    } else {
                        gMenuState = MENU_FIRMWARE_UPDATE;
                    }
                    break;
                case 18:  // Also accept 'h' or 'H' - Help (handled above but keep for consistency)
                    printHelp();
                    break;
                case 0:
                    gMenuState = MENU_EXIT;
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
                case 0:
                    gMenuState = MENU_WIFI_FUNCTIONS;
                    break;
                default:
                    printf("Feature in progress. Use [0] to return to Wi-Fi Functions menu.\n");
                    break;
            }
            break;
            
        case MENU_SECURITY_TLS:
            switch (choice) {
                case 0:
                    gMenuState = MENU_WIFI_FUNCTIONS;
                    break;
                default:
                    printf("Feature in progress. Use [0] to return to Wi-Fi Functions menu.\n");
                    break;
            }
            break;
            
        case MENU_BLUETOOTH_FUNCTIONS:
            switch (choice) {
                case 1:
                    gMenuState = MENU_SPS;
                    break;
                case 2:
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
                    printf("Discover characteristics - not yet implemented\n");
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
                    printf("Invalid choice!\n");
                    break;
            }
            break;
            
        case MENU_GATT_SERVER:
            switch (choice) {
                case 1:
                    gattServerAddService();
                    break;
                case 2:
                    printf("Add characteristic - not yet implemented\n");
                    break;
                case 3:
                    gattServerSetCharacteristic();
                    break;
                case 4:
                    printf("Send notification - not yet implemented\n");
                    break;
                case 0:
                    gMenuState = MENU_BLUETOOTH_FUNCTIONS;
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
                    if (!gConnected) {
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
                        gConnected = true;
                        
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
                    if (!gConnected) {
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
                        
                        gConnected = true;
                        
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

static bool connectDevice(const char *comPort)
{
    if (gConnected) {
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
    
    // Create mutex for URC event handling
    U_CX_MUTEX_CREATE(gUrcMutex);
    
    // Register URC handlers for Wi-Fi link and network events
    uCxWifiRegisterLinkUp(&gUcxHandle, linkUpUrc);
    uCxWifiRegisterLinkDown(&gUcxHandle, linkDownUrc);
    uCxWifiRegisterStationNetworkUp(&gUcxHandle, networkUpUrc);
    uCxWifiRegisterStationNetworkDown(&gUcxHandle, networkDownUrc);
    
    // Register URC handlers for socket events
    uCxSocketRegisterConnect(&gUcxHandle, sockConnected);
    uCxSocketRegisterDataAvailable(&gUcxHandle, socketDataAvailable);
    
    // Register URC handlers for SPS events
    uCxSpsRegisterConnect(&gUcxHandle, spsConnected);
    uCxSpsRegisterDisconnect(&gUcxHandle, spsDisconnected);
    uCxSpsRegisterDataAvailable(&gUcxHandle, spsDataAvailable);
    
    // Register URC handler for system events
    uCxSystemRegisterStartup(&gUcxHandle, startupUrc);
    
    // Register URC handlers for ping/diagnostics
    uCxDiagnosticsRegisterPingResponse(&gUcxHandle, pingResponseUrc);
    uCxDiagnosticsRegisterPingComplete(&gUcxHandle, pingCompleteUrc);
    
    // Register URC handlers for MQTT events
    uCxMqttRegisterConnect(&gUcxHandle, mqttConnectedUrc);
    uCxMqttRegisterDataAvailable(&gUcxHandle, mqttDataAvailableUrc);
    
    // Register URC handlers for Bluetooth events
    uCxBluetoothRegisterConnect(&gUcxHandle, btConnected);
    uCxBluetoothRegisterDisconnect(&gUcxHandle, btDisconnected);
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "UCX initialized successfully");

    // Turn off echo to avoid "Unexpected data" warnings
    printf("Disabling AT echo...\n");
    int32_t result = uCxSystemSetEchoOff(&gUcxHandle);
    if (result != 0) {
        printf("Warning: Failed to disable echo (error %d), continuing anyway...\n", result);
    }
    
    // Enable extended error codes for better error diagnostics
    printf("Enabling extended error codes...\n");
    result = uCxSystemSetExtendedError(&gUcxHandle, U_EXTENDED_ERRORS_ON);
    if (result != 0) {
        printf("Warning: Failed to enable extended error codes (error %d), continuing anyway...\n", result);
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
    
    // AT+GSN - Serial number
    const char *serialNumber = NULL;
    if (uCxGeneralGetSerialNumberBegin(&gUcxHandle, &serialNumber) && serialNumber != NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Serial Number:    %s", serialNumber);
        uCxEnd(&gUcxHandle);
    } else {
        uCxEnd(&gUcxHandle);
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Serial Number:    (not available)");
    }
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "-------------------");
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "");
    
    gConnected = true;
    return true;
}

static void disconnectDevice(void)
{
    if (!gConnected) {
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
    
    // Unregister socket event handlers
    uCxSocketRegisterConnect(&gUcxHandle, NULL);
    uCxSocketRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister SPS event handlers
    uCxSpsRegisterConnect(&gUcxHandle, NULL);
    uCxSpsRegisterDisconnect(&gUcxHandle, NULL);
    uCxSpsRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister system event handlers
    uCxSystemRegisterStartup(&gUcxHandle, NULL);
    
    // Unregister diagnostics/ping event handlers
    uCxDiagnosticsRegisterPingResponse(&gUcxHandle, NULL);
    uCxDiagnosticsRegisterPingComplete(&gUcxHandle, NULL);
    
    // Unregister MQTT event handlers
    uCxMqttRegisterConnect(&gUcxHandle, NULL);
    uCxMqttRegisterDataAvailable(&gUcxHandle, NULL);
    
    // Unregister Bluetooth event handlers
    uCxBluetoothRegisterConnect(&gUcxHandle, NULL);
    uCxBluetoothRegisterDisconnect(&gUcxHandle, NULL);
    
    printf("All URC handlers unregistered.\n");
    
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
    
    gConnected = false;
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
    
    if (connectDevice(gComPort)) {
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
    
    if (recommendedPort && recommendedPort[0] != '\0') {
        printf("\nEnter COM port name or press Enter to use recommended [%s]: ", recommendedPort);
    } else {
        printf("\nEnter COM port name (e.g., COM31) or press Enter to use last saved port: ");
    }
    
    if (fgets(input, sizeof(input), stdin)) {
        // Remove trailing newline
        char *end = strchr(input, '\n');
        if (end) *end = '\0';
        
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
        if (recommendedPort && recommendedPort[0] != '\0') {
            char *result = (char*)malloc(strlen(recommendedPort) + 1);
            if (result) {
                strcpy(result, recommendedPort);
                return result;
            } else {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to allocate memory for COM port string");
            }
        }
    }
    
    return NULL;  // No recommendation and user pressed Enter without input
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
        
        printf("=========================================================\n");
    }
}

// ============================================================================
// AT COMMANDS & DIAGNOSTICS
// ============================================================================

static void executeAtTest(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- AT Test ---\n");
    if (uCxLogIsEnabled()) {
        printf(">>> WATCH BELOW FOR AT TRAFFIC <<<\n");
        printf("===================================\n");
        // Test if logging works at all
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Starting AT test - logging is enabled");
    }
    
    // Simple AT command using UCX API
    int32_t result = uCxGeneralAttention(&gUcxHandle);
    
    if (uCxLogIsEnabled()) {
        printf("===================================\n");
    }
    
    if (result == 0) {
        printf("Result: OK - Device responded successfully\n");
    } else {
        printf("Result: ERROR (code %d)\n", result);
    }
}

static void executeAti9(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- ATI9 Device Information ---\n");
    if (uCxLogIsEnabled()) {
        printf("===================================\n");
    }
    
    // Use uCxGeneralGetIdentInfoBegin to get device info
    uCxGeneralGetIdentInfo_t info;
    
    if (uCxGeneralGetIdentInfoBegin(&gUcxHandle, &info)) {
        if (uCxLogIsEnabled()) {
            printf("===================================\n");
        }
        printf("Application Version: %s\n", info.application_version);
        printf("Unique Identifier:   %s\n", info.unique_identifier);
        
        // Call uCxEnd to complete the command
        uCxEnd(&gUcxHandle);
    } else {
        if (uCxLogIsEnabled()) {
            printf("===================================\n");
        }
        printf("ERROR: Failed to get device information\n");
    }
}

static void executeModuleReboot(void)
{
    if (!gConnected) {
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
            
            // Disable echo again after reboot
            printf("Disabling echo...\n");
            result = uCxSystemSetEchoOff(&gUcxHandle);
            if (result == 0) {
                printf("Echo disabled.\n");
            } else {
                printf("Warning: Failed to disable echo (error %d)\n", result);
            }
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
            
            // Try to disable echo again after reboot
            printf("Disabling echo...\n");
            result = uCxSystemSetEchoOff(&gUcxHandle);
            if (result == 0) {
                printf("Echo disabled.\n");
            } else {
                printf("Warning: Failed to disable echo (error %d)\n", result);
            }
        } else {
            printf(" timeout!\n");
            printf("Module may have shut down completely (no +STARTUP received).\n");
        }
    } else {
        printf("ERROR: Failed to send AT+CPWROFF (error %d)\n", result);
    }
}

// ============================================================================
// BLUETOOTH OPERATIONS
// ============================================================================

static void showBluetoothStatus(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Status ---\n");
    
    // Get Bluetooth mode
    uBtMode_t btMode;
    int32_t result = uCxBluetoothGetMode(&gUcxHandle, &btMode);
    
    if (result == 0) {
        printf("Bluetooth Mode: ");
        switch (btMode) {
            case 0: printf("Disabled\n"); break;
            case 1: printf("Central\n"); break;
            case 2: printf("Peripheral\n"); break;
            case 3: printf("Central + Peripheral\n"); break;
            default: printf("Unknown (%d)\n", btMode); break;
        }
        
        if (btMode != 0) {
            // List active connections
            printf("\nActive Connections:\n");
            
            uCxBluetoothListConnectionsBegin(&gUcxHandle);
            
            uCxBluetoothListConnections_t conn;
            int connCount = 0;
            
            while (uCxBluetoothListConnectionsGetNext(&gUcxHandle, &conn)) {
                connCount++;
                printf("  Handle %d: %02X:%02X:%02X:%02X:%02X:%02X (%s)\n",
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
                printf("  No devices connected\n");
            }
            
            uCxEnd(&gUcxHandle);
        }
    } else {
        printf("ERROR: Failed to get Bluetooth mode (code %d)\n", result);
    }
}

// ============================================================================
// WIFI OPERATIONS
// ============================================================================

static void showWifiStatus(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Status ---\n");
    
    // Get connection status
    uCxWifiStationStatus_t status;
    
    // Check if connected
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &status)) {
        int32_t connState = status.rspWifiStatusIdInt.int_val;
        uCxEnd(&gUcxHandle);
        
        if (connState == 2) {
            printf("Status: Connected\n");
            
            // Get SSID
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_SSID, &status)) {
                printf("SSID: %s\n", status.rspWifiStatusIdStr.ssid);
                uCxEnd(&gUcxHandle);
            }
            
            // Get RSSI
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &status)) {
                int32_t rssi = status.rspWifiStatusIdInt.int_val;
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
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Device Scan ---\n");
    printf("Scanning for devices... (this may take 10-15 seconds)\n\n");
    
    // Set 30 second timeout for scan command (scan can take time)
    uCxAtClientSetCommandTimeout(gUcxHandle.pAtClient, 30000, false);
    
    // Start discovery (type 0 = general discovery, timeout in milliseconds 10000 = 10 sec)
    uCxBluetoothDiscovery3Begin(&gUcxHandle, 0, 0, 10000);
    
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
    uCxBluetoothDiscovery_t device;
    
    // Get discovered devices and deduplicate
    bool gotResponse;
    do {
        gotResponse = uCxBluetoothDiscovery3GetNext(&gUcxHandle, &device);
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
    if (!gConnected) {
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
    if (!gConnected) {
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
    
    printf("\nEnter connection handle to disconnect: ");
    
    char input[64];
    if (fgets(input, sizeof(input), stdin)) {
        int32_t handle = atoi(input);
        
        // Verify the handle exists in our tracking
        bool found = false;
        for (int i = 0; i < gBtConnectionCount; i++) {
            if (gBtConnections[i].handle == handle) {
                found = true;
                break;
            }
        }
        
        if (!found) {
            printf("ERROR: Invalid connection handle %d\n", handle);
            return;
        }
        
        printf("Disconnecting handle %d...\n", handle);
        
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
    if (!gConnected) {
        return;
    }
    
    // Clear existing tracking
    gBtConnectionCount = 0;
    memset(gBtConnections, 0, sizeof(gBtConnections));
    
    // Query active connections from module
    uCxBluetoothListConnectionsBegin(&gUcxHandle);
    
    uCxBluetoothListConnections_t conn;
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
    if (!gConnected) {
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
        printf("==============================================================\n");
        
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
        
        printf("==============================================================\n");
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
    printf("==============================================================\n");
    printf("                   CONNECTION SUMMARY\n");
    printf("==============================================================\n");
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
    printf("==============================================================\n");
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
    printf("==============================================================\n");
}

static void wifiConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Connect ---\n");
    
    // Check if already connected and disconnect if necessary
    uCxWifiStationStatus_t connStatus;
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &connStatus)) {
        int32_t connState = connStatus.rspWifiStatusIdInt.int_val;
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
        if (uCxWifiStationSetSecurityWpa(&gUcxHandle, 0, password, U_WPA_THRESHOLD_WPA2) != 0) {
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
            rssi = rssiStatus.rspWifiStatusIdInt.int_val;
            if (rssi != -32768) {
                printf("Signal strength: %d dBm\n", rssi);
            }
            uCxEnd(&gUcxHandle);
        }
        
        // Get IP address using WiFi Station Network Status (AT+UWSNST)
        uSockIpAddress_t ipAddr;
        char ipStr[40];  // Allow for IPv6
        char gatewayStr[40] = "";  // Store gateway for ping test
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_IPV4, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                printf("IP address: %s\n", ipStr);
            }
        }
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_SUBNET, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                printf("Subnet mask: %s\n", ipStr);
            }
        }
        
        if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_GATE_WAY, &ipAddr) == 0) {
            if (uCxIpAddressToString(&ipAddr, gatewayStr, sizeof(gatewayStr)) > 0) {
                printf("Gateway: %s\n", gatewayStr);
            }
        }
        
        // Get channel number
        int32_t channel = 0;
        uCxWifiStationStatus_t channelStatus;
        if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CHANNEL, &channelStatus)) {
            channel = channelStatus.rspWifiStatusIdInt.int_val;
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
    if (!gConnected) {
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
    if (!gConnected) {
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

