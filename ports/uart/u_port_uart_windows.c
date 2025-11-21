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
 * @brief Windows UART port implementation
 */

#include "u_port.h"

#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "u_cx_log.h"
#include "u_cx_at_client.h"
#include "u_port_uart.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    HANDLE hComPort;
} uPortUartHandle;

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
 * PUBLIC FUNCTIONS - UART PORT LAYER API
 * -------------------------------------------------------------- */

int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;
    DWORD dwBytesWritten = 0;

    if (pHandle == NULL || pData == NULL || length == 0) {
        return -1;
    }

    if (!WriteFile(pHandle->hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "WriteFile failed, error: %lu", dwError);
        if (dwError == ERROR_ACCESS_DENIED) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR,
                "COM port access denied - port may be in use or device disconnected");
        }
        return -1;
    }

    return (int32_t)dwBytesWritten;
}

int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;
    DWORD dwBytesRead = 0;

    if (pHandle == NULL || length == 0) {
        return -1;
    }

    (void)timeoutMs;  // Timeout handled by COM port configuration

    if (pData == NULL) {
        return 0;
    }

    if (!ReadFile(pHandle->hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        return -1;
    }

    return (int32_t)dwBytesRead;
}

uPortUartHandle_t uPortUartOpen(const char *pDevName, int32_t baudRate, bool useFlowControl)
{
    if (pDevName == NULL) {
        return NULL;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)malloc(sizeof(uPortUartHandle));
    if (pHandle == NULL) {
        return NULL;
    }

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "Opening %s at %d with %s flow control",
                  pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    pHandle->hComPort = openComPort(pDevName, baudRate, useFlowControl);
    if (pHandle->hComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open COM port");
        free(pHandle);
        return NULL;
    }

    return (uPortUartHandle_t)pHandle;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle->hComPort != INVALID_HANDLE_VALUE) {
        CloseHandle(pHandle->hComPort);
        pHandle->hComPort = INVALID_HANDLE_VALUE;
    }

    free(pHandle);
}
