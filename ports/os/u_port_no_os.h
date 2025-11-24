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
 * @brief "No OS" port layer example
 *
 * This port shows you how you can run ucxclient on a system without
 * mutex- and thread support.
 *
 * This example port uses Linux implementation for U_CX_PORT_GET_TIME_MS()
 * and UART driver. Normally you will need to implement this part
 * for your specific target.
 */

#ifndef U_PORT_NO_OS_H
#define U_PORT_NO_OS_H

#include <stdint.h>
#include <stdbool.h>

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_PORT_NO_OS
# define U_PORT_NO_OS
#endif

#define U_CX_MUTEX_HANDLE                       bool
#define U_CX_MUTEX_CREATE(mutex)                mutex = false
#define U_CX_MUTEX_DELETE(mutex)                ((void)mutex)

#define U_CX_MUTEX_UNLOCK(mutex)     \
    do {                             \
        U_CX_AT_PORT_ASSERT(mutex);  \
        mutex = false;               \
    } while(0);

#define U_CX_MUTEX_LOCK(mutex)       \
    do {                             \
        U_CX_AT_PORT_ASSERT(!mutex); \
        mutex = true;                \
    } while(0);

#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs)   uCxMutexTryLock(&mutex, timeoutMs)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uCxMutexTryLock(bool *pMutex, uint32_t timeoutMs);

#endif