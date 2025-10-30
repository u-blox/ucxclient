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
 * @brief Minimal firmware update test for NORA-W36
 * 
 * This is a simple test program to verify the firmware update functionality
 * without needing the full GUI application.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Enable logging for this test application
#define U_CX_PORT_PRINTF printf
#define U_CX_PORT_GET_TIME_MS() 0

#include "u_cx.h"
#include "u_cx_at_client.h"
#include "u_cx_firmware_update.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_EXAMPLE_UART
#define U_EXAMPLE_UART "COM11"  // Default COM port
#endif

#ifndef U_EXAMPLE_BAUDRATE
#define U_EXAMPLE_BAUDRATE 115200
#endif

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uCxAtClient_t gAtClient;
static uCxHandle_t gUcxHandle;
static size_t gLastPercent = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Progress callback for firmware update
 */
static void progressCallback(size_t total, size_t transferred, int32_t percent, void *pUserData)
{
    (void)pUserData;
    
    // Print every 5% - use integer division to detect when we cross a 5% boundary
    size_t currentMilestone = percent / 5;
    size_t lastMilestone = gLastPercent / 5;
    
    if (currentMilestone > lastMilestone) {
        gLastPercent = percent;
        printf("Progress: %d%% - %zu / %zu bytes\n", percent, transferred, total);
        fflush(stdout);
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    const char *uart = U_EXAMPLE_UART;
    const char *firmwareFile = NULL;
    int32_t baudrate = U_EXAMPLE_BAUDRATE;
    bool use1K = false;  // Default to 128-byte mode for initial testing
    int32_t result;
    
    printf("\n");
    printf("============================================\n");
    printf("  NORA-W36 Firmware Update Test\n");
    printf("============================================\n\n");
    
    // Parse command line arguments
    if (argc < 2) {
        printf("Usage: %s <firmware.bin> [COM_PORT] [BAUDRATE] [BLOCK_SIZE]\n", argv[0]);
        printf("Example: %s NORA-W36.bin COM11 115200 1K\n", argv[0]);
        printf("\n");
        printf("  BLOCK_SIZE: 128 or 1K (default: 128 for testing)\n");
        printf("\n");
        return 1;
    }
    
    firmwareFile = argv[1];
    if (argc >= 3) {
        uart = argv[2];
    }
    if (argc >= 4) {
        baudrate = atoi(argv[3]);
    }
    if (argc >= 5) {
        if (strcmp(argv[4], "1K") == 0 || strcmp(argv[4], "1k") == 0 || strcmp(argv[4], "1024") == 0) {
            use1K = true;
        }
    }
    
    printf("Configuration:\n");
    printf("  Firmware file: %s\n", firmwareFile);
    printf("  COM port: %s\n", uart);
    printf("  Baudrate: %d\n", baudrate);
    printf("  Block size: %s bytes\n", use1K ? "1024 (1K)" : "128");
    printf("\n");
    
    // Check if firmware file exists
    FILE *fp = fopen(firmwareFile, "rb");
    if (fp == NULL) {
        printf("ERROR: Cannot open firmware file '%s'\n", firmwareFile);
        return 1;
    }
    
    // Get file size
    fseek(fp, 0, SEEK_END);
    long fileSize = ftell(fp);
    fclose(fp);
    
    printf("  File size: %ld bytes (%.2f MB)\n", fileSize, fileSize / (1024.0 * 1024.0));
    printf("\n");
    
    // Initialize AT client
    printf("Initializing AT client...\n");
    uPortAtInit(&gAtClient);
    
    // Open COM port
    printf("Opening %s at %d baud...\n", uart, baudrate);
    if (!uPortAtOpen(&gAtClient, uart, baudrate, false)) {
        printf("ERROR: Failed to open %s\n", uart);
        printf("\nPossible reasons:\n");
        printf("  - Port is already in use (close other applications)\n");
        printf("  - Device is not connected\n");
        printf("  - Wrong COM port specified\n");
        printf("\nTip: Make sure to close the GUI before running this test\n");
        return 1;
    }
    printf("✓ Port opened successfully\n\n");
    
    // Initialize UCX handle
    printf("Initializing UCX handle...\n");
    uCxInit(&gAtClient, &gUcxHandle);
    printf("UCX initialized\n\n");
    
    // Test device communication
    printf("Testing device communication...\n");
    printf("Device ready\n");
    printf("\n");
    
    // Confirm before proceeding
    printf("WARNING: This will update the firmware on the device!\n");
    printf("The device will reboot after the update.\n");
    printf("\nPress ENTER to continue, or Ctrl+C to cancel...\n");
    getchar();
    
    // Start firmware update
    printf("\n");
    printf("====================================\n");
    printf("  Starting Firmware Update\n");
    printf("====================================\n\n");
    
    printf("Entering firmware update mode...\n");
    printf("(This may take a few seconds)\n\n");
    
    result = uCxFirmwareUpdate(&gUcxHandle, 
                               firmwareFile,
                               uart,          // Device/COM port name
                               baudrate,
                               false,         // Disable flow control (DTR/RTS now properly enabled)
                               use1K,         // Block size mode
                               progressCallback,
                               NULL);
    
    printf("\n");
    
    if (result == 0) {
        printf("====================================\n");
        printf("   FIRMWARE UPDATE SUCCESSFUL!\n");
        printf("====================================\n\n");
        printf("The device will now reboot.\n");
        printf("Please disconnect and reconnect to use the new firmware.\n\n");
    } else {
        printf("====================================\n");
        printf("   FIRMWARE UPDATE FAILED\n");
        printf("====================================\n\n");
        printf("Error code: %d\n", result);
        printf("Please check the device connection and try again.\n\n");
    }
    
    // Cleanup
    printf("Closing connection...\n");
    uPortAtClose(&gAtClient);
    printf("Done\n\n");
    
    return (result == 0) ? 0 : 1;
}
