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
 * @brief Zephyr port layer example
 */

#ifndef U_PORT_ZEPHYR_H
#define U_PORT_ZEPHYR_H

#include <stdint.h>
#include <stdbool.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_PORT_ZEPHYR

#define U_CX_MUTEX_HANDLE                     struct k_mutex
#define U_CX_MUTEX_CREATE(mutex)              k_mutex_init(&mutex)
#define U_CX_MUTEX_DELETE(mutex)
#define U_CX_MUTEX_LOCK(mutex)                k_mutex_lock(&mutex, K_FOREVER);
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) k_mutex_lock(&mutex, K_MSEC(timeoutMs))
#define U_CX_MUTEX_UNLOCK(mutex)              k_mutex_unlock(&mutex)

#define U_CX_PORT_GET_TIME_MS()               (int32_t)k_uptime_get_32()

#ifdef CONFIG_UCXCLIENT_URC_QUEUE
# define U_CX_USE_URC_QUEUE  1
#else
# define U_CX_USE_URC_QUEUE  0
#endif

#ifdef CONFIG_ZTEST
# include <zephyr/ztest.h>
# define U_CX_AT_PORT_ASSERT(COND) zassert_true(COND)
#endif

#ifndef U_EXAMPLE_UART
# define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
# define U_EXAMPLE_UART DEVICE_DT_GET(UART_DEVICE_NODE)->name
#endif

#ifndef U_EXAMPLE_SSID
# define U_EXAMPLE_SSID "ubx"
#endif

#ifndef U_EXAMPLE_WPA_PSK
# define U_EXAMPLE_WPA_PSK "fooooooo"
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

#endif