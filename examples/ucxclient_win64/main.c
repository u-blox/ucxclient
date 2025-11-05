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
 * This is the main entry point with global variable definitions.
 */

#include "common.h"
#include "urc_handlers.h"
#include "settings.h"
#include "device_connection.h"
#include "com_port_detect.h"
#include "api_commands.h"
#include "bluetooth.h"
#include "wifi.h"
#include "socket.h"
#include "sps.h"
#include "gatt.h"
#include "mqtt.h"
#include "http.h"
#include "firmware_update.h"
#include "ui_menus.h"

// ----------------------------------------------------------------
// Global Variable Definitions
// ----------------------------------------------------------------

// FTDI Library
HMODULE gFtd2xxModule = NULL;
PFN_FT_ListDevices gpFT_ListDevices = NULL;
PFN_FT_Open gpFT_Open = NULL;
PFN_FT_OpenEx gpFT_OpenEx = NULL;
PFN_FT_GetComPortNumber gpFT_GetComPortNumber = NULL;
PFN_FT_Close gpFT_Close = NULL;

// UCX Handles
uCxAtClient_t gUcxAtClient;
uCxHandle_t gUcxHandle;
bool gConnected = false;

// Socket tracking
int32_t gCurrentSocket = -1;

// Settings (saved to file)
char gComPort[16] = "COM31";           // Default COM port
char gLastDeviceModel[64] = "";        // Last connected device model
char gWifiSsid[64] = "";               // Last Wi-Fi SSID
char gWifiPassword[64] = "";           // Last Wi-Fi password
char gRemoteAddress[128] = "";         // Last remote address/hostname

// Dynamic firmware paths
ProductFirmwarePath_t gProductFirmwarePaths[MAX_PRODUCT_PATHS];
int gProductFirmwarePathCount = 0;

// Device information
char gDeviceModel[64] = "";            // Device model (e.g., "NORA-W36")
char gDeviceFirmware[64] = "";         // Firmware version (e.g., "3.1.0")

// URC event handling
U_CX_MUTEX_HANDLE gUrcMutex;
volatile uint32_t gUrcEventFlags = 0;

// Ping test results
volatile int32_t gPingSuccess = 0;
volatile int32_t gPingFailed = 0;
volatile int32_t gPingAvgTime = 0;
volatile int32_t gPingTimes[MAX_PING_TIMES];
volatile int32_t gPingCount = 0;

// Reboot timing
volatile ULONGLONG gStartupTimestamp = 0;

// Menu state
MenuState_t gMenuState = MENU_MAIN;

// API Commands
ApiCommand_t *gApiCommands = NULL;
int gApiCommandCount = 0;

// Settings file path
char gSettingsFilePath[MAX_PATH] = "";

// ----------------------------------------------------------------
// Main Entry Point
// ----------------------------------------------------------------

int main(int argc, char *argv[])
{
    // Set console to UTF-8 to properly display Unicode box drawing characters
    SetConsoleOutputCP(CP_UTF8);
    
    // Initialize settings file path (next to executable)
    getExecutableDirectory(gSettingsFilePath, sizeof(gSettingsFilePath));
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
