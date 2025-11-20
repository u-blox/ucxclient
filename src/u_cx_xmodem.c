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
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "u_cx_xmodem.h"
#include "u_cx_log.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_XMODEM_SOH             0x01    /**< Start of 128-byte block */
#define U_CX_XMODEM_STX             0x02    /**< Start of 1K block */
#define U_CX_XMODEM_EOT             0x04    /**< End of transmission */
#define U_CX_XMODEM_ACK             0x06    /**< Acknowledge */
#define U_CX_XMODEM_NAK             0x15    /**< Negative acknowledge */
#define U_CX_XMODEM_CAN             0x18    /**< Cancel */
#define U_CX_XMODEM_CCHR            0x43    /**< 'C' - CRC mode request */

#define U_CX_XMODEM_BLOCK_SIZE_128  128
#define U_CX_XMODEM_BLOCK_SIZE_1K   1024
#define U_CX_XMODEM_HEADER_SIZE     3       /**< SOH/STX + block_num + block_num_complement */
#define U_CX_XMODEM_CRC_SIZE        2

#define U_CX_XMODEM_DEFAULT_TIMEOUT_MS  15000  /**< 15 second timeout */
#define U_CX_XMODEM_MAX_RETRIES         3
#define U_CX_XMODEM_START_TIMEOUT_MS    10000  /**< 10 seconds for initial handshake*/

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static uint16_t xmodemCrc16(const uint8_t *pBuf, size_t len);
static int32_t xmodemWaitForStart(uCxXmodemConfig_t *pConfig, int32_t timeoutMs);
static int32_t xmodemSendBlock(uCxXmodemConfig_t *pConfig, uint8_t blockNum,
                               const uint8_t *pData, size_t dataLen, size_t blockSize,
                               int32_t timeoutMs);
static int32_t xmodemSendEot(uCxXmodemConfig_t *pConfig, int32_t timeoutMs);

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * Calculate CRC16-CCITT for XMODEM
 */
static uint16_t xmodemCrc16(const uint8_t *pBuf, size_t len)
{
    uint16_t crc = 0;

    for (size_t i = 0; i < len; i++) {
        crc ^= (uint16_t)pBuf[i] << 8;
        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ 0x1021;
            } else {
                crc = crc << 1;
            }
        }
    }

    return crc;
}

/**
 * Wait for receiver to send start signal ('C' for CRC mode or NAK for checksum mode)
 */
static int32_t xmodemWaitForStart(uCxXmodemConfig_t *pConfig, int32_t timeoutMs)
{
    uint8_t startChar;
    int32_t bytesRead;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();
    int32_t attemptsCount = 0;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Waiting for start signal (timeout=%dms)...", timeoutMs);

    while ((U_CX_PORT_GET_TIME_MS() - startTime) < timeoutMs) {
        bytesRead = uPortUartRead(pConfig->uartHandle, &startChar, 1, 100);

        if (bytesRead == 1) {
            attemptsCount++;
            U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Received byte 0x%02X (attempt %d)", startChar, attemptsCount);

            if (startChar == U_CX_XMODEM_CCHR) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Receiver ready (CRC mode) - starting transfer");
                return 0;
            } else if (startChar == U_CX_XMODEM_NAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Receiver ready (checksum mode - not supported)");
                return -2;  // Checksum mode not supported, only CRC
            } else if (startChar == U_CX_XMODEM_CAN) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Transfer cancelled by receiver");
                return -3;
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance, "XMODEM: Ignoring unexpected byte 0x%02X", startChar);
            }
        }
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Timeout waiting for start signal (received %d bytes in %dms)",
                    attemptsCount, (int32_t)(U_CX_PORT_GET_TIME_MS() - startTime));
    return -1;
}

/**
 * Send a single XMODEM block with retries
 */
