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
 * @brief Example of how to do a HTTP GET request using the uCx API
 *
 * This example will:
 * - Setup WiFi
 * - Create a TCP socket and connect to EXAMPLE_URL
 * - Send "GET /"
 * - Print server response output
 *
 * This example works with both OS and no-OS configurations.
 * The build system creates two executables from this same source:
 * - http_example: Uses POSIX port (with background RX task)
 * - http_example_no_os: Uses no-OS port (manual RX polling)
 *
 * Execute with following args:
 * http_example [uart_device] [wifi_ssid] [wifi_psk]
 *
 * All arguments are optional and default to U_EXAMPLE_UART, U_EXAMPLE_SSID,
 * and U_EXAMPLE_WPA_PSK from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_http.h"
#include "u_cx_system.h"
#include "u_cx_general.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define EXAMPLE_URL "www.google.com"

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_HTTP_RESPONSE      (1 << 1)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "networkUpUrc");
    exampleSignalEvent(URC_FLAG_NETWORK_UP);
}

static void httpRequestStatus(struct uCxHandle *puCxHandle, int32_t session_id, int32_t status_code, const char *description)
{
    (void)puCxHandle;
    (void)session_id;
    (void)description;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "HTTP response: %" PRId32, status_code);
    exampleSignalEvent(URC_FLAG_HTTP_RESPONSE);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "http_example",
        "Example of how to do a HTTP GET request using the uCx API.\n"
        "Connects to WiFi, sends HTTP GET to www.google.com, and prints response.\n"
        "This example will also illustrate how to change UART baud rate on the module.",
        "[uart_device] [wifi_ssid] [wifi_psk]");

    uCxHandle_t ucxHandle;
    int32_t ret;
    const char *pDevice = U_EXAMPLE_UART;
    const char *pSsid = U_EXAMPLE_SSID;
    const char *pWpaPsk = U_EXAMPLE_WPA_PSK;

    if (argc >= 2) {
        pDevice = argv[1];
    }
    if (argc >= 3) {
        pSsid = argv[2];
    }
    if (argc >= 4) {
        pWpaPsk = argv[3];
    }

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - connection will not work");
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.");
    }

    // Initialize example utilities and AT client
    int currentBaud = 115200;
    uCxAtClient_t *pClient = exampleInit(pDevice, currentBaud, false);
    if (pClient == NULL) {
        goto fail;
    }

    uCxInit(pClient, &ucxHandle);

    // Check communication with device - try different baud rates if needed
    uCxAtClientSetCommandTimeout(pClient, 500, false);
    if (uCxGeneralAttention(&ucxHandle) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "No response from device at 115200 baud - try 921600 baud");
        uCxAtClientClose(pClient);
        currentBaud = 921600;
        uCxAtClientOpen(pClient, currentBaud, false);
        if (uCxGeneralAttention(&ucxHandle) != 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "No response from device");
            goto fail;
        }
    }

    // Register URC callbacks
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxHttpRegisterRequestStatus(&ucxHandle, httpRequestStatus);

    // Reboot the module to ensure a clean state
    uCxSystemReboot(&ucxHandle);
    if (currentBaud == 921600) {
        // If module was at 921600 baud, after reboot it will be back to 115200
        U_CX_PORT_SLEEP_MS(200);
        uCxAtClientClose(pClient);
        currentBaud = 115200;
        uCxAtClientOpen(pClient, currentBaud, false);
    }
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Increase UART speed for better throughput
    // No flow control: RTS/CTS are not wired on the Nucleo setups (module param 0)
    ret = uCxSystemSetUartSettings3(&ucxHandle, 921600, 0, 1);
    if (ret < 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "uCxSystemSetUartSettings3() failed");
        goto fail;
    }
    // Re-open AT client at new speed
    uCxAtClientClose(pClient);
    U_CX_PORT_SLEEP_MS(200);
    uCxAtClientOpen(pClient, 921600, false);
    if (uCxGeneralAttention(&ucxHandle) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "No response from device at 921600 baud");
        goto fail;
    }

    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);
    exampleWaitEvent(URC_FLAG_NETWORK_UP, 20);

    const int32_t sessionId = 0;

    // Configure HTTP connection
    ret = uCxHttpSetConnectionParams2(&ucxHandle, sessionId, EXAMPLE_URL);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetConnectionParams2() returned %" PRId32, ret);

    // Set request path
    ret = uCxHttpSetRequestPath(&ucxHandle, sessionId, "/");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetRequestPath() returned %" PRId32, ret);

    // Send GET request
    ret = uCxHttpGetRequest(&ucxHandle, sessionId);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpGetRequest() returned %" PRId32, ret);

    // Wait for response
    exampleWaitEvent(URC_FLAG_HTTP_RESPONSE, 10);

    // Read response headers in chunks
    uCxHttpGetHeader_t headerRsp;
    printf("HTTP Headers:\n");
    do {
        if (uCxHttpGetHeader2Begin(&ucxHandle, sessionId, 512, &headerRsp)) {
            printf("%.*s", (int)headerRsp.byte_array_data.length, headerRsp.byte_array_data.pData);
            uCxEnd(&ucxHandle);
        } else {
            break;
        }
    } while (headerRsp.more_to_read);
    printf("\n");

    // Read response body
    uint8_t rxData[512];
    int32_t moreToRead;
    do {
        ret = uCxHttpGetBody(&ucxHandle, sessionId, sizeof(rxData) - 1, rxData, &moreToRead);
        if (ret > 0) {
            rxData[ret] = 0;
            printf("%s", rxData);
        }
    } while (ret > 0 && moreToRead);

    // Disconnect HTTP session
    uCxHttpDisconnect(&ucxHandle, sessionId);

    // Reboot module to restore default UART settings
    uCxSystemReboot(&ucxHandle);

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;

fail:
    if (pClient != NULL) {
        uCxAtClientClose(pClient);
        uCxAtClientDeinit(pClient);
        uPortDeinit();
    }
    return 1;
}
