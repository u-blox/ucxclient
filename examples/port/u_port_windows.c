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
 * 1. USE_UART_POLLED (default, RECOMMENDED)
 *    - Simple polling loop checking for data
 *    - Most reliable URC handling
 *    - Predictable timing (10ms polling interval)
 *    - Easy to understand and debug
 *    - Best for production use
 * 
 * 2. USE_UART_EVENT_DRIVEN (experimental)
 *    - Uses WaitCommEvent() for efficient event-driven processing
 *    - Minimal CPU usage
 *    - May have URC timing issues
 *    - Use for testing/comparison
 * 
 * 3. USE_UART_FTDI (experimental)
 *    - Uses FTDI D2XX API directly (bypasses Windows COM driver)
 *    - Event-driven with FT_SetEventNotification and circular buffer
 *    - May have URC timing issues
 *    - Good for FTDI devices like NORA-W36 EVK (when working)
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
#include <stdlib.h>
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
#define USE_UART_POLLED        // Simple polled mode (RECOMMENDED - most reliable)
//#define USE_UART_EVENT_DRIVEN     // Event-driven with WaitCommEvent (experimental)
//#define USE_UART_FTDI          // FTDI D2XX API with event-driven circular buffer (experimental)

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
    HANDLE hDataAvailEvent; // Event signaled when data added to circular buffer
    OVERLAPPED overlapped;  // For WaitCommEvent
    OVERLAPPED ovRead;      // For read operations
    OVERLAPPED ovWrite;     // For write operations
    // Internal RX circular buffer to avoid character-by-character overlapped I/O
    uint8_t rxCircBuf[8192];
    volatile size_t rxCircHead;  // Write position (updated by RX thread)
    volatile size_t rxCircTail;  // Read position (updated by AT client)
    CRITICAL_SECTION rxCircLock;
#endif
    
#if defined(USE_UART_POLLED)
    // Polled mode specific fields
    int32_t pollIntervalMs; // Polling interval in milliseconds
#endif
    
#if defined(USE_UART_FTDI)
    // FTDI D2XX specific fields
    void *pFtdiHandle;      // FT_HANDLE (opaque FTDI handle)
    HANDLE hFtdiEvent;      // Event for FT_SetEventNotification()
    // Internal RX circular buffer (same as event-driven mode)
    uint8_t rxCircBuf[8192];
    volatile size_t rxCircHead;  // Write position (updated by RX thread)
    volatile size_t rxCircTail;  // Read position (updated by AT client)
    CRITICAL_SECTION rxCircLock;
    HANDLE hDataAvailEvent; // Event signaled when data added to circular buffer
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

    // Open COM port - use overlapped I/O only for event-driven mode
#if defined(USE_UART_EVENT_DRIVEN)
    hComPort = CreateFileA(
        fullPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,                    // No sharing
        NULL,                 // Default security
        OPEN_EXISTING,        // Open existing port
        FILE_FLAG_OVERLAPPED, // Use overlapped I/O for event notifications
        NULL                  // No template
    );
#else
    // Polled mode: Use synchronous I/O (no FILE_FLAG_OVERLAPPED)
    // Note: FTDI mode doesn't use COM port at all - it uses FTDI D2XX API directly
    hComPort = CreateFileA(
        fullPortName,
        GENERIC_READ | GENERIC_WRITE,
        0,                    // No sharing
        NULL,                 // Default security
        OPEN_EXISTING,        // Open existing port
        0,                    // Synchronous I/O
        NULL                  // No template
    );
#endif

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

    // Do an initial check for any data already in the buffer
    COMSTAT comStat;
    DWORD dwErrors;
    ClearCommError(pCtx->hComPort, &dwErrors, &comStat);
    if (comStat.cbInQue > 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, 
                       "RX thread: %lu bytes already in queue on startup", comStat.cbInQue);
    }

    while (!pCtx->bTerminateRxTask) {
        // Check for data BEFORE waiting (WaitCommEvent only fires on NEW data)
        COMSTAT comStatPre;
        DWORD dwErrorsPre;
        ClearCommError(pCtx->hComPort, &dwErrorsPre, &comStatPre);
        
        if (comStatPre.cbInQue > 0) {
            // Data already available - read it immediately using overlapped I/O
            uint8_t tempBuf[1024];
            DWORD toRead = (comStatPre.cbInQue > sizeof(tempBuf)) ? sizeof(tempBuf) : comStatPre.cbInQue;
            DWORD bytesRead = 0;
            
            // Must use overlapped I/O since port opened with FILE_FLAG_OVERLAPPED
            OVERLAPPED readOverlapped;
            memset(&readOverlapped, 0, sizeof(OVERLAPPED));
            readOverlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);  // Manual-reset for GetOverlappedResult
            
            BOOL readResult = ReadFile(pCtx->hComPort, tempBuf, toRead, &bytesRead, &readOverlapped);
            if (!readResult) {
                DWORD dwError = GetLastError();
                if (dwError == ERROR_IO_PENDING) {
                    // Wait for the read to complete
                    if (GetOverlappedResult(pCtx->hComPort, &readOverlapped, &bytesRead, TRUE)) {
                        readResult = TRUE;  // Read completed successfully
                    } else {
                        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance,
                                       "RX thread: GetOverlappedResult FAILED with error %lu", GetLastError());
                    }
                } else {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance,
                                   "RX thread: ReadFile FAILED with error %lu", dwError);
                }
            }
            
            CloseHandle(readOverlapped.hEvent);
            
            if (readResult && bytesRead > 0) {
                EnterCriticalSection(&pCtx->rxCircLock);
                for (DWORD i = 0; i < bytesRead; i++) {
                    size_t nextHead = (pCtx->rxCircHead + 1) % sizeof(pCtx->rxCircBuf);
                    if (nextHead != pCtx->rxCircTail) {
                        pCtx->rxCircBuf[pCtx->rxCircHead] = tempBuf[i];
                        pCtx->rxCircHead = nextHead;
                    } else {
                        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pCtx->pClient->instance,
                                       "RX circular buffer full! Dropping data.");
                        break;
                    }
                }
                LeaveCriticalSection(&pCtx->rxCircLock);
                SetEvent(pCtx->hDataAvailEvent);
            }
            // Continue loop to check for more data before waiting
            continue;
        }
        
        // No data available - set up async wait for new data
        // Prepare overlapped structure and reset the event before each wait
        dwEvtMask = 0;
        ResetEvent(pCtx->hCommEvent);
        memset(&pCtx->overlapped, 0, sizeof(OVERLAPPED));
        pCtx->overlapped.hEvent = pCtx->hCommEvent;

        // Start asynchronous wait for COM port events (RX notification)
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
                        // Drain all available data into circular buffer
                        COMSTAT comStat;
                        DWORD dwErrors;
                        int drainCount = 0;
                        bool firstRead = true;
                        
                        do {
                            if (firstRead) {
                                Sleep(2); // ensure first byte fully available
                                firstRead = false;
                            }
                            
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
                            
                            // Read available data into circular buffer
                            if (comStat.cbInQue > 0) {
                                uint8_t tempBuf[1024];
                                DWORD toRead = (comStat.cbInQue > sizeof(tempBuf)) ? sizeof(tempBuf) : comStat.cbInQue;
                                DWORD bytesRead = 0;
                                
                                if (ReadFile(pCtx->hComPort, tempBuf, toRead, &bytesRead, NULL) && bytesRead > 0) {
                                    EnterCriticalSection(&pCtx->rxCircLock);
                                    for (DWORD i = 0; i < bytesRead; i++) {
                                        size_t nextHead = (pCtx->rxCircHead + 1) % sizeof(pCtx->rxCircBuf);
                                        if (nextHead != pCtx->rxCircTail) {
                                            pCtx->rxCircBuf[pCtx->rxCircHead] = tempBuf[i];
                                            pCtx->rxCircHead = nextHead;
                                        } else {
                                            // Buffer full - data lost
                                            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pCtx->pClient->instance,
                                                           "RX circular buffer full! Dropping data.");
                                            break;
                                        }
                                    }
                                    LeaveCriticalSection(&pCtx->rxCircLock);
                                    // Signal that data is available
                                    SetEvent(pCtx->hDataAvailEvent);
                                }
                            }
                            
                            drainCount++;
                            // Small guard to avoid infinite loop if cbInQue not reliable
                            if (drainCount > 100) break;
                            
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
                // Drain all available data before rearming WaitCommEvent
                COMSTAT comStat;
                DWORD dwErrors;
                int drainCount = 0;
                bool firstRead = true;
                
                do {
                    if (firstRead) {
                        Sleep(2); // ensure first byte fully available
                        firstRead = false;
                    }
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
                    
                    drainCount++;
                    // Small guard to avoid infinite loop if cbInQue not reliable
                    if (drainCount > 100) break;
                    
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
 * Event-driven with circular buffer (same pattern as COM port mode)
 * See: D2XX Programmer's Guide - FT_SetEventNotification()
 * -------------------------------------------------------------- */

// Include minimal FTDI D2XX declarations (ftd2xx.lib is in examples/ftdi/ - linked via CMakeLists.txt)
#include "../ftdi/ftd2xx_minimal.h"

static DWORD WINAPI rxThread(LPVOID lpParam)
{
    uPortContext_t *pCtx = (uPortContext_t *)lpParam;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX thread started (FTDI D2XX mode)");
    
    // Set event notification for RXCHAR
    FT_STATUS ftStatus = FT_SetEventNotification((FT_HANDLE)pCtx->pFtdiHandle, 
                                                  FT_EVENT_RXCHAR, 
                                                  pCtx->hFtdiEvent);
    if (ftStatus != FT_OK) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                       "FT_SetEventNotification failed: %d", ftStatus);
        return 1;
    }
    
    while (!pCtx->bTerminateRxTask) {
        // Wait for either data event or stop event (with 50ms timeout for polling fallback)
        HANDLE waitHandles[2] = { pCtx->hFtdiEvent, pCtx->hStopEvent };
        DWORD waitResult = WaitForMultipleObjects(2, waitHandles, FALSE, 50);
        
        // Check queue status regardless of wait result to work around FTDI event notification issues
        DWORD dwQueueBytes = 0;
        ftStatus = FT_GetQueueStatus((FT_HANDLE)pCtx->pFtdiHandle, &dwQueueBytes);
        
        // --- PATCH BEGIN: Force FTDI USB poll for pending URCs ---
        // FTDI latency bug workaround:
        // Even if FT_EVENT_RXCHAR or FT_GetQueueStatus() doesn't trigger,
        // performing a zero-length or 1-byte FT_Read() forces a USB IN transfer,
        // flushing any data buffered in the FTDI chip immediately.
        {
            DWORD dummyRead = 0;
            uint8_t dummyBuf[1];
            FT_STATUS ftPoll = FT_Read((FT_HANDLE)pCtx->pFtdiHandle, dummyBuf, 0, &dummyRead);
            if (ftPoll != FT_OK) {
                // Some drivers ignore 0-length reads; fall back to 1-byte poll
                FT_Read((FT_HANDLE)pCtx->pFtdiHandle, dummyBuf, 1, &dummyRead);
            }
        }
        // --- PATCH END ---
        
        if (waitResult == WAIT_OBJECT_0 || (waitResult == WAIT_TIMEOUT && dwQueueBytes > 0)) {
            // Data event signaled OR timeout with data available - drain ALL available data
            // Keep reading until queue is empty to handle complete multi-line responses
            int loopCount = 0;
            
            do {
                // Re-check queue status in loop
                ftStatus = FT_GetQueueStatus((FT_HANDLE)pCtx->pFtdiHandle, &dwQueueBytes);
                
                if (ftStatus == FT_OK && dwQueueBytes > 0) {
                    // Read available data (bulk read up to 1024 bytes)
                    uint8_t tempBuf[1024];
                    DWORD toRead = (dwQueueBytes > sizeof(tempBuf)) ? sizeof(tempBuf) : dwQueueBytes;
                    DWORD bytesRead = 0;
                    
                    ftStatus = FT_Read((FT_HANDLE)pCtx->pFtdiHandle, tempBuf, toRead, &bytesRead);
                    
                    if (ftStatus == FT_OK && bytesRead > 0) {
                        // Copy to circular buffer
                        EnterCriticalSection(&pCtx->rxCircLock);
                        
                        for (DWORD i = 0; i < bytesRead; i++) {
                            size_t nextHead = (pCtx->rxCircHead + 1) % sizeof(pCtx->rxCircBuf);
                            if (nextHead != pCtx->rxCircTail) {
                                pCtx->rxCircBuf[pCtx->rxCircHead] = tempBuf[i];
                                pCtx->rxCircHead = nextHead;
                            } else {
                                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pCtx->pClient->instance, 
                                               "RX circular buffer full!");
                                break;
                            }
                        }
                        
                        LeaveCriticalSection(&pCtx->rxCircLock);
                        SetEvent(pCtx->hDataAvailEvent);
                    }
                    
                    // Only sleep if we read less than buffer size (more data likely coming)
                    if (bytesRead >= sizeof(tempBuf)) {
                        // Full buffer read - immediately check for more data
                        Sleep(0);  // Yield CPU but check again immediately
                    } else if (bytesRead > 0) {
                        // Partial read - give device a moment to send more
                        Sleep(2);  // 2ms delay for partial reads
                    }
                }
                
                loopCount++;
                if (loopCount > 100) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pCtx->pClient->instance, 
                                   "RX drain loop exceeded 100 iterations");
                    break;
                }
                
            } while (dwQueueBytes > 0);
            
        } else if (waitResult == WAIT_OBJECT_0 + 1) {
            // Stop event signaled
            break;
        } else if (waitResult == WAIT_TIMEOUT) {
            // Timeout with no data - just loop again (this is the polling mechanism)
            continue;
        } else {
            // WAIT_FAILED or unexpected result
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pCtx->pClient->instance, 
                           "WaitForMultipleObjects failed: %lu", waitResult);
            break;
        }
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

#if defined(USE_UART_FTDI)
    // FTDI mode: Check FTDI handle
    if (pCtx->pFtdiHandle == NULL) {
        return -1;
    }
#else
    // COM port mode: Check COM handle
    if (pCtx->hComPort == INVALID_HANDLE_VALUE) {
        return -1;
    }
#endif

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
#elif defined(USE_UART_FTDI)
    // FTDI mode: Use FTDI D2XX write function
    FT_STATUS ftStatus = FT_Write((FT_HANDLE)pCtx->pFtdiHandle, (LPVOID)pData, (DWORD)length, &dwBytesWritten);
    if (ftStatus != FT_OK) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "FT_Write failed, status: %d", ftStatus);
        return -1;
    }
#else
    // Polled mode: Use synchronous I/O
    if (!WriteFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesWritten, NULL)) {
        DWORD dwError = GetLastError();
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "WriteFile failed, error: %lu", dwError);
        if (dwError == ERROR_ACCESS_DENIED) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                "COM port access denied - port may be in use by another application or device disconnected");
        }
        return -1;
    }
#endif

    return (int32_t)dwBytesWritten;
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    DWORD dwBytesRead = 0;
    
    // Suppress unused parameter warning (timeoutMs is used in FTDI mode and timeout paths)
    (void)timeoutMs;

#if defined(USE_UART_FTDI)
    // FTDI mode: Check FTDI handle
    if (pCtx->pFtdiHandle == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "uartRead: FTDI handle is NULL");
        return -1;
    }
#else
    // COM port mode: Check COM handle
    if (pCtx->hComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "uartRead: COM handle is INVALID");
        return -1;
    }
#endif

#if defined(USE_UART_FTDI)
    // FTDI mode: Check if RX thread is paused (for XMODEM transfer)
    if (pCtx->bTerminateRxTask) {
        // RX thread is paused - read directly from FTDI
        FT_STATUS ftStatus;
        DWORD dwBytesToRead = (DWORD)length;
        int32_t startTime = (int32_t)GetTickCount();
        
        // Poll for data with timeout
        while (dwBytesRead < dwBytesToRead) {
            DWORD dwAvailable = 0;
            ftStatus = FT_GetQueueStatus((FT_HANDLE)pCtx->pFtdiHandle, &dwAvailable);
            if (ftStatus != FT_OK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "FT_GetQueueStatus failed, status: %d", ftStatus);
                return -1;
            }
            
            if (dwAvailable > 0) {
                // Read available data
                DWORD dwToRead = dwAvailable;
                if (dwToRead > (dwBytesToRead - dwBytesRead)) {
                    dwToRead = dwBytesToRead - dwBytesRead;
                }
                
                DWORD dwRead = 0;
                ftStatus = FT_Read((FT_HANDLE)pCtx->pFtdiHandle, (uint8_t *)pData + dwBytesRead, dwToRead, &dwRead);
                if (ftStatus != FT_OK) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "FT_Read failed, status: %d", ftStatus);
                    return -1;
                }
                dwBytesRead += dwRead;
                
                // If we got some data, return immediately (XMODEM needs byte-by-byte)
                if (dwBytesRead > 0) {
                    break;
                }
            }
            
            // Check timeout
            if (timeoutMs > 0) {
                int32_t elapsed = (int32_t)GetTickCount() - startTime;
                if (elapsed >= timeoutMs) {
                    break;  // Timeout expired
                }
                // Small delay to avoid busy-waiting
                Sleep(1);
            } else {
                // No timeout, return immediately
                break;
            }
        }
    } else {
        // RX thread is running - read from circular buffer
        uint8_t *pBuf = (uint8_t *)pData;
        int32_t startTime = (int32_t)GetTickCount();
        
        while (dwBytesRead < length) {
            EnterCriticalSection(&pCtx->rxCircLock);
            
            // Check if data available in circular buffer
            while (pCtx->rxCircTail != pCtx->rxCircHead && dwBytesRead < length) {
                pBuf[dwBytesRead++] = pCtx->rxCircBuf[pCtx->rxCircTail];
                pCtx->rxCircTail = (pCtx->rxCircTail + 1) % sizeof(pCtx->rxCircBuf);
            }
            
            LeaveCriticalSection(&pCtx->rxCircLock);
            
            // If we got some data, return immediately
            if (dwBytesRead > 0) {
                break;
            }
            
            // No data available - wait for data or timeout
            if (timeoutMs > 0) {
                int32_t elapsed = (int32_t)GetTickCount() - startTime;
                int32_t remaining = timeoutMs - elapsed;
                if (remaining <= 0) {
                    break;  // Timeout expired
                }
                // Wait for data to arrive (or timeout)
                DWORD waitResult = WaitForSingleObject(pCtx->hDataAvailEvent, (DWORD)remaining);
                if (waitResult == WAIT_TIMEOUT) {
                    break;
                }
                // Loop again to read the data that arrived
            } else {
                // No timeout, return immediately
                break;
            }
        }
    }
