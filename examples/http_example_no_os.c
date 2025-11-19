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
 * @brief No OS variant of the HTTP example
 *
 * This will do the same things as http_example.c but without using mutexes.
 *
 * You need to define the following defines for successful connection:
 * U_EXAMPLE_UART
 * U_EXAMPLE_SSID
 * U_EXAMPLE_WPA_PSK
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_cx_at_util.h"
#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_urc.h"
#include "u_cx_wifi.h"
#include "u_cx_http.h"
#include "u_cx_system.h"

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

static volatile uint32_t gUrcEventFlags = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

// For no OS this function will "poll" uCxAtClientHandleRx()
static bool waitEvent(uCxAtClient_t *pClient, uint32_t evtFlag, uint32_t timeoutS)
{
    int32_t timeoutMs = (int32_t)timeoutS * 1000;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "waitEvent(%d, %d)", evtFlag, timeoutS);
    do {
        uCxAtClientHandleRx(pClient);
        if (gUrcEventFlags & evtFlag) {
            return true;
        }
    } while (U_CX_PORT_GET_TIME_MS() - startTime < timeoutMs);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Timeout waiting for: %d", evtFlag);
    return false;
}

static void signalEvent(uint32_t evtFlag)
{
    gUrcEventFlags |= evtFlag;
}

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "networkUpUrc");
    signalEvent(URC_FLAG_NETWORK_UP);
}

static void httpRequestStatus(struct uCxHandle *puCxHandle, int32_t session_id, int32_t status_code, const char *description)
{
    (void)puCxHandle;
    (void)session_id;
    (void)description;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "HTTP response: %d", status_code);
    signalEvent(URC_FLAG_HTTP_RESPONSE);
}

static void sleepMs(int32_t timeMs)
{
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "sleepMs(%d)", timeMs);
    int32_t startTime = U_CX_PORT_GET_TIME_MS();
    while(U_CX_PORT_GET_TIME_MS() - startTime < timeMs);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(void)
{
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;

    const char *pDevice = U_EXAMPLE_UART;
    const char *pSsid = U_EXAMPLE_SSID;
    const char *pWpaPsk = U_EXAMPLE_WPA_PSK;

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - connection will not work")
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
            "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.")
    }

    uPortAtInit(&client);
    if (!uPortAtOpen(&client, pDevice, 115200, true)) {
        return 1;
    }

    uCxInit(&client, &ucxHandle);
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxHttpRegisterRequestStatus(&ucxHandle, httpRequestStatus);

    uCxSystemReboot(&ucxHandle);
    sleepMs(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);
    waitEvent(&client, URC_FLAG_NETWORK_UP, 20);

    int32_t ret;
    const int32_t sessionId = 0;

    // Configure HTTP connection
    ret = uCxHttpSetConnectionParams2(&ucxHandle, sessionId, EXAMPLE_URL);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, client.instance, "uCxHttpSetConnectionParams2() returned %d", ret);

    // Set request path
    ret = uCxHttpSetRequestPath(&ucxHandle, sessionId, "/");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, client.instance, "uCxHttpSetRequestPath() returned %d", ret);

    // Send GET request
    ret = uCxHttpGetRequest(&ucxHandle, sessionId);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, client.instance, "uCxHttpGetRequest() returned %d", ret);

    // Wait for response
    waitEvent(&client, URC_FLAG_HTTP_RESPONSE, 10);

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

    uPortAtClose(&client);
}