static int32_t xmodemSendBlock(uCxXmodemConfig_t *pConfig, uint8_t blockNum,
                               const uint8_t *pData, size_t dataLen, size_t blockSize,
                               int32_t timeoutMs)
{
    uint8_t packet[U_CX_XMODEM_HEADER_SIZE + U_CX_XMODEM_BLOCK_SIZE_1K + U_CX_XMODEM_CRC_SIZE];
    size_t packetSize = U_CX_XMODEM_HEADER_SIZE + blockSize + U_CX_XMODEM_CRC_SIZE;
    uint8_t response;
    int32_t bytesRead;

    // Build packet header
    packet[0] = (blockSize == U_CX_XMODEM_BLOCK_SIZE_1K) ? U_CX_XMODEM_STX : U_CX_XMODEM_SOH;
    packet[1] = blockNum;
    packet[2] = ~blockNum;  // Block number complement (bitwise NOT)

#if U_CX_XMODEM_VERBOSE_DEBUG
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                    "XMODEM: Block header: [0]=0x%02X (%s), [1]=0x%02X (num=%u), [2]=0x%02X (~num=%u)",
                    packet[0], (packet[0] == U_CX_XMODEM_STX) ? "STX/1K" : "SOH/128",
                    packet[1], packet[1], packet[2], packet[2]);
#endif

    // Copy data and pad with 0x1A (EOF/Ctrl-Z) if needed
    memset(&packet[3], 0x1A, blockSize);
    if (dataLen > 0) {
        memcpy(&packet[3], pData, dataLen);  // Only copy actual data length
    }

#if U_CX_XMODEM_VERBOSE_DEBUG
    if (dataLen < blockSize) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                        "XMODEM: Padding block %u with %zu bytes of 0x1A (data=%zu, block=%zu)",
                        blockNum, blockSize - dataLen, dataLen, blockSize);
    }
#endif

    // Calculate and append CRC16-CCITT
    uint16_t crc = xmodemCrc16(&packet[3], blockSize);
    packet[3 + blockSize] = (uint8_t)((crc >> 8) & 0xFF);      // CRC high byte
    packet[3 + blockSize + 1] = (uint8_t)(crc & 0xFF);         // CRC low byte

#if U_CX_XMODEM_VERBOSE_DEBUG
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                    "XMODEM: Block %u: CRC16=0x%04X, packet size=%zu bytes",
                    blockNum, crc, packetSize);
#endif

    // Try sending the block with retries
    for (int32_t retry = 0; retry < U_CX_XMODEM_MAX_RETRIES; retry++) {
#if U_CX_XMODEM_VERBOSE_DEBUG
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                        "XMODEM: >>> Sending block %u (try %d/%d, %zu bytes)...",
                        blockNum, retry + 1, U_CX_XMODEM_MAX_RETRIES, packetSize);
#endif

        // Send packet
        int32_t bytesWritten = uPortUartWrite(pConfig->uartHandle, packet, packetSize);
        if (bytesWritten != (int32_t)packetSize) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                            "XMODEM: Write error on block %u (wrote %d of %zu bytes)",
                            blockNum, bytesWritten, packetSize);
            continue;
        }

        // Wait for response
        bytesRead = 0;
        int32_t startTime = U_CX_PORT_GET_TIME_MS();

        while ((U_CX_PORT_GET_TIME_MS() - startTime) < timeoutMs) {
            bytesRead = uPortUartRead(pConfig->uartHandle, &response, 1, 100);

            if (bytesRead == 1) {
                if (response == U_CX_XMODEM_ACK) {
#if U_CX_XMODEM_VERBOSE_DEBUG
                    int32_t elapsed = U_CX_PORT_GET_TIME_MS() - startTime;
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                                    "XMODEM: <<< Block %u ACKed after %dms", blockNum, elapsed);
#endif
                    return 0;  // Success
                } else if (response == U_CX_XMODEM_NAK) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance,
                                    "XMODEM: <<< Block %u NAKed, retrying...", blockNum);
                    break;  // Retry
                } else if (response == U_CX_XMODEM_CAN) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                                    "XMODEM: <<< Transfer cancelled by receiver");
                    return -3;
                } else {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance,
                                    "XMODEM: <<< Unexpected response 0x%02X", response);
                }
            }
        }

        if (bytesRead != 1) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance,
                            "XMODEM: Timeout waiting for ACK on block %u", blockNum);
        }
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Failed to send block %u after %d retries", blockNum, U_CX_XMODEM_MAX_RETRIES);
    return -1;
}

/**
 * Send End of Transmission
 */
static int32_t xmodemSendEot(uCxXmodemConfig_t *pConfig, int32_t timeoutMs)
{
    uint8_t eot = U_CX_XMODEM_EOT;
    uint8_t response;
    int32_t bytesRead;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: >>> Sending EOT (0x04)...");

    int32_t bytesWritten = uPortUartWrite(pConfig->uartHandle, &eot, 1);
    if (bytesWritten != 1) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Failed to write EOT (wrote %d bytes)", bytesWritten);
        return -1;
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: <<< Waiting for final ACK (timeout=%dms)...", timeoutMs);

    // Wait for final ACK
    int32_t startTime = U_CX_PORT_GET_TIME_MS();
    int32_t readAttempts = 0;

    while ((U_CX_PORT_GET_TIME_MS() - startTime) < timeoutMs) {
        bytesRead = uPortUartRead(pConfig->uartHandle, &response, 1, 100);

        if (bytesRead == 1) {
            readAttempts++;
            int32_t elapsed = U_CX_PORT_GET_TIME_MS() - startTime;

            if (response == U_CX_XMODEM_ACK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                                "XMODEM: <<< EOT ACKed (0x06) after %dms - TRANSFER COMPLETE!", elapsed);
                return 0;
            } else if (response == U_CX_XMODEM_NAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance,
                                "XMODEM: <<< EOT NAKed (0x15) - receiver may want EOT resent");
                // Some receivers NAK the first EOT - try sending it again
                // But don't implement retry here yet, just log it
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pConfig->instance,
                                "XMODEM: <<< Unexpected response to EOT: 0x%02X (attempt %d, elapsed %dms)",
                                response, readAttempts, elapsed);
            }
        }
    }

    int32_t totalElapsed = U_CX_PORT_GET_TIME_MS() - startTime;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                    "XMODEM: Timeout waiting for ACK after EOT (waited %dms, read attempts=%d)",
                    totalElapsed, readAttempts);
    return -1;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxXmodemInit(const char *pUartDevName, uCxXmodemConfig_t *pConfig)
{
    if (pConfig != NULL) {
        memset(pConfig, 0, sizeof(uCxXmodemConfig_t));
        pConfig->pUartDevName = pUartDevName;
        pConfig->uartHandle = NULL;
        pConfig->use1K = true;  // Use 1K blocks by default for better performance
        pConfig->timeoutMs = U_CX_XMODEM_DEFAULT_TIMEOUT_MS;
        pConfig->instance = 0;
        pConfig->opened = false;
    }
}

int32_t uCxXmodemOpen(uCxXmodemConfig_t *pConfig, int32_t baudRate, bool flowControl)
{
    if (pConfig == NULL || pConfig->pUartDevName == NULL) {
        return U_CX_ERROR_INVALID_PARAMETER;
    }

    if (pConfig->opened) {
        return U_CX_ERROR_ALREADY_EXISTS;
    }

    // Open UART
    pConfig->uartHandle = uPortUartOpen(pConfig->pUartDevName, baudRate, flowControl);
    if (pConfig->uartHandle == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                        "XMODEM: Failed to open UART device '%s'", pConfig->pUartDevName);
        return U_CX_ERROR_IO;
    }

    pConfig->opened = true;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                    "XMODEM: Opened UART '%s' at %d baud", pConfig->pUartDevName, baudRate);
    return 0;
}

void uCxXmodemClose(uCxXmodemConfig_t *pConfig)
{
    if (pConfig != NULL && pConfig->opened) {
        uPortUartClose(pConfig->uartHandle);
        pConfig->uartHandle = NULL;
        pConfig->opened = false;
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Closed UART");
    }
}

int32_t uCxXmodemSend(uCxXmodemConfig_t *pConfig,
                      size_t dataLen,
                      uCxXmodemDataCallback_t dataCallback,
                      uCxXmodemProgressCallback_t progressCallback,
                      void *pUserData)
{
    if (pConfig == NULL || !pConfig->opened ||
        dataLen == 0 || dataCallback == NULL) {
        return -1;
    }

    size_t blockSize = pConfig->use1K ? U_CX_XMODEM_BLOCK_SIZE_1K : U_CX_XMODEM_BLOCK_SIZE_128;
    uint8_t blockBuffer[U_CX_XMODEM_BLOCK_SIZE_1K];

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Starting transfer (%zu bytes, %zu-byte blocks)",
                    dataLen, blockSize);

    // Wait for receiver to initiate transfer
    int32_t result = xmodemWaitForStart(pConfig, U_CX_XMODEM_START_TIMEOUT_MS);
    if (result != 0) {
        return result;
    }

    // Send all blocks
    uint8_t blockNum = 1;  // XMODEM spec: first block is 1, not 0
    size_t offset = 0;
    size_t totalBlocks = (dataLen + blockSize - 1) / blockSize;  // Ceiling division
    size_t blocksSent = 0;

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                    "XMODEM: Transfer details: total_size=%zu, block_size=%zu, total_blocks=%zu",
                    dataLen, blockSize, totalBlocks);

    while (offset < dataLen) {
        size_t remainingBytes = dataLen - offset;
        size_t requestLen = (remainingBytes < blockSize) ? remainingBytes : blockSize;
        blocksSent++;

        // Request data from callback
        int32_t bytesRead = dataCallback(blockBuffer, offset, requestLen, pUserData);
        if (bytesRead < 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                            "XMODEM: Data callback error at offset %zu", offset);
            return -4;
        }
        if (bytesRead == 0 && remainingBytes > 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                            "XMODEM: Data callback returned 0 bytes (expected %zu)", requestLen);
            return -4;
        }

#if U_CX_XMODEM_VERBOSE_DEBUG
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                        "XMODEM: === Block %zu/%zu (num=%u, offset=%zu, data=%d, remaining=%zu) ===",
                        blocksSent, totalBlocks, blockNum, offset, bytesRead, remainingBytes);
#endif

        // Send block
        result = xmodemSendBlock(pConfig, blockNum, blockBuffer, (size_t)bytesRead, blockSize,
                                pConfig->timeoutMs);
        if (result != 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance,
                            "XMODEM: === TRANSFER FAILED at block %u (sent %zu/%zu blocks) ===",
                            blockNum, blocksSent, totalBlocks);
            return result;
        }

        offset += (size_t)bytesRead;

        // Increment block number with natural 8-bit wraparound per XMODEM spec
        blockNum = (uint8_t)((blockNum + 1) % 256);

        // Call progress callback
        if (progressCallback != NULL) {
            progressCallback(dataLen, offset, pUserData);
        }

        // Give receiver time to process the block (especially for flash writes)
        if (blockSize == U_CX_XMODEM_BLOCK_SIZE_1K && offset < dataLen) {
            int32_t delayEnd = U_CX_PORT_GET_TIME_MS() + 10;
            while (U_CX_PORT_GET_TIME_MS() < delayEnd) {
                // Busy wait
            }
        }
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance,
                    "XMODEM: === All blocks sent (%zu/%zu) - sending EOT ===",
                    blocksSent, totalBlocks);

    // Send EOT
    result = xmodemSendEot(pConfig, pConfig->timeoutMs);
    if (result != 0) {
        return result;
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: Transfer completed successfully");
    return 0;
}

#if U_CX_XMODEM_FILE_SUPPORT

// File context for file-based transfer
typedef struct {
    FILE *pFile;
    size_t fileSize;
} uFileContext_t;

// Data callback for file-based transfer
static int32_t fileDataCallback(uint8_t *pBuffer, size_t offset, size_t maxLen, void *pUserData)
{
    uFileContext_t *pCtx = (uFileContext_t *)pUserData;

    (void)maxLen;  // maxLen is used via fread which handles the size

    // Seek to offset
    if (fseek(pCtx->pFile, (long)offset, SEEK_SET) != 0) {
        return -1;
    }

    // Read data
    size_t bytesRead = fread(pBuffer, 1, maxLen, pCtx->pFile);
    return (int32_t)bytesRead;
}

int32_t uCxXmodemSendFile(uCxXmodemConfig_t *pConfig,
                          const char *pFilePath,
                          uCxXmodemProgressCallback_t progressCallback,
                          void *pUserData)
{
    if (pConfig == NULL || pFilePath == NULL) {
        return -1;
    }

    (void)pUserData;  // User data passed through to progress callback

    // Open file
    FILE *pFile = fopen(pFilePath, "rb");
    if (pFile == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Failed to open file: %s", pFilePath);
        return -1;
    }

    // Get file size
    fseek(pFile, 0, SEEK_END);
    long fileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);

    if (fileSize <= 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pConfig->instance, "XMODEM: Invalid file size: %ld", fileSize);
        fclose(pFile);
        return -1;
    }

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pConfig->instance, "XMODEM: File size: %ld bytes", fileSize);

    // Setup file context
    uFileContext_t fileCtx = {
        .pFile = pFile,
        .fileSize = (size_t)fileSize
    };

    // Send file using callback-based API
    int32_t result = uCxXmodemSend(pConfig, (size_t)fileSize,
                                   fileDataCallback, progressCallback, &fileCtx);

    // Cleanup
    fclose(pFile);

    return result;
}

#endif // U_CX_XMODEM_FILE_SUPPORT