#elif defined(USE_UART_EVENT_DRIVEN)
    // Event-driven mode: Read from circular buffer filled by RX thread
    uint8_t *pBuf = (uint8_t *)pData;
    int32_t startTime = (int32_t)GetTickCount();
    
    while (dwBytesRead < length) {
        EnterCriticalSection(&pCtx->rxCircLock);
        
        // Check if data available in circular buffer
        while (pCtx->rxCircTail != pCtx->rxCircHead && dwBytesRead < length) {
            pBuf[dwBytesRead++] = pCtx->rxCircBuf[pCtx->rxCircTail];
            pCtx->rxCircTail = (pCtx->rxCircTail + 1) % sizeof(pCtx->rxCircBuf);
        }
        
        LeaveCriticalSection(&pCtx->rxCircLock);
        
        // If we got some data, return immediately
        if (dwBytesRead > 0) {
            break;
        }
        
        // No data available - wait for data or timeout
        if (timeoutMs > 0) {
            int32_t elapsed = (int32_t)GetTickCount() - startTime;
            int32_t remaining = timeoutMs - elapsed;
            if (remaining <= 0) {
                break;  // Timeout expired
            }
            // Wait for data to arrive (or timeout)
            DWORD waitResult = WaitForSingleObject(pCtx->hDataAvailEvent, (DWORD)remaining);
            if (waitResult == WAIT_TIMEOUT) {
                break;
            }
            // Loop again to read the data that arrived
        } else {
            // No timeout, return immediately
            break;
        }
    }
#else
    // Polled mode: Use synchronous I/O
    // Windows COM timeouts are already configured in openComPort()
    if (!ReadFile(pCtx->hComPort, pData, (DWORD)length, &dwBytesRead, NULL)) {
        DWORD dwError = GetLastError();
        if (dwError != ERROR_TIMEOUT) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "ReadFile failed, error: %lu", dwError);
            if (dwError == ERROR_ACCESS_DENIED) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                    "COM port access denied - port may be in use by another application or device disconnected");
            }
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
    static char urcBuf[4096];  // Increased from 1024 to 4096 for multiple URCs (e.g., WiFi scan results)
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
    context.hDataAvailEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // Auto-reset for data availability
    // Initialize circular buffer
    memset(context.rxCircBuf, 0, sizeof(context.rxCircBuf));
    context.rxCircHead = 0;
    context.rxCircTail = 0;
    InitializeCriticalSection(&context.rxCircLock);
#elif defined(USE_UART_POLLED)
    context.pollIntervalMs = 10;  // Poll every 10ms (adjustable for debugging)
