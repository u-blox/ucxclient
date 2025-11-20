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

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

#include "u_cx_log.h"
#include "u_port.h"
#include "os/u_port_windows.h"

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static int64_t gBootTime = 0;
static uPortLogCallback_t gLogCallback = NULL;
static void *gLogUserData = NULL;

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

void uPortDelayMs(int32_t delayMs)
{
    Sleep((DWORD)delayMs);
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

void uPortMutexUnlock(HANDLE handle)
{
    ReleaseMutex(handle);
}

int32_t uPortMutexCreate(HANDLE *pHandle)
{
    HANDLE hMutex;
    
    hMutex = CreateMutex(NULL, FALSE, NULL);
    if (hMutex == NULL) {
        return -1;
    }
    
    *pHandle = hMutex;
    return 0;
}

void uPortMutexDelete(HANDLE handle)
{
    if (handle != NULL) {
        CloseHandle(handle);
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - LOGGING API
 * -------------------------------------------------------------- */

void uPortRegisterLogCallback(uPortLogCallback_t callback, void *pUserData)
{
    gLogCallback = callback;
    gLogUserData = pUserData;
}

void uPortLogPrintf(const char *pFormat, ...)
{
    char buffer[512];
    va_list args;
    
    va_start(args, pFormat);
    vsnprintf(buffer, sizeof(buffer), pFormat, args);
    va_end(args);
    
    if (gLogCallback != NULL) {
        gLogCallback(buffer, gLogUserData);
    } else {
        // Default: print to console
        printf("%s", buffer);
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

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    // Forward declaration - implemented in u_port_uart_windows.c
    extern void uPortUartStartBgRxTask(uCxAtClient_t *pClient);
    uPortUartStartBgRxTask(pClient);
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    // Forward declaration - implemented in u_port_uart_windows.c
    extern void uPortUartStopBgRxTask(uCxAtClient_t *pClient);
    uPortUartStopBgRxTask(pClient);
}

#endif /* _WIN32 */
