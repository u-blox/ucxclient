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
 * @brief Implementation of example utility functions
 *
 * This module provides common utilities for examples that work seamlessly
 * in both OS and no-OS environments. The implementation automatically detects
 * the build configuration using compile-time macros (U_PORT_NO_OS) and
 * adapts the behavior accordingly.
 */

#include "example_utils.h"
#include "u_cx_log.h"
#include "u_cx_at_client.h"
#include "u_port_uart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static U_CX_MUTEX_HANDLE gEventMutex;
static volatile uint32_t gEventFlags = 0;
static bool gEventMutexInitialized = false;
static uCxAtClient_t *gpClient = NULL;

// AT client buffers and config (must be static!)
static char gRxBuf[U_EXAMPLE_AT_RX_BUFFER_SIZE];
static char gUrcBuf[U_EXAMPLE_AT_RX_BUFFER_SIZE];
static uCxAtClient_t gClient;
static uCxAtClientConfig_t gConfig;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uCxAtClient_t *exampleInit(const char *pUartDevice,
                           int32_t baudRate, bool flowControl)
{
    // Initialize port layer
    uPortInit();

    // Initialize AT client with buffers and UART device name
    gConfig.pRxBuffer = gRxBuf;
    gConfig.rxBufferLen = sizeof(gRxBuf);
    gConfig.pUrcBuffer = gUrcBuf;
    gConfig.urcBufferLen = sizeof(gUrcBuf);
    gConfig.pUartDevName = pUartDevice;

    uCxAtClientInit(&gConfig, &gClient);

    // Open UART connection
    if (uCxAtClientOpen(&gClient, baudRate, flowControl) != 0) {
        fprintf(stderr, "Failed to open UART: %s\n", pUartDevice);
        return NULL;
    }

#if defined(U_PORT_WINDOWS)
    // Surface FTDI latency timer / IO mode so silent misconfiguration is visible
    uPortUartPrintDiagnostics(pUartDevice);
#endif

    // Store client pointer for use in other functions
    gpClient = &gClient;

    return &gClient;
}

bool exampleWaitEventEx(uint32_t evtFlag, uint32_t timeoutS, bool silent)
{
    int32_t timeoutMs = (int32_t)timeoutS * 1000;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    if (!gEventMutexInitialized) {
        U_CX_MUTEX_CREATE(gEventMutex);
        gEventMutexInitialized = true;
    }

    if (!silent) {
        U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "waitEvent(%" PRIu32 ", %" PRIu32 ")", evtFlag, timeoutS);
    }
    do {
        // In no-OS mode, we need to manually handle RX to process URCs
#if EXAMPLE_NO_OS_MODE
        if (gpClient != NULL) {
            uCxAtClientHandleRx(gpClient);
        }
#endif

        U_CX_MUTEX_TRY_LOCK(gEventMutex, 100);
        if (gEventFlags & evtFlag) {
            gEventFlags &= ~evtFlag;  // Clear the flag
            return true;
        }
    } while (U_CX_PORT_GET_TIME_MS() - startTime < timeoutMs);

    if (!silent) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Timeout waiting for: %" PRIu32, evtFlag);
    }
    return false;
}

bool exampleWaitEvent(uint32_t evtFlag, uint32_t timeoutS)
{
    return exampleWaitEventEx(evtFlag, timeoutS, false);
}

void exampleSignalEvent(uint32_t evtFlag)
{
    if (!gEventMutexInitialized) {
        U_CX_MUTEX_CREATE(gEventMutex);
        gEventMutexInitialized = true;
    }

    gEventFlags |= evtFlag;
    U_CX_MUTEX_UNLOCK(gEventMutex);
}

void exampleCheckHelp(int argc, char **argv, const char *exampleName,
                      const char *description, const char *usage)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("%s\n\n", exampleName);
            printf("%s\n\n", description);
            printf("Usage: %s %s\n\n", exampleName, usage);
            printf("All arguments are optional and default to values from config.local.h\n");
            printf("(U_EXAMPLE_UART, U_EXAMPLE_SSID, U_EXAMPLE_WPA_PSK)\n");
            exit(0);
        }
    }
}
