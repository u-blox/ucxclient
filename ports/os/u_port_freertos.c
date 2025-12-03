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
 * @brief FreeRTOS OS port implementation.
 *
 * Provides mutex, threading, and time functions using FreeRTOS APIs.
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_cx_log.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_PORT_FREERTOS_RX_TASK_STACK_SIZE
#define U_PORT_FREERTOS_RX_TASK_STACK_SIZE    (2048)
#endif

#ifndef U_PORT_FREERTOS_RX_TASK_PRIORITY
#define U_PORT_FREERTOS_RX_TASK_PRIORITY      (configMAX_PRIORITIES - 2)
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    uCxAtClient_t *pClient;
    TaskHandle_t rxTaskHandle;
    volatile bool terminateRxTask;
} uPortRxContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uint32_t gBootTime = 0;
static uPortRxContext_t gRxContext;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void rxTask(void *pArg)
{
    uPortRxContext_t *pCtx = (uPortRxContext_t *)pArg;

    while (!pCtx->terminateRxTask) {
        vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay
        uCxAtClientHandleRx(pCtx->pClient);
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pCtx->pClient->instance, "RX task terminated");
    vTaskDelete(NULL);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uPortInit(void)
{
    if (gBootTime == 0) {
        gBootTime = xTaskGetTickCount();
    }
}

void uPortDeinit(void)
{
    // Nothing to do for FreeRTOS
}

int32_t uPortGetTickTimeMs(void)
{
    return (int32_t)((xTaskGetTickCount() - gBootTime) * portTICK_PERIOD_MS);
}

int32_t uPortMutexTryLock(SemaphoreHandle_t mutex, uint32_t timeoutMs)
{
    TickType_t ticks;

    if (timeoutMs == 0) {
        ticks = 0;
    } else if (timeoutMs == UINT32_MAX) {
        ticks = portMAX_DELAY;
    } else {
        ticks = pdMS_TO_TICKS(timeoutMs);
    }

    BaseType_t ret = xSemaphoreTake(mutex, ticks);
    return (ret == pdTRUE) ? 0 : -1;
}

void uPortBgRxTaskCreate(uCxAtClient_t *pClient)
{
    memset(&gRxContext, 0, sizeof(gRxContext));
    gRxContext.pClient = pClient;
    gRxContext.terminateRxTask = false;

    xTaskCreate(
        rxTask,
        "ucxRx",
        U_PORT_FREERTOS_RX_TASK_STACK_SIZE,
        &gRxContext,
        U_PORT_FREERTOS_RX_TASK_PRIORITY,
        &gRxContext.rxTaskHandle
    );
}

void uPortBgRxTaskDestroy(uCxAtClient_t *pClient)
{
    (void)pClient;
    gRxContext.terminateRxTask = true;

    // Wait for task to terminate (it will delete itself)
    while (eTaskGetState(gRxContext.rxTaskHandle) != eDeleted) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
