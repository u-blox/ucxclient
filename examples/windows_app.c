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
 * This is much simpler than Python GUI - direct C calls, no GC issues!
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <conio.h>  // For _kbhit() and _getch()

// Include ucxclient headers
#include "u_cx_at_client.h"
#include "u_cx.h"
#include "u_cx_general.h"
#include "u_cx_system.h"
#include "u_cx_bluetooth.h"
#include "u_cx_wifi.h"

// Port layer
#include "port/u_port.h"

// Application version
#define APP_VERSION "1.0.0"

// Global handles
static uCxAtClient_t gAtClient;
static uCxHandle_t gUcxHandle;
static bool gConnected = false;
static char gComPort[16] = "COM31";  // Default COM port

// Menu state
typedef enum {
    MENU_MAIN,
    MENU_BLUETOOTH,
    MENU_WIFI,
    MENU_EXIT
} MenuState_t;

static MenuState_t gMenuState = MENU_MAIN;

// Forward declarations
static void printHeader(void);
static void printMenu(void);
static void handleUserInput(void);
static bool connectDevice(const char *comPort);
static void disconnectDevice(void);
static void listApiCommands(void);
static void executeAtTest(void);
static void executeAti9(void);
static void showBluetoothStatus(void);
static void showWifiStatus(void);
static void bluetoothMenu(void);
static void wifiMenu(void);

// Main function
int main(int argc, char *argv[])
{
    // Check for COM port argument
    if (argc > 1) {
        strncpy(gComPort, argv[1], sizeof(gComPort) - 1);
        gComPort[sizeof(gComPort) - 1] = '\0';
    }
    
    printHeader();
    
    // Try to auto-connect
    printf("Attempting to connect to %s...\n", gComPort);
    if (connectDevice(gComPort)) {
        printf("Connected successfully!\n\n");
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
    
    printf("\nGoodbye!\n");
    return 0;
}

static void printHeader(void)
{
    printf("\n");
    printf("========================================\n");
    printf("  u-connectXpress Console App v%s\n", APP_VERSION);
    printf("========================================\n");
    printf("Simple C application for NORA-W36\n");
    printf("No Python, no DLL complexity!\n");
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
            printf("\n");
            printf("  [1] Connect to device\n");
            printf("  [2] Disconnect\n");
            printf("  [3] List API commands\n");
            printf("  [4] AT test (basic communication)\n");
            printf("  [5] ATI9 (device info)\n");
            printf("  [6] Bluetooth menu\n");
            printf("  [7] WiFi menu\n");
            printf("  [0] Exit\n");
            break;
            
        case MENU_BLUETOOTH:
            printf("--- Bluetooth Menu ---\n");
            printf("  [1] Show BT status\n");
            printf("  [2] Scan for devices\n");
            printf("  [3] List connections\n");
            printf("  [0] Back to main menu\n");
            break;
            
        case MENU_WIFI:
            printf("--- WiFi Menu ---\n");
            printf("  [1] Show WiFi status\n");
            printf("  [2] Scan networks\n");
            printf("  [3] Connect to network\n");
            printf("  [4] Disconnect\n");
            printf("  [0] Back to main menu\n");
            break;
            
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
                    listApiCommands();
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
                    printf("BT scan not yet implemented\n");
                    break;
                case 3:
                    printf("BT connections not yet implemented\n");
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
                    printf("WiFi scan not yet implemented\n");
                    break;
                case 3:
                    printf("WiFi connect not yet implemented\n");
                    break;
                case 4:
                    printf("WiFi disconnect not yet implemented\n");
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
    
    // Initialize UCX handle
    uCxInit(&gAtClient, &gUcxHandle);
    
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
    
    // Close COM port
    uPortAtClose(&gAtClient);
    
    gConnected = false;
    printf("Disconnected.\n");
}

static void listApiCommands(void)
{
    printf("\n--- Available API Commands ---\n");
    printf("General:\n");
    printf("  - uCxGeneralGetManufacturerIdentificationBegin\n");
    printf("  - uCxGeneralGetDeviceModelIdentificationBegin\n");
    printf("  - uCxGeneralGetSoftwareVersionBegin\n");
    printf("  - uCxGeneralGetIdentInfoBegin\n");
    printf("\n");
    printf("System:\n");
    printf("  - uCxSystemStoreConfiguration\n");
    printf("  - uCxSystemDefaultSettings\n");
    printf("  - uCxSystemReboot\n");
    printf("\n");
    printf("Bluetooth:\n");
    printf("  - uCxBluetoothGetMode\n");
    printf("  - uCxBluetoothListConnectionsBegin\n");
    printf("  - uCxBluetoothDiscoverBegin\n");
    printf("\n");
    printf("WiFi:\n");
    printf("  - uCxWifiStationStatusBegin\n");
    printf("  - uCxWifiStationScanDefaultBegin\n");
    printf("  - uCxWifiStationConnectBegin\n");
    printf("  - uCxWifiStationDisconnectBegin\n");
    printf("\n");
    printf("Press Enter to continue...");
    getchar();
}

static void executeAtTest(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- AT Test ---\n");
    
    // Simple AT command
    int32_t result = uCxAtClientExecSimpleCmd(&gAtClient, "AT");
    
    if (result == 0) {
        printf("Result: OK\n");
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
    
    // Use uCxGeneralGetIdentInfoBegin to get device info
    uCxGeneralGetIdentInfo_t info;
    
    if (uCxGeneralGetIdentInfoBegin(&gUcxHandle, &info)) {
        printf("Application Version: %s\n", info.application_version);
        printf("Unique Identifier:   %s\n", info.unique_identifier);
        
        // Call uCxEnd to complete the command
        uCxEnd(&gUcxHandle);
    } else {
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
    int32_t btMode;
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
                       conn.bd_addr.address.address[0],
                       conn.bd_addr.address.address[1],
                       conn.bd_addr.address.address[2],
                       conn.bd_addr.address.address[3],
                       conn.bd_addr.address.address[4],
                       conn.bd_addr.address.address[5],
                       conn.bd_addr.type == 0 ? "Public" : "Random");
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
