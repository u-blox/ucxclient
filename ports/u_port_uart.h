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
 * @brief UART port abstraction layer
 *
 * This header defines the UART interface that must be implemented
 * by each platform port.
 */

#ifndef U_PORT_UART_H
#define U_PORT_UART_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** UART handle - platform-specific implementation */
typedef void *uPortUartHandle_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Open UART device
 *
 * Opens and configures a UART device for communication.
 *
 * @param[in]  pDevName        Device name (e.g., "/dev/ttyUSB0", "COM3", etc.)
 * @param      baudRate        Baud rate (e.g., 115200)
 * @param      useFlowControl  true to enable RTS/CTS hardware flow control
 * @return                     UART handle on success, NULL on failure
 */
uPortUartHandle_t uPortUartOpen(const char *pDevName, int32_t baudRate, bool useFlowControl);

/**
 * @brief Close UART device
 *
 * Closes a UART device previously opened with uPortUartOpen().
 *
 * @param[in]  handle  UART handle from uPortUartOpen()
 */
void uPortUartClose(uPortUartHandle_t handle);

/**
 * @brief Write data to UART
 *
 * Writes data to the UART device. This function should block until
 * all data is written or an error occurs.
 *
 * @param[in]  handle  UART handle from uPortUartOpen()
 * @param[in]  pData   Pointer to data to write
 * @param      length  Number of bytes to write
 * @return             Number of bytes actually written, or negative on error
 */
int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length);

/**
 * @brief Read data from UART
 *
 * Reads data from the UART device with timeout.
 *
 * @param[in]  handle     UART handle from uPortUartOpen()
 * @param[out] pData      Buffer to store received data
 * @param      length     Maximum number of bytes to read
 * @param      timeoutMs  Timeout in milliseconds (0 = non-blocking, -1 = blocking)
 * @return                Number of bytes actually read, or negative on error
 */
int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_H
