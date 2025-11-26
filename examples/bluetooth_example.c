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
 * @brief Example of how to scan, connect, and discover GATT services using the uCx API
 *
 * This example will:
 * - Enable Bluetooth
 * - Scan for Bluetooth devices
 * - Connect to a specific device (U_EXAMPLE_BT_ADDR)
 * - Perform GATT service discovery
 * - Disconnect
 *
 * This example works with both OS and no-OS configurations.
 * The build system creates two executables from this same source:
 * - bluetooth_example: Uses POSIX port (with background RX task)
 * - bluetooth_example_no_os: Uses no-OS port (manual RX polling)
 *
 * Execute with following args:
 * bluetooth_example <uart_device> "<bt_address>"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_bluetooth.h"
#include "u_cx_gatt_client.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_EXAMPLE_BT_ADDR
#define U_EXAMPLE_BT_ADDR ""
#endif

#define URC_FLAG_BT_CONNECTED       (1 << 0)
#define URC_FLAG_BT_DISCONNECTED    (1 << 1)

#define MAX_SCAN_RESULTS 20

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    uBtLeAddress_t address;
    int32_t rssi;
    char name[64];
} ScanResult_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static ScanResult_t gScanResults[MAX_SCAN_RESULTS];
static int32_t gScanCount = 0;
static int32_t gConnectionHandle = -1;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void btConnectUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr)
{
    (void)bd_addr;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                    "Bluetooth connected, handle: %d", conn_handle);
    gConnectionHandle = conn_handle;
    exampleSignalEvent(URC_FLAG_BT_CONNECTED);
}

static void btDisconnectUrc(struct uCxHandle *puCxHandle, int32_t conn_handle)
{
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                    "Bluetooth disconnected, handle: %d", conn_handle);
    gConnectionHandle = -1;
    exampleSignalEvent(URC_FLAG_BT_DISCONNECTED);
}

static void scanResultUrc(struct uCxHandle *puCxHandle, uBtLeAddress_t *bd_addr, 
                         int32_t rssi, uint8_t frameType, uByteArray_t *data)
{
    (void)puCxHandle;
    (void)frameType;
    
    if (gScanCount >= MAX_SCAN_RESULTS) {
        return;
    }
    
    // Check if we already have this device
    for (int32_t i = 0; i < gScanCount; i++) {
        if (memcmp(&gScanResults[i].address, bd_addr, sizeof(uBtLeAddress_t)) == 0) {
            // Update RSSI if stronger
            if (rssi > gScanResults[i].rssi) {
                gScanResults[i].rssi = rssi;
            }
            return;
        }
    }
    
    // Add new device
    memcpy(&gScanResults[gScanCount].address, bd_addr, sizeof(uBtLeAddress_t));
    gScanResults[gScanCount].rssi = rssi;
    gScanResults[gScanCount].name[0] = '\0';
    
    // Try to extract name from advertising data
    if (data && data->length > 0) {
        const uint8_t *pData = data->pData;
        size_t offset = 0;
        
        while (offset < data->length - 1) {
            uint8_t len = pData[offset];
            if (len == 0 || offset + len >= data->length) {
                break;
            }
            
            uint8_t type = pData[offset + 1];
            // AD Type 0x09 = Complete Local Name, 0x08 = Shortened Local Name
            if (type == 0x09 || type == 0x08) {
                int32_t nameLen = len - 1;
                if (nameLen > 63) nameLen = 63;
                memcpy(gScanResults[gScanCount].name, &pData[offset + 2], nameLen);
                gScanResults[gScanCount].name[nameLen] = '\0';
                break;
            }
            offset += len + 1;
        }
    }
    
    gScanCount++;
}

static void printBtAddress(const uBtLeAddress_t *addr)
{
    for (int i = 0; i < 6; i++) {
        printf("%02X", addr->address[i]);
        if (i < 5) printf(":");
    }
}

static bool parseBtAddress(const char *str, uBtLeAddress_t *addr)
{
    // Parse address in format AA:BB:CC:DD:EE:FF or AABBCCDDEEFF
    int values[6];
    int count;
    
    // Try with colons first
    count = sscanf(str, "%2x:%2x:%2x:%2x:%2x:%2x",
                   &values[0], &values[1], &values[2],
                   &values[3], &values[4], &values[5]);
    
    if (count != 6) {
        // Try without colons
        count = sscanf(str, "%2x%2x%2x%2x%2x%2x",
                       &values[0], &values[1], &values[2],
                       &values[3], &values[4], &values[5]);
    }
    
    if (count == 6) {
        for (int i = 0; i < 6; i++) {
            addr->address[i] = (uint8_t)values[i];
        }
        addr->type = U_BD_ADDRESS_TYPE_PUBLIC;
        return true;
    }
    
    return false;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char **argv)
{
    uCxHandle_t ucxHandle;

#ifdef U_PORT_POSIX
    if (argc != 3) {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "Syntax: %s <device> <bt_address>\n", argv[0]);
        fprintf(stderr, "Example: %s /dev/ttyUSB0 AA:BB:CC:DD:EE:FF\n", argv[0]);
        exit(1);
    }
    const char *pDevice = argv[1];
    const char *pBtAddr = argv[2];
#else
    (void)argc;
    (void)argv;
    const char *pDevice = U_EXAMPLE_UART;
    const char *pBtAddr = U_EXAMPLE_BT_ADDR;

    if (*pBtAddr == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Bluetooth not configured - connection will not work");
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
            "- You need to define U_EXAMPLE_UART and U_EXAMPLE_BT_ADDR.");
    }
#endif

    // Parse target BT address
    uBtLeAddress_t targetAddr;
    bool hasTargetAddr = parseBtAddress(pBtAddr, &targetAddr);

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, true);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);
    uCxBluetoothRegisterConnect(&ucxHandle, btConnectUrc);
    uCxBluetoothRegisterDisconnect(&ucxHandle, btDisconnectUrc);

    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Enable Bluetooth in central mode
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Enabling Bluetooth...");
    if (uCxBluetoothSetMode(&ucxHandle, U_BT_MODE_CENTRAL) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to enable Bluetooth");
        goto cleanup;
    }

    U_CX_PORT_SLEEP_MS(500);

    // Start scanning using discovery command
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Starting Bluetooth scan...");
    gScanCount = 0;
    
    // Use discovery command with timeout
    uCxAtClientSetCommandTimeout(pClient, 10000, false);
    uCxBluetoothDiscoveryDefaultBegin(&ucxHandle);
    
    // Collect scan results
    uCxBtDiscoveryDefault_t device;
    while (uCxBluetoothDiscoveryDefaultGetNext(&ucxHandle, &device)) {
        scanResultUrc(&ucxHandle, &device.bd_addr, device.rssi, 0, &device.data);
    }
    uCxEnd(&ucxHandle);
    
    // Restore timeout
    uCxAtClientSetCommandTimeout(pClient, U_CX_DEFAULT_CMD_TIMEOUT_MS, false);

    // Display scan results
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Found %d device(s):", gScanCount);
    for (int32_t i = 0; i < gScanCount; i++) {
        printf("  [%d] ", i + 1);
        printBtAddress(&gScanResults[i].address);
        printf(" RSSI: %d dBm", gScanResults[i].rssi);
        if (gScanResults[i].name[0] != '\0') {
            printf(" Name: %s", gScanResults[i].name);
        }
        printf("\n");
    }

    // Try to connect if target address was provided
    if (!hasTargetAddr) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "No target Bluetooth address provided, skipping connection");
        goto cleanup;
    }

    // Find the target device in scan results
    int32_t targetIndex = -1;
    for (int32_t i = 0; i < gScanCount; i++) {
        if (memcmp(&gScanResults[i].address, &targetAddr, sizeof(uBtLeAddress_t)) == 0) {
            targetIndex = i;
            break;
        }
    }

    if (targetIndex < 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Target device not found in scan results");
        printf("Target address: ");
        printBtAddress(&targetAddr);
        printf("\n");
        goto cleanup;
    }

    // Connect to target device
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Connecting to device...");
    printf("Connecting to: ");
    printBtAddress(&targetAddr);
    printf("\n");

    if (uCxBluetoothConnect(&ucxHandle, &targetAddr) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to initiate connection");
        goto cleanup;
    }

    // Wait for connection
    if (exampleWaitEvent(URC_FLAG_BT_CONNECTED, 10)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                        "Successfully connected! Handle: %d", gConnectionHandle);

        // Perform GATT service discovery
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Starting GATT service discovery...");
        
        // Discover primary services
        uCxGattClientDiscoverPrimaryServicesBegin(&ucxHandle, gConnectionHandle);
        
        int serviceCount = 0;
        uCxGattClientDiscoverPrimaryServices_t serviceRsp;
        while (uCxGattClientDiscoverPrimaryServicesGetNext(&ucxHandle, &serviceRsp)) {
            serviceCount++;
            printf("  Service %d: Start Handle: 0x%04X, End Handle: 0x%04X\n", 
                   serviceCount, serviceRsp.start_handle, serviceRsp.end_handle);
            if (serviceRsp.uuid.length > 0) {
                printf("    UUID: ");
                for (size_t i = 0; i < serviceRsp.uuid.length; i++) {
                    printf("%02X", serviceRsp.uuid.pData[i]);
                }
                printf("\n");
            }
        }
        uCxEnd(&ucxHandle);
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Found %d services", serviceCount);

        // Keep connection active for a moment
        U_CX_PORT_SLEEP_MS(2000);

        // Disconnect
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Disconnecting...");
        uCxBluetoothDisconnect(&ucxHandle, gConnectionHandle);
        
        // Wait for disconnect event
        exampleWaitEvent(URC_FLAG_BT_DISCONNECTED, 5);
        
    } else {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Timeout waiting for connection");
    }

cleanup:
    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
