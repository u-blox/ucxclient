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
 * @brief Main port layer header
 *
 * This header provides the platform abstraction layer for ucxclient.
 * It includes mutex, time, and UART abstractions.
 * Platform-specific implementations should be included based on the
 * build configuration.
 */

#ifndef U_PORT_H
#define U_PORT_H

#include <stdint.h>
#include <stdbool.h>

/* Forward declarations */
typedef struct uCxAtClient uCxAtClient_t;

#include "u_port_uart.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * PLATFORM SELECTION
 * -------------------------------------------------------------- */

/* Include platform-specific port implementation first to define macros */
#if defined(__ZEPHYR__) && defined(CONFIG_UCXCLIENT)
# include "os/u_port_zephyr.h"
#elif defined(U_PORT_NO_OS)
# include "os/u_port_no_os.h"
#elif defined(U_PORT_POSIX)
# include "os/u_port_posix.h"
#elif defined(U_PORT_WINDOWS)
# include "os/u_port_windows.h"
#endif

/* If you want to inject your own port you can use U_CX_PORT_HEADER_FILE
 * to include a custom port header file
 */
#ifdef U_CX_PORT_HEADER_FILE
# include U_CX_PORT_HEADER_FILE
#endif

/* ----------------------------------------------------------------
 * MUTEX ABSTRACTION
 * -------------------------------------------------------------- */

/* Porting layer for mutexes */
#ifndef U_CX_MUTEX_HANDLE
# error "U_CX_MUTEX_XXX defines must be defined by the platform port"
/* The following is needed for implementing mutexes (Posix example):
 * #define U_CX_MUTEX_HANDLE                     pthread_mutex_t
 * #define U_CX_MUTEX_CREATE(mutex)              pthread_mutex_init(&mutex, NULL)
 * #define U_CX_MUTEX_DELETE(mutex)              pthread_mutex_destroy(&mutex)
 * #define U_CX_MUTEX_LOCK(mutex)                pthread_mutex_lock(&mutex)
 * #define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(&mutex, timeoutMs)
 * #define U_CX_MUTEX_UNLOCK(mutex)              pthread_mutex_unlock(&mutex)
 */
#endif

/* ----------------------------------------------------------------
 * ASSERT ABSTRACTION
 * -------------------------------------------------------------- */

/* U_CX_AT_PORT_ASSERT is used for asserts. To disable asserts define it to nothing:
 * #define U_CX_AT_PORT_ASSERT(COND)
 */
#ifndef U_CX_AT_PORT_ASSERT
# include <assert.h>
# define U_CX_AT_PORT_ASSERT(COND) assert(COND)
#endif

/* ----------------------------------------------------------------
 * PRINTF ABSTRACTION
 * -------------------------------------------------------------- */

/* Porting layer for printf() */
#ifndef U_CX_PORT_PRINTF
# include <stdio.h>
# define U_CX_PORT_PRINTF   printf
#endif

/* ----------------------------------------------------------------
 * TIME ABSTRACTION
 * -------------------------------------------------------------- */

/* Porting layer for getting time in milliseconds */
#ifndef U_CX_PORT_GET_TIME_MS
extern int32_t uPortGetTickTimeMs(void);
# define U_CX_PORT_GET_TIME_MS()   uPortGetTickTimeMs()
#endif

/* ----------------------------------------------------------------
 * PORT INITIALIZATION
 * -------------------------------------------------------------- */

/**
 * @brief Initialize port layer
 *
 * Should be called once at startup before using any port functions.
 * This initializes platform-specific resources (threads, timers, etc.)
 */
void uPortInit(void);

/**
 * @brief Deinitialize port layer
 *
 * Cleans up resources allocated by uPortInit().
 */
void uPortDeinit(void);

/* ----------------------------------------------------------------
 * RX TASK ABSTRACTION
 * -------------------------------------------------------------- */

/**
 * @brief Create background RX task for AT client
 *
 * Sets up platform-specific automatic RX handling (thread, work queue, etc.).
 * This function is called automatically by uCxAtClientInit().
 *
 * **Implementation is optional**: If not implemented or implemented as a stub,
 * the user must call uCxAtClientHandleRx() manually to process incoming data.
 *
 * @param pClient  Pointer to AT client instance
 */
void uPortBgRxTaskCreate(uCxAtClient_t *pClient);

/**
 * @brief Destroy background RX task
 *
 * Stops and cleans up background RX task resources.
 * This function is called automatically by uCxAtClientDeinit().
 *
 * **Implementation is optional**: If uPortBgRxTaskCreate() is a stub,
 * this can also be a stub.
 *
 * @param pClient  Pointer to AT client instance
 */
void uPortBgRxTaskDestroy(uCxAtClient_t *pClient);

#ifdef __cplusplus
}
#endif

#endif // U_PORT_H
