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
 * @brief Windows OS port implementation (mutex, time, logging)
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "u_cx_log.h"
#include "u_cx_at_client.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

typedef struct {
    uCxAtClient_t *pClient;
    HANDLE rxThread;
    volatile bool terminateRxTask;
} uPortRxContext_t;

static int64_t gBootTime = 0;
static uPortRxContext_t gRxContext;

/* ----------------------------------------------------------------
 * STATIC FUNCTION DECLARATIONS
 * -------------------------------------------------------------- */

static int64_t getTickTimeMs(void);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS - TIME
 * -------------------------------------------------------------- */

static int64_t getTickTimeMs(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;

    if (!QueryPerformanceFrequency(&frequency)) {
        return 0;
    }

    if (!QueryPerformanceCounter(&counter)) {
        return 0;
    }

    return (counter.QuadPart * 1000LL) / frequency.QuadPart;
}

static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortRxContext_t *pCtx = (uPortRxContext_t *)lpParam;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance,
                    "RX thread started");

    while (!pCtx->terminateRxTask) {
        uCxAtClientHandleRx(pCtx->pClient);
        // Sleep for polling interval (10ms)
        Sleep(10);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated");
    return 0;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - TIME API
 * -------------------------------------------------------------- */

int32_t uPortGetTickTimeMs(void)
{
    if (gBootTime == 0) {
        gBootTime = getTickTimeMs();
    }

    return (int32_t)(getTickTimeMs() - gBootTime);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - MUTEX API
 * -------------------------------------------------------------- */

int32_t uPortMutexTryLock(HANDLE handle, int32_t timeoutMs)
{
    DWORD dwTimeout = (timeoutMs < 0) ? INFINITE : (DWORD)timeoutMs;
    DWORD dwResult;

    dwResult = WaitForSingleObject(handle, dwTimeout);

    if (dwResult == WAIT_OBJECT_0) {
        return 0;
    } else if (dwResult == WAIT_TIMEOUT) {
        return -2;  // Timeout
    } else {
        return -1;  // Error
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - PORT INITIALIZATION
 * -------------------------------------------------------------- */

void uPortInit(void)
{
    // Initialize boot time
    if (gBootTime == 0) {
        gBootTime = getTickTimeMs();
    }
}

void uPortDeinit(void)
{
    // Nothing to clean up for Windows minimal port
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - BACKGROUND RX TASK
 * -------------------------------------------------------------- */

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    memset(&gRxContext, 0, sizeof(gRxContext));
    gRxContext.pClient = pClient;
    gRxContext.terminateRxTask = false;

    // Create RX thread
    gRxContext.rxThread = CreateThread(NULL, 0, rxThread, &gRxContext, 0, NULL);
    if (gRxContext.rxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to create RX thread");
        gRxContext.pClient = NULL;
        return;
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Background RX thread started");
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;

    if (gRxContext.pClient == NULL || gRxContext.rxThread == NULL) {
        return;
    }

    gRxContext.terminateRxTask = true;
    WaitForSingleObject(gRxContext.rxThread, 5000);
    CloseHandle(gRxContext.rxThread);
    gRxContext.rxThread = NULL;
    gRxContext.pClient = NULL;
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Background RX thread stopped");
}
