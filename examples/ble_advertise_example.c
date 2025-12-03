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
 * @brief Example of BLE advertising using the uCx API
 *
 * This example will:
 * - Initialize the AT client
 * - Set Bluetooth mode to BLE
 * - Configure and start legacy BLE advertising
 * - Advertise for a specified duration
 *
 * Execute with following args:
 * ble_advertise_example <uart_device> [device_name]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_bluetooth.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define DEFAULT_DEVICE_NAME "uCxExample"
#define ADVERTISE_DURATION_S 30

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    uCxHandle_t ucxHandle;
    const char *pDeviceName = DEFAULT_DEVICE_NAME;

#ifdef U_PORT_POSIX
    if (argc < 2) {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "Syntax: %s <device> [device_name]\n", argv[0]);
        exit(1);
    }
    const char *pDevice = argv[1];
    if (argc >= 3) {
        pDeviceName = argv[2];
    }
#else
    (void)argc;
    (void)argv;
    const char *pDevice = U_EXAMPLE_UART;
#endif

    printf("===========================================\n");
    printf("BLE Advertise Example\n");
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

    int32_t ret;

    // Set Bluetooth mode to BLE peripheral
    printf("Setting Bluetooth mode to BLE...\n");
    ret = uCxBluetoothSetMode(&ucxHandle, U_BT_MODE_PERIPHERAL);
    if (ret != 0) {
        printf("Failed to set Bluetooth mode: %" PRId32 "\n", ret);
    }

    // Set local device name
    printf("Setting device name to: %s\n", pDeviceName);
    ret = uCxBluetoothSetLocalName(&ucxHandle, pDeviceName);
    if (ret != 0) {
        printf("Failed to set device name: %" PRId32 "\n", ret);
    }

    // Configure advertising interval (in units of 0.625ms)
    // Min: 160 (100ms), Max: 320 (200ms)
    printf("Configuring advertising parameters...\n");
    ret = uCxBluetoothSetLegacyAdvertisementConfig(&ucxHandle, 160, 320);
    if (ret != 0) {
        printf("Failed to configure advertisement: %" PRId32 "\n", ret);
    }

    // Start legacy advertising
    printf("Starting BLE advertising...\n");
    ret = uCxBluetoothLegacyAdvertisementStart(&ucxHandle);
    if (ret != 0) {
        printf("Failed to start advertising: %" PRId32 "\n", ret);
        goto cleanup;
    }

    printf("\n-------------------------------------------\n");
    printf("Device is now advertising as: %s\n", pDeviceName);
    printf("Advertising for %d seconds...\n", ADVERTISE_DURATION_S);
    printf("(Use a BLE scanner app to find this device)\n");
    printf("-------------------------------------------\n");

    // Wait for specified duration
    for (int i = ADVERTISE_DURATION_S; i > 0; i--) {
        printf("\rTime remaining: %d seconds ", i);
        fflush(stdout);
        U_CX_PORT_SLEEP_MS(1000);
    }
    printf("\n");

    // Stop advertising
    printf("\nStopping advertising...\n");
    ret = uCxBluetoothLegacyAdvertisementStop(&ucxHandle);
    if (ret != 0) {
        printf("Failed to stop advertising: %" PRId32 "\n", ret);
    }

    printf("Advertising stopped.\n");

cleanup:
    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
