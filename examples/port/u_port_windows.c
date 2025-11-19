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
 * @brief Minimal Windows port implementation (polled mode only)
 *
 * This is a streamlined version of u_port_windows.c containing ONLY
 * the polled UART implementation. This minimal port is recommended for:
 * - Simple applications that don't need FTDI or event-driven modes
 * - Easier debugging and understanding of the port layer
 * - Reduced code size and complexity
 * 
 * FEATURES:
 * - Polled RX mode (10ms intervals)
 * - Standard Windows COM port support
 * - COM port enumeration
 * - Basic error handling
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
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "u_cx_log.h"
#include "u_port.h"
#include "u_port_windows.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

// This minimal port only supports polled mode
#define USE_UART_POLLED

#define MAX_COM_PORTS 256

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    HANDLE hComPort;
    uCxAtClient_t *pClient;
    HANDLE hRxThread;
    HANDLE hStopEvent;
    volatile bool bTerminateRxTask;
    DWORD dwThreadId;
    int pollIntervalMs;  // Polling interval (default 10ms)
} uPortContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uCxAtClientConfig_t *gPConfig = NULL;
static int64_t gBootTime = 0;
static HANDLE gHComPort = INVALID_HANDLE_VALUE;
static uPortLogCallback_t gLogCallback = NULL;
static void *gLogUserData = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTION DECLARATIONS
 * -------------------------------------------------------------- */

static int64_t getTickTimeMs(void);
static HANDLE openComPort(const char *pDevName, int baudRate, bool useFlowControl);
static DWORD WINAPI rxThread(LPVOID lpParam);
static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length);
static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS - TIME
 * -------------------------------------------------------------- */

static int64_t getTickTimeMs(void)
{
    LARGE_INTEGER frequency;
    LARGE_INTEGER counter;
    
    if (!QueryPerformanceFrequency(&frequency)) {
        return GetTickCount64();
    }
    
    if (!QueryPerformanceCounter(&counter)) {
        return GetTickCount64();
    }
    
    return (counter.QuadPart * 1000LL) / frequency.QuadPart;
}

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS - COM PORT
 * -------------------------------------------------------------- */

static HANDLE openComPort(const char *pDevName, int baudRate, bool useFlowControl)
{
    char fullPortName[32];
    HANDLE hComPort;
    DCB dcb = {0};
    COMMTIMEOUTS timeouts = {0};

    // Format port name (add "\\\\.\\" prefix for ports > COM9)
    if (strncmp(pDevName, "COM", 3) == 0) {
        snprintf(fullPortName, sizeof(fullPortName), "\\\\.\\%s", pDevName);
    } else {
        snprintf(fullPortName, sizeof(fullPortName), "%s", pDevName);
    }

    // Open COM port
    hComPort = CreateFileA(fullPortName, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                          OPEN_EXISTING, 0, NULL);

    if (hComPort == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        (void)dwError;
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open %s, error: %lu", fullPortName, dwError);
        return INVALID_HANDLE_VALUE;
    }

    // Set larger COM port buffers (16KB each)
    if (!SetupComm(hComPort, 16384, 16384)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "SetupComm failed, using default buffer sizes");
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
    dcb.fBinary = TRUE;
    
    if (useFlowControl) {
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutxCtsFlow = TRUE;
    } else {
        dcb.fRtsControl = RTS_CONTROL_ENABLE;
        dcb.fOutxCtsFlow = FALSE;
    }
    
    dcb.fDtrControl = DTR_CONTROL_ENABLE;
    dcb.fOutxDsrFlow = FALSE;
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
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 100;
    timeouts.WriteTotalTimeoutMultiplier = 0;
    timeouts.WriteTotalTimeoutConstant = 1000;

    if (!SetCommTimeouts(hComPort, &timeouts)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "SetCommTimeouts failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    // Purge any existing data
    PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    return hComPort;
}

/* ----------------------------------------------------------------
 * RX THREAD - POLLED MODE
 * -------------------------------------------------------------- */

static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    COMSTAT comStat;
    DWORD dwErrors;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, 
                    "RX thread started (polled mode, %dms interval)", pCtx->pollIntervalMs);

    while (!pCtx->bTerminateRxTask) {
        // Check if stop event is signaled
        if (WaitForSingleObject(pCtx->hStopEvent, 0) == WAIT_OBJECT_0) {
            break;
        }

        // Check for available data
        ClearCommError(pCtx->hComPort, &dwErrors, &comStat);
        
        // Check for UART errors
        if (dwErrors != 0) {
            if (dwErrors & CE_RXOVER) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "UART RX buffer overrun! Data lost.");
            }
            if (dwErrors & CE_OVERRUN) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "UART hardware overrun! Data corrupted.");
            }
            if (dwErrors & CE_FRAME) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "UART framing error! Data corrupted.");
            }
            if (dwErrors & CE_BREAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "UART break condition detected.");
            }
            if (dwErrors & CE_RXPARITY) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "UART parity error! Data corrupted.");
            }
        }
        
        // Process data if available
        if (comStat.cbInQue > 0) {
            uCxAtClientHandleRx(pCtx->pClient);
        }
        
        // Sleep for polling interval
        Sleep(pCtx->pollIntervalMs);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated");
    return 0;
}

/* ----------------------------------------------------------------
 * UART READ/WRITE CALLBACKS
 * -------------------------------------------------------------- */

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    DWORD dwBytesWritten = 0;

    if (!WriteFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "WriteFile failed, error: %lu", dwError);
        if (dwError == ERROR_ACCESS_DENIED) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                "COM port access denied - port may be in use or device disconnected");
        }
        return -1;
    }

    return (int32_t)dwBytesWritten;
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    DWORD dwBytesRead = 0;
    
    (void)timeoutMs;  // Timeout handled by COM port configuration
    (void)pClient;

    if (!ReadFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        return -1;
    }

    return (int32_t)dwBytesRead;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - TIME
 * -------------------------------------------------------------- */

int32_t uPortGetTickTimeMs(void)
{
    return (int32_t)(getTickTimeMs() - gBootTime);
}

void uPortDelayMs(uint32_t delayMs)
{
    Sleep((DWORD)delayMs);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - COM PORT ENUMERATION
 * -------------------------------------------------------------- */

int uPortEnumerateComPorts(char pPortList[][16], int maxPorts)
{
    HKEY hKey;
    DWORD dwIndex = 0;
    char valueName[256];
    DWORD valueNameSize;
    BYTE valueData[256];
    DWORD valueDataSize;
    DWORD valueType;
    int portCount = 0;
    
    if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DEVICEMAP\\SERIALCOMM", 
                      0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return 0;
    }
    
    while (portCount < maxPorts) {
        valueNameSize = sizeof(valueName);
        valueDataSize = sizeof(valueData);
        
        if (RegEnumValueA(hKey, dwIndex, valueName, &valueNameSize, 
                         NULL, &valueType, valueData, &valueDataSize) != ERROR_SUCCESS) {
            break;
        }
        
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

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - AT PORT
 * -------------------------------------------------------------- */

void uPortAtInit(uCxAtClient_t *pClient)
{
    static uPortContext_t context;
    static char rxBuf[8192];
#if U_CX_USE_URC_QUEUE == 1
    static char urcBuf[4096];
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
    context.pollIntervalMs = 10;  // Poll every 10ms

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

    pCtx->hComPort = openComPort(pDevName, baudRate, useFlowControl);
    if (pCtx->hComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open COM port");
        return false;
    }

    gHComPort = pCtx->hComPort;
    pCtx->bTerminateRxTask = false;

    // Create RX thread
    pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
    if (pCtx->hRxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to create RX thread");
        CloseHandle(pCtx->hComPort);
        pCtx->hComPort = INVALID_HANDLE_VALUE;
        gHComPort = INVALID_HANDLE_VALUE;
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
        WaitForSingleObject(pCtx->hRxThread, 5000);
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

/* ----------------------------------------------------------------
 * PAUSE/RESUME/FLUSH FUNCTIONS
 * 
 * These functions are provided for compatibility but are currently
 * stubbed out. Uncomment the implementation if you need raw serial
 * access for protocols like XMODEM.
 * -------------------------------------------------------------- */

void uPortAtPauseRx(uCxAtClient_t *pClient)
{
    // STUBBED: Uncomment below to enable RX thread pausing for raw serial access
    /*
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Pausing RX thread...");
    
    if (pCtx != NULL && pCtx->hRxThread != NULL) {
        pCtx->bTerminateRxTask = true;
        SetEvent(pCtx->hStopEvent);
        WaitForSingleObject(pCtx->hRxThread, 5000);
        CloseHandle(pCtx->hRxThread);
        pCtx->hRxThread = NULL;
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "RX thread paused");
    }
    */
    (void)pClient;
}

void uPortAtResumeRx(uCxAtClient_t *pClient)
{
    // STUBBED: Uncomment below to enable RX thread resuming after raw serial access
    /*
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Resuming RX thread...");
    
    if (pCtx != NULL && pCtx->hRxThread == NULL) {
        pCtx->bTerminateRxTask = false;
        ResetEvent(pCtx->hStopEvent);
        pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
        if (pCtx->hRxThread == NULL) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to resume RX thread");
        } else {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "RX thread resumed");
        }
    }
    */
    (void)pClient;
}

void uPortAtFlush(uCxAtClient_t *pClient)
{
    // STUBBED: Uncomment below to enable buffer flushing
    /*
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Flushing serial buffers...");
    
    if (pCtx->hComPort != INVALID_HANDLE_VALUE) {
        PurgeComm(pCtx->hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        
        if (pClient->pConfig->pRxBuffer != NULL) {
            memset(pClient->pConfig->pRxBuffer, 0, pClient->pConfig->rxBufferLen);
        }
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Serial buffers flushed");
    }
    */
    (void)pClient;
}

/* ----------------------------------------------------------------
 * MUTEX FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortMutexTryLock(HANDLE mutex, uint32_t timeoutMs)
{
    DWORD dwTimeout = (timeoutMs == 0xFFFFFFFF) ? INFINITE : (DWORD)timeoutMs;
    DWORD dwResult = WaitForSingleObject(mutex, dwTimeout);
    
    if (dwResult == WAIT_OBJECT_0) {
        return 0;  // Success
    } else if (dwResult == WAIT_TIMEOUT) {
        return -2;  // Timeout
    } else {
        return -1;  // Error
    }
}

void uPortMutexUnlock(HANDLE mutex)
{
    ReleaseMutex(mutex);
}

HANDLE uPortMutexCreate(void)
{
    return CreateMutex(NULL, FALSE, NULL);
}

void uPortMutexDelete(HANDLE mutex)
{
    if (mutex != NULL) {
        CloseHandle(mutex);
    }
}

/* ----------------------------------------------------------------
 * LOGGING FUNCTIONS
 * -------------------------------------------------------------- */

void uPortRegisterLogCallback(uPortLogCallback_t callback, void *pUserData)
{
    gLogCallback = callback;
    gLogUserData = pUserData;
}

void uPortLogPrintf(const char *format, ...)
{
    char buffer[512];
    va_list args;
    
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    
    if (gLogCallback != NULL) {
        gLogCallback(buffer, gLogUserData);
    }
    
    printf("%s", buffer);
}

#endif /* _WIN32 */
