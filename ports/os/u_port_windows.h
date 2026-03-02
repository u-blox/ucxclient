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
 * @brief Windows port example
 *
 * This port shows how to run ucxclient on Windows using Win32 API.
 * It provides COM port access, Windows threading, and synchronization.
 */

#ifndef U_PORT_WINDOWS_H
#define U_PORT_WINDOWS_H

#include <stdint.h>
#include <stdbool.h>

#ifndef WIN32_LEAN_AND_MEAN
# define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <sys/types.h>

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_PORT_WINDOWS
# define U_PORT_WINDOWS
#endif

#define U_CX_MUTEX_HANDLE                     CRITICAL_SECTION
#define U_CX_MUTEX_CREATE(mutex)              InitializeCriticalSectionAndSpinCount(&(mutex), 4000)
#define U_CX_MUTEX_DELETE(mutex)              DeleteCriticalSection(&(mutex))
#define U_CX_MUTEX_LOCK(mutex)                EnterCriticalSection(&(mutex))
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(&(mutex), timeoutMs)
#define U_CX_MUTEX_UNLOCK(mutex)              LeaveCriticalSection(&(mutex))

#define U_CX_PORT_SLEEP_MS(ms)                Sleep(ms)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Get milliseconds since boot
  *
  * @return Time in milliseconds
  */
int32_t uPortGetTickTimeMs(void);

/**
  * @brief Windows implementation of U_CX_MUTEX_TRY_LOCK()
  *
  * @param pCs        Pointer to the CRITICAL_SECTION to try to lock
  * @param timeoutMs  Timeout in milliseconds (0 = non-blocking try)
  * @return           0 on success, negative value on timeout/failure
  */
int32_t uPortMutexTryLock(CRITICAL_SECTION *pCs, int32_t timeoutMs);

#endif /* U_PORT_WINDOWS_H */