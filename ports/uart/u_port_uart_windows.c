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

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <setupapi.h>
#include <devguid.h>
#include <regstr.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "u_port.h"

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

/**
 * @brief Set the FTDI USB-serial Latency Timer to the given value (ms).
 *
 * FTDI chips buffer received bytes and deliver them to the host USB
 * endpoint only when the buffer is full OR the Latency Timer fires.
 * The factory default is 16 ms, which adds up to 16 ms of hidden
 * latency on every short response from the module.
 *
 * This function finds the COM port's device node via SetupAPI and
 * writes the LatencyTimer DWORD in the Device Parameters registry key.
 * The FTDI driver picks up the new value on the next port open, so we
 * call this *before* CreateFile().
 *
 * Non-FTDI ports simply won't have the key — that's fine, we ignore errors.
 */
static void setFtdiLatencyTimer(const char *pPortName, DWORD latencyMs)
{
    HDEVINFO devInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL,
                                           DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE) return;

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++) {
        // Open Device Parameters key and check PortName value
        HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL,
                                         0, DIREG_DEV, KEY_READ | KEY_WRITE);
        if (hKey == INVALID_HANDLE_VALUE) continue;

        char portName[32] = {0};
        DWORD portNameSize = sizeof(portName);
        DWORD regType = 0;
        LONG rc = RegQueryValueExA(hKey, "PortName", NULL, &regType,
                                   (LPBYTE)portName, &portNameSize);
        if (rc == ERROR_SUCCESS && regType == REG_SZ &&
            _stricmp(portName, pPortName) == 0) {
            // Found the right port — set LatencyTimer
            DWORD oldLatency = 0;
            DWORD oldSize = sizeof(oldLatency);
            DWORD oldType = 0;
            RegQueryValueExA(hKey, "LatencyTimer", NULL, &oldType,
                             (LPBYTE)&oldLatency, &oldSize);
            if (oldLatency != latencyMs) {
                rc = RegSetValueExA(hKey, "LatencyTimer", 0, REG_DWORD,
                                    (const BYTE *)&latencyMs, sizeof(latencyMs));
                if (rc == ERROR_SUCCESS) {
                    U_CX_LOG_LINE(U_CX_LOG_CH_DBG,
                        "FTDI %s LatencyTimer: %lu -> %lu ms", pPortName,
                        oldLatency, latencyMs);
                }
            }
            RegCloseKey(hKey);
            break;
        }
        RegCloseKey(hKey);
    }
    SetupDiDestroyDeviceInfoList(devInfo);
}

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

    // Set FTDI Latency Timer to 1ms for minimum USB polling latency.
    // Must be done before CreateFile so the FTDI driver picks it up.
    setFtdiLatencyTimer(pDevName, 1);

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

#if U_CX_EVENT_DRIVEN_IO == 1
    // Set timeouts – optimized for event-driven bulk reads.
    //
    // Per MSDN, when ReadIntervalTimeout = MAXDWORD and both multiplier
    // and constant are > 0, ReadFile behaves as:
    //   "return immediately with bytes already in the driver buffer;
    //    if the buffer is empty, wait up to ReadTotalTimeoutConstant ms
    //    for the first byte, then return whatever has arrived."
    //
    // This gives us the best of both worlds:
    //   - Zero-copy fast path when data is already buffered (no 1ms gap wait)
    //   - Short 1ms blocking wait when idle (minimizes mutex contention)
    //   - No Sleep() poll loop needed in the RX thread
    timeouts.ReadIntervalTimeout = MAXDWORD;
    timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
    timeouts.ReadTotalTimeoutConstant = 1;
#else
    // Original polled timeouts
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 100;
#endif
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
