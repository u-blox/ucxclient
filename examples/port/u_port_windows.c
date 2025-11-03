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
 * 
 * UART IMPLEMENTATION MODES:
 * ==========================
 * This file supports three different UART RX implementations:
 * 
 * 1. USE_UART_EVENT_DRIVEN (default, recommended for production)
 *    - Uses WaitCommEvent() for efficient event-driven processing
 *    - Minimal CPU usage, fast response time
 *    - Loops until RX buffer is completely drained
 *    - Good for production use
 * 
 * 2. USE_UART_POLLED (for debugging)
 *    - Simple polling loop checking for data
 *    - Easy to understand and debug
 *    - Predictable timing (10ms polling interval)
 *    - Good for troubleshooting timing issues
 * 
 * 3. USE_UART_FTDI (future implementation)
 *    - Uses FTDI D2XX API directly
 *    - Bypasses Windows COM driver
 *    - See D2XX Programmer's Guide for API details
 * 
 * To switch modes, uncomment the desired #define below in the
 * "COMPILE-TIME MACROS" section.
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
#include <string.h>
#include <assert.h>

#include "u_cx_log.h"
#include "u_port.h"
#include "u_port_windows.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

// UART RX Implementation Selection
// Uncomment ONE of the following to select UART implementation:
#define USE_UART_POLLED        // Simple polled mode (good for debugging)
//#define USE_UART_EVENT_DRIVEN     // Event-driven with WaitCommEvent (default)
// #define USE_UART_FTDI          // FTDI D2XX API (future implementation)

#if defined(USE_UART_POLLED) + defined(USE_UART_EVENT_DRIVEN) + defined(USE_UART_FTDI) != 1
#error "Exactly ONE UART implementation must be defined"
#endif

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
    
#if defined(USE_UART_EVENT_DRIVEN)
    // Event-driven mode specific fields
    HANDLE hCommEvent;      // Event for WaitCommEvent (data arrival notification)
    HANDLE hReadEvent;      // Event for read operations
    HANDLE hWriteEvent;     // Event for write operations
    OVERLAPPED overlapped;  // For WaitCommEvent
    OVERLAPPED ovRead;      // For read operations
    OVERLAPPED ovWrite;     // For write operations
#endif
    
#if defined(USE_UART_POLLED)
    // Polled mode specific fields
    int32_t pollIntervalMs; // Polling interval in milliseconds
#endif
    
#if defined(USE_UART_FTDI)
    // FTDI D2XX specific fields (future)
    void *pFtdiHandle;      // FT_HANDLE
#endif
    
    DWORD dwThreadId;
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
static uPortLogCallback_t gLogCallback = NULL;
static void *gLogUserData = NULL;

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

    // Open COM port with FILE_FLAG_OVERLAPPED for event-driven I/O
    hComPort = CreateFileA(
        fullPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,                    // No sharing
        NULL,                 // Default security
        OPEN_EXISTING,        // Open existing port
        FILE_FLAG_OVERLAPPED, // Use overlapped I/O for event notifications
        NULL                  // No template
    );

    if (hComPort == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        (void)dwError;  // Mark as intentionally unused when logging is disabled
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Failed to open %s, error: %lu", fullPortName, dwError);
        return INVALID_HANDLE_VALUE;
    }

    // Set larger COM port buffers for XMODEM transfers (16KB each)
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
    dcb.fBinary = TRUE;  // CRITICAL: Enable binary mode (no character processing)
    
    if (useFlowControl) {
        dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
        dcb.fOutxCtsFlow = TRUE;
    } else {
        // IMPORTANT: Even without flow control handshaking, keep RTS asserted
        // Many devices (including NORA-W36) require RTS to be high to remain active
        dcb.fRtsControl = RTS_CONTROL_ENABLE;  // Changed from DISABLE to ENABLE
        dcb.fOutxCtsFlow = FALSE;
    }
    
    // CRITICAL: Enable DTR to keep device active
    // Many UART devices (including u-blox modules) need DTR asserted
    dcb.fDtrControl = DTR_CONTROL_ENABLE;  // Changed from DISABLE to ENABLE
    dcb.fOutxDsrFlow = FALSE;
    dcb.fDsrSensitivity = FALSE;
    dcb.fOutX = FALSE;  // Disable XON/XOFF output flow control
    dcb.fInX = FALSE;   // Disable XON/XOFF input flow control
    dcb.fErrorChar = FALSE;  // Don't replace error characters
    dcb.fNull = FALSE;  // Don't discard NULL bytes (critical for binary!)
    dcb.fAbortOnError = FALSE;  // Continue on errors

    if (!SetCommState(hComPort, &dcb)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "SetCommState failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    // Set timeouts for reliable XMODEM transfers
    // ReadIntervalTimeout: Maximum time between two consecutive bytes
    //   Setting to 0 means "no interval timeout" - only total timeout matters
    // ReadTotalTimeoutMultiplier: Timeout per byte to read (ms)
    // ReadTotalTimeoutConstant: Base timeout regardless of bytes requested (ms)
    timeouts.ReadIntervalTimeout = 0;          // No interval timeout (read all available data)
    timeouts.ReadTotalTimeoutMultiplier = 0;   // No per-byte timeout
    timeouts.ReadTotalTimeoutConstant = 100;   // 100ms base timeout
    timeouts.WriteTotalTimeoutMultiplier = 0;  // No per-byte write timeout
    timeouts.WriteTotalTimeoutConstant = 1000; // 1 second write timeout (for flash writing)

    if (!SetCommTimeouts(hComPort, &timeouts)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "SetCommTimeouts failed");
        CloseHandle(hComPort);
        return INVALID_HANDLE_VALUE;
    }

    // Purge any existing data
    PurgeComm(hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);

    return hComPort;
}

#if defined(USE_UART_POLLED)
/* ----------------------------------------------------------------
 * RX THREAD - POLLED MODE
 * Simple polling implementation for debugging
 * -------------------------------------------------------------- */
static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    COMSTAT comStat;
    DWORD dwErrors;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread started (polled mode, %dms interval)", pCtx->pollIntervalMs);

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
                               "[POLLED] UART RX buffer overrun! Data lost.");
            }
            if (dwErrors & CE_OVERRUN) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "[POLLED] UART hardware overrun! Data corrupted.");
            }
            if (dwErrors & CE_FRAME) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "[POLLED] UART framing error! Data corrupted.");
            }
            if (dwErrors & CE_BREAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "[POLLED] UART break condition detected.");
            }
            if (dwErrors & CE_RXPARITY) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "[POLLED] UART parity error! Data corrupted.");
            }
        }
        
        // Process data if available
        if (comStat.cbInQue > 0) {
            uCxAtClientHandleRx(pCtx->pClient);
        }
        
        // Sleep for polling interval
        Sleep(pCtx->pollIntervalMs);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated (polled mode)");
    return 0;
}

#elif defined(USE_UART_EVENT_DRIVEN)
/* ----------------------------------------------------------------
 * RX THREAD - EVENT-DRIVEN MODE
 * Uses WaitCommEvent for efficient event-driven RX processing
 * -------------------------------------------------------------- */
static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    DWORD dwEvtMask;
    DWORD dwWaitResult;
    HANDLE waitHandles[2];

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread started (event-driven)");

    // Setup events to wait for
    waitHandles[0] = pCtx->hStopEvent;    // Stop event (index 0)
    waitHandles[1] = pCtx->hCommEvent;    // Communication event (index 1)

    // Set the COM port event mask (notify on data received)
    if (!SetCommMask(pCtx->hComPort, EV_RXCHAR)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, "SetCommMask failed");
        return 1;
    }

    while (!pCtx->bTerminateRxTask) {
        // Start asynchronous wait for COM port events
        dwEvtMask = 0;
        memset(&pCtx->overlapped, 0, sizeof(OVERLAPPED));
        pCtx->overlapped.hEvent = pCtx->hCommEvent;

        if (!WaitCommEvent(pCtx->hComPort, &dwEvtMask, &pCtx->overlapped)) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_IO_PENDING) {
                // Operation is pending - wait for completion or stop event
                dwWaitResult = WaitForMultipleObjects(2, waitHandles, FALSE, INFINITE);
                
                if (dwWaitResult == WAIT_OBJECT_0) {
                    // Stop event signaled - cancel pending operation and exit
                    CancelIo(pCtx->hComPort);
                    break;
                } else if (dwWaitResult == WAIT_OBJECT_0 + 1) {
                    // Communication event signaled - data received
                    DWORD dwBytesTransferred;
                    if (GetOverlappedResult(pCtx->hComPort, &pCtx->overlapped, &dwBytesTransferred, FALSE)) {
                        // Process all available data (multiple URCs may have arrived)
                        COMSTAT comStat;
                        DWORD dwErrors;
                        do {
                            uCxAtClientHandleRx(pCtx->pClient);
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
                        } while (comStat.cbInQue > 0);
                    }
                }
            } else {
                // WaitCommEvent failed with error other than IO_PENDING
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                               "WaitCommEvent failed, error: %lu", dwError);
                break;
            }
        } else {
            // WaitCommEvent completed immediately (data already available)
            if (dwEvtMask & EV_RXCHAR) {
                // Process all available data (multiple URCs may have arrived)
                COMSTAT comStat;
                DWORD dwErrors;
                do {
                    uCxAtClientHandleRx(pCtx->pClient);
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
                } while (comStat.cbInQue > 0);
            }
        }
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated (event-driven)");
    return 0;
}

