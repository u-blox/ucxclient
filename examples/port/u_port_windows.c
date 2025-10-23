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
 * @brief Windows port implementation
 *
 * This implementation provides Windows COM port support for the ucxclient.
 * It uses Win32 API for serial communication, threading, and synchronization.
 */

#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "u_cx_log.h"
#include "u_port.h"
#include "u_port_windows.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define MAX_COM_PORTS 256

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    HANDLE hComPort;
    uCxAtClient_t *pClient;
    HANDLE hRxThread;
    HANDLE hStopEvent;
    DWORD dwThreadId;
    volatile bool bTerminateRxTask;
} uPortContext_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static DWORD WINAPI rxThread(LPVOID lpParam);
static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length);
static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs);

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint64_t gBootTime = 0;
static HANDLE gHComPort = INVALID_HANDLE_VALUE;
static uCxAtClientConfig_t *gPConfig = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t getTickTimeMs(void)
{
    // Get lower 32 bits of tick count and cast to signed int32_t
    // This intentionally wraps around every ~49.7 days which is acceptable for tick time
    uint64_t tick64 = GetTickCount64();
    return (int32_t)(tick64 & 0xFFFFFFFFULL);
}

static HANDLE openComPort(const char *pPortName, int baudRate, bool useFlowControl)
{
    HANDLE hComPort;
    DCB dcb = {0};
    COMMTIMEOUTS timeouts = {0};
    char fullPortName[32];

    // Format port name (e.g., "COM3" becomes "\\\\.\\COM3")
    if (strncmp(pPortName, "COM", 3) == 0) {
        snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", pPortName);
    } else {
        snprintf(fullPortName, sizeof(fullPortName), "%s", pPortName);
    }

    // Open COM port
    hComPort = CreateFileA(
        fullPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,                    // No sharing
        NULL,                 // Default security
        OPEN_EXISTING,        // Open existing port
        0,                    // Non-overlapped I/O
        NULL                  // No template
    );

    if (hComPort == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        (void)dwError;  // Mark as intentionally unused when logging is disabled
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open %s, error: %lu", fullPortName, dwError);
        return INVALID_HANDLE_VALUE;
    }

    // Configure COM port
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(hComPort, &dcb)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "GetCommState failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    dcb.BaudRate = baudRate;
    dcb.ByteSize = 8;
    dcb.Parity = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    
    if (useFlowControl) {
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutxCtsFlow = TRUE;
    } else {
        dcb.fRtsControl = RTS_CONTROL_DISABLE;
        dcb.fOutxCtsFlow = FALSE;
    }
    
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;
    dcb.fErrorChar = FALSE;
    dcb.fNull = FALSE;
    dcb.fAbortOnError = FALSE;

    if (!SetCommState(hComPort, &dcb)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "SetCommState failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    // Set timeouts
    timeouts.ReadIntervalTimeout = 50;         // Max time between chars (ms)
    timeouts.ReadTotalTimeoutMultiplier = 10;  // ms per char
    timeouts.ReadTotalTimeoutConstant = 100;   // Base timeout (ms)
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;

    if (!SetCommTimeouts(hComPort, &timeouts)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "SetCommTimeouts failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    // Purge any existing data
    PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    return hComPort;
}

static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    DWORD dwWaitResult;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread started");

    while (!pCtx->bTerminateRxTask) {
        // Wait for data or stop event
        dwWaitResult = WaitForSingleObject(pCtx->hStopEvent, 100);
        
        if (dwWaitResult == WAIT_OBJECT_0) {
            break; // Stop event signaled
        }

        // Check for received data
        uCxAtClientHandleRx(pCtx->pClient);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated");
    return 0;
}

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    (void)pClient;
    HANDLE hComPort = *((HANDLE *)pStreamHandle);
    DWORD dwBytesWritten = 0;

    if (hComPort == INVALID_HANDLE_VALUE) {
        return -1;
    }

    if (!WriteFile(hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        (void)dwError;  // Mark as intentionally unused when logging is disabled
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "WriteFile failed, error: %lu", dwError);
        return -1;
    }

    return (int32_t)dwBytesWritten;
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    (void)pClient;
    HANDLE hComPort = *((HANDLE *)pStreamHandle);
    DWORD dwBytesRead = 0;
    COMMTIMEOUTS timeouts = {0};

    if (hComPort == INVALID_HANDLE_VALUE) {
        return -1;
    }

    // Set timeout for this read operation
    if (timeoutMs == 0) {
        // Non-blocking read
        timeouts.ReadIntervalTimeout = MAXDWORD;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = 0;
    } else {
        // Blocking read with timeout
        timeouts.ReadIntervalTimeout = 0;
        timeouts.ReadTotalTimeoutMultiplier = 0;
        timeouts.ReadTotalTimeoutConstant = timeoutMs;
    }
    timeouts.WriteTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 100;

    SetCommTimeouts(hComPort, &timeouts);

    if (!ReadFile(hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_TIMEOUT) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "ReadFile failed, error: %lu", dwError);
            return -1;
        }
    }

    return (int32_t)dwBytesRead;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortMutexTryLock(HANDLE mutex, uint32_t timeoutMs)
{
    DWORD dwResult = WaitForSingleObject(mutex, timeoutMs);
    
    if (dwResult == WAIT_OBJECT_0) {
        return 0; // Success
    } else if (dwResult == WAIT_TIMEOUT) {
        return -1; // Timeout
    } else {
        return -2; // Error
    }
}

int32_t uPortGetTickTimeMs(void)
{
    // Compute the difference using the lower 32-bit wrapped values
    // This handles tick count wrapping correctly since both values wrap at the same point
    int32_t currentTick = getTickTimeMs();
    int32_t bootTick = (int32_t)(gBootTime & 0xFFFFFFFFULL);
    return currentTick - bootTick;
}

int32_t uPortEnumerateComPorts(char pPortList[][16], int32_t maxPorts)
{
    HKEY hKey;
    DWORD dwIndex = 0;
    char valueName[256];
    char valueData[256];
    DWORD valueNameSize, valueDataSize;
    int32_t portCount = 0;
    
    // Open registry key for serial comm ports
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, 
                     "HARDWARE\\DEVICEMAP\\SERIALCOMM", 
                     0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return -1;
    }
    
    while (portCount < maxPorts) {
        valueNameSize = sizeof(valueName);
        valueDataSize = sizeof(valueData);
        
        if (RegEnumValueA(hKey, dwIndex, valueName, &valueNameSize, 
                         NULL, NULL, (LPBYTE)valueData, &valueDataSize) != ERROR_SUCCESS) {
            break;
        }
        
        // valueData contains the COM port name (e.g., "COM3")
        snprintf(pPortList[portCount], 16, "%s", valueData);
        portCount++;
        dwIndex++;
    }
    
    RegCloseKey(hKey);
    return portCount;
}

bool uPortIsComPortAvailable(const char *pPortName)
{
    char fullPortName[32];
    HANDLE hComPort;
    
    // Format port name
    if (strncmp(pPortName, "COM", 3) == 0) {
        snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", pPortName);
    } else {
        snprintf(fullPortName, sizeof(fullPortName), "%s", pPortName);
    }
    
    hComPort = CreateFileA(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, 
                          NULL, OPEN_EXISTING, 0, NULL);
    
    if (hComPort == INVALID_HANDLE_VALUE) {
        return false;
    }
    
    CloseHandle(hComPort);
    return true;
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
    context.hComPort = INVALID_HANDLE_VALUE;
    context.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

    // Current implementation only supports one instance
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
    assert(pCtx->hComPort == INVALID_HANDLE_VALUE);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Opening %s at %d with %s flow control",
                    pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    gHComPort = openComPort(pDevName, baudRate, useFlowControl);
    if (gHComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open COM port");
        return false;
    }
    
    pCtx->hComPort = gHComPort;
    pCtx->bTerminateRxTask = false;

    // Reset stop event
    ResetEvent(pCtx->hStopEvent);

    // Create RX thread
    pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
    if (pCtx->hRxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to create RX thread");
        CloseHandle(gHComPort);
        gHComPort = INVALID_HANDLE_VALUE;
        pCtx->hComPort = INVALID_HANDLE_VALUE;
        return false;
    }

    return true;
}

void uPortAtClose(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx->hRxThread != NULL) {
        pCtx->bTerminateRxTask = true;
        SetEvent(pCtx->hStopEvent);
        WaitForSingleObject(pCtx->hRxThread, 5000); // Wait up to 5 seconds
        CloseHandle(pCtx->hRxThread);
        pCtx->hRxThread = NULL;
    }

    if (pCtx->hComPort != INVALID_HANDLE_VALUE) {
        CloseHandle(pCtx->hComPort);
        pCtx->hComPort = INVALID_HANDLE_VALUE;
        gHComPort = INVALID_HANDLE_VALUE;
    }

    if (pCtx->hStopEvent != NULL) {
        CloseHandle(pCtx->hStopEvent);
        pCtx->hStopEvent = NULL;
    }

    gPConfig = NULL;
}

#endif /* _WIN32 */