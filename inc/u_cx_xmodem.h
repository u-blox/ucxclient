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
 * @brief XMODEM protocol implementation for firmware updates
 *
 * This module provides XMODEM/XMODEM-1K protocol support for binary transfers.
 *
 */

#ifndef U_CX_XMODEM_H
#define U_CX_XMODEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "u_cx_at_config.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/**
 * XMODEM context structure
 */
typedef struct {
    const char *pUartDevName;   /**< UART device name (e.g., "UART0", "/dev/ttyUSB0") */
    uPortUartHandle_t uartHandle; /**< Internal UART handle */
    bool use1K;                 /**< Use 1K blocks (XMODEM-1K) instead of 128-byte blocks */
    int32_t timeoutMs;          /**< Timeout for receiving ACK/NAK (milliseconds) */
    int32_t instance;           /**< Instance number for logging */
    volatile bool opened;       /**< UART opened state */
} uCxXmodemConfig_t;

/**
 * XMODEM data request callback
 *
 * Called by the XMODEM transfer function to retrieve data to send.
 *
 * @param[out] pBuffer    Buffer to fill with data
 * @param      offset     Offset in the total data stream
 * @param      maxLen     Maximum bytes to write to pBuffer
 * @param      pUserData  User data pointer
 * @return                Number of bytes written to pBuffer (0 = end of data, <0 = error)
 */
typedef int32_t (*uCxXmodemDataCallback_t)(uint8_t *pBuffer, size_t offset, size_t maxLen, void *pUserData);

/**
 * XMODEM transfer progress callback
 *
 * @param totalBytes      Total bytes to transfer
 * @param bytesTransferred  Bytes transferred so far
 * @param pUserData       User data pointer
 */
typedef void (*uCxXmodemProgressCallback_t)(size_t totalBytes, size_t bytesTransferred, void *pUserData);

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Initialize XMODEM configuration with defaults
 *
 * @param[in]  pUartDevName  UART device name (e.g., "UART0", "/dev/ttyUSB0")
 * @param[out] pConfig       Pointer to configuration structure to initialize
 */
void uCxXmodemInit(const char *pUartDevName, uCxXmodemConfig_t *pConfig);

/**
 * @brief Open UART connection for XMODEM transfer
 *
 * @param[in] pConfig      Pointer to XMODEM configuration
 * @param[in] baudRate     UART baud rate (e.g., 115200)
 * @param[in] flowControl  Enable hardware flow control (true/false)
 * @return                 0 on success, negative value on error
 */
int32_t uCxXmodemOpen(uCxXmodemConfig_t *pConfig, int32_t baudRate, bool flowControl);

/**
 * @brief Close UART connection
 *
 * @param[in] pConfig  Pointer to XMODEM configuration
 */
void uCxXmodemClose(uCxXmodemConfig_t *pConfig);

/**
 * @brief Send data using XMODEM protocol with callback-based data retrieval
 *
 * This function sends data using the XMODEM protocol with CRC16 error checking.
 * It waits for the receiver to initiate the transfer by sending 'C' or NAK,
 * then repeatedly calls the data callback to retrieve blocks of data to send.
 *
 * Must be called after uCxXmodemOpen().
 *
 * @param[in] pConfig         XMODEM configuration
 * @param     dataLen         Total length of data to transfer in bytes
 * @param     dataCallback    Callback to retrieve data blocks
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData       User data pointer passed to callbacks
 * @return                    0 on success, negative error code on failure
 */
int32_t uCxXmodemSend(uCxXmodemConfig_t *pConfig,
                      size_t dataLen,
                      uCxXmodemDataCallback_t dataCallback,
                      uCxXmodemProgressCallback_t progressCallback,
                      void *pUserData);

#if U_CX_XMODEM_FILE_SUPPORT
/**
 * @brief Send file using XMODEM protocol (convenience function)
 *
 * Reads the specified file and sends it using XMODEM protocol.
 * This function is only available when U_CX_XMODEM_FILE_SUPPORT is defined.
 *
 * Must be called after uCxXmodemOpen().
 *
 * @param[in] pConfig    XMODEM configuration
 * @param[in] pFilePath  Path to file to send
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData  User data pointer passed to callback
 * @return               0 on success, negative error code on failure
 */
int32_t uCxXmodemSendFile(uCxXmodemConfig_t *pConfig,
                          const char *pFilePath,
                          uCxXmodemProgressCallback_t progressCallback,
                          void *pUserData);
#endif // U_CX_XMODEM_FILE_SUPPORT

#endif // U_CX_XMODEM_H
