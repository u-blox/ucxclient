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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "u_cx_at_xmodem.h"
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

#define U_CX_XMODEM_DEFAULT_TIMEOUT_MS  15000  /**< 15 second timeout matches working Python implementation */
#define U_CX_XMODEM_DEFAULT_MAX_RETRIES 10
#define U_CX_XMODEM_START_TIMEOUT_MS    60000  /**< 60 seconds for initial handshake (Python uses 60s) */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static uint16_t xmodemCrc16(const uint8_t *pBuf, size_t len);
static int32_t xmodemWaitForStart(uCxAtClient_t *pClient, int32_t timeoutMs);
static int32_t xmodemSendBlock(uCxAtClient_t *pClient, uint8_t blockNum, 
                               const uint8_t *pData, size_t dataLen, size_t blockSize,
                               int32_t timeoutMs, int32_t maxRetries);
static int32_t xmodemSendEot(uCxAtClient_t *pClient, int32_t timeoutMs);

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
static int32_t xmodemWaitForStart(uCxAtClient_t *pClient, int32_t timeoutMs)
{
    uint8_t startChar;
    int32_t bytesRead;
    int32_t startTime = uPortGetTickTimeMs();
    int32_t attemptsCount = 0;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Waiting for start signal (timeout=%dms)...", timeoutMs);
    
    while ((uPortGetTickTimeMs() - startTime) < timeoutMs) {
        bytesRead = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle, &startChar, 1, 100);
        
        if (bytesRead == 1) {
            attemptsCount++;
            U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Received byte 0x%02X (attempt %d)", startChar, attemptsCount);
            
            if (startChar == U_CX_XMODEM_CCHR) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Receiver ready (CRC mode) - starting transfer");
                return 0;
            } else if (startChar == U_CX_XMODEM_NAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Receiver ready (checksum mode - not supported)");
                return -2;  // Checksum mode not supported, only CRC
            } else if (startChar == U_CX_XMODEM_CAN) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Transfer cancelled by receiver");
                return -3;
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "XMODEM: Ignoring unexpected byte 0x%02X", startChar);
            }
        }
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Timeout waiting for start signal (received %d bytes in %dms)", 
                    attemptsCount, (int32_t)(uPortGetTickTimeMs() - startTime));
    return -1;
}

/**
 * Send a single XMODEM block with retries
 */
static int32_t xmodemSendBlock(uCxAtClient_t *pClient, uint8_t blockNum, 
                               const uint8_t *pData, size_t dataLen, size_t blockSize,
                               int32_t timeoutMs, int32_t maxRetries)
{
    uint8_t packet[U_CX_XMODEM_HEADER_SIZE + U_CX_XMODEM_BLOCK_SIZE_1K + U_CX_XMODEM_CRC_SIZE];
    size_t packetSize = U_CX_XMODEM_HEADER_SIZE + blockSize + U_CX_XMODEM_CRC_SIZE;
    uint8_t response;
    int32_t bytesRead;
    
    // Build packet header
    packet[0] = (blockSize == U_CX_XMODEM_BLOCK_SIZE_1K) ? U_CX_XMODEM_STX : U_CX_XMODEM_SOH;
    packet[1] = blockNum;
    packet[2] = ~blockNum;  // Block number complement (bitwise NOT = 255 - blockNum)
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                    "XMODEM: Block header: [0]=0x%02X (%s), [1]=0x%02X (num=%u), [2]=0x%02X (~num=%u)",
                    packet[0], (packet[0] == U_CX_XMODEM_STX) ? "STX/1K" : "SOH/128", 
                    packet[1], packet[1], packet[2], packet[2]);
    
    // Copy data and pad with 0x1A (EOF/Ctrl-Z) if needed
    memset(&packet[3], 0x1A, blockSize);
    memcpy(&packet[3], pData, dataLen);  // Only copy actual data length
    
    if (dataLen < blockSize) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                        "XMODEM: Padding block %u with %zu bytes of 0x1A (data=%zu, block=%zu)",
                        blockNum, blockSize - dataLen, dataLen, blockSize);
    }
    
    // Calculate and append CRC16-CCITT
    uint16_t crc = xmodemCrc16(&packet[3], blockSize);
    packet[3 + blockSize] = (crc >> 8) & 0xFF;      // CRC high byte
    packet[3 + blockSize + 1] = crc & 0xFF;         // CRC low byte
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                    "XMODEM: Block %u: CRC16=0x%04X, packet size=%zu bytes, first 16 bytes: %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X",
                    blockNum, crc, packetSize,
                    packet[3], packet[4], packet[5], packet[6], packet[7], packet[8], packet[9], packet[10],
                    packet[11], packet[12], packet[13], packet[14], packet[15], packet[16], packet[17], packet[18]);
    
    // Try sending the block with retries
    for (int32_t retry = 0; retry < maxRetries; retry++) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                        "XMODEM: >>> Sending block %u (try %d/%d, %zu bytes)...", 
                        blockNum, retry + 1, maxRetries, packetSize);
        
        // Send packet
        int32_t bytesWritten = pClient->pConfig->write(pClient, pClient->pConfig->pStreamHandle, packet, packetSize);
        if (bytesWritten != (int32_t)packetSize) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                            "XMODEM: Write error on block %u (wrote %d of %zu bytes)", 
                            blockNum, bytesWritten, packetSize);
            continue;
        }
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: <<< Waiting for response (timeout=%dms)...", timeoutMs);
        
        // Wait for response
        bytesRead = 0;
        int32_t startTime = uPortGetTickTimeMs();
        int32_t readAttempts = 0;
        
        while ((uPortGetTickTimeMs() - startTime) < timeoutMs) {
            bytesRead = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle, &response, 1, 100);
            
            if (bytesRead == 1) {
                readAttempts++;
                int32_t elapsed = uPortGetTickTimeMs() - startTime;
                
                if (response == U_CX_XMODEM_ACK) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                                    "XMODEM: <<< Block %u ACKed (0x06) after %dms", blockNum, elapsed);
                    return 0;  // Success
                } else if (response == U_CX_XMODEM_NAK) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                                    "XMODEM: <<< Block %u NAKed (0x15) after %dms, retrying...", 
                                    blockNum, elapsed);
                    break;  // Retry
                } else if (response == U_CX_XMODEM_CAN) {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                                    "XMODEM: <<< Transfer cancelled by receiver (CAN=0x18)");
                    return -3;
                } else {
                    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                                    "XMODEM: <<< Unexpected response 0x%02X (attempt %d, elapsed %dms)", 
                                    response, readAttempts, elapsed);
                }
            }
        }
        
        int32_t totalElapsed = uPortGetTickTimeMs() - startTime;
        if (bytesRead != 1) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                            "XMODEM: Timeout waiting for ACK on block %u (waited %dms, read attempts=%d)", 
                            blockNum, totalElapsed, readAttempts);
        }
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Failed to send block %u after %d retries", blockNum, maxRetries);
    return -1;
}

/**
 * Send End of Transmission
 */
static int32_t xmodemSendEot(uCxAtClient_t *pClient, int32_t timeoutMs)
{
    uint8_t eot = U_CX_XMODEM_EOT;
    uint8_t response;
    int32_t bytesRead;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: >>> Sending EOT (0x04)...");
    
    int32_t bytesWritten = pClient->pConfig->write(pClient, pClient->pConfig->pStreamHandle, &eot, 1);
    if (bytesWritten != 1) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Failed to write EOT (wrote %d bytes)", bytesWritten);
        return -1;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: <<< Waiting for final ACK (timeout=%dms)...", timeoutMs);
    
    // Wait for final ACK
    int32_t startTime = uPortGetTickTimeMs();
    int32_t readAttempts = 0;
    
    while ((uPortGetTickTimeMs() - startTime) < timeoutMs) {
        bytesRead = pClient->pConfig->read(pClient, pClient->pConfig->pStreamHandle, &response, 1, 100);
        
        if (bytesRead == 1) {
            readAttempts++;
            int32_t elapsed = uPortGetTickTimeMs() - startTime;
            
            if (response == U_CX_XMODEM_ACK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                                "XMODEM: <<< EOT ACKed (0x06) after %dms - TRANSFER COMPLETE!", elapsed);
                return 0;
            } else if (response == U_CX_XMODEM_NAK) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                                "XMODEM: <<< EOT NAKed (0x15) - receiver may want EOT resent");
                // Some receivers NAK the first EOT - try sending it again
                // But don't implement retry here yet, just log it
            } else {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, 
                                "XMODEM: <<< Unexpected response to EOT: 0x%02X (attempt %d, elapsed %dms)", 
                                response, readAttempts, elapsed);
            }
        }
    }
    
    int32_t totalElapsed = uPortGetTickTimeMs() - startTime;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                    "XMODEM: Timeout waiting for ACK after EOT (waited %dms, read attempts=%d)", 
                    totalElapsed, readAttempts);
    return -1;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxAtClientXmodemConfigInit(uCxXmodemConfig_t *pConfig)
{
    if (pConfig != NULL) {
        pConfig->use1K = true;  // Use 1K blocks by default for better performance
        pConfig->timeoutMs = U_CX_XMODEM_DEFAULT_TIMEOUT_MS;
        pConfig->maxRetries = U_CX_XMODEM_DEFAULT_MAX_RETRIES;
    }
}

int32_t uCxAtClientXmodemSend(uCxAtClient_t *pClient, 
                              const uint8_t *pData, 
                              size_t dataLen,
                              const uCxXmodemConfig_t *pConfig,
                              uCxXmodemProgressCallback_t progressCallback,
                              void *pUserData)
{
    if (pClient == NULL || pData == NULL || dataLen == 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Invalid parameters");
        return -1;
    }
    
    // Use default config if not provided
    uCxXmodemConfig_t defaultConfig;
    if (pConfig == NULL) {
        uCxAtClientXmodemConfigInit(&defaultConfig);
        pConfig = &defaultConfig;
    }
    
    size_t blockSize = pConfig->use1K ? U_CX_XMODEM_BLOCK_SIZE_1K : U_CX_XMODEM_BLOCK_SIZE_128;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Starting transfer (%zu bytes, %zu-byte blocks)", 
                    dataLen, blockSize);
    
    // Wait for receiver to initiate transfer
    int32_t result = xmodemWaitForStart(pClient, U_CX_XMODEM_START_TIMEOUT_MS);
    if (result != 0) {
        return result;
    }
    
    // Send all blocks
    uint8_t blockNum = 1;  // XMODEM spec: first block is 1, not 0
    size_t offset = 0;
    size_t totalBlocks = (dataLen + blockSize - 1) / blockSize;  // Ceiling division
    size_t blocksSent = 0;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                    "XMODEM: Transfer details: total_size=%zu, block_size=%zu, total_blocks=%zu",
                    dataLen, blockSize, totalBlocks);
    
    while (offset < dataLen) {
        size_t remainingBytes = dataLen - offset;
        size_t currentBlockSize = (remainingBytes < blockSize) ? remainingBytes : blockSize;
        blocksSent++;
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                        "XMODEM: === Block %u/%zu (num=%u, offset=%zu, data=%zu/%zu, remaining=%zu) ===",
                        blocksSent, totalBlocks, blockNum, offset, currentBlockSize, blockSize, remainingBytes);
        
        // Send block - pass actual data size and block size separately
        result = xmodemSendBlock(pClient, blockNum, &pData[offset], currentBlockSize, blockSize,
                                pConfig->timeoutMs, pConfig->maxRetries);
        if (result != 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, 
                            "XMODEM: === TRANSFER FAILED at block %u (sent %zu/%zu blocks) ===",
                            blockNum, blocksSent, totalBlocks);
            return result;
        }
        
        offset += currentBlockSize;
        
        // Increment block number with natural 8-bit wraparound per XMODEM spec
        // Sequence: 1, 2, 3, ..., 255, 0, 1, 2, 3, ...
        // Note: spec says "when reaches 256 it is reset to 0"
        blockNum = (blockNum + 1) % 256;
        
        // Call progress callback
        if (progressCallback != NULL) {
            progressCallback(dataLen, offset, pUserData);
        }
        
        // Give receiver time to process the block (especially for flash writes)
        // Python implementation that works uses 10ms delay between blocks
        if (blockSize == U_CX_XMODEM_BLOCK_SIZE_1K && offset < dataLen) {
            // For 1K blocks, give module time to write to flash
            // Use busy-wait since we're already in a timing-critical section
            int32_t delayEnd = uPortGetTickTimeMs() + 10;
            while (uPortGetTickTimeMs() < delayEnd) {
                // Busy wait
            }
        }
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, 
                    "XMODEM: === All blocks sent (%zu/%zu) - sending EOT ===",
                    blocksSent, totalBlocks);
    
    // Send EOT
    result = xmodemSendEot(pClient, pConfig->timeoutMs);
    if (result != 0) {
        return result;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: Transfer completed successfully");
    return 0;
}

int32_t uCxAtClientXmodemSendFile(uCxAtClient_t *pClient,
                                  const char *pFilePath,
                                  bool use1K,
                                  uCxXmodemProgressCallback_t progressCallback,
                                  void *pUserData)
{
    if (pClient == NULL || pFilePath == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Invalid parameters");
        return -1;
    }
    
    // Open file
    FILE *pFile = fopen(pFilePath, "rb");
    if (pFile == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Failed to open file: %s", pFilePath);
        return -1;
    }
    
    // Get file size
    fseek(pFile, 0, SEEK_END);
    long fileSize = ftell(pFile);
    fseek(pFile, 0, SEEK_SET);
    
    if (fileSize <= 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Invalid file size: %ld", fileSize);
        fclose(pFile);
        return -1;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "XMODEM: File size: %ld bytes", fileSize);
    
    // Allocate buffer
    uint8_t *pBuffer = (uint8_t *)malloc(fileSize);
    if (pBuffer == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Failed to allocate %ld bytes", fileSize);
        fclose(pFile);
        return -1;
    }
    
    // Read file
    size_t bytesRead = fread(pBuffer, 1, fileSize, pFile);
    fclose(pFile);
    
    if (bytesRead != (size_t)fileSize) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "XMODEM: Failed to read file (read %zu of %ld bytes)", 
                        bytesRead, fileSize);
        free(pBuffer);
        return -1;
    }
    
    // Configure XMODEM
    uCxXmodemConfig_t config;
    uCxAtClientXmodemConfigInit(&config);
    config.use1K = use1K;
    
    // Send file data
    int32_t result = uCxAtClientXmodemSend(pClient, pBuffer, fileSize, &config, 
                                           progressCallback, pUserData);
    
    // Cleanup
    free(pBuffer);
    
    return result;
}
