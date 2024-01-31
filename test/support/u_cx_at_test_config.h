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

#include <stdbool.h>
#include "unity.h"

/** @file
 * @brief Configuration file used during test
 */
#ifndef U_CX_AT_TEST_CONFIG_H
#define U_CX_AT_TEST_CONFIG_H

#define U_CX_AT_PORT_ASSERT(COND) TEST_ASSERT(COND)

#define U_CX_PORT_PRINTF(...)

#define U_CX_MUTEX_HANDLE                       bool
#define U_CX_MUTEX_CREATE(mutex)                mutex = false
#define U_CX_MUTEX_DELETE(mutex)                ((void)mutex)

#define U_CX_MUTEX_UNLOCK(mutex)    \
    do {                            \
        TEST_ASSERT_TRUE_MESSAGE(mutex, "Mutex unlock failed in file '" __FILE__ "'"); \
        mutex = false;               \
    } while(0);

#define U_CX_MUTEX_LOCK(mutex)      \
    do {                            \
        TEST_ASSERT_FALSE_MESSAGE(mutex, "Mutex lock failed in file '" __FILE__ "'"); \
        mutex = true;               \
    } while(0);

#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs)   uCxMutexTryLock(&mutex, timeoutMs)

inline int32_t uCxMutexTryLock(bool *pMutex, int32_t timeoutMs)
{
    (void)timeoutMs;
    int32_t ret = *pMutex ? -1 : 0;
    *pMutex = true;
    return ret;
}

#endif // U_CX_AT_CONFIG_H