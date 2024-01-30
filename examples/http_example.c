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

#include "u_cx_at_client.h"
#include "u_cx_at_util.h"
#include "u_cx.h"
#include "u_cx_urc.h"
#include "u_cx_wifi.h"
#include "u_cx_socket.h"
#include "u_cx_system.h"

#define EXAMPLE_URL "www.google.com"

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_SOCK_CONNECTED     (1 << 1)
#define URC_FLAG_SOCK_DATA          (1 << 2)

static uint64_t gBootTime;
static int gUartFd;
static sem_t gUrcSem;
static volatile uint32_t gUrcEventFlags = 0;
uCxHandle_t ucxHandle;


typedef struct {
    int uartFd;
    uCxAtClient_t *pClient;
} uRxThreadArgs_t;

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

static int32_t uCxAtRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    (void)pClient;
    int uartFd = *((int *)pStreamHandle);

    if (timeoutMs == 0) {
        int available = 0;
        ioctl(uartFd, FIONREAD, &available);
        if (available == 0) {
            return 0;
        }
    }

    return (int32_t)read(uartFd, pData, length);
}

static int32_t uCxAtWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    (void)pClient;
    int uartFd = *((int *)pStreamHandle);

    return (int32_t)write(uartFd, pData, length);
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

static int32_t getTickTimeMs(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    uint64_t timeMs = (time.tv_sec * 1000) + (time.tv_nsec / (1000 * 1000));
    return (int32_t)timeMs;
}

int32_t uPortGetTickTimeMs(void)
{
    int32_t timeMs = getTickTimeMs() - gBootTime;
    return timeMs;
}

static int openUart(const char *pDevName, int baudRate, bool useFlowControl)
{
    int uartFd;
    speed_t speed;
    if (baudRate == 9600) {
        speed = B9600;
    } else if (baudRate == 19200) {
        speed = B19200;
    } else if (baudRate == 38400) {
        speed = B38400;
    } else if (baudRate == 57600) {
        speed = B57600;
    } else if (baudRate == 115200) {
        speed = B115200;
    } else if (baudRate == 230400) {
        speed = B230400;
    } else if (baudRate == 460800) {
        speed = B460800;
    } else if (baudRate == 921600) {
        speed = B921600;
    } else {
        return -1;
    }

    uartFd = open(pDevName, O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (uartFd < 0) {
        return uartFd;
    }

    struct termios options;
    tcgetattr(uartFd, &options);
    cfmakeraw(&options);
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    if (useFlowControl) {
        options.c_cflag |= CRTSCTS;
    } else {
        options.c_cflag &= ~CRTSCTS;
    }
    // Set timed read with 100 ms timeout
    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 1;
    if (tcsetattr(uartFd, TCSANOW, &options) != 0) {
        return -1;
    }
    tcflush(uartFd, TCIOFLUSH);

    return uartFd;
}

static void *readTask(void *pArg)
{
    uRxThreadArgs_t *pRxArgs = (uRxThreadArgs_t *)pArg;
    struct pollfd fds[1];
    fds[0].fd = pRxArgs->uartFd;
    fds[0].events = POLLIN ;

    while (1) {
        int pollrc = poll(fds, 1, 100);
        if (pollrc < 0) {
            perror("poll");
            break;
        }
        if (fds[0].revents & POLLIN) {
            uCxAtClientHandleRx(pRxArgs->pClient);
        }
    }
    return NULL;
}

int main(int argc, char **argv)
{
    uCxAtClient_t client;

    static char rxBuf[1024];
#if U_CX_USE_URC_QUEUE == 1
    static char urcBuf[1024];
#endif
    static uCxAtClientConfig_t config = {
        .pRxBuffer = &rxBuf[0],
        .rxBufferLen = sizeof(rxBuf),
#if U_CX_USE_URC_QUEUE == 1
        .pUrcBuffer = &urcBuf[0],
        .urcBufferLen = sizeof(urcBuf),
#endif
        .pStreamHandle = NULL,
        .write = uCxAtWrite,
        .read = uCxAtRead
    };

    if (argc != 4) {
        fprintf(stderr, "Invalid arguments\n");
        fprintf(stderr, "Syntax: %s <device> <SSID> <WPA_PSK>\n", argv[0]);
        exit(1);
    }
    const char *pDevice = argv[1];
    const char *pSsid = argv[2];
    const char *pWpaPsk = argv[3];

    sem_init(&gUrcSem, 0, 0);

    gBootTime = getTickTimeMs();

    gUartFd = openUart(pDevice, 115200, true);
    if (gUartFd < 0) {
        printf("Failed to open UART\n");
        return 1;
    }
    config.pStreamHandle = &gUartFd;

    pthread_t readThread;
    uRxThreadArgs_t rxArgs = {
        .uartFd = gUartFd,
        .pClient = &client
    };

    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    // So that the thread is tidied-up when it exits
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = 9;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&readThread, &attr, readTask, &rxArgs);

    uCxAtClientInit(&config, &client);

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

    sem_destroy(&gUrcSem);
}
