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
 * @brief "No OS" port layer example
 *
 * This port shows you how you can run ucxclient on a system without
 * mutex- and thread support.
 *
 * This example port uses Linux implementation for U_CX_PORT_GET_TIME_MS()
 * and UART driver. Normally you will need to implement this part
 * for your specific target.
 */

#include <time.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <errno.h>
#include <string.h>

#include "u_cx_log.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    int uartFd;
    uCxAtClient_t *pClient;
} uPortContext_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint64_t gBootTime = 0;
static int gUartFd;
static uCxAtClientConfig_t *gPConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t getTickTimeMs(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    uint64_t timeMs = (time.tv_sec * 1000) + (time.tv_nsec / (1000 * 1000));
    return (int32_t)(timeMs % (1000 * 60 * 60 * 24));
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

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    (void)pClient;
    int uartFd = *((int *)pStreamHandle);

    return (int32_t)write(uartFd, pData, length);
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
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

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortGetTickTimeMs(void)
{
    return getTickTimeMs() - gBootTime;
}

void uPortAtInit(uCxAtClient_t *pClient)
{
    static uPortContext_t context;
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
        .pStreamHandle = &context,
        .write = uartWrite,
        .read = uartRead
    };

    memset(&context, 0, sizeof(context));
    context.pClient = pClient;
    context.uartFd = -1;

    // Current implementation of this port only support one instance
    assert(gPConfig == NULL);
    gPConfig = &config;

    if (gBootTime == 0) {
        gBootTime = getTickTimeMs();
    }
    uCxAtClientInit(&config, pClient);
}

bool uPortAtOpen(uCxAtClient_t *pClient, const char *pDevName, int baudRate, bool useFlowControl)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    assert(pClient->pConfig != NULL);
    assert(pCtx != NULL);
    assert(pCtx->uartFd == -1);
    gUartFd = openUart(pDevName, baudRate, useFlowControl);
    if (gUartFd < 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open UART");
        return false;
    }
    pCtx->uartFd = gUartFd;

    return true;
}

void uPortAtClose(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    assert(pCtx->uartFd != -1);

    close(pCtx->uartFd);
    pCtx->uartFd = -1;
}

int32_t uCxMutexTryLock(bool *pMutex, int32_t timeoutMs)
{
    (void)timeoutMs;
    int32_t ret = *pMutex ? -1 : 0;
    *pMutex = true;
    return ret;
}
