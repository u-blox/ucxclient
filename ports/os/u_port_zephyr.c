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
 * @brief Zephyr OS port implementation.
 *
 * Provides time functions using Zephyr kernel APIs.
 * Mutex functions are handled via macros in u_port_zephyr.h.
 */

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>

#include "u_port.h"
#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    struct k_work rxWork;
    uCxAtClient_t *pClient;
} uPortRxContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortRxContext_t gRxContext;
bool gDisableRxWorker = false;  // Used during test

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void rxTask(struct k_work *pItem)
{
    uPortRxContext_t *pCtx = CONTAINER_OF(pItem, uPortRxContext_t, rxWork);
    uCxAtClientHandleRx(pCtx->pClient);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uPortInit(void)
{
    // Nothing to do for Zephyr - kernel is already initialized
}

void uPortDeinit(void)
{
    // Nothing to do for Zephyr
}

int32_t uPortGetTickTimeMs(void)
{
    return (int32_t)k_uptime_get_32();
}

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    gRxContext.pClient = pClient;
    k_work_init(&gRxContext.rxWork, rxTask);
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;
    k_work_cancel(&gRxContext.rxWork);
}

void uPortRxSignal(void)
{
    if (!k_work_is_pending(&gRxContext.rxWork) && !gDisableRxWorker) {
        k_work_submit(&gRxContext.rxWork);
    }
}
