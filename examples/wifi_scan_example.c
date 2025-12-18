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
 * @brief Example of WiFi network scanning using the uCx API
 *
 * This example will:
 * - Initialize the AT client
 * - Perform a WiFi scan
 * - Print discovered networks with SSID, RSSI, channel and security info
 *
 * Execute with following args:
 * wifi_scan_example [uart_device]
 *
 * The uart_device argument is optional and defaults to U_EXAMPLE_UART
 * from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void printMacAddress(const uMacAddress_t *pMac)
{
    printf("%02X:%02X:%02X:%02X:%02X:%02X",
           pMac->address[0], pMac->address[1], pMac->address[2],
           pMac->address[3], pMac->address[4], pMac->address[5]);
}

static const char *getSecurityString(int32_t authSuites)
{
    // Decode authentication suites bitfield
    // Bit 0 = shared secret, 1 = PSK, 2 = EAP, 3 = WPA, 4 = WPA2, 5 = WPA3
    if (authSuites == 0) {
        return "Open";
    }

    static char secStr[64];
    secStr[0] = '\0';

    if (authSuites & (1 << 5)) {
        strcat(secStr, "WPA3 ");
    }
    if (authSuites & (1 << 4)) {
        strcat(secStr, "WPA2 ");
    }
    if (authSuites & (1 << 3)) {
        strcat(secStr, "WPA ");
    }
    if (authSuites & (1 << 2)) {
        strcat(secStr, "EAP ");
    }
    if (authSuites & (1 << 1)) {
        strcat(secStr, "PSK ");
    }
    if (authSuites & (1 << 0)) {
        strcat(secStr, "SharedKey ");
    }

    // Remove trailing space
    size_t len = strlen(secStr);
    if (len > 0 && secStr[len - 1] == ' ') {
        secStr[len - 1] = '\0';
    }

    return secStr;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "wifi_scan_example",
        "Example of WiFi network scanning using the uCx API.\n"
        "Scans for nearby WiFi networks and prints their SSID, channel, RSSI, and security.",
        "[uart_device]");

    uCxHandle_t ucxHandle;
    const char *pDevice = U_EXAMPLE_UART;

    if (argc >= 2) {
        pDevice = argv[1];
    }

    printf("===========================================\n");
    printf("WiFi Scan Example\n");
    printf("===========================================\n");

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, false);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);

    // Reboot to ensure clean state
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Start WiFi scan (active mode)
    printf("\nStarting WiFi scan...\n");
    printf("-------------------------------------------\n");
    printf("%-32s  Ch  RSSI  Security\n", "SSID");
    printf("-------------------------------------------\n");

    uCxWifiStationScan1Begin(&ucxHandle, U_WIFI_SCAN_MODE_ACTIVE);

    uCxWifiStationScan_t network;
    int networkCount = 0;

    while (uCxWifiStationScan1GetNext(&ucxHandle, &network)) {
        networkCount++;

        // Print SSID (truncate if too long)
        char ssidDisplay[33];
        strncpy(ssidDisplay, network.ssid, 32);
        ssidDisplay[32] = '\0';

        printf("%-32s  %2" PRId32 "  %4" PRId32 "  %s\n",
               ssidDisplay[0] ? ssidDisplay : "(hidden)",
               network.channel,
               network.rssi,
               getSecurityString(network.authentication_suites));
    }
    uCxEnd(&ucxHandle);

    printf("-------------------------------------------\n");
    printf("Scan complete. Found %d network(s).\n", networkCount);

    // Also print detailed info for first few networks
    if (networkCount > 0) {
        printf("\nRescanning for detailed info...\n");
        uCxWifiStationScan1Begin(&ucxHandle, U_WIFI_SCAN_MODE_ACTIVE);

        int detailed = 0;
        while (uCxWifiStationScan1GetNext(&ucxHandle, &network) && detailed < 3) {
            printf("\nNetwork: %s\n", network.ssid[0] ? network.ssid : "(hidden)");
            printf("  BSSID:   ");
            printMacAddress(&network.bssid);
            printf("\n");
            printf("  Channel: %" PRId32 "\n", network.channel);
            printf("  RSSI:    %" PRId32 " dBm\n", network.rssi);
            printf("  Auth:    %s (0x%02" PRIx32 ")\n",
                   getSecurityString(network.authentication_suites),
                   network.authentication_suites);
            printf("  Unicast: 0x%02" PRIx32 "\n", network.unicast_ciphers);
            printf("  Group:   0x%02" PRIx32 "\n", network.group_ciphers);
            detailed++;
        }
        uCxEnd(&ucxHandle);
    }

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
