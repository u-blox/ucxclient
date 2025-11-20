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
 * @brief Windows UART port implementation (polled mode)
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
#include "u_cx_at_client.h"
#include "u_port.h"
#include "u_port_uart.h"
#include "os/u_port_windows.h"

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
    volatile bool bTerminateRxTask;
    DWORD dwThreadId;
    int pollIntervalMs;  // Polling interval (default 10ms)
} uPortUartContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static HANDLE gHComPort = INVALID_HANDLE_VALUE;
static uPortUartContext_t *gActiveUart = NULL;  // Currently active UART context

/* ----------------------------------------------------------------
 * STATIC FUNCTION DECLARATIONS
 * -------------------------------------------------------------- */

static HANDLE openComPort(const char *pDevName, int baudRate, bool useFlowControl);
static DWORD WINAPI rxThread(LPVOID lpParam);

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
    uPortUartContext_t *pCtx = (uPortUartContext_t *)lpParam;
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
 * PUBLIC FUNCTIONS - UART PORT LAYER API
 * -------------------------------------------------------------- */

int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    DWORD dwBytesWritten = 0;

    if (!WriteFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, "WriteFile failed, error: %lu", dwError);
        if (dwError == ERROR_ACCESS_DENIED) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                "COM port access denied - port may be in use or device disconnected");
        }
        return -1;
    }

    return (int32_t)dwBytesWritten;
}

int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    DWORD dwBytesRead = 0;
    
    (void)timeoutMs;  // Timeout handled by COM port configuration

    if (!ReadFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        return -1;
    }

    return (int32_t)dwBytesRead;
}

uPortUartHandle_t uPortUartOpen(const char *pDevName, int32_t baudRate, bool useFlowControl)
{
    static uPortUartContext_t context;
    
    memset(&context, 0, sizeof(context));
    context.pClient = NULL;  // Will be set via uPortBgRxTaskCreate
    context.hComPort = INVALID_HANDLE_VALUE;
    context.hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    context.pollIntervalMs = 10;  // Poll every 10ms

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Opening %s at %d with %s flow control",
                  pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    context.hComPort = openComPort(pDevName, baudRate, useFlowControl);
    if (context.hComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open COM port");
        return NULL;
    }

    gHComPort = context.hComPort;
    context.bTerminateRxTask = false;
    gActiveUart = &context;

    return (uPortUartHandle_t)&context;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    
    if (pCtx == NULL) {
        return;
    }
    
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
}

void uPortUartPauseRx(uPortUartHandle_t handle)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    
    if (pCtx == NULL) {
        return;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "Pausing RX thread...");
    
    if (pCtx->hRxThread != NULL) {
        pCtx->bTerminateRxTask = true;
        SetEvent(pCtx->hStopEvent);
        WaitForSingleObject(pCtx->hRxThread, 5000);
        CloseHandle(pCtx->hRxThread);
        pCtx->hRxThread = NULL;
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread paused");
    }
}

void uPortUartResumeRx(uPortUartHandle_t handle)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    
    if (pCtx == NULL) {
        return;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "Resuming RX thread...");
    
    if (pCtx->hRxThread == NULL) {
        pCtx->bTerminateRxTask = false;
        ResetEvent(pCtx->hStopEvent);
        pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
        if (pCtx->hRxThread == NULL) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, "Failed to resume RX thread");
        } else {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread resumed");
        }
    }
}

void uPortUartFlush(uPortUartHandle_t handle)
{
    uPortUartContext_t *pCtx = (uPortUartContext_t *)handle;
    
    if (pCtx == NULL) {
        return;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "Flushing serial buffers...");
    
    if (pCtx->hComPort != INVALID_HANDLE_VALUE) {
        PurgeComm(pCtx->hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "Serial buffers flushed");
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS - COM PORT ENUMERATION
 * -------------------------------------------------------------- */

int uPortUartEnumeratePorts(char pPortList[][16], int maxPorts)
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

bool uPortUartIsPortAvailable(const char *pPortName)
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
 * BACKGROUND RX TASK MANAGEMENT
 * -------------------------------------------------------------- */

void uPortUartStartBgRxTask(uCxAtClient_t *pClient)
{
    if (gActiveUart == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                        "uPortUartStartBgRxTask called before uPortUartOpen");
        return;
    }
    
    if (gActiveUart->pClient != NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "RX task already running");
        return;
    }
    
    gActiveUart->pClient = pClient;
    
    // Create RX thread
    gActiveUart->hRxThread = CreateThread(NULL, 0, rxThread, gActiveUart, 0, &gActiveUart->dwThreadId);
    if (gActiveUart->hRxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to create RX thread");
        gActiveUart->pClient = NULL;
        return;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Background RX thread started");
}

void uPortUartStopBgRxTask(uCxAtClient_t *pClient)
{
    (void)pClient;
    
    if (gActiveUart == NULL || gActiveUart->hRxThread == NULL) {
        return;
    }
    
    gActiveUart->bTerminateRxTask = true;
    SetEvent(gActiveUart->hStopEvent);
    WaitForSingleObject(gActiveUart->hRxThread, 5000);
    CloseHandle(gActiveUart->hRxThread);
    gActiveUart->hRxThread = NULL;
    gActiveUart->pClient = NULL;
    
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Background RX thread stopped");
}

#endif /* _WIN32 */