#elif defined(USE_UART_FTDI)
/* ----------------------------------------------------------------
 * RX THREAD - FTDI D2XX MODE
 * Uses FTDI D2XX API directly (future implementation)
 * See: D2XX Programmer's Guide
 * -------------------------------------------------------------- */
static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread started (FTDI D2XX mode)");
    
    // TODO: Implement FTDI D2XX RX thread
    // - Use FT_SetEventNotification() or FT_GetQueueStatus()
    // - Process data with FT_Read()
    // - Call uCxAtClientHandleRx() when data available
    
    while (!pCtx->bTerminateRxTask) {
        if (WaitForSingleObject(pCtx->hStopEvent, 100) == WAIT_OBJECT_0) {
            break;
        }
        // TODO: FTDI D2XX implementation
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread terminated (FTDI D2XX mode)");
    return 0;
}

#else
#error "No UART implementation selected"
#endif

/* ----------------------------------------------------------------
 * UART READ/WRITE FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    DWORD dwBytesWritten = 0;

    if (pCtx->hComPort == INVALID_HANDLE_VALUE) {
        return -1;
    }

#if defined(USE_UART_EVENT_DRIVEN)
    // Event-driven mode: Use overlapped I/O
    ResetEvent(pCtx->hWriteEvent);
    memset(&pCtx->ovWrite, 0, sizeof(OVERLAPPED));
    pCtx->ovWrite.hEvent = pCtx->hWriteEvent;

    if (!WriteFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesWritten, &pCtx->ovWrite)) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_IO_PENDING) {
            if (WaitForSingleObject(pCtx->hWriteEvent, 1000) == WAIT_OBJECT_0) {
                if (!GetOverlappedResult(pCtx->hComPort, &pCtx->ovWrite, &dwBytesWritten, FALSE)) {
                    dwError = GetLastError();
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "WriteFile failed, error: %lu", dwError);
                    return -1;
                }
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Write timeout");
                return -1;
            }
        } else {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "WriteFile failed, error: %lu", dwError);
            return -1;
        }
    }
#else
    // Polled/FTDI mode: Use synchronous I/O
    if (!WriteFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "WriteFile failed, error: %lu", dwError);
        return -1;
    }
#endif

    return (int32_t)dwBytesWritten;
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    DWORD dwBytesRead = 0;

    if (pCtx->hComPort == INVALID_HANDLE_VALUE) {
        return -1;
    }

#if defined(USE_UART_EVENT_DRIVEN)
    // Event-driven mode: Use overlapped I/O
    ResetEvent(pCtx->hReadEvent);
    memset(&pCtx->ovRead, 0, sizeof(OVERLAPPED));
    pCtx->ovRead.hEvent = pCtx->hReadEvent;

    if (!ReadFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesRead, &pCtx->ovRead)) {
        DWORD dwError = GetLastError();
        if (dwError == ERROR_IO_PENDING) {
            DWORD dwTimeout = (timeoutMs == 0) ? 0 : (DWORD)timeoutMs;
            DWORD dwWaitResult = WaitForSingleObject(pCtx->hReadEvent, dwTimeout);
            
            if (dwWaitResult == WAIT_OBJECT_0) {
                if (!GetOverlappedResult(pCtx->hComPort, &pCtx->ovRead, &dwBytesRead, FALSE)) {
                    dwError = GetLastError();
                    if (dwError != ERROR_TIMEOUT) {
                        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "ReadFile failed, error: %lu", dwError);
                    }
                    return -1;
                }
            } else if (dwWaitResult == WAIT_TIMEOUT) {
                CancelIo(pCtx->hComPort);
                return 0;
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Read wait failed");
                CancelIo(pCtx->hComPort);
                return -1;
            }
        } else {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "ReadFile failed, error: %lu", dwError);
            return -1;
        }
    }
#else
    // Polled/FTDI mode: Use synchronous I/O
    // Windows COM timeouts are already configured in openComPort()
    if (!ReadFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_TIMEOUT) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "ReadFile failed, error: %lu", dwError);
        }
        return -1;
    }
#endif

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

void uPortDelayMs(uint32_t delayMs)
{
    Sleep(delayMs);
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
    static char rxBuf[8192];  // Increased from 1024 to 8192 for XMODEM transfers
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
    
#if defined(USE_UART_EVENT_DRIVEN)
    context.hCommEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // Auto-reset event for WaitCommEvent
    context.hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);   // Manual-reset for read operations
    context.hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual-reset for write operations
#elif defined(USE_UART_POLLED)
    context.pollIntervalMs = 10;  // Poll every 10ms (adjustable for debugging)
#elif defined(USE_UART_FTDI)
    context.pFtdiHandle = NULL;   // Will be initialized when FTDI support is added
#endif

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

void uPortAtPauseRx(uCxAtClient_t *pClient)
{
    // Temporarily pause the RX thread to allow raw serial access (e.g., for XMODEM)
    // CRITICAL: The RX thread calls uCxAtClientHandleRx() which consumes bytes and
    // tries to parse them as AT responses. During XMODEM transfer, this causes ACK
    // bytes (0x06) to be consumed before the XMODEM code can read them, leading to
    // timeouts and retries. We must stop the RX thread during XMODEM.
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && pCtx->hRxThread != NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Pausing RX thread for raw serial access...");
        
        pCtx->bTerminateRxTask = true;
        SetEvent(pCtx->hStopEvent);

        // Wait for RX thread to terminate
        WaitForSingleObject(pCtx->hRxThread, 5000); // Wait up to 5 seconds
        CloseHandle(pCtx->hRxThread);
        pCtx->hRxThread = NULL;
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "RX thread paused - raw serial access enabled");
    }
}

void uPortAtResumeRx(uCxAtClient_t *pClient)
{
    // Resume the RX thread after raw serial access
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx != NULL && pCtx->hRxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Resuming RX thread...");
        
        pCtx->bTerminateRxTask = false;
        ResetEvent(pCtx->hStopEvent);

        // Recreate RX thread
        pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
        if (pCtx->hRxThread == NULL) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to resume RX thread");
        } else {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "RX thread resumed - AT command mode restored");
        }
    }
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

#if defined(USE_UART_EVENT_DRIVEN)
    if (pCtx->hCommEvent != NULL) {
        CloseHandle(pCtx->hCommEvent);
        pCtx->hCommEvent = NULL;
    }

    if (pCtx->hReadEvent != NULL) {
        CloseHandle(pCtx->hReadEvent);
        pCtx->hReadEvent = NULL;
    }

    if (pCtx->hWriteEvent != NULL) {
        CloseHandle(pCtx->hWriteEvent);
        pCtx->hWriteEvent = NULL;
    }
#elif defined(USE_UART_FTDI)
    // TODO: Close FTDI handle if open
    if (pCtx->pFtdiHandle != NULL) {
        // FT_Close(pCtx->pFtdiHandle);
        pCtx->pFtdiHandle = NULL;
    }
#endif

    gPConfig = NULL;
}

void uPortAtFlush(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx->hComPort != INVALID_HANDLE_VALUE) {
        // Flush both input and output buffers
        PurgeComm(pCtx->hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        
        // Also clear the AT client's internal RX buffer
        if (pClient->pConfig->pRxBuffer != NULL) {
            memset(pClient->pConfig->pRxBuffer, 0, pClient->pConfig->rxBufferLen);
        }
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Serial buffers flushed");
    }
}

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
    
    // Send to callback if registered
    if (gLogCallback != NULL) {
        gLogCallback(buffer, gLogUserData);
    }
    
    // Also print to stdout for console visibility
    printf("%s", buffer);
}

#endif /* _WIN32 */