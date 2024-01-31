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
 * @brief Linux example for doing HTTP GET request using NORA-W36
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
#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/poll.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_cx_at_util.h"
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

static sem_t gUrcSem;
static volatile uint32_t gUrcEventFlags = 0;
uCxHandle_t ucxHandle;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static bool waitEvent(uint32_t evtFlag, uint32_t timeout_s)
{
    int ret;
    struct timespec ts;
    struct timespec start_ts;
    clock_gettime(CLOCK_REALTIME, &start_ts);
    do {
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += 1;
        ret = sem_timedwait(&gUrcSem, &ts);
        if ((ret == 0) && (gUrcEventFlags & evtFlag)) {
            return true;
        }
    } while ((ret = -1) && (errno == ETIMEDOUT));
    return false;
}

static void signalEvent(uint32_t evtFlag)
{
    gUrcEventFlags |= evtFlag;
    sem_post(&gUrcSem);
}

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    printf("networkUpUrc\n");
    signalEvent(URC_FLAG_NETWORK_UP);
}

static void sockConnected(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    printf("sockConnected\n");
    signalEvent(URC_FLAG_SOCK_CONNECTED);
}

static void socketData(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes)
{
    signalEvent(URC_FLAG_SOCK_DATA);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char **argv)
{
    uCxAtClient_t client;

    if (argc != 4) {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "Syntax: %s <device> <SSID> <WPA_PSK>\n", argv[0]);
        exit(1);
    }
    const char *pDevice = argv[1];
    const char *pSsid = argv[2];
    const char *pWpaPsk = argv[3];

    sem_init(&gUrcSem, 0, 0);

    uPortAtInit(&client);
    if (!uPortAtOpen(&client, pDevice, 115200, true)) {
        return 1;
    }

    uCxInit(&client, &ucxHandle);
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxSocketRegisterConnect(&ucxHandle, sockConnected);
    uCxSocketRegisterDataAvailable(&ucxHandle, socketData);

    uCxSystemReboot(&ucxHandle);
    sleep(4);
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
    printf("uCxSocketWrite() returned %d\n", ret);
    waitEvent(URC_FLAG_SOCK_DATA, 5);

    uint8_t rxData[512];
    do {
        ret = uCxSocketRead(&ucxHandle, sockHandle, sizeof(rxData) - 1, &rxData[0]);
        if (ret == 0) {
            usleep(10 * 1000);
            ret = uCxSocketRead(&ucxHandle, sockHandle, sizeof(rxData) - 1, &rxData[0]);
        }
        if (ret > 0) {
            rxData[ret] = 0;
            printf("%s", rxData);
        }
    } while (ret > 0);

    uPortAtClose(&client);

    sem_destroy(&gUrcSem);
}
