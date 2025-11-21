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
 * http_example <uart_device> "<wifi_ssid>" "<wifi_psk>"
 */

#include <stdio.h>
#include <stdlib.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_http.h"
#include "u_cx_system.h"
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
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "HTTP response: %d", status_code);
    exampleSignalEvent(URC_FLAG_HTTP_RESPONSE);
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
    uCxHttpRegisterRequestStatus(&ucxHandle, httpRequestStatus);

    uCxSystemReboot(&ucxHandle);
    exampleSleepMs(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);
    exampleWaitEvent(URC_FLAG_NETWORK_UP, 20);

    int32_t ret;
    const int32_t sessionId = 0;

    // Configure HTTP connection
    ret = uCxHttpSetConnectionParams2(&ucxHandle, sessionId, EXAMPLE_URL);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetConnectionParams2() returned %d", ret);

    // Set request path
    ret = uCxHttpSetRequestPath(&ucxHandle, sessionId, "/");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetRequestPath() returned %d", ret);

    // Send GET request
    ret = uCxHttpGetRequest(&ucxHandle, sessionId);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpGetRequest() returned %d", ret);

    // Wait for response
    exampleWaitEvent(URC_FLAG_HTTP_RESPONSE, 10);

    // Read response headers
    uCxHttpGetHeader_t headerRsp;
    if (uCxHttpGetHeader1Begin(&ucxHandle, sessionId, &headerRsp)) {
        printf("HTTP Headers:\n");
        printf("%.*s\n", (int)headerRsp.byte_array_data.length, headerRsp.byte_array_data.pData);
        uCxEnd(&ucxHandle);
    }

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

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
