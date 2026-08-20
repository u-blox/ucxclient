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
                } else {
                    U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
                        "FTDI %s LatencyTimer: failed to set %lu -> %lu ms (error %ld, try Device Manager)",
                        pPortName, oldLatency, latencyMs, rc);
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
    // Polled timeouts — 10ms gives reasonable responsiveness
    // without the complexity of event-driven IO.
    timeouts.ReadIntervalTimeout = 0;
    timeouts.ReadTotalTimeoutMultiplier = 0;
    timeouts.ReadTotalTimeoutConstant = 10;
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

    // timeoutMs is not used per-call on Windows — the COM port timeout
    // is set once at open time (1ms for event-driven, 100ms for polled).
    // If per-call timeout control is needed in the future, SetCommTimeouts
    // would need to be called here before ReadFile.
    (void)timeoutMs;

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

void uPortUartFlushRx(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle->hComPort != INVALID_HANDLE_VALUE) {
        // PURGE_RXCLEAR: Clears the input buffer (if the device driver has one)
        // PURGE_RXABORT: Terminates all outstanding overlapped read operations
        PurgeComm(pHandle->hComPort, PURGE_RXCLEAR | PURGE_RXABORT);
    }
}

void uPortUartFlushTx(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle->hComPort != INVALID_HANDLE_VALUE) {
        // First flush - wait for pending writes to complete
        FlushFileBuffers(pHandle->hComPort);
        // Then purge any remaining data in the driver buffer
        PurgeComm(pHandle->hComPort, PURGE_TXCLEAR | PURGE_TXABORT);
    }
}

void uPortUartPrintDiagnostics(const char *pPortName)
{
    if (pPortName == NULL) return;

    HDEVINFO devInfo = SetupDiGetClassDevs(&GUID_DEVCLASS_PORTS, NULL, NULL,
                                           DIGCF_PRESENT);
    if (devInfo == INVALID_HANDLE_VALUE) {
        printf("  [UART] Cannot enumerate COM ports\n");
        return;
    }

    SP_DEVINFO_DATA devInfoData;
    devInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    bool found = false;

    for (DWORD i = 0; SetupDiEnumDeviceInfo(devInfo, i, &devInfoData); i++) {
        HKEY hKey = SetupDiOpenDevRegKey(devInfo, &devInfoData, DICS_FLAG_GLOBAL,
                                         0, DIREG_DEV, KEY_READ);
        if (hKey == INVALID_HANDLE_VALUE) continue;

        char portName[32] = {0};
        DWORD portNameSize = sizeof(portName);
        DWORD regType = 0;
        LONG rc = RegQueryValueExA(hKey, "PortName", NULL, &regType,
                                   (LPBYTE)portName, &portNameSize);
        if (rc != ERROR_SUCCESS || regType != REG_SZ ||
            _stricmp(portName, pPortName) != 0) {
            RegCloseKey(hKey);
            continue;
        }

        found = true;

        // Read FTDI LatencyTimer
        DWORD latency = 0;
        DWORD latencySize = sizeof(latency);
        DWORD latencyType = 0;
        rc = RegQueryValueExA(hKey, "LatencyTimer", NULL, &latencyType,
                              (LPBYTE)&latency, &latencySize);
        bool hasFtdi = (rc == ERROR_SUCCESS && latencyType == REG_DWORD);

        RegCloseKey(hKey);

        // Get device description (driver friendly name)
        char desc[256] = {0};
        SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData,
            SPDRP_FRIENDLYNAME, NULL, (PBYTE)desc, sizeof(desc), NULL);

        char driver[256] = {0};
        SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData,
            SPDRP_DRIVER, NULL, (PBYTE)driver, sizeof(driver), NULL);

        char mfg[128] = {0};
        SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData,
            SPDRP_MFG, NULL, (PBYTE)mfg, sizeof(mfg), NULL);

        char hwId[256] = {0};
        SetupDiGetDeviceRegistryPropertyA(devInfo, &devInfoData,
            SPDRP_HARDWAREID, NULL, (PBYTE)hwId, sizeof(hwId), NULL);

        printf("  [UART] Port: %s\n", pPortName);
        printf("  [UART] Device: %s\n", desc[0] ? desc : "(unknown)");
        printf("  [UART] Manufacturer: %s\n", mfg[0] ? mfg : "(unknown)");
        printf("  [UART] Hardware ID: %s\n", hwId[0] ? hwId : "(unknown)");
        if (hasFtdi) {
            printf("  [UART] FTDI LatencyTimer: %lu ms\n", latency);
            if (latency > 2) {
                printf("  [UART] *** WARNING: FTDI LatencyTimer is %lu ms (should be 1 ms) ***\n", latency);
                printf("  [UART] High latency adds ~%lu ms per short USB transfer.\n", latency);
                printf("  [UART] Fix: Device Manager -> %s -> Properties ->\n", desc);
                printf("  [UART]       Port Settings -> Advanced -> Latency Timer -> 1\n");
            }
        } else {
            printf("  [UART] FTDI LatencyTimer: N/A (not FTDI)\n");
        }
#if U_CX_EVENT_DRIVEN_IO == 1
        printf("  [UART] IO mode: event-driven (MAXDWORD/1ms)\n");
#else
        printf("  [UART] IO mode: polled (10ms timeout)\n");
#endif
        break;
    }

    if (!found) {
        printf("  [UART] Port %s: not found in device list\n", pPortName);
    }

    SetupDiDestroyDeviceInfoList(devInfo);
}
