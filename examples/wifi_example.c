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
 * @brief Example of how to connect to Wi-Fi using the uCx API
 *
 * This example will:
 * - Setup WiFi
 * - Connect to the specified SSID with WPA PSK
 * - Wait for network up event
 *
 * This example works with both OS and no-OS configurations.
 * The build system creates two executables from this same source:
 * - wifi_example: Uses POSIX port (with background RX task)
 * - wifi_example_no_os: Uses no-OS port (manual RX polling)
 *
 * Execute with following args:
 * wifi_example <uart_device> "<wifi_ssid>" "<wifi_psk>"
 */

#include <stdio.h>
#include <stdlib.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_NETWORK_DOWN       (1 << 1)

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** Network UP ***");
    exampleSignalEvent(URC_FLAG_NETWORK_UP);
}

static void networkDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** Network DOWN ***");
    exampleSignalEvent(URC_FLAG_NETWORK_DOWN);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char **argv)
{
    uCxHandle_t ucxHandle;

#ifdef U_PORT_POSIX
    if (argc != 4) {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "Syntax: %s <device> <SSID> <WPA_PSK>\n", argv[0]);
        exit(1);
    }
    const char *pDevice = argv[1];
    const char *pSsid = argv[2];
    const char *pWpaPsk = argv[3];
#else
    (void)argc;
    (void)argv;
    const char *pDevice = U_EXAMPLE_UART;
    const char *pSsid = U_EXAMPLE_SSID;
    const char *pWpaPsk = U_EXAMPLE_WPA_PSK;

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - connection will not work");
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
            "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.");
    }
#endif

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, true);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxWifiRegisterStationNetworkDown(&ucxHandle, networkDownUrc);

    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Configure and connect to Wi-Fi
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Setting Wi-Fi security...");
    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Setting connection parameters...");
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Connecting to Wi-Fi...");
    uCxWifiStationConnect(&ucxHandle, 0);
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Waiting for network up event...");
    if (exampleWaitEvent(URC_FLAG_NETWORK_UP, 20)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Successfully connected to Wi-Fi!");
    } else {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Timeout waiting for network up");
    }

    // Keep connection active for 10 seconds
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Maintaining connection for 10 seconds...");
    U_CX_PORT_SLEEP_MS(10000);

    // Disconnect
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Disconnecting...");
    uCxWifiStationDisconnect(&ucxHandle);

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}