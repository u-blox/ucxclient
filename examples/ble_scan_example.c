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
 * @brief Example of BLE device discovery/scanning using the uCx API
 *
 * This example will:
 * - Initialize the AT client
 * - Set Bluetooth mode to BLE
 * - Start BLE discovery for 5 seconds
 * - Print discovered devices with RSSI and name
 *
 * Execute with following args:
 * ble_scan_example [uart_device]
 *
 * The uart_device argument is optional and defaults to U_EXAMPLE_UART
 * from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_bluetooth.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define BLE_SCAN_DURATION_MS 5000

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void printDeviceAddress(const uBtLeAddress_t *pAddr)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           pAddr->address[0], pAddr->address[1], pAddr->address[2],
           pAddr->address[3], pAddr->address[4], pAddr->address[5]);
    if (pAddr->type == U_BD_ADDRESS_TYPE_PUBLIC) {
        printf(" (public)");
    } else {
        printf(" (random)");
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "ble_scan_example",
        "Example of BLE device discovery/scanning using the uCx API.\n"
        "Scans for nearby BLE devices and prints their address, name, and RSSI.",
        "[uart_device]");

    uCxHandle_t ucxHandle;
    const char *pDevice = U_EXAMPLE_UART;

    if (argc >= 2) {
        pDevice = argv[1];
    }

    printf("===========================================\n");
    printf("BLE Scan Example\n");
    printf("===========================================\n");

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, true);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);

    // Reboot to ensure clean state
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Set Bluetooth mode to BLE only
    printf("Setting Bluetooth mode to BLE...\n");
    int32_t ret = uCxBluetoothSetMode(&ucxHandle, U_BT_MODE_CENTRAL_PERIPHERAL);
    if (ret != 0) {
        printf("Failed to set Bluetooth mode: %" PRId32 "\n", ret);
    }

    // Start BLE discovery
    printf("\nStarting BLE discovery for %d seconds...\n", BLE_SCAN_DURATION_MS / 1000);
    printf("-------------------------------------------\n");

    uCxBluetoothDiscovery3Begin(&ucxHandle,
                                U_BT_DISCOVERY_TYPE_ALL_NO_DUPLICATES,
                                U_BT_DISCOVERY_MODE_ACTIVE,
                                BLE_SCAN_DURATION_MS);

    uCxBtDiscovery_t device;
    int deviceCount = 0;

    while (uCxBluetoothDiscovery3GetNext(&ucxHandle, &device)) {
        deviceCount++;
        printf("\nDevice #%d:\n", deviceCount);
        printf("  Address: ");
        printDeviceAddress(&device.bd_addr);
        printf("\n");
        printf("  Name:    %s\n", device.device_name[0] ? device.device_name : "(unknown)");
        printf("  RSSI:    %" PRId32 " dBm\n", device.rssi);
        printf("  Type:    %s\n",
               device.data_type == U_BT_DATA_TYPE_ADVERTISE_DATA ? "Advertisement" : "Scan Response");
    }
    uCxEnd(&ucxHandle);

    printf("\n-------------------------------------------\n");
    printf("Discovery complete. Found %d device(s).\n", deviceCount);

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
