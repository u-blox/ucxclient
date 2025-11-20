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
 * @brief Example utility functions for ucxclient examples
 *
 * This module provides common utility functions that work with both
 * OS-based and no-OS implementations. It automatically detects the
 * build configuration and adapts functionality accordingly:
 *
 * - In OS mode: Uses background RX task and mutex-based sleep
 * - In no-OS mode: Manually polls RX in event waiting and uses busy-wait sleep
 *
 * This allows the same example code to be compiled for different platforms
 * by simply changing the port layer (u_port_posix.c vs u_port_no_os.c).
 */

#ifndef EXAMPLE_UTILS_H
#define EXAMPLE_UTILS_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

// Auto-detect no-OS mode
#ifdef U_PORT_NO_OS
# define EXAMPLE_NO_OS_MODE 1
#else
# define EXAMPLE_NO_OS_MODE 0
#endif

#ifndef U_EXAMPLE_UART
# ifdef U_PORT_ZEPHYR
#  define UART_DEVICE_NODE DT_CHOSEN(zephyr_shell_uart)
#  define U_EXAMPLE_UART DEVICE_DT_GET(UART_DEVICE_NODE)->name
# else
#  define U_EXAMPLE_UART "/dev/ttyUSB0"
# endif
#endif

#ifndef U_EXAMPLE_AT_RX_BUFFER_SIZE
# define U_EXAMPLE_AT_RX_BUFFER_SIZE   1024
#endif
#ifndef U_EXAMPLE_AT_URC_BUFFER_SIZE
# define U_EXAMPLE_AT_URC_BUFFER_SIZE  512
#endif

#ifndef U_EXAMPLE_SSID
# define U_EXAMPLE_SSID "ubx"
#endif

#ifndef U_EXAMPLE_WPA_PSK
# define U_EXAMPLE_WPA_PSK ""
#endif

// Forward declaration to avoid including u_cx_at_client.h
typedef struct uCxAtClient uCxAtClient_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize example utilities and AT client.
 *
 * This function must be called before using other example utility functions.
 * It initializes the port layer, allocates internal buffers for AT client
 * operation, and opens the UART connection.
 *
 * The function automatically adapts to the build configuration:
 * - OS mode: Creates background RX task for processing incoming data
 * - No-OS mode: Requires manual RX polling via exampleWaitEvent()
 *
 * @param pUartDevice  UART device name (e.g., "/dev/ttyUSB0")
 * @param baudRate     UART baud rate (typically 115200)
 * @param flowControl  Enable hardware flow control (typically true)
 * @return             Pointer to initialized AT client, or NULL on failure
 */
uCxAtClient_t *exampleInit(const char *pUartDevice,
                           int32_t baudRate, bool flowControl);

/**
 * Wait for an event flag with timeout.
 *
 * Waits for a specific event flag to be set by a URC callback.
 * Automatically handles RX polling in no-OS mode to process incoming URCs.
 *
 * The event flag is automatically cleared when detected.
 *
 * @param evtFlag    Event flag to wait for (bit mask)
 * @param timeoutS   Timeout in seconds
 * @return           true if event occurred, false if timeout
 */
bool exampleWaitEvent(uint32_t evtFlag, uint32_t timeoutS);

/**
 * Signal an event flag.
 *
 * Sets an event flag to notify exampleWaitEvent() that an event has occurred.
 * This is typically called from URC callback functions.
 *
 * @param evtFlag   Event flag to signal (bit mask)
 */
void exampleSignalEvent(uint32_t evtFlag);

/**
 * Sleep for specified milliseconds.
 *
 * Implementation varies based on build configuration:
 * - OS mode: Uses mutex-based sleep (non-blocking for other threads)
 * - No-OS mode: Uses busy-wait loop
 *
 * @param timeMs    Time to sleep in milliseconds
 */
void exampleSleepMs(uint32_t timeMs);

#ifdef __cplusplus
}
#endif

#endif // EXAMPLE_UTILS_H
