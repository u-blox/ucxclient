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
 * @brief POSIX OS port implementation.
 *
 * Provides mutex, threading, and time functions using POSIX APIs.
 */

#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_cx_log.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    uCxAtClient_t *pClient;
    pthread_t rxThread;
    volatile bool terminateRxTask;
} uPortRxContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static int32_t gBootTime = 0;
static uPortRxContext_t gRxContext;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t getTickTimeMs(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC_RAW, &time);
    int64_t timeMs = (time.tv_sec * 1000) + (time.tv_nsec / (1000 * 1000));
    return (int32_t)(timeMs % (1000 * 60 * 60 * 24));
}

// Create a time structure by adding the specified number of
// milliseconds to the current clock time.
static void msToTimeSpec(uint32_t ms, struct timespec *pTime, bool fromNow)
{
    struct timespec now = {0};
    if (fromNow) {
        timespec_get(&now, TIME_UTC);
    }
    pTime->tv_sec = now.tv_sec + ms / 1000;
    pTime->tv_nsec = now.tv_nsec + (ms % 1000) * 1000000;
    if (pTime->tv_nsec >= 1000000000) {
        pTime->tv_nsec -= 1000000000;
        pTime->tv_sec++;
    }
}

static void *rxTask(void *pArg)
{
    uPortRxContext_t *pCtx = (uPortRxContext_t *)pArg;

    while (!pCtx->terminateRxTask) {
        U_CX_PORT_SLEEP_MS(10);
        uCxAtClientHandleRx(pCtx->pClient);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX task terminated");
    return NULL;
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
    // Nothing to do for POSIX
}

int32_t uPortGetTickTimeMs(void)
{
    return getTickTimeMs() - gBootTime;
}

int32_t uPortSleepMs(int32_t ms)
{
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    nanosleep(&ts, NULL);
    return 0;
}

int32_t uPortMutexTryLock(pthread_mutex_t *pMutex, uint32_t timeoutMs)
{
    int32_t ret;
    if (timeoutMs == 0) {
        ret = pthread_mutex_trylock(pMutex);
    } else {
        struct timespec time;
        msToTimeSpec(timeoutMs, &time, true);
        ret = pthread_mutex_timedlock(pMutex, &time);
    }

    return ret;
}

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    memset(&gRxContext, 0, sizeof(gRxContext));
    gRxContext.pClient = pClient;
    gRxContext.terminateRxTask = false;

    pthread_attr_t attr;
    struct sched_param param;
    pthread_attr_init(&attr);
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = 9;
    pthread_attr_setschedparam(&attr, &param);
    pthread_create(&gRxContext.rxThread, &attr, rxTask, &gRxContext);
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;
    gRxContext.terminateRxTask = true;
    pthread_join(gRxContext.rxThread, NULL);
}
