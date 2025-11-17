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

// Include u_port for port close/reopen
#include "u_port.h"

// Platform-specific sleep
#ifdef _WIN32
#include <windows.h>
#define SLEEP_MS(x) Sleep(x)
#else
#include <unistd.h>
#define SLEEP_MS(x) usleep((x) * 1000)
#endif

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
static int32_t enterFirmwareUpdateMode(uCxHandle_t *puCxHandle, int32_t baudRate, bool useFlowControl);

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
static int32_t enterFirmwareUpdateMode(uCxHandle_t *puCxHandle, int32_t baudRate, bool useFlowControl)
{
    char cmd[64];
    int32_t bytesWritten;
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Entering firmware update mode at %d baud (flow control: %s)...", 
                    baudRate, useFlowControl ? "ON" : "OFF");
    
    // Build AT+USYFWUS=<baudrate>[,<flow_control>] command
    // CRITICAL: This command does NOT return OK or ERROR!
    // It returns "\r\n> C" where:
    //   \r\n = newline
    //   >    = firmware update prompt
    //   C    = XMODEM CRC mode start signal
    // We must send this command directly without using the AT client parser!
    if (useFlowControl) {
        snprintf(cmd, sizeof(cmd), "AT+USYFWUS=%d,1\r", baudRate);
    } else {
        snprintf(cmd, sizeof(cmd), "AT+USYFWUS=%d\r", baudRate);
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Sending command: %s", cmd);
    
    // Send command directly (bypass AT client parser since no OK/ERROR response)
    bytesWritten = puCxHandle->pAtClient->pConfig->write(
        puCxHandle->pAtClient,
        puCxHandle->pAtClient->pConfig->pStreamHandle,
        cmd,
        strlen(cmd)
    );
    
    if (bytesWritten != (int32_t)strlen(cmd)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, 
                        "Failed to send firmware update command (wrote %d of %zu bytes)", 
                        bytesWritten, strlen(cmd));
        return U_CX_ERROR_IO;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Firmware update mode command sent successfully");
    
    return 0;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uCxFirmwareUpdate(uCxHandle_t *puCxHandle,
                          const char *pFirmwareFile,
                          const char *pDeviceName,
                          int32_t baudRate,
                          bool useFlowControl,
                          bool use1K,
                          uCxFirmwareUpdateProgress_t progressCallback,
                          void *pUserData)
{
    if (puCxHandle == NULL || pFirmwareFile == NULL || pDeviceName == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, "Invalid parameters");
        return -1;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "=== Firmware Update Started ===");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Firmware file: %s", pFirmwareFile);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Block size: %s bytes", use1K ? "1024 (1K)" : "128");
    
    // Use default baudrate if not specified
    if (baudRate <= 0) {
        baudRate = 115200;  // Default baudrate
    }
    
    // Enter firmware update mode - tell module what flow control to expect
    // AT+USYFWUS=<baudrate>[,<flow_control>]
    int32_t result = enterFirmwareUpdateMode(puCxHandle, baudRate, useFlowControl);
    if (result != 0) {
        return result;
    }
    
    // Give module time to process the command
    // Module responds with "\r\n> C" but we need to clear buffers
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Waiting for module to enter firmware update mode...");
    SLEEP_MS(1000);  // 1 second for module to switch modes
    
    // CRITICAL: Must close and reopen port to:
    // 1. Clear all RX/TX buffers (the AT response "\r\n> C" is in the buffer)
    // 2. Ensure clean state for XMODEM transfer
    // 3. This matches the working s-center tool behavior
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Closing port to clear buffers...");
    uPortAtClose(puCxHandle->pAtClient);
    
    SLEEP_MS(500);  // Wait for port to fully close
    
    // Reopen with the same flow control setting we told the module to use
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Reopening port (flow control: %s)...", 
                    useFlowControl ? "ON" : "OFF");
    if (!uPortAtOpen(puCxHandle->pAtClient, pDeviceName, baudRate, useFlowControl)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, "Failed to reopen port");
        return U_CX_ERROR_IO;
    }
    
    // Flush buffers to clear any stale data
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Flushing serial buffers for XMODEM transfer...");
    uPortAtFlush(puCxHandle->pAtClient);
    SLEEP_MS(100);
    uPortAtFlush(puCxHandle->pAtClient);
    SLEEP_MS(100);
    uPortAtFlush(puCxHandle->pAtClient);
    SLEEP_MS(100);
    uPortAtFlush(puCxHandle->pAtClient);
    SLEEP_MS(100);
    uPortAtFlush(puCxHandle->pAtClient);
    
    // CRITICAL: Pause the RX thread before XMODEM transfer!
    // The RX thread continuously calls uCxAtClientHandleRx() which reads bytes from the
    // serial port and tries to parse them as AT responses. During XMODEM transfer, this
    // causes ACK bytes (0x06) to be consumed before the XMODEM code can read them,
    // leading to timeouts, retries, and eventual module reboot with ERROR:46.
    // By pausing the RX thread, we give XMODEM exclusive access to the serial port.
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                    "Pausing RX thread for XMODEM transfer (prevents byte consumption)...");
    uPortAtPauseRx(puCxHandle->pAtClient);
    
    // Prepare progress callback context
    progressCallbackContext_t callbackContext;
    callbackContext.userCallback = progressCallback;
    callbackContext.pUserData = pUserData;
    
    // Transfer firmware via XMODEM
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Starting XMODEM transfer...");
    result = uCxAtClientXmodemSendFile(puCxHandle->pAtClient,
                                       pFirmwareFile,
                                       use1K,  // Block size mode
                                       xmodemProgressWrapper,
                                       &callbackContext);
    
    // Resume RX thread regardless of transfer result
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Resuming RX thread...");
    uPortAtResumeRx(puCxHandle->pAtClient);
    
    if (result != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, "XMODEM transfer failed: %d", result);
        return result;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "=== Firmware Update Complete ===");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, puCxHandle->pAtClient->instance, "Module will now reboot. Please close and reopen the connection.");
    
    return 0;
}

