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
 * @brief Windows port example
 *
 * This port shows how to run ucxclient on Windows using Win32 API.
 * It provides COM port access, Windows threading, and synchronization.
 */

#ifndef U_PORT_WINDOWS_H
#define U_PORT_WINDOWS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_MUTEX_HANDLE                     HANDLE
#define U_CX_MUTEX_CREATE(mutex)              (mutex = CreateMutex(NULL, FALSE, NULL))
#define U_CX_MUTEX_DELETE(mutex)              do { if (mutex != NULL) { CloseHandle(mutex); mutex = NULL; } } while(0)
#define U_CX_MUTEX_LOCK(mutex)                WaitForSingleObject(mutex, INFINITE)
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(mutex, timeoutMs)
#define U_CX_MUTEX_UNLOCK(mutex)              ReleaseMutex(mutex)

/* Default COM port settings */
#ifndef U_EXAMPLE_UART
# define U_EXAMPLE_UART "COM3"
#endif

#ifndef U_EXAMPLE_SSID
# define U_EXAMPLE_SSID "ubx"
#endif

#ifndef U_EXAMPLE_WPA_PSK
# define U_EXAMPLE_WPA_PSK ""
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    HANDLE hComPort;
    HANDLE hRxThread;
    HANDLE hStopEvent;
    DWORD dwThreadId;
    volatile bool bTerminateRxTask;
} uPortWindowsContext_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Windows implementation of U_CX_MUTEX_TRY_LOCK()
  *
  * @param mutex      The mutex handle to try to lock
  * @param timeoutMs  Timeout in milliseconds
  * @return           0 on success, negative value on timeout
  */
int32_t uPortMutexTryLock(HANDLE mutex, uint32_t timeoutMs);

/**
  * @brief Get current tick time in milliseconds
  *
  * @return Current time in milliseconds (32-bit, wraps around)
  */
int32_t uPortGetTickTimeMs(void);

/**
  * @brief Enumerate available COM ports on the system
  *
  * @param pPortList    Array to store port names (e.g., "COM1", "COM2")
  * @param maxPorts     Maximum number of ports to enumerate
  * @return             Number of ports found, or negative on error
  */
int32_t uPortEnumerateComPorts(char pPortList[][16], int32_t maxPorts);

/**
  * @brief Check if a COM port is available
  *
  * @param pPortName    Port name (e.g., "COM3")
  * @return             true if port exists and is available, false otherwise
  */
bool uPortIsComPortAvailable(const char *pPortName);

#endif /* U_PORT_WINDOWS_H */