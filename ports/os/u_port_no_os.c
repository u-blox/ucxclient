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
 * @brief No OS port implementation.
 *
 * Provides stub mutex functions and platform-based timing for systems without OS support.
 * NOTE: This is just an example implementation; you will need to adapt the timing functions
 *       for your specific platform.
 */

#include <stdint.h>
#include <stdbool.h>

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
#else
# include <time.h>
#endif

#include "u_port.h"

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static int32_t gBootTime = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t getTickTimeMs(void)
{
#ifdef _WIN32
    return (int32_t)GetTickCount();
#else
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    int64_t timeMs = (time.tv_sec * 1000) + (time.tv_nsec / (1000 * 1000));
    return (int32_t)(timeMs % (1000 * 60 * 60 * 24));
#endif
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uPortInit(void)
{
    if (gBootTime == 0) {
        gBootTime = getTickTimeMs();
    }
}

void uPortDeinit(void)
{
    // Nothing to do for no_os
}

int32_t uPortGetTickTimeMs(void)
{
    return getTickTimeMs() - gBootTime;
}

int32_t uPortSleepMs(int32_t ms)
{
    int32_t startTime = getTickTimeMs();
    while (getTickTimeMs() - startTime < ms) {
        // Busy wait
    }
    return 0;
}

int32_t uCxMutexTryLock(bool *pMutex, uint32_t timeoutMs)
{
    (void)timeoutMs;
    int32_t ret = *pMutex ? -1 : 0;
    *pMutex = true;
    return ret;
}

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    (void)pClient;
    // No background RX task in no-OS - user must call uCxAtClientHandleRx() manually
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;
    // Nothing to do
}
