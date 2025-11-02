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
 * @brief Posix port example
 */

#ifndef U_PORT_POSIX_H
#define U_PORT_POSIX_H

#include <stdint.h>
#include <stdbool.h>
#include <pthread.h>

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_MUTEX_HANDLE                     pthread_mutex_t
#define U_CX_MUTEX_CREATE(mutex)              pthread_mutex_init(&mutex, NULL)
#define U_CX_MUTEX_DELETE(mutex)              pthread_mutex_destroy(&mutex)
#define U_CX_MUTEX_LOCK(mutex)                pthread_mutex_lock(&mutex)
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(&mutex, timeoutMs)
#define U_CX_MUTEX_UNLOCK(mutex)              pthread_mutex_unlock(&mutex)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Posix implementation of U_CX_MUTEX_TRY_LOCK()
  *
  * @return 0 on success, negative value on timeout
  */
int32_t uPortMutexTryLock(pthread_mutex_t *pMutex, uint32_t timeoutMs);

/**
  * @brief Delay execution for specified milliseconds
  *
  * @param delayMs  Number of milliseconds to delay
  */
void uPortDelayMs(uint32_t delayMs);

#endif