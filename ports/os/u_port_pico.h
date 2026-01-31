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
 * @brief Raspberry Pi Pico port layer for ucxclient
 *
 * Supports all Pico family boards:
 * - Raspberry Pi Pico (RP2040)
 * - Raspberry Pi Pico 2 (RP2350, ARM or RISC-V)
 *
 * Bare-metal implementation using Pico SDK primitives.
 * Uses critical sections for mutual exclusion (no RTOS required).
 */

#ifndef U_PORT_PICO_H
#define U_PORT_PICO_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/critical_section.h"
#include "pico/time.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_PORT_PICO
# define U_PORT_PICO
#endif

/* ----------------------------------------------------------------
 * MUTEX - Using Pico SDK critical sections (bare-metal safe)
 * -------------------------------------------------------------- */

/** Mutex handle type for RP2040 - uses Pico SDK critical section */
#define U_CX_MUTEX_HANDLE critical_section_t

/** Create/initialize a mutex */
#define U_CX_MUTEX_CREATE(mutex) critical_section_init(&(mutex))

/** Delete/deinitialize a mutex */
#define U_CX_MUTEX_DELETE(mutex) critical_section_deinit(&(mutex))

/** Lock a mutex */
#define U_CX_MUTEX_LOCK(mutex) critical_section_enter_blocking(&(mutex))

/** Try to lock a mutex with timeout (returns 0 on success)
 * Note: Pico critical sections don't support timeout - uses polling
 */
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(&(mutex), (timeoutMs))

/** Unlock a mutex */
#define U_CX_MUTEX_UNLOCK(mutex) critical_section_exit(&(mutex))

/* ----------------------------------------------------------------
 * TIME
 * -------------------------------------------------------------- */

/** Get current time in milliseconds */
static inline uint32_t uPortGetTimeMs(void) {
    return (uint32_t)(to_ms_since_boot(get_absolute_time()) & 0xFFFFFFFF);
}
#define U_CX_PORT_GET_TIME_MS() uPortGetTimeMs()

/** Sleep for specified milliseconds */
#define U_CX_PORT_SLEEP_MS(ms) sleep_ms(ms)

/* ----------------------------------------------------------------
 * PRINTF
 * -------------------------------------------------------------- */

/** Printf implementation - uses Pico SDK stdio (routed to USB or UART) */
#define U_CX_PORT_PRINTF printf

/* ----------------------------------------------------------------
 * ASSERT
 * -------------------------------------------------------------- */

/** Assert - triggers Pico panic on failure */
#define U_CX_AT_PORT_ASSERT(COND) do { \
    if (!(COND)) { \
        printf("ASSERT FAILED: %s at %s:%d\n", #COND, __FILE__, __LINE__); \
        panic("ucxclient assert"); \
    } \
} while(0)

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Try to lock mutex with timeout
 * 
 * Note: Pico SDK critical sections are non-recursive and don't support
 * timeout natively. This implementation uses polling with sleep.
 * 
 * @param pMutex    Pointer to critical section
 * @param timeoutMs Timeout in milliseconds
 * @return          0 on success, -1 on timeout
 */
int32_t uPortMutexTryLock(critical_section_t *pMutex, uint32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif /* U_PORT_PICO_H */
