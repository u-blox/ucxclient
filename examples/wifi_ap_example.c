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
 * @brief Example of activating a WiFi Access Point using the uCx API
 *
 * This example will:
 * - Initialize the AT client
 * - Configure WiFi AP parameters (SSID, channel, security)
 * - Activate the Access Point
 * - Run for a specified duration
 * - Deactivate the Access Point
 *
 * Execute with following args:
 * wifi_ap_example [uart_device] [ssid] [passphrase]
 *
 * All arguments are optional and default to U_EXAMPLE_UART, U_EXAMPLE_SSID,
 * and U_EXAMPLE_WPA_PSK from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

// Note: We use U_EXAMPLE_SSID and U_EXAMPLE_WPA_PSK as defaults for the AP
// These can be overridden via command line arguments
#define DEFAULT_AP_CHANNEL U_WIFI_CHANNEL_6
#define AP_DURATION_S 60

#define URC_FLAG_AP_UP              (1 << 0)
#define URC_FLAG_AP_DOWN            (1 << 1)
#define URC_FLAG_STATION_CONNECTED  (1 << 2)
#define URC_FLAG_STATION_DISCONNECTED (1 << 3)

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void apUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "AP is up");
    exampleSignalEvent(URC_FLAG_AP_UP);
}

static void apDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "AP is down");
    exampleSignalEvent(URC_FLAG_AP_DOWN);
}

static void stationConnectedUrc(struct uCxHandle *puCxHandle, uMacAddress_t *mac)
{
    (void)puCxHandle;
    printf("Station connected: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac->address[0], mac->address[1], mac->address[2],
           mac->address[3], mac->address[4], mac->address[5]);
    exampleSignalEvent(URC_FLAG_STATION_CONNECTED);
}

static void stationDisconnectedUrc(struct uCxHandle *puCxHandle, uMacAddress_t *mac)
{
    (void)puCxHandle;
    printf("Station disconnected: %02X:%02X:%02X:%02X:%02X:%02X\n",
           mac->address[0], mac->address[1], mac->address[2],
           mac->address[3], mac->address[4], mac->address[5]);
    exampleSignalEvent(URC_FLAG_STATION_DISCONNECTED);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "wifi_ap_example",
        "Example of activating a WiFi Access Point using the uCx API.\n"
        "Creates a WiFi hotspot that other devices can connect to.",
        "[uart_device] [ssid] [passphrase]");

    uCxHandle_t ucxHandle;
    const char *pDevice = U_EXAMPLE_UART;
    const char *pApSsid = U_EXAMPLE_SSID;
    const char *pApPassphrase = U_EXAMPLE_WPA_PSK;

    if (argc >= 2) {
        pDevice = argv[1];
    }
    if (argc >= 3) {
        pApSsid = argv[2];
    }
    if (argc >= 4) {
        pApPassphrase = argv[3];
    }

    printf("===========================================\n");
    printf("WiFi Access Point Example\n");
    printf("===========================================\n");

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, true);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);

    // Register URC callbacks for AP events
    uCxWifiRegisterApUp(&ucxHandle, apUpUrc);
    uCxWifiRegisterApDown(&ucxHandle, apDownUrc);
    uCxWifiRegisterApStationAssociated(&ucxHandle, stationConnectedUrc);
    uCxWifiRegisterApStationDisassociated(&ucxHandle, stationDisconnectedUrc);

    // Reboot to ensure clean state
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    int32_t ret;

    // Set AP connection parameters (SSID and channel)
    printf("Configuring Access Point...\n");
    printf("  SSID:       %s\n", pApSsid);
    printf("  Channel:    %d\n", DEFAULT_AP_CHANNEL);

    ret = uCxWifiApSetConnectionParams2(&ucxHandle, pApSsid, DEFAULT_AP_CHANNEL);
    if (ret != 0) {
        printf("Failed to set AP connection params: %" PRId32 "\n", ret);
        goto cleanup;
    }

    // Set AP security (WPA2 with passphrase)
    if (strlen(pApPassphrase) >= 8) {
        printf("  Security:   WPA2-PSK\n");
        printf("  Passphrase: %s\n", pApPassphrase);
        ret = uCxWifiApSetSecurityWpa1(&ucxHandle, pApPassphrase);
        if (ret != 0) {
            printf("Failed to set AP security: %" PRId32 "\n", ret);
            goto cleanup;
        }
    } else {
        printf("  Security:   Open (passphrase too short)\n");
        ret = uCxWifiApSetSecurityOpen(&ucxHandle);
        if (ret != 0) {
            printf("Failed to set AP security: %" PRId32 "\n", ret);
            goto cleanup;
        }
    }

    // Activate the Access Point
    printf("\nActivating Access Point...\n");
    ret = uCxWifiApActivate(&ucxHandle);
    if (ret != 0) {
        printf("Failed to activate AP: %" PRId32 "\n", ret);
        goto cleanup;
    }

    // Wait for AP to be up
    if (!exampleWaitEvent(URC_FLAG_AP_UP, 10)) {
        printf("Timeout waiting for AP to come up\n");
        goto cleanup;
    }

    printf("\n-------------------------------------------\n");
    printf("Access Point is now active!\n");
    printf("  SSID: %s\n", pApSsid);
    printf("  Running for %d seconds...\n", AP_DURATION_S);
    printf("  Waiting for stations to connect...\n");
    printf("-------------------------------------------\n");

    // Wait for station connections during the specified duration
    int timeRemaining = AP_DURATION_S;
    while (timeRemaining > 0) {
        printf("\rTime remaining: %d seconds ", timeRemaining);
        fflush(stdout);

        // Wait for 1 second or until a station event occurs (silent to avoid log spam)
        if (exampleWaitEventEx(URC_FLAG_STATION_CONNECTED | URC_FLAG_STATION_DISCONNECTED, 1, true)) {
            // Event occurred, continue waiting
        }
        timeRemaining--;
    }
    printf("\n");

    // Deactivate the Access Point
    printf("\nDeactivating Access Point...\n");
    ret = uCxWifiApDeactivate(&ucxHandle);
    if (ret != 0) {
        printf("Failed to deactivate AP: %" PRId32 "\n", ret);
    }

    printf("Access Point deactivated.\n");

cleanup:
    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