int32_t uCxFirmwareUpdateFromData(uCxHandle_t *puCxHandle,
                                  const uint8_t *pFirmwareData,
                                  size_t dataLen,
                                  int32_t baudRate,
                                  bool use1K,
                                  uCxFirmwareUpdateProgress_t progressCallback,
                                  void *pUserData)
{
    if (puCxHandle == NULL || pFirmwareData == NULL || dataLen == 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, "Invalid parameters");
        return -1;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "=== Firmware Update Started ===");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Firmware size: %zu bytes", dataLen);
    
    // Use default baudrate if not specified
    if (baudRate <= 0) {
        baudRate = 115200;
    }
    
    // Enter firmware update mode (no flow control for memory-based update)
    int32_t result = enterFirmwareUpdateMode(puCxHandle, baudRate, false);
    if (result != 0) {
        return result;
    }
    
    // Give module time to enter firmware mode
    SLEEP_MS(1000);
    
    // Prepare progress callback context
    progressCallbackContext_t callbackContext;
    callbackContext.userCallback = progressCallback;
    callbackContext.pUserData = pUserData;
    
    // Configure XMODEM
    uCxXmodemConfig_t config;
    uCxAtClientXmodemConfigInit(&config);
    config.use1K = use1K;
    
    // Transfer firmware via XMODEM
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Starting XMODEM transfer...");
    result = uCxAtClientXmodemSend(puCxHandle->pAtClient,
                                   pFirmwareData,
                                   dataLen,
                                   &config,
                                   xmodemProgressWrapper,
                                   &callbackContext);
    
    if (result != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, puCxHandle->pAtClient->instance, "XMODEM transfer failed: %d", result);
        return result;
    }
    
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "=== Firmware Update Complete ===");
    U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, puCxHandle->pAtClient->instance, "Module will now reboot. Please close and reopen the connection.");
    
    return 0;
}
