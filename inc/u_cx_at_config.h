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
 * @brief Configuration file for ucxclient
 */
#ifndef U_CX_AT_CONFIG_H
#define U_CX_AT_CONFIG_H

/* To override the default settings you can define U_CX_AT_CONFIG_FILE
 * to include a custom configuration header file
 */
#ifdef U_CX_AT_CONFIG_FILE
# include U_CX_AT_CONFIG_FILE
#endif


/* U_CX_AT_PORT_ASSERT is used for asserts. To disable asserts just define U_CX_AT_PORT_ASSERT
 * to nothing like this:
 * #define U_CX_AT_PORT_ASSERT(COND)
 */
#ifndef U_CX_AT_PORT_ASSERT
# include <assert.h>
# define U_CX_AT_PORT_ASSERT(COND) assert(COND)
#endif

/* Porting layer for printf().*/
#ifndef U_CX_PORT_PRINTF
# include <stdio.h>
# define U_CX_PORT_PRINTF   printf
#endif

/* Porting layer for mutexes.*/
#ifndef U_CX_MUTEX_HANDLE
// Default to Posix Threads
#include <pthread.h>
# define U_CX_MUTEX_HANDLE          pthread_mutex_t
# define U_CX_MUTEX_CREATE(mutex)   pthread_mutex_init(&mutex, NULL)
# define U_CX_MUTEX_DELETE(mutex)   pthread_mutex_destroy(&mutex);
# define U_CX_MUTEX_LOCK(mutex)     pthread_mutex_lock(&mutex)
# define U_CX_MUTEX_TRY_LOCK(mutex) pthread_mutex_trylock(&mutex)
# define U_CX_MUTEX_UNLOCK(mutex)   pthread_mutex_unlock(&mutex)
#endif

/* Porting layer for getting time in ms.*/
#ifndef U_CX_PORT_GET_TIME_MS
extern int32_t uPortGetTickTimeMs(void);
# define U_CX_PORT_GET_TIME_MS()   uPortGetTickTimeMs()
#endif

/* Configuration for enabling URC queue
 *
 * With "U_CX_USE_URC_QUEUE 1" you can execute new AT commands directly
 * in the URC callback. However, this comes with some penalty as you need
 * to provide the AT client with an extra URC queue buffer and whenever an
 * URC is received it will be copied from the rxBuffer to the URC buffer.
 *
 * NOTE: With "U_CX_USE_URC_QUEUE 0" you must never execute any AT command
 *       directly from the URC callback.
 */
#ifndef U_CX_USE_URC_QUEUE
# define U_CX_USE_URC_QUEUE 1
#endif

/* Configuration for enabling logging of AT protocol.*/
#ifndef U_CX_LOG_AT
# define U_CX_LOG_AT 1
#endif

/* Configuration for enabling logging of warning messages.*/
#ifndef U_CX_LOG_WARNING
# define U_CX_LOG_WARNING 1
#endif

/* Configuration for enabling logging of debug messages.*/
#ifndef U_CX_LOG_DEBUG
# define U_CX_LOG_DEBUG 0
#endif

/* Configuration for enabling ANSI color for logs.*/
#ifndef U_CX_LOG_USE_ANSI_COLOR
# define U_CX_LOG_USE_ANSI_COLOR 1
#endif

/** Error code configuration.
 *
 *  This can be used for controlling the codes returned by
 *  uCxAtClientExecSimpleCmd*(), uCxAtClientCmdGetRspParamsF() and
 *  uCxAtClientCmdEnd();
 */
#ifndef U_CX_EXTENDED_ERROR_OFFSET
// If extended error codes are used (i.e. AT server responds with "ERROR:<err_code>")
// then this value will be returned transparently with U_CX_EXTENDED_ERROR_OFFSET offset,
// i.e. return value will be: U_CX_EXTENDED_ERROR_OFFSET-<err_code>
# define U_CX_EXTENDED_ERROR_OFFSET 0
#endif

#ifndef U_CX_ERROR_STATUS_ERROR
// Return value when AT server responds with status "ERROR"
# define U_CX_ERROR_STATUS_ERROR    -1
#endif

#ifndef U_CX_ERROR_CMD_TIMEOUT
// Return value on command timeout
# define U_CX_ERROR_CMD_TIMEOUT     -0x10000
#endif

#ifndef U_CX_ERROR_IO
// Return value when IO (read) returns negative value
# define U_CX_ERROR_IO              -0x10001
#endif

#endif // U_CX_AT_CONFIG_H