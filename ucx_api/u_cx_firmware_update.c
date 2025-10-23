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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "u_cx_firmware_update.h"
#include "u_cx_system.h"
#include "u_cx_at_client.h"
#include "u_cx_at_xmodem.h"
#include "u_cx_log.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define FIRMWARE_UPDATE_TIMEOUT_MS  60000  /**< Timeout for entering update mode */

/* ----------------------------------------------------------------
 * TYPES
 * ---------------------------------------------------------- */

typedef struct {
    uCxFirmwareUpdateProgress_t userCallback;
    void *pUserData;
} progressCallbackContext_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static void xmodemProgressWrapper(size_t totalBytes, size_t bytesTransferred, void *pUserData);
static int32_t changeBaudRate(uCxHandle_t *puCxHandle, int32_t newBaudRate);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * Wrapper for XMODEM progress callback to add percentage calculation
 */
static void xmodemProgressWrapper(size_t totalBytes, size_t bytesTransferred, void *pUserData)
{
    progressCallbackContext_t *pContext = (progressCallbackContext_t *)pUserData;
    
    if (pContext != NULL && pContext->userCallback != NULL) {
        int32_t percent = (int32_t)((bytesTransferred * 100) / totalBytes);
        pContext->userCallback(totalBytes, bytesTransferred, percent, pContext->pUserData);
    }
}

/**
 * Change UART baudrate
 * 
 * Note: This requires platform-specific implementation to actually change
 * the host's UART baudrate. This function only sends the AT command.
 */
static int32_t changeBaudRate(uCxHandle_t *puCxHandle, int32_t newBaudRate)
{
    char cmd[64];
    int32_t result;
    
    U_CX_LOG_LINE_I("Changing baudrate to %d...", newBaudRate);
    
    // Build AT+USYST=<baudrate>,<flowctrl> command
    snprintf(cmd, sizeof(cmd), "AT+USYST=%d,1", newBaudRate);
    
    // Send command
    result = uCxAtClientExecSimpleCmd(puCxHandle->pAtClient, cmd, NULL);
    if (result != 0) {
        U_CX_LOG_LINE_E("Failed to change baudrate: %d", result);
        return result;
    }
    
    U_CX_LOG_LINE_I("Baudrate command sent successfully");
    U_CX_LOG_LINE_W("NOTE: You must manually reconfigure your host UART to %d baud", newBaudRate);
    
    return 0;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uCxFirmwareUpdate(uCxHandle_t *puCxHandle,
                          const char *pFirmwareFile,
                          int32_t baudRate,
                          uCxFirmwareUpdateProgress_t progressCallback,
                          void *pUserData)
{
    if (puCxHandle == NULL || pFirmwareFile == NULL) {
        U_CX_LOG_LINE_E("Invalid parameters");
        return -1;
    }
    
    U_CX_LOG_LINE_I("=== Firmware Update Started ===");
    U_CX_LOG_LINE_I("Firmware file: %s", pFirmwareFile);
    
    // Change baudrate if requested
    if (baudRate > 0) {
        int32_t result = changeBaudRate(puCxHandle, baudRate);
        if (result != 0) {
            return result;
        }
        
        // Give time for baudrate change
        uCxAtClientDelayMs(puCxHandle->pAtClient, 500);
    }
    
    // Prepare progress callback context
    progressCallbackContext_t callbackContext = {
        .userCallback = progressCallback,
        .pUserData = pUserData
    };
    
    // Transfer firmware via XMODEM
    U_CX_LOG_LINE_I("Starting XMODEM transfer...");
    int32_t result = uCxXmodemSendFile(puCxHandle->pAtClient,
                                       pFirmwareFile,
                                       true,  // Use 1K blocks for faster transfer
                                       xmodemProgressWrapper,
                                       &callbackContext);
    
    if (result != 0) {
        U_CX_LOG_LINE_E("XMODEM transfer failed: %d", result);
        return result;
    }
    
    U_CX_LOG_LINE_I("=== Firmware Update Complete ===");
    U_CX_LOG_LINE_W("Module will now reboot. Please close and reopen the connection.");
    
    return 0;
}

int32_t uCxFirmwareUpdateFromData(uCxHandle_t *puCxHandle,
                                  const uint8_t *pFirmwareData,
                                  size_t dataLen,
                                  int32_t baudRate,
                                  uCxFirmwareUpdateProgress_t progressCallback,
                                  void *pUserData)
{
    if (puCxHandle == NULL || pFirmwareData == NULL || dataLen == 0) {
        U_CX_LOG_LINE_E("Invalid parameters");
        return -1;
    }
    
    U_CX_LOG_LINE_I("=== Firmware Update Started ===");
    U_CX_LOG_LINE_I("Firmware size: %zu bytes", dataLen);
    
    // Change baudrate if requested
    if (baudRate > 0) {
        int32_t result = changeBaudRate(puCxHandle, baudRate);
        if (result != 0) {
            return result;
        }
        
        // Give time for baudrate change
        uCxAtClientDelayMs(puCxHandle->pAtClient, 500);
    }
    
    // Prepare progress callback context
    progressCallbackContext_t callbackContext = {
        .userCallback = progressCallback,
        .pUserData = pUserData
    };
    
    // Configure XMODEM
    uCxXmodemConfig_t config;
    uCxXmodemConfigInit(&config);
    config.use1K = true;  // Use 1K blocks for faster transfer
    
    // Transfer firmware via XMODEM
    U_CX_LOG_LINE_I("Starting XMODEM transfer...");
    int32_t result = uCxXmodemSend(puCxHandle->pAtClient,
                                   pFirmwareData,
                                   dataLen,
                                   &config,
                                   xmodemProgressWrapper,
                                   &callbackContext);
    
    if (result != 0) {
        U_CX_LOG_LINE_E("XMODEM transfer failed: %d", result);
        return result;
    }
    
    U_CX_LOG_LINE_I("=== Firmware Update Complete ===");
    U_CX_LOG_LINE_W("Module will now reboot. Please close and reopen the connection.");
    
    return 0;
}
