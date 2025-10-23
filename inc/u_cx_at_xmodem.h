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
 * @brief XMODEM protocol implementation for firmware updates
 */

#ifndef U_CX_AT_XMODEM_H
#define U_CX_AT_XMODEM_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/**
 * XMODEM transfer configuration
 */
typedef struct {
    bool use1K;                 /**< Use 1K blocks (XMODEM-1K) instead of 128-byte blocks */
    int32_t timeoutMs;          /**< Timeout for receiving ACK/NAK (milliseconds) */
    int32_t maxRetries;         /**< Maximum number of retries per block */
} uCxXmodemConfig_t;

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
 * @param[out] pConfig  Pointer to configuration structure to initialize
 */
void uCxXmodemConfigInit(uCxXmodemConfig_t *pConfig);

/**
 * @brief Send file data using XMODEM protocol
 * 
 * This function sends data over the AT client's serial interface using
 * the XMODEM protocol with CRC16 error checking. It waits for the receiver
 * to initiate the transfer by sending 'C' or NAK.
 * 
 * @param[in] pClient         Pointer to AT client (must be open)
 * @param[in] pData           Pointer to data buffer to send
 * @param     dataLen         Length of data in bytes
 * @param[in] pConfig         Optional XMODEM configuration (NULL for defaults)
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData       User data pointer passed to callback
 * @return                    0 on success, negative error code on failure
 */
int32_t uCxXmodemSend(uCxAtClient_t *pClient, 
                      const uint8_t *pData, 
                      size_t dataLen,
                      const uCxXmodemConfig_t *pConfig,
                      uCxXmodemProgressCallback_t progressCallback,
                      void *pUserData);

/**
 * @brief Send file using XMODEM protocol (convenience function)
 * 
 * Reads the specified file and sends it using XMODEM protocol.
 * 
 * @param[in] pClient    Pointer to AT client (must be open)
 * @param[in] pFilePath  Path to file to send
 * @param     use1K      Use 1K blocks (faster) instead of 128-byte blocks
 * @param     progressCallback  Optional progress callback (NULL to disable)
 * @param     pUserData  User data pointer passed to callback
 * @return               0 on success, negative error code on failure
 */
int32_t uCxXmodemSendFile(uCxAtClient_t *pClient,
                          const char *pFilePath,
                          bool use1K,
                          uCxXmodemProgressCallback_t progressCallback,
                          void *pUserData);

#endif // U_CX_AT_XMODEM_H
