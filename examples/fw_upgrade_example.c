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
 * @brief Example of how to perform firmware upgrade using XMODEM
 *
 * This example demonstrates upgrading module firmware using the
 * AT+USYFWUS command followed by XMODEM protocol transfer.
 *
 * Execute with following args:
 * fw_upgrade_example <uart_device> <firmware_file>
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include "u_cx.h"
#include "u_cx_system.h"
#include "u_cx_general.h"
#include "u_cx_xmodem.h"
#include "u_cx_log.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define FW_UPGRADE_BAUD     921600
#define FW_UPGRADE_TIMEOUT  15000

#define URC_FLAG_MODULE_STARTED  (1 << 0)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    FILE *pFile;
    size_t fileSize;
} FirmwareContext_t;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * Startup event callback - called when module boots up
 */
static void startupCallback(uCxHandle_t *puCxHandle)
{
    (void)puCxHandle;
    printf("Module startup event received\n");
    exampleSignalEvent(URC_FLAG_MODULE_STARTED);
}

/**
 * Progress callback for firmware transfer
 */
static void progressCallback(size_t totalBytes, size_t bytesTransferred, void *pUserData)
{
    (void)pUserData;
    int percent = (int)((bytesTransferred * 100) / totalBytes);
    printf("\rFirmware upgrade progress: %d%% (%zu/%zu bytes)",
           percent, bytesTransferred, totalBytes);
    fflush(stdout);

    if (bytesTransferred >= totalBytes) {
        printf("\n");
    }
}

/**
 * Data callback for XMODEM transfer
 */
static int32_t firmwareDataCallback(uint8_t *pBuffer, size_t offset, size_t maxLen, void *pUserData)
{
    FirmwareContext_t *pCtx = (FirmwareContext_t *)pUserData;

    if (pCtx == NULL || pCtx->pFile == NULL) {
        return -1;
    }

    // Check if we've reached end of file
    if (offset >= pCtx->fileSize) {
        return 0;
    }

    // Seek to offset
    if (fseek(pCtx->pFile, (long)offset, SEEK_SET) != 0) {
        return -1;
    }

    // Read data
    size_t remaining = pCtx->fileSize - offset;
    size_t toRead = (maxLen < remaining) ? maxLen : remaining;
    size_t bytesRead = fread(pBuffer, 1, toRead, pCtx->pFile);

    return (int32_t)bytesRead;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char **argv)
{
    int32_t result;
    uCxHandle_t ucxHandle;

#ifdef U_PORT_POSIX
    if (argc != 3) {
        printf("Usage: %s <uart_device> <firmware_file>\n", argv[0]);
        printf("Example: %s /dev/ttyUSB0 firmware.bin\n", argv[0]);
        return -1;
    }

    const char *pUartDev = argv[1];
    const char *pFirmwareFile = argv[2];
#else
    (void)argc;
    (void)argv;
    // In no-OS mode, use hardcoded values (would typically come from config)
    const char *pUartDev = U_EXAMPLE_UART;
    const char *pFirmwareFile = "firmware.bin";

    printf("No-OS firmware upgrade mode\n");
    printf("UART device: %s\n", pUartDev);
    printf("Firmware file: %s\n", pFirmwareFile);
#endif

    // Get file size
    struct stat st;
    if (stat(pFirmwareFile, &st) != 0) {
        printf("ERROR: Cannot stat file: %s\n", pFirmwareFile);
        return -1;
    }

    // Open firmware file
    FILE *pFile = fopen(pFirmwareFile, "rb");
    if (pFile == NULL) {
        printf("ERROR: Cannot open file: %s\n", pFirmwareFile);
        return -1;
    }

    printf("Firmware file: %s (%ld bytes)\n", pFirmwareFile, (long)st.st_size);

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pUartDev, 115200, true);
    if (pClient == NULL) {
        fclose(pFile);
        return -1;
    }

    // Initialize uCX API
    uCxInit(pClient, &ucxHandle);

    // Check communication with module
    printf("Checking module communication...\n");
    result = uCxGeneralAttention(&ucxHandle);
    if (result != 0) {
        printf("ERROR: No response from module (AT command failed: %d)\n", result);
        printf("Please check:\n");
        printf("  - Module is powered on\n");
        printf("  - UART connection is correct\n");
        printf("  - Baud rate is 115200\n");
        uCxAtClientClose(pClient);
        uCxAtClientDeinit(pClient);
        fclose(pFile);
        return -1;
    }
    printf("Module communication OK\n");

    printf("Starting firmware upgrade...\n");

    // Issue firmware update command to enter bootloader mode
    result = uCxSystemStartSerialFirmwareUpdate2(&ucxHandle, FW_UPGRADE_BAUD, 1);
    if (result != 0) {
        printf("ERROR: AT+USYFWUS failed: %d\n", result);
        uCxAtClientClose(pClient);
        uCxAtClientDeinit(pClient);
        fclose(pFile);
        return -1;
    }

    printf("Module entered firmware update mode\n");

    // Close AT client UART - we'll reopen it at higher baud for XMODEM
    uCxAtClientClose(pClient);

    // Initialize XMODEM
    uCxXmodemConfig_t xmodemConfig;
    uCxXmodemInit(pUartDev, &xmodemConfig);
    xmodemConfig.use1K = true;
    xmodemConfig.timeoutMs = FW_UPGRADE_TIMEOUT;

    // Open XMODEM UART at firmware upgrade baud rate
    result = uCxXmodemOpen(&xmodemConfig, FW_UPGRADE_BAUD, true);
    if (result != 0) {
        printf("ERROR: Failed to open XMODEM UART: %d\n", result);
        uCxAtClientOpen(pClient, 115200, false);
        fclose(pFile);
        return -1;
    }

    // Set up firmware context
    FirmwareContext_t fwCtx = {
        .pFile = pFile,
        .fileSize = (size_t)st.st_size
    };

    // Perform XMODEM transfer
    printf("Starting XMODEM transfer...\n");
    result = uCxXmodemSend(&xmodemConfig, fwCtx.fileSize,
                          firmwareDataCallback, progressCallback, &fwCtx);

    // Close XMODEM
    uCxXmodemClose(&xmodemConfig);

    // Close firmware file
    fclose(pFile);

    if (result != 0) {
        printf("ERROR: XMODEM transfer failed: %d\n", result);
        // Try to reopen AT client
        uCxAtClientOpen(pClient, 115200, false);
        return -1;
    }

    printf("Firmware upgrade completed successfully!\n");
    printf("Module will reboot automatically.\n");

    // Reopen AT client at default baud (module resets to 115200)
    printf("Waiting for module to reboot...\n");

    result = uCxAtClientOpen(pClient, 115200, false);
    if (result != 0) {
        printf("ERROR: Failed to reopen AT client: %d\n", result);
        printf("You may need to power cycle the module.\n");
        uCxAtClientDeinit(pClient);
        uPortDeinit();
        return -1;
    }

    // Re-initialize uCX API and register for startup event
    uCxInit(pClient, &ucxHandle);
    uCxGeneralRegisterStartup(&ucxHandle, startupCallback);

    // Wait for module startup event (with timeout)
    printf("Waiting for module startup event...\n");
    if (exampleWaitEvent(URC_FLAG_MODULE_STARTED, 30)) {
        printf("Module is back online and ready\n");
    } else {
        printf("WARNING: Timeout waiting for module startup\n");
        printf("Module may still be booting or requires power cycle.\n");
    }

    // Cleanup
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;
}
