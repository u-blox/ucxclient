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

#ifndef UCXCLIENT_COMMON_H
#define UCXCLIENT_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <windows.h>
#include <stdbool.h>
#include <conio.h>
#include <winhttp.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>

// Disable MSVC warnings for auto-generated UCX API headers
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4200 4201)
#endif

// Include ucxclient headers
#include "../../inc/u_cx_at_client.h"
#include "../../inc/u_cx_at_params.h"
#include "../../inc/u_cx_log.h"
#include "../../ucx_api/u_cx.h"
#include "../../ucx_api/u_cx_general.h"
#include "../../ucx_api/u_cx_system.h"
#include "../../ucx_api/u_cx_bluetooth.h"
#include "../../ucx_api/u_cx_wifi.h"
#include "../../ucx_api/u_cx_socket.h"
#include "../../ucx_api/u_cx_mqtt.h"
#include "../../ucx_api/u_cx_security.h"
#include "../../ucx_api/u_cx_diagnostics.h"
#include "../../ucx_api/u_cx_gatt_client.h"
#include "../../ucx_api/u_cx_gatt_server.h"
#include "../../ucx_api/u_cx_sps.h"
#include "../../ucx_api/u_cx_firmware_update.h"

// Port layer
#include "../port/u_port.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// ----------------------------------------------------------------
// Application Configuration
// ----------------------------------------------------------------

#define APP_VERSION "1.0.0"
#define SETTINGS_FILENAME "ucxclient_win64_settings.ini"
#define MAX_DATA_BUFFER 1000
#define MAX_PING_TIMES 10
#define MAX_PRODUCT_PATHS 10

// ----------------------------------------------------------------
// URC Event Flags
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// FTDI Types and Function Pointers
// ----------------------------------------------------------------

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

// ----------------------------------------------------------------
// Type Definitions
// ----------------------------------------------------------------

// Dynamic firmware path storage per product
typedef struct {
    char productName[64];
    char lastFirmwarePath[256];
} ProductFirmwarePath_t;

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

// API Command structure for dynamic listing
typedef struct {
    char atCommand[128];
    char ucxApi[128];
    char description[256];
    char chapter[128];
} ApiCommand_t;

// ----------------------------------------------------------------
// Global Variables (extern declarations)
// ----------------------------------------------------------------

// FTDI Library
extern HMODULE gFtd2xxModule;
extern PFN_FT_ListDevices gpFT_ListDevices;
extern PFN_FT_Open gpFT_Open;
extern PFN_FT_OpenEx gpFT_OpenEx;
extern PFN_FT_GetComPortNumber gpFT_GetComPortNumber;
extern PFN_FT_Close gpFT_Close;

// UCX Handles
extern uCxAtClient_t gUcxAtClient;
extern uCxHandle_t gUcxHandle;
extern bool gConnected;

// Socket tracking
extern int32_t gCurrentSocket;

// Settings (saved to file)
extern char gComPort[16];
extern char gLastDeviceModel[64];
extern char gWifiSsid[64];
extern char gWifiPassword[64];
extern char gRemoteAddress[128];

// Dynamic firmware paths
extern ProductFirmwarePath_t gProductFirmwarePaths[MAX_PRODUCT_PATHS];
extern int gProductFirmwarePathCount;

// Device information
extern char gDeviceModel[64];
extern char gDeviceFirmware[64];

// URC event handling
extern U_CX_MUTEX_HANDLE gUrcMutex;
extern volatile uint32_t gUrcEventFlags;

// Ping test results
extern volatile int32_t gPingSuccess;
extern volatile int32_t gPingFailed;
extern volatile int32_t gPingAvgTime;
extern volatile int32_t gPingTimes[MAX_PING_TIMES];
extern volatile int32_t gPingCount;

// Reboot timing
extern volatile ULONGLONG gStartupTimestamp;

// Menu state
extern MenuState_t gMenuState;

// API Commands
extern ApiCommand_t *gApiCommands;
extern int gApiCommandCount;

// Settings file path
extern char gSettingsFilePath[MAX_PATH];

#endif // UCXCLIENT_COMMON_H
