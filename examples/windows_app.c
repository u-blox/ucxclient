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
#include <windows.h>  // For Sleep() and CreateFile()
#include <stdbool.h>
#include <conio.h>  // For _kbhit() and _getch()
#include <winhttp.h>  // For HTTP client to fetch from GitHub

#pragma comment(lib, "winhttp.lib")

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
#include "u_cx_gatt_client.h"
#include "u_cx_gatt_server.h"
#include "u_cx_sps.h"

// Port layer
#include "port/u_port.h"

// Application version
#define APP_VERSION "1.0.0"

// Settings file
#define SETTINGS_FILE "windows_app_settings.ini"

// URC Event flags
#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_NETWORK_DOWN       (1 << 1)

// Global handles
static uCxAtClient_t gAtClient;
static uCxHandle_t gUcxHandle;
static bool gConnected = false;

// Socket tracking
static int32_t gCurrentSocket = -1;

// Settings (saved to file)
static char gComPort[16] = "COM31";           // Default COM port
static char gWifiSsid[64] = "";               // Last WiFi SSID
static char gWifiPassword[64] = "";           // Last WiFi password
static char gRemoteAddress[128] = "";         // Last remote address/hostname

// URC event handling
static U_CX_MUTEX_HANDLE gUrcMutex;
static volatile uint32_t gUrcEventFlags = 0;

// Menu state
typedef enum {
    MENU_MAIN,
    MENU_BLUETOOTH,
    MENU_WIFI,
    MENU_SOCKET,
    MENU_SPS,
    MENU_API_LIST,
    MENU_EXIT
} MenuState_t;

static MenuState_t gMenuState = MENU_MAIN;

// API Command structure for dynamic listing
typedef struct {
    char atCommand[128];
    char ucxApi[128];
    char description[256];
} ApiCommand_t;

static ApiCommand_t *gApiCommands = NULL;
static int gApiCommandCount = 0;

// Forward declarations
static void printHeader(void);
static void printMenu(void);
static void handleUserInput(void);
static bool connectDevice(const char *comPort);
static void disconnectDevice(void);
static void listAvailableComPorts(void);
static char* selectComPortFromList(void);
static void listAllApiCommands(void);
static bool fetchApiCommandsFromGitHub(const char *product, const char *version);
static void parseYamlCommands(const char *yamlContent);
static void freeApiCommands(void);
static char* fetchLatestVersion(const char *product);
static char* httpGetRequest(const wchar_t *server, const wchar_t *path);
static void executeAtTest(void);
static void executeAti9(void);
static void showBluetoothStatus(void);
static void showWifiStatus(void);
static void bluetoothMenu(void);
static void bluetoothScan(void);
static void bluetoothConnect(void);
static void wifiMenu(void);
static void wifiScan(void);
static void wifiConnect(void);
static void wifiDisconnect(void);
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
static void spsSendString(void);
static void spsSendBinary(void);
static void spsDisconnect(void);

// ----------------------------------------------------------------
// HTTP Helper Functions
// ----------------------------------------------------------------

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
        }
    }
    
    free(response);
    return latestVersion;
}

static bool fetchApiCommandsFromGitHub(const char *product, const char *version)
{
    printf("Fetching API commands for %s version %s from GitHub...\n", product, version);
    
    wchar_t path[512];
    swprintf(path, 512, L"/u-blox/u-connectXpress/main/%S/%S/compiled_product.yaml", product, version);
    
    char *yamlContent = httpGetRequest(L"raw.githubusercontent.com", path);
    
    if (!yamlContent) {
        printf("ERROR: Failed to fetch YAML file from GitHub\n");
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
    if (!gApiCommands) return;
    
    gApiCommandCount = 0;
    
    // Find the command_groups section
    const char *cmdGroups = strstr(yamlContent, "command_groups:");
    if (!cmdGroups) {
        printf("ERROR: Could not find command_groups in YAML\n");
        return;
    }
    
    // Parse each AT command entry (look for "      AT" with 6 spaces - the indentation level under commands:)
    const char *ptr = cmdGroups;
    while ((ptr = strstr(ptr + 1, "\n      AT")) != NULL) {
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
                if (!newCommands) break;
                gApiCommands = newCommands;
            }
            
            strncpy(gApiCommands[gApiCommandCount].atCommand, atCmd, 127);
            gApiCommands[gApiCommandCount].atCommand[127] = '\0';
            strncpy(gApiCommands[gApiCommandCount].ucxApi, ucxApi, 127);
            gApiCommands[gApiCommandCount].ucxApi[127] = '\0';
            strncpy(gApiCommands[gApiCommandCount].description, desc, 255);
            gApiCommands[gApiCommandCount].description[255] = '\0';
            
            gApiCommandCount++;
        }
    }
    
    printf("Parsed %d API commands\n", gApiCommandCount);
}

static void freeApiCommands(void)
{
    if (gApiCommands) {
        free(gApiCommands);
        gApiCommands = NULL;
    }
    gApiCommandCount = 0;
}

// ----------------------------------------------------------------
// Password Obfuscation Helper Functions
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// Socket Functions
// ----------------------------------------------------------------

static void socketCreateTcp(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Create TCP Socket ---\n");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_PROTOCOL_TCP, &socketHandle);
    
    if (result == 0) {
        printf("Successfully created TCP socket\n");
        printf("Socket handle: %d\n", socketHandle);
        gCurrentSocket = socketHandle;
    } else {
        printf("ERROR: Failed to create socket (code %d)\n", result);
    }
}

static void socketCreateUdp(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Create UDP Socket ---\n");
    
    int32_t socketHandle = -1;
    int32_t result = uCxSocketCreate1(&gUcxHandle, U_PROTOCOL_UDP, &socketHandle);
    
    if (result == 0) {
        printf("Successfully created UDP socket\n");
        printf("Socket handle: %d\n", socketHandle);
        gCurrentSocket = socketHandle;
    } else {
        printf("ERROR: Failed to create socket (code %d)\n", result);
    }
}

static void socketConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentSocket < 0) {
        printf("ERROR: No socket created. Create a socket first.\n");
        return;
    }
    
    printf("\n--- Connect Socket ---\n");
    printf("Socket handle: %d\n", gCurrentSocket);
    
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
            printf("Using saved address: %s\n", hostname);
        }
    }
    
    printf("Enter port: ");
    scanf("%d", &port);
    getchar(); // consume newline
    
    printf("Connecting to %s:%d...\n", hostname, port);
    
    int32_t result = uCxSocketConnect(&gUcxHandle, gCurrentSocket, hostname, port);
    
    if (result == 0) {
        printf("Successfully connected\n");
        // Save the address
        strncpy(gRemoteAddress, hostname, sizeof(gRemoteAddress) - 1);
        gRemoteAddress[sizeof(gRemoteAddress) - 1] = '\0';
        saveSettings();
    } else {
        printf("ERROR: Failed to connect (code %d)\n", result);
    }
}

static void socketSendData(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentSocket < 0) {
        printf("ERROR: No socket created/connected. Connect a socket first.\n");
        return;
    }
    
    printf("\n--- Send Socket Data ---\n");
    printf("Socket handle: %d\n", gCurrentSocket);
    printf("Enter data to send: ");
    
    char data[1001];
    if (fgets(data, sizeof(data), stdin)) {
        char *end = strchr(data, '\n');
        if (end) *end = '\0';
        
        size_t len = strlen(data);
        printf("Sending %zu bytes...\n", len);
        
        int32_t result = uCxSocketWrite(&gUcxHandle, gCurrentSocket, (uint8_t*)data, len);
        
        if (result >= 0) {
            printf("Successfully sent %d bytes\n", result);
        } else {
            printf("ERROR: Failed to send data (code %d)\n", result);
        }
    }
}

static void socketReadData(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentSocket < 0) {
        printf("ERROR: No socket created/connected\n");
        return;
    }
    
    printf("\n--- Read Socket Data ---\n");
    printf("Socket handle: %d\n", gCurrentSocket);
    printf("Enter number of bytes to read (max 1000): ");
    
    int length;
    scanf("%d", &length);
    getchar(); // consume newline
    
    if (length <= 0 || length > 1000) {
        printf("ERROR: Invalid length. Must be 1-1000\n");
        return;
    }
    
    uint8_t buffer[1001];
    int32_t result = uCxSocketRead(&gUcxHandle, gCurrentSocket, length, buffer);
    
    if (result > 0) {
        buffer[result] = '\0';  // Null terminate for display
        printf("Received %d bytes:\n", result);
        printf("%s\n", buffer);
    } else if (result == 0) {
        printf("No data available\n");
    } else {
        printf("ERROR: Failed to read data (code %d)\n", result);
    }
}

static void socketClose(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    if (gCurrentSocket < 0) {
        printf("ERROR: No socket to close\n");
        return;
    }
    
    printf("\n--- Close Socket ---\n");
    printf("Closing socket %d...\n", gCurrentSocket);
    
    int32_t result = uCxSocketClose(&gUcxHandle, gCurrentSocket);
    
    if (result == 0) {
        printf("Successfully closed socket\n");
        gCurrentSocket = -1;
    } else {
        printf("ERROR: Failed to close socket (code %d)\n", result);
    }
}

static void socketListStatus(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Socket Status ---\n");
    
    uCxSocketListStatusBegin(&gUcxHandle);
    
    uCxSocketListStatus_t status;
    int count = 0;
    
    while (uCxSocketListStatusGetNext(&gUcxHandle, &status)) {
        count++;
        printf("Socket %d: Protocol=%s, Status=%s\n",
               status.socket_handle,
               status.protocol == U_PROTOCOL_TCP ? "TCP" : "UDP",
               status.socket_status == 0 ? "Not Connected" :
               status.socket_status == 1 ? "Listening" : "Connected");
    }
    
    uCxEnd(&gUcxHandle);
    
    if (count == 0) {
        printf("  No sockets\n");
    }
    
    if (gCurrentSocket >= 0) {
        printf("\nCurrent socket: %d\n", gCurrentSocket);
    }
}

// ----------------------------------------------------------------
// SPS Functions
// ----------------------------------------------------------------

static void spsEnableService(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Enable SPS Service ---\n");
    
    int32_t result = uCxSpsSetServiceEnable(&gUcxHandle, U_SPS_SERVICE_OPTION_ENABLE_SPS_SERVICE);
    
    if (result == 0) {
        printf("Successfully enabled SPS service\n");
        printf("NOTE: SPS will be active after reboot\n");
    } else {
        printf("ERROR: Failed to enable SPS (code %d)\n", result);
    }
}

static void spsConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Connect SPS ---\n");
    printf("Enter Bluetooth connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Connecting SPS on connection %d...\n", connHandle);
    
    int32_t result = uCxSpsConnect2(&gUcxHandle, connHandle, 0);  // No flow control
    
    if (result == 0) {
        printf("Successfully initiated SPS connection\n");
        printf("Wait for +UESPSC URC event...\n");
    } else {
        printf("ERROR: Failed to connect SPS (code %d)\n", result);
    }
}

static void spsSendString(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Send SPS String Data ---\n");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Enter data to send: ");
    char data[1001];
    if (fgets(data, sizeof(data), stdin)) {
        char *end = strchr(data, '\n');
        if (end) *end = '\0';
        
        size_t len = strlen(data);
        int32_t result = uCxSpsWrite(&gUcxHandle, connHandle, (uint8_t*)data, len);
        
        if (result >= 0) {
            printf("Successfully sent %d bytes\n", result);
        } else {
            printf("ERROR: Failed to send data (code %d)\n", result);
        }
    }
}

static void spsSendBinary(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Send SPS Binary Data ---\n");
    printf("Enter connection handle: ");
    
    int connHandle;
    scanf("%d", &connHandle);
    getchar(); // consume newline
    
    printf("Enter hex data (e.g., 48656C6C6F): ");
    char hexInput[2001];
    if (fgets(hexInput, sizeof(hexInput), stdin)) {
        char *end = strchr(hexInput, '\n');
        if (end) *end = '\0';
        
        // Convert hex string to bytes
        size_t hexLen = strlen(hexInput);
        if (hexLen % 2 != 0) {
            printf("ERROR: Hex string must have even number of characters\n");
            return;
        }
        
        uint8_t data[1000];
        size_t dataLen = hexLen / 2;
        
        for (size_t i = 0; i < dataLen; i++) {
            sscanf(&hexInput[i * 2], "%2hhx", &data[i]);
        }
        
        int32_t result = uCxSpsWrite(&gUcxHandle, connHandle, data, dataLen);
        
        if (result >= 0) {
            printf("Successfully sent %d bytes\n", result);
        } else {
            printf("ERROR: Failed to send data (code %d)\n", result);
        }
    }
}

static void spsDisconnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Disconnect SPS ---\n");
    printf("NOTE: To disconnect SPS, disconnect the Bluetooth connection\n");
    printf("      or close the socket that SPS is running over\n");
}

// ----------------------------------------------------------------
// Main Function
// ----------------------------------------------------------------

// Main function
int main(int argc, char *argv[])
{
    // Load settings from file
    loadSettings();
    
    // Check for COM port argument
    if (argc > 1) {
        strncpy(gComPort, argv[1], sizeof(gComPort) - 1);
        gComPort[sizeof(gComPort) - 1] = '\0';
    } else {
        // No argument provided - show available ports and let user choose
        printf("No COM port specified. Available ports:\n\n");
        listAvailableComPorts();
        
        char *selectedPort = selectComPortFromList();
        if (selectedPort) {
            strncpy(gComPort, selectedPort, sizeof(gComPort) - 1);
            gComPort[sizeof(gComPort) - 1] = '\0';
            free(selectedPort);
        } else {
            printf("No port selected. Using last saved port: %s\n", gComPort);
        }
    }
    
    printHeader();
    
    // Enable UCX logging to see AT commands and responses
    printf("Enabling UCX logging (AT commands, responses, debug info)...\n");
    uCxLogEnable();
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Windows Console App started");
    printf("UCX logging is now active - you'll see detailed AT traffic below\n\n");
    
    // Try to auto-connect
    printf("Attempting to connect to %s...\n", gComPort);
    if (connectDevice(gComPort)) {
        printf("Connected successfully!\n\n");
        saveSettings();  // Save successful port
    } else {
        printf("Failed to connect. You can try again from the menu.\n\n");
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
    
    printf("\nGoodbye!\n");
    return 0;
}

static void printHeader(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  u-connectXpress ucxclient App v%s\n", APP_VERSION);
    printf("========================================\n");
    printf("Simple C application for NORA-B26 and NORA-W36\n");
    printf("\n");
    printf("NOTE: UCX Logging is %s\n", uCxLogIsEnabled() ? "ENABLED" : "DISABLED");
    printf("      AT commands/responses will appear in this console\n");
    printf("      Use menu option [8] to toggle logging on/off\n");
    printf("\n");
}

static void printMenu(void)
{
    printf("\n");
    
    switch (gMenuState) {
        case MENU_MAIN:
            printf("--- Main Menu ---\n");
            if (gConnected) {
                printf("  Connected to: %s\n", gComPort);
            } else {
                printf("  Status: Not connected\n");
            }
            printf("  UCX Logging: %s\n", uCxLogIsEnabled() ? "ENABLED" : "DISABLED");
            printf("\n");
            printf("  [1] Connect to device\n");
            printf("  [2] Disconnect\n");
            printf("  [3] List API commands\n");
            printf("  [4] AT test (basic communication)\n");
            printf("  [5] ATI9 (device info)\n");
            printf("  [6] Bluetooth menu\n");
            printf("  [7] WiFi menu\n");
            printf("  [8] Toggle UCX logging (AT traffic)\n");
            printf("  [9] Socket menu (TCP/UDP)\n");
            printf("  [a] SPS menu (Bluetooth Serial)\n");
            printf("  [0] Exit\n");
            break;
            
        case MENU_BLUETOOTH:
            printf("--- Bluetooth Menu ---\n");
            printf("  [1] Show BT status\n");
            printf("  [2] Scan for devices\n");
            printf("  [3] Connect to device\n");
            printf("  [4] List active connections\n");
            printf("  [0] Back to main menu\n");
            break;
            
        case MENU_WIFI:
            printf("--- WiFi Menu ---\n");
            printf("  [1] Show WiFi status\n");
            printf("  [2] Scan networks\n");
            printf("  [3] Connect to network\n");
            printf("  [4] Disconnect from network\n");
            printf("  [0] Back to main menu\n");
            break;
            
        case MENU_SOCKET:
            printf("--- Socket Menu (TCP/UDP) ---\n");
            printf("  [1] Create TCP socket\n");
            printf("  [2] Create UDP socket\n");
            printf("  [3] Connect socket\n");
            printf("  [4] Send data\n");
            printf("  [5] Read data\n");
            printf("  [6] Close socket\n");
            printf("  [7] List sockets\n");
            printf("  [0] Back to main menu\n");
            break;
            
        case MENU_SPS:
            printf("--- SPS Menu (Bluetooth Serial Port Service) ---\n");
            printf("  [1] Enable SPS service\n");
            printf("  [2] Connect SPS on BT connection\n");
            printf("  [3] Send string data\n");
            printf("  [4] Send binary data\n");
            printf("  [5] Disconnect SPS\n");
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
    
    switch (gMenuState) {
        case MENU_MAIN:
            switch (choice) {
                case 1: {
                    printf("Enter COM port (e.g., COM31): ");
                    if (fgets(input, sizeof(input), stdin)) {
                        input[strcspn(input, "\n")] = 0;
                        if (strlen(input) > 0) {
                            strncpy(gComPort, input, sizeof(gComPort) - 1);
                            connectDevice(gComPort);
                        }
                    }
                    break;
                }
                case 2:
                    disconnectDevice();
                    break;
                case 3:
                    gMenuState = MENU_API_LIST;
                    break;
                case 4:
                    executeAtTest();
                    break;
                case 5:
                    executeAti9();
                    break;
                case 6:
                    gMenuState = MENU_BLUETOOTH;
                    break;
                case 7:
                    gMenuState = MENU_WIFI;
                    break;
                case 8:
                    if (uCxLogIsEnabled()) {
                        uCxLogDisable();
                        printf("UCX logging DISABLED\n");
                    } else {
                        uCxLogEnable();
                        printf("UCX logging ENABLED\n");
                        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Logging re-enabled from menu");
                    }
                    break;
                case 9:
                    gMenuState = MENU_SOCKET;
                    break;
                case 10:  // Also accept 'a' or 'A'
                    gMenuState = MENU_SPS;
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
                    wifiScan();
                    break;
                case 3:
                    wifiConnect();
                    break;
                case 4:
                    wifiDisconnect();
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
                    gMenuState = MENU_MAIN;
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
                    spsSendString();
                    break;
                case 4:
                    spsSendBinary();
                    break;
                case 5:
                    spsDisconnect();
                    break;
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

static bool connectDevice(const char *comPort)
{
    if (gConnected) {
        printf("Already connected. Disconnect first.\n");
        return false;
    }
    
    printf("Connecting to %s...\n", comPort);
    
    // Initialize AT client (like windows_test.c)
    uPortAtInit(&gAtClient);
    
    // Open COM port
    if (!uPortAtOpen(&gAtClient, comPort, 115200, false)) {
        printf("ERROR: Failed to open %s\n", comPort);
        return false;
    }
    
    printf("COM port opened successfully\n");
    
    // Turn off echo to avoid "Unexpected data" warnings
    printf("Disabling AT echo...\n");
    int32_t result = uCxAtClientExecSimpleCmd(&gAtClient, "ATE0");
    if (result != 0) {
        printf("Warning: Failed to disable echo (error %d), continuing anyway...\n", result);
    }
    
    // Initialize UCX handle
    uCxInit(&gAtClient, &gUcxHandle);
    
    // Create mutex for URC event handling
    U_CX_MUTEX_CREATE(gUrcMutex);
    
    // Register URC handlers for WiFi network events
    uCxWifiRegisterStationNetworkUp(&gUcxHandle, networkUpUrc);
    uCxWifiRegisterStationNetworkDown(&gUcxHandle, networkDownUrc);
    
    printf("UCX initialized successfully\n");
    
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
    
    // Delete mutex
    U_CX_MUTEX_DELETE(gUrcMutex);
    
    // Close COM port
    uPortAtClose(&gAtClient);
    
    gConnected = false;
    printf("Disconnected.\n");
}

// Load settings from file
static void loadSettings(void)
{
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (f) {
        char line[256];
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
            else if (strncmp(line, "wifi_ssid=", 10) == 0) {
                strncpy(gWifiSsid, line + 10, sizeof(gWifiSsid) - 1);
                gWifiSsid[sizeof(gWifiSsid) - 1] = '\0';
            }
            else if (strncmp(line, "wifi_password=", 14) == 0) {
                // Deobfuscate password from hex
                char obfuscated[128];
                strncpy(obfuscated, line + 14, sizeof(obfuscated) - 1);
                obfuscated[sizeof(obfuscated) - 1] = '\0';
                deobfuscatePassword(obfuscated, gWifiPassword, sizeof(gWifiPassword));
            }
            else if (strncmp(line, "remote_address=", 15) == 0) {
                strncpy(gRemoteAddress, line + 15, sizeof(gRemoteAddress) - 1);
                gRemoteAddress[sizeof(gRemoteAddress) - 1] = '\0';
            }
        }
        fclose(f);
    }
}

// Save settings to file
static void saveSettings(void)
{
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (f) {
        fprintf(f, "last_port=%s\n", gComPort);
        fprintf(f, "wifi_ssid=%s\n", gWifiSsid);
        
        // Obfuscate password before saving
        char obfuscatedPassword[128];
        obfuscatePassword(gWifiPassword, obfuscatedPassword, sizeof(obfuscatedPassword));
        fprintf(f, "wifi_password=%s\n", obfuscatedPassword);
        
        fprintf(f, "remote_address=%s\n", gRemoteAddress);
        fclose(f);
    }
}

// List available COM ports
static void listAvailableComPorts(void)
{
    // Create buffer for port names
    char ports[32][16];
    int count = 0;
    
    // Note: uPortEnumerateComPorts may not be available in all port implementations
    // For Windows, we'll use a simple approach that checks common port names
    for (int i = 1; i <= 256; i++) {
        char portName[16];
        snprintf(portName, sizeof(portName), "COM%d", i);
        
        // Try to open the port to see if it exists
        HANDLE hPort = CreateFile(portName, GENERIC_READ | GENERIC_WRITE,
                                  0, NULL, OPEN_EXISTING, 0, NULL);
        if (hPort != INVALID_HANDLE_VALUE) {
            CloseHandle(hPort);
            printf("  %s\n", portName);
            count++;
            if (count < 32) {
                strcpy(ports[count-1], portName);
            }
        }
    }
    
    if (count == 0) {
        printf("  No COM ports found.\n");
    }
}

// Select COM port from list
static char* selectComPortFromList(void)
{
    char input[64];
    printf("\nEnter COM port name (e.g., COM31) or press Enter to use last saved port: ");
    
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
            }
        }
    }
    
    return NULL;  // User pressed Enter without input
}

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
                
                // Display commands
                for (int i = 0; i < gApiCommandCount; i++) {
                    printf("[%d]\n", i + 1);
                    if (gApiCommands[i].atCommand[0] != '\0') {
                        printf("  AT Command: %s\n", gApiCommands[i].atCommand);
                    }
                    if (gApiCommands[i].ucxApi[0] != '\0') {
                        printf("  UCX API:    %s\n", gApiCommands[i].ucxApi);
                    }
                    if (gApiCommands[i].description[0] != '\0') {
                        printf("  Description: %s\n", gApiCommands[i].description);
                    }
                    printf("\n");
                    
                    // Pause every 20 commands
                    if ((i + 1) % 20 == 0 && i + 1 < gApiCommandCount) {
                        printf("--- Press Enter for more commands ---");
                        getchar();
                    }
                }
                
                printf("========================================\n");
                printf("Total: %d commands\n", gApiCommandCount);
            }
            
            if (versionToUse != version && versionToUse) {
                free(versionToUse);
            }
        }
    } else {
        // Static fallback list
        printf("\n--- GENERAL API (u_cx_general.h) ---\n");
        printf("  uCxGeneralGetManufacturerIdentificationBegin()  - Get manufacturer ID\n");
        printf("  uCxGeneralGetDeviceModelIdentificationBegin()   - Get device model\n");
        printf("  uCxGeneralGetSoftwareVersionBegin()             - Get software version\n");
        printf("  uCxGeneralGetIdentInfoBegin()                   - Get identification info\n");
        printf("  uCxGeneralGetSerialNumberBegin()                - Get device serial number\n");
        printf("\n");
        
        printf("--- SYSTEM API (u_cx_system.h) ---\n");
        printf("  uCxSystemStoreConfiguration()                   - Store current config to flash\n");
        printf("  uCxSystemDefaultSettings()                      - Reset to factory defaults\n");
        printf("  uCxSystemReboot()                               - Reboot the module\n");
        printf("  uCxSystemGetLocalAddressBegin()                 - Get local MAC addresses\n");
        printf("\n");
        
        printf("--- BLUETOOTH API (u_cx_bluetooth.h) ---\n");
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
        
        printf("--- WIFI API (u_cx_wifi.h) ---\n");
        printf("  uCxWifiStationSetConnectionParamsBegin()        - Set WiFi connection params\n");
        printf("  uCxWifiStationConnectBegin()                    - Connect to WiFi network\n");
        printf("  uCxWifiStationDisconnectBegin()                 - Disconnect from WiFi\n");
        printf("  uCxWifiStationStatusBegin()                     - Get WiFi connection status\n");
        printf("  uCxWifiStationScanDefaultBegin()                - Scan for WiFi networks\n");
        printf("  uCxWifiStationScanDefaultGetNext()              - Get next scan result\n");
        printf("  uCxWifiApSetConnectionParamsBegin()             - Set AP mode params\n");
        printf("  uCxWifiApStartBegin()                           - Start AP mode\n");
        printf("  uCxWifiApStopBegin()                            - Stop AP mode\n");
        printf("  uCxWifiApGetStationListBegin()                  - List connected stations\n");
        printf("\n");
        
        printf("--- SOCKET API (u_cx_socket.h) ---\n");
        printf("  uCxSocketCreate()                               - Create TCP/UDP socket\n");
        printf("  uCxSocketConnect()                              - Connect socket to remote\n");
        printf("  uCxSocketListen()                               - Listen for connections\n");
        printf("  uCxSocketAccept()                               - Accept incoming connection\n");
        printf("  uCxSocketClose()                                - Close socket\n");
        printf("  uCxSocketWrite()                                - Write data to socket\n");
        printf("  uCxSocketRead()                                 - Read data from socket\n");
        printf("\n");
        
        printf("--- MQTT API (u_cx_mqtt.h) ---\n");
        printf("  uCxMqttConnectBegin()                           - Connect to MQTT broker\n");
        printf("  uCxMqttDisconnect()                             - Disconnect from broker\n");
        printf("  uCxMqttPublishBegin()                           - Publish message to topic\n");
        printf("  uCxMqttSubscribeBegin()                         - Subscribe to topic\n");
        printf("  uCxMqttUnsubscribeBegin()                       - Unsubscribe from topic\n");
        printf("\n");
        
        printf("--- SECURITY API (u_cx_security.h) ---\n");
        printf("  uCxSecurityTlsCertificateStoreBegin()           - Store TLS certificate\n");
        printf("  uCxSecurityTlsCertificateRemove()               - Remove certificate\n");
        printf("  uCxSecurityTlsCertificateListBegin()            - List stored certificates\n");
        printf("\n");
        
        printf("--- GATT CLIENT API (u_cx_gatt_client.h) ---\n");
        printf("  uCxGattClientDiscoverAllPrimaryServicesBegin()  - Discover GATT services\n");
        printf("  uCxGattClientDiscoverCharacteristicsBegin()     - Discover characteristics\n");
        printf("  uCxGattClientWriteCharacteristicBegin()         - Write to characteristic\n");
        printf("  uCxGattClientReadCharacteristicBegin()          - Read from characteristic\n");
        printf("  uCxGattClientSubscribeBegin()                   - Subscribe to notifications\n");
        printf("\n");
        
        printf("--- GATT SERVER API (u_cx_gatt_server.h) ---\n");
        printf("  uCxGattServerAddServiceBegin()                  - Add GATT service\n");
        printf("  uCxGattServerAddCharacteristicBegin()           - Add characteristic\n");
        printf("  uCxGattServerSetCharacteristicValueBegin()      - Set characteristic value\n");
        printf("\n");
        
        printf("--- SPS API (u_cx_sps.h) ---\n");
        printf("  uCxSpsConnect()                                 - Connect SPS channel\n");
        printf("  uCxSpsDisconnect()                              - Disconnect SPS channel\n");
        printf("  uCxSpsWrite()                                   - Write SPS data\n");
        printf("  uCxSpsRead()                                    - Read SPS data\n");
        printf("\n");
        
        printf("=========================================================\n");
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

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
    
    // Simple AT command
    int32_t result = uCxAtClientExecSimpleCmd(&gAtClient, "AT");
    
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

static void showWifiStatus(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- WiFi Status ---\n");
    
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
        printf("ERROR: Failed to get WiFi status\n");
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
    
    // Start discovery (type 0 = general discovery, timeout in milliseconds 10000 = 10 sec)
    uCxBluetoothDiscovery3Begin(&gUcxHandle, 0, 0, 10000);
    
    int deviceCount = 0;
    uCxBluetoothDiscovery_t device;
    
    // Get discovered devices
    while (uCxBluetoothDiscovery3GetNext(&gUcxHandle, &device)) {
        deviceCount++;
        printf("Device %d:\n", deviceCount);
        printf("  Address: %02X:%02X:%02X:%02X:%02X:%02X\n",
               device.bd_addr.address[0], device.bd_addr.address[1],
               device.bd_addr.address[2], device.bd_addr.address[3],
               device.bd_addr.address[4], device.bd_addr.address[5]);
        
        if (device.device_name && device.device_name[0] != '\0') {
            printf("  Name: %s\n", device.device_name);
        }
        
        printf("  RSSI: %d dBm\n", device.rssi);
        printf("\n");
    }
    
    uCxEnd(&gUcxHandle);
    
    if (deviceCount == 0) {
        printf("No devices found.\n");
    } else {
        printf("Found %d device(s).\n", deviceCount);
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

static void bluetoothConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Bluetooth Connect ---\n");
    printf("Enter Bluetooth address (format: XX:XX:XX:XX:XX:XX): ");
    
    char addrStr[20];
    if (fgets(addrStr, sizeof(addrStr), stdin)) {
        // Parse MAC address
        uBtLeAddress_t addr;
        addr.type = U_BD_ADDRESS_TYPE_PUBLIC;
        
        int values[6];
        if (sscanf(addrStr, "%02x:%02x:%02x:%02x:%02x:%02x",
                   &values[0], &values[1], &values[2], &values[3], &values[4], &values[5]) == 6) {
            
            for (int i = 0; i < 6; i++) {
                addr.address[i] = (uint8_t)values[i];
            }
            
            printf("Connecting to device...\n");
            
            // uCxBluetoothConnect returns conn handle on success, negative on error
            int32_t connHandle = uCxBluetoothConnect(&gUcxHandle, &addr);
            if (connHandle >= 0) {
                printf("Connected successfully! Connection handle: %d\n", connHandle);
            } else {
                printf("ERROR: Failed to connect to device (error: %d)\n", connHandle);
            }
        } else {
            printf("ERROR: Invalid MAC address format\n");
        }
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

static void wifiScan(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- WiFi Network Scan ---\n");
    printf("Scanning for networks... (this may take several seconds)\n\n");
    
    // Start WiFi scan
    uCxWifiStationScanDefaultBegin(&gUcxHandle);
    
    int networkCount = 0;
    uCxWifiStationScanDefault_t network;
    
    // Get scan results
    while (uCxWifiStationScanDefaultGetNext(&gUcxHandle, &network)) {
        networkCount++;
        printf("Network %d:\n", networkCount);
        printf("  SSID: %s\n", network.ssid);
        printf("  BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
               network.bssid.address[0], network.bssid.address[1],
               network.bssid.address[2], network.bssid.address[3],
               network.bssid.address[4], network.bssid.address[5]);
        printf("  Channel: %d\n", network.channel);
        printf("  RSSI: %d dBm\n", network.rssi);
        
        // Print security type based on authentication suites
        printf("  Security: ");
        if (network.authentication_suites == 0) {
            printf("Open\n");
        } else {
            if (network.authentication_suites & (1 << 5)) printf("WPA3 ");
            if (network.authentication_suites & (1 << 4)) printf("WPA2 ");
            if (network.authentication_suites & (1 << 3)) printf("WPA ");
            if (network.authentication_suites & (1 << 1)) printf("PSK ");
            if (network.authentication_suites & (1 << 2)) printf("EAP ");
            printf("(0x%X)\n", network.authentication_suites);
        }
        printf("\n");
    }
    
    uCxEnd(&gUcxHandle);
    
    if (networkCount == 0) {
        printf("No networks found.\n");
    } else {
        printf("Found %d network(s).\n", networkCount);
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

static void wifiConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- WiFi Connect ---\n");
    
    char ssid[64];
    char password[64];
    
    // Show saved SSID if available
    if (strlen(gWifiSsid) > 0) {
        printf("Last SSID: %s\n", gWifiSsid);
    }
    
    printf("Enter SSID (or press Enter to use saved): ");
    if (fgets(ssid, sizeof(ssid), stdin)) {
        // Remove trailing newline
        char *end = strchr(ssid, '\n');
        if (end) *end = '\0';
        
        // If empty and we have saved SSID, use it
        if (strlen(ssid) == 0 && strlen(gWifiSsid) > 0) {
            strncpy(ssid, gWifiSsid, sizeof(ssid) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
            strncpy(password, gWifiPassword, sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
            printf("Using saved credentials for '%s'\n", ssid);
        } else {
            printf("Enter password (or press Enter for open network): ");
            if (fgets(password, sizeof(password), stdin)) {
                // Remove trailing newline
                end = strchr(password, '\n');
                if (end) *end = '\0';
            }
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
        
        // Try to connect
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
            uCxWifiStationStatus_t status;
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &status)) {
                int32_t rssi = status.rspWifiStatusIdInt.int_val;
                if (rssi != -32768) {
                    printf("Signal strength: %d dBm\n", rssi);
                }
                uCxEnd(&gUcxHandle);
            }
            
            // Get IP address using WiFi Station Network Status (AT+UWSNST)
            uSockIpAddress_t ipAddr;
            char ipStr[40];  // Allow for IPv6
            
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
                if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                    printf("Gateway: %s\n", ipStr);
                }
            }
            
            // Save WiFi credentials for next time
            strncpy(gWifiSsid, ssid, sizeof(gWifiSsid) - 1);
            gWifiSsid[sizeof(gWifiSsid) - 1] = '\0';
            strncpy(gWifiPassword, password, sizeof(gWifiPassword) - 1);
            gWifiPassword[sizeof(gWifiPassword) - 1] = '\0';
            saveSettings();
        } else {
            printf("Connection failed - timeout waiting for network up event\n");
        }
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}

static void wifiDisconnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- WiFi Disconnect ---\n");
    printf("Disconnecting from WiFi...\n");
    
    if (uCxWifiStationDisconnect(&gUcxHandle) == 0) {
        printf("Disconnected successfully.\n");
    } else {
        printf("ERROR: Failed to disconnect\n");
    }
    
    printf("\nPress Enter to continue...");
    getchar();
}
