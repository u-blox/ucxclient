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
 * @brief FreeRTOS port layer
 *
 * This port provides ucxclient support for FreeRTOS-based systems.
 */

#ifndef U_PORT_FREERTOS_H
#define U_PORT_FREERTOS_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_MUTEX_HANDLE                     SemaphoreHandle_t
#define U_CX_MUTEX_CREATE(mutex)              mutex = xSemaphoreCreateMutex()
#define U_CX_MUTEX_DELETE(mutex)              vSemaphoreDelete(mutex)
#define U_CX_MUTEX_LOCK(mutex)                xSemaphoreTake(mutex, portMAX_DELAY)
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(mutex, timeoutMs)
#define U_CX_MUTEX_UNLOCK(mutex)              xSemaphoreGive(mutex)

#define U_CX_PORT_SLEEP_MS(ms)                vTaskDelay(pdMS_TO_TICKS(ms))

/* Example main function abstraction for FreeRTOS
 * Examples use app_main() which is called from the FreeRTOS task */
#define U_EXAMPLE_MAIN                        app_main

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief FreeRTOS implementation of U_CX_MUTEX_TRY_LOCK()
 *
 * @param mutex      FreeRTOS semaphore handle
 * @param timeoutMs  Timeout in milliseconds (0 = non-blocking)
 * @return           0 on success, negative value on timeout
 */
int32_t uPortMutexTryLock(SemaphoreHandle_t mutex, uint32_t timeoutMs);

#endif // U_PORT_FREERTOS_H