#elif defined(USE_UART_FTDI)
    context.pFtdiHandle = NULL;   // Will be initialized in uPortAtOpen
    context.hFtdiEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // Auto-reset event for FTDI notifications
    context.hDataAvailEvent = CreateEvent(NULL, FALSE, FALSE, NULL);  // Auto-reset for data availability
    // Initialize circular buffer (same as event-driven mode)
    memset(context.rxCircBuf, 0, sizeof(context.rxCircBuf));
    context.rxCircHead = 0;
    context.rxCircTail = 0;
    InitializeCriticalSection(&context.rxCircLock);
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

#if defined(USE_UART_FTDI)
    // FTDI D2XX mode: Open FTDI device by description or index
    assert(pCtx->pFtdiHandle == NULL);
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Opening FTDI device: %s at %d baud",
                    pDevName, baudRate);
    
    // FTDI D2XX API device open and configuration
    FT_HANDLE ftHandle = NULL;
    FT_STATUS ftStatus;
    int deviceIndex = -1;
    
    // If user passed "COMxx", enumerate FTDI devices to find matching one
    if (strncmp(pDevName, "COM", 3) == 0) {
        // Extract COM port number from "COMxx" string
        int comPortNum = atoi(pDevName + 3);  // Skip "COM" prefix
        
        uPortLogPrintf("FTDI mode: Looking for FTDI device on %s (port %d)...\n", pDevName, comPortNum);
        
        // Get number of FTDI devices
        DWORD numDevs = 0;
        ftStatus = FT_CreateDeviceInfoList(&numDevs);
        if (ftStatus != FT_OK || numDevs == 0) {
            uPortLogPrintf("No FTDI devices found, status: %d\n", ftStatus);
            return false;
        }
        
        uPortLogPrintf("Found %lu FTDI device(s), checking each...\n", numDevs);
        
        // Allocate device info list
        FT_DEVICE_LIST_INFO_NODE *devInfo = (FT_DEVICE_LIST_INFO_NODE *)malloc(sizeof(FT_DEVICE_LIST_INFO_NODE) * numDevs);
        if (devInfo == NULL) {
            uPortLogPrintf("Failed to allocate device info list\n");
            return false;
        }
        
        // Get device info list
        ftStatus = FT_GetDeviceInfoList(devInfo, &numDevs);
        if (ftStatus != FT_OK) {
            uPortLogPrintf("FT_GetDeviceInfoList failed, status: %d\n", ftStatus);
            free(devInfo);
            return false;
        }
        
        // Iterate through devices to find the one on the specified COM port
        for (DWORD i = 0; i < numDevs; i++) {
            // Try to open this device temporarily to check its COM port
            FT_HANDLE tempHandle;
            ftStatus = FT_Open(i, &tempHandle);
            if (ftStatus == FT_OK) {
                // Get the COM port number for this device
                LONG comPort = -1;
                ftStatus = FT_GetComPortNumber(tempHandle, &comPort);
                
                uPortLogPrintf("  Device %lu: %s (SN: %s) - ", i, 
                              devInfo[i].Description, devInfo[i].SerialNumber);
                
                if (ftStatus == FT_OK && comPort > 0) {
                    uPortLogPrintf("COM%ld\n", comPort);
                    
                    // Check if this matches our target COM port
                    if (comPort == comPortNum) {
                        uPortLogPrintf("  -> Match! This is the device we want.\n");
                        deviceIndex = (int)i;
                        ftHandle = tempHandle;  // Keep this handle open
                        break;
                    }
                } else {
                    uPortLogPrintf("(no COM port assigned)\n");
                }
                
                // Close if not a match
                if (ftHandle == NULL) {
                    FT_Close(tempHandle);
                }
            }
        }
        
        free(devInfo);
        
        if (deviceIndex < 0) {
            uPortLogPrintf("No FTDI device found on %s\n", pDevName);
            return false;
        }
        
    } else {
        // User passed device description (e.g., "NORA-W36") or index (e.g., "0")
        // Try to open by description first
        ftStatus = FT_OpenEx((PVOID)pDevName, FT_OPEN_BY_DESCRIPTION, &ftHandle);
        if (ftStatus != FT_OK) {
            // Try as device index
            deviceIndex = atoi(pDevName);
            ftStatus = FT_Open(deviceIndex, &ftHandle);
            if (ftStatus != FT_OK) {
                uPortLogPrintf("Failed to open FTDI device '%s', status: %d\n", pDevName, ftStatus);
                return false;
            }
        }
    }
    
    uPortLogPrintf("FTDI device opened successfully\n");
    
    // Configure FTDI device
    uPortLogPrintf("Configuring FTDI device:\n");
    uPortLogPrintf("  Baud rate:      %d\n", baudRate);
    uPortLogPrintf("  Data bits:      8\n");
    uPortLogPrintf("  Stop bits:      1\n");
    uPortLogPrintf("  Parity:         None\n");
    uPortLogPrintf("  Flow control:   %s\n", useFlowControl ? "RTS/CTS" : "None");
    uPortLogPrintf("  USB buffer:     128 bytes IN/OUT\n");
    uPortLogPrintf("  Latency timer:  1 ms\n");
    uPortLogPrintf("  Read timeout:   100 ms\n");
    uPortLogPrintf("  Write timeout:  1000 ms\n");
    
    FT_SetBaudRate(ftHandle, baudRate);
    FT_SetDataCharacteristics(ftHandle, FT_BITS_8, FT_STOP_BITS_1, FT_PARITY_NONE);
    if (useFlowControl) {
        FT_SetFlowControl(ftHandle, FT_FLOW_RTS_CTS, 0, 0);
    } else {
        FT_SetFlowControl(ftHandle, FT_FLOW_NONE, 0, 0);
    }
    
    // Set timeouts (similar to COM port configuration)
    FT_SetTimeouts(ftHandle, 100, 1000);  // Read timeout 100ms, write timeout 1000ms
    
    // Optimize USB transfer parameters for IMMEDIATE URC delivery
    // Smaller buffer = more frequent USB transfers = lower latency for URCs
    FT_SetUSBParameters(ftHandle, 128, 128);  // 128 bytes IN/OUT (sweet spot for low latency)
    FT_SetLatencyTimer(ftHandle, 2);          // 1ms latency (was 2ms, even more aggressive)
    
    // Purge any stale data
    FT_Purge(ftHandle, FT_PURGE_RX | FT_PURGE_TX);
    
    pCtx->pFtdiHandle = ftHandle;
    
    // Recreate events if they were closed (happens when port is closed and reopened)
    if (pCtx->hFtdiEvent == NULL) {
        pCtx->hFtdiEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    if (pCtx->hStopEvent == NULL) {
        pCtx->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    if (pCtx->hDataAvailEvent == NULL) {
        pCtx->hDataAvailEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    
    // Reinitialize critical section if needed (it gets deleted when port is closed)
    // We need to check if it's been deleted by trying to enter it - but that's unsafe.
    // Instead, we'll just always reinitialize it here since it's safe to do so.
    // Note: DeleteCriticalSection is called in uPortAtClose, so we must reinitialize here.
    InitializeCriticalSection(&pCtx->rxCircLock);
    
#else
    // COM port mode (event-driven or polled)
    assert(pCtx->hComPort == INVALID_HANDLE_VALUE);
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Opening %s at %d with %s flow control",
                    pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    gHComPort = openComPort(pDevName, baudRate, useFlowControl);
    if (gHComPort == INVALID_HANDLE_VALUE) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open COM port");
        return false;
    }
    
    pCtx->hComPort = gHComPort;
    
    // Recreate events if they were closed (happens when port is closed and reopened)
    if (pCtx->hStopEvent == NULL) {
        pCtx->hStopEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
#if defined(USE_UART_EVENT_DRIVEN)
    if (pCtx->hCommEvent == NULL) {
        pCtx->hCommEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    if (pCtx->hReadEvent == NULL) {
        pCtx->hReadEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    if (pCtx->hWriteEvent == NULL) {
        pCtx->hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    }
    if (pCtx->hDataAvailEvent == NULL) {
        pCtx->hDataAvailEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    }
    
    // Reinitialize critical section (it gets deleted when port is closed)
    InitializeCriticalSection(&pCtx->rxCircLock);
#endif
#endif

    pCtx->bTerminateRxTask = false;

    // Reset stop event
    ResetEvent(pCtx->hStopEvent);

    // Create RX thread
    pCtx->hRxThread = CreateThread(NULL, 0, rxThread, pCtx, 0, &pCtx->dwThreadId);
    if (pCtx->hRxThread == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to create RX thread");
#if defined(USE_UART_FTDI)
        FT_Close((FT_HANDLE)pCtx->pFtdiHandle);
        pCtx->pFtdiHandle = NULL;
#else
        CloseHandle(gHComPort);
        gHComPort = INVALID_HANDLE_VALUE;
        pCtx->hComPort = INVALID_HANDLE_VALUE;
#endif
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
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Entry (pCtx=%p, hRxThread=%p)", pCtx, pCtx ? pCtx->hRxThread : NULL);
    
    if (pCtx != NULL && pCtx->hRxThread != NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Pausing RX thread for raw serial access...");
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Setting bTerminateRxTask=true...");
        pCtx->bTerminateRxTask = true;
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Calling SetEvent(hStopEvent=%p)...", pCtx->hStopEvent);
        SetEvent(pCtx->hStopEvent);

        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Waiting for RX thread to terminate...");
        // Wait for RX thread to terminate
        DWORD waitResult = WaitForSingleObject(pCtx->hRxThread, 5000); // Wait up to 5 seconds
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: WaitForSingleObject returned %lu", waitResult);
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Closing thread handle...");
        CloseHandle(pCtx->hRxThread);
        pCtx->hRxThread = NULL;
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "RX thread paused - raw serial access enabled");
    } else {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtPauseRx: Nothing to pause (pCtx=%p, hRxThread=%p)", 
                        pCtx, pCtx ? pCtx->hRxThread : NULL);
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
    
    if (pCtx->hDataAvailEvent != NULL) {
        CloseHandle(pCtx->hDataAvailEvent);
        pCtx->hDataAvailEvent = NULL;
    }
    
    DeleteCriticalSection(&pCtx->rxCircLock);
#elif defined(USE_UART_FTDI)
    // Close FTDI handle if open
    if (pCtx->pFtdiHandle != NULL) {
        FT_Close((FT_HANDLE)pCtx->pFtdiHandle);
        pCtx->pFtdiHandle = NULL;
    }
    
    // Close FTDI-specific events
    if (pCtx->hFtdiEvent != NULL) {
        CloseHandle(pCtx->hFtdiEvent);
        pCtx->hFtdiEvent = NULL;
    }
    
    if (pCtx->hDataAvailEvent != NULL) {
        CloseHandle(pCtx->hDataAvailEvent);
        pCtx->hDataAvailEvent = NULL;
    }
    
    DeleteCriticalSection(&pCtx->rxCircLock);
#endif

    gPConfig = NULL;
}

void uPortAtFlush(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: Entry (pCtx=%p)", pCtx);
    
#if defined(USE_UART_FTDI)
    // FTDI mode: use FT_Purge
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: FTDI mode, pFtdiHandle=%p", pCtx->pFtdiHandle);
    if (pCtx->pFtdiHandle != NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: Calling FT_Purge...");
        FT_STATUS ftStatus = FT_Purge((FT_HANDLE)pCtx->pFtdiHandle, FT_PURGE_RX | FT_PURGE_TX);
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: FT_Purge returned status=%d", ftStatus);
        
        // Also clear the AT client's internal RX buffer
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: Clearing AT client RX buffer...");
        if (pClient->pConfig->pRxBuffer != NULL) {
            memset(pClient->pConfig->pRxBuffer, 0, pClient->pConfig->rxBufferLen);
        }
        
        // Clear circular buffer
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uPortAtFlush: Clearing circular buffer...");
        EnterCriticalSection(&pCtx->rxCircLock);
        pCtx->rxCircHead = 0;
        pCtx->rxCircTail = 0;
        LeaveCriticalSection(&pCtx->rxCircLock);
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Serial buffers flushed (FTDI)");
    }
#else
    // COM port mode: use PurgeComm
    if (pCtx->hComPort != INVALID_HANDLE_VALUE) {
        // Flush both input and output buffers
        PurgeComm(pCtx->hComPort, PURGE_RXCLEAR | PURGE_TXCLEAR);
        
        // Also clear the AT client's internal RX buffer
        if (pClient->pConfig->pRxBuffer != NULL) {
            memset(pClient->pConfig->pRxBuffer, 0, pClient->pConfig->rxBufferLen);
        }
        
#if defined(USE_UART_EVENT_DRIVEN)
        // Clear circular buffer (used by event-driven mode)
        EnterCriticalSection(&pCtx->rxCircLock);
        pCtx->rxCircHead = 0;
        pCtx->rxCircTail = 0;
        LeaveCriticalSection(&pCtx->rxCircLock);
#endif
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Serial buffers flushed");
    }
#endif
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