/*
 * Copyright 2024 u-blox
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
 * Execute with following args:
 * http_example <uart_device> "<wifi_ssid>" "<wifi_psk>"
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
#include "u_cx_socket.h"
#include "u_cx_system.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define EXAMPLE_URL "www.google.com"

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_SOCK_CONNECTED     (1 << 1)
#define URC_FLAG_SOCK_DATA          (1 << 2)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static U_CX_MUTEX_HANDLE gUrcSem;
static volatile uint32_t gUrcEventFlags = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static bool waitEvent(uint32_t evtFlag, uint32_t timeoutS)
{
    int32_t timeoutMs = timeoutS * 1000;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "waitEvent(%d, %d)", evtFlag, timeoutS);
    do {
        U_CX_MUTEX_TRY_LOCK(gUrcSem, 100);
        if (gUrcEventFlags & evtFlag) {
            return true;
        }
    } while (U_CX_PORT_GET_TIME_MS() - startTime < timeoutMs);

    U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Timeout waiting for: %d", evtFlag);
    return false;
}

static void signalEvent(uint32_t evtFlag)
{
    gUrcEventFlags |= evtFlag;
    U_CX_MUTEX_UNLOCK(gUrcSem);
}

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "networkUpUrc");
    signalEvent(URC_FLAG_NETWORK_UP);
}

static void sockConnected(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    (void)puCxHandle;
    (void)socket_handle;
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "sockConnected");
    signalEvent(URC_FLAG_SOCK_CONNECTED);
}

static void socketData(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)socket_handle;
    (void)number_bytes;
    signalEvent(URC_FLAG_SOCK_DATA);
}

static void sleepMs(int32_t timeMs)
{
    // Hack to implement sleep using a mutex
    static bool sleepMutexInit = false;
    static U_CX_MUTEX_HANDLE sleepMutex;
    if (!sleepMutexInit) {
        U_CX_MUTEX_CREATE(sleepMutex);
        U_CX_MUTEX_LOCK(sleepMutex);
        sleepMutexInit = true;
    }
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "sleepMs(%d)", timeMs);
    U_CX_MUTEX_TRY_LOCK(sleepMutex, timeMs);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char **argv)
{
    uCxAtClient_t client;
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
    const char *pDevice = U_EXAMPLE_UART;
    const char *pSsid = U_EXAMPLE_SSID;
    const char *pWpaPsk = U_EXAMPLE_WPA_PSK;

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - connection will not work")
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
            "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.")
    }
#endif

    U_CX_MUTEX_CREATE(gUrcSem);

    uPortAtInit(&client);
    if (!uPortAtOpen(&client, pDevice, 115200, true)) {
        return 1;
    }

    uCxInit(&client, &ucxHandle);
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxSocketRegisterConnect(&ucxHandle, sockConnected);
    uCxSocketRegisterDataAvailable(&ucxHandle, socketData);

    uCxSystemReboot(&ucxHandle);
    sleepMs(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);
    waitEvent(URC_FLAG_NETWORK_UP, 20);

    int32_t ret;
    int32_t sockHandle;

    uCxSocketCreate1(&ucxHandle, U_PROTOCOL_TCP, &sockHandle);
    uCxSocketConnect(&ucxHandle, sockHandle, EXAMPLE_URL, 80);
    waitEvent(URC_FLAG_SOCK_CONNECTED, 5);
    ret = uCxSocketWrite(&ucxHandle, sockHandle, (uint8_t *)"GET /\r\n", 7);
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "uCxSocketWrite() returned %d", ret);
    waitEvent(URC_FLAG_SOCK_DATA, 5);

    uint8_t rxData[512];
    do {
        ret = uCxSocketRead(&ucxHandle, sockHandle, sizeof(rxData) - 1, &rxData[0]);
        if (ret == 0) {
            sleepMs(10);
            ret = uCxSocketRead(&ucxHandle, sockHandle, sizeof(rxData) - 1, &rxData[0]);
        }
        if (ret > 0) {
            rxData[ret] = 0;
            printf("%s", rxData);
        }
    } while (ret > 0);

    uPortAtClose(&client);

    U_CX_MUTEX_DELETE(gUrcSem);
}
