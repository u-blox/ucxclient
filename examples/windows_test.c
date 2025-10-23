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
 * @brief Windows console test application for ucxclient
 *
 * This is a simple console application to test the Windows port of ucxclient.
 * It can be used to verify COM port connectivity and basic AT commands.
 *
 * Usage: windows_test.exe [COM_PORT] [SSID] [WPA_PSK]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>  // For _kbhit() and _getch()

#include "u_port.h"
#include "u_cx_at_client.h"
#include "u_cx_at_util.h"
#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_urc.h"
#include "u_cx_wifi.h"
#include "u_cx_system.h"
#include "u_port_windows.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define DEFAULT_COM_PORT "COM3"
#define DEFAULT_SSID "ubx"
#define DEFAULT_WPA_PSK ""

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void printUsage(const char *pProgName)
{
    printf("Usage: %s [COM_PORT] [SSID] [WPA_PSK]\n", pProgName);
    printf("  COM_PORT: Windows COM port (e.g., COM3, COM4) - default: %s\n", DEFAULT_COM_PORT);
    printf("  SSID:     WiFi network name - default: %s\n", DEFAULT_SSID);
    printf("  WPA_PSK:  WiFi password - default: (empty)\n");
    printf("\nExample: %s COM4 MyNetwork MyPassword\n", pProgName);
    printf("\nAvailable COM ports:\n");
    
    // List available COM ports
    char portList[32][16];
    int32_t portCount = uPortEnumerateComPorts(portList, 32);
    
    if (portCount > 0) {
        for (int32_t i = 0; i < portCount; i++) {
            bool available = uPortIsComPortAvailable(portList[i]);
            printf("  %s %s\n", portList[i], available ? "(available)" : "(in use)");
        }
    } else {
        printf("  No COM ports found\n");
    }
}

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    printf("*** Network UP ***\n");
}

static void networkDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    printf("*** Network DOWN ***\n");
}

static void sleepMs(int32_t timeMs)
{
    Sleep(timeMs);
}

static bool waitForKey(int32_t timeoutMs)
{
    int32_t startTime = GetTickCount();
    
    while ((GetTickCount() - startTime) < timeoutMs) {
        if (_kbhit()) {
            _getch(); // Consume the key
            return true;
        }
        Sleep(10);
    }
    return false;
}

/* ----------------------------------------------------------------
 * MAIN
 * -------------------------------------------------------------- */

int main(int argc, char *argv[])
{
    const char *pComPort = DEFAULT_COM_PORT;
    const char *pSsid = DEFAULT_SSID;
    const char *pWpaPsk = DEFAULT_WPA_PSK;
    
    uCxAtClient_t client;
    uCxHandle_t ucxHandle;
    int32_t status;

    printf("=== ucxclient Windows Test Application ===\n\n");

    // Parse command line arguments
    if (argc > 1) {
        if (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        }
        pComPort = argv[1];
    }
    if (argc > 2) {
        pSsid = argv[2];
    }
    if (argc > 3) {
        pWpaPsk = argv[3];
    }

    printf("Configuration:\n");
    printf("  COM Port: %s\n", pComPort);
    printf("  SSID:     %s\n", pSsid);
    printf("  WPA PSK:  %s\n", strlen(pWpaPsk) > 0 ? "***" : "(none)");
    printf("\n");

    // Check if COM port is available
    if (!uPortIsComPortAvailable(pComPort)) {
        printf("ERROR: COM port %s is not available or in use\n", pComPort);
        printf("Available ports:\n");
        printUsage(argv[0]);
        return 1;
    }

    // Initialize AT client
    printf("Initializing AT client...\n");
    uPortAtInit(&client);

    // Open COM port
    printf("Opening %s at 115200 baud with flow control...\n", pComPort);
    if (!uPortAtOpen(&client, pComPort, 115200, true)) {
        printf("ERROR: Failed to open %s\n", pComPort);
        return 1;
    }

    printf("COM port opened successfully!\n\n");

    // Initialize uCx API
    uCxInit(&client, &ucxHandle);
    
    // Register network callbacks
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxWifiRegisterStationNetworkDown(&ucxHandle, networkDownUrc);

    printf("Testing basic AT commands...\n");

    // Test basic connectivity
    printf("1. Testing AT command response...\n");
    status = uCxAtClientExecSimpleCmd(&client, "AT");
    printf("   AT response: %s\n", (status == 0) ? "OK" : "ERROR");

    if (status != 0) {
        printf("ERROR: No response from module. Check connections and power.\n");
        goto cleanup;
    }

    // Get module information
    printf("\n2. Getting module information...\n");
    status = uCxAtClientExecSimpleCmd(&client, "ATI");
    printf("   Module info query: %s\n", (status == 0) ? "OK" : "ERROR");

    // Disable echo
    printf("\n3. Disabling echo...\n");
    status = uCxSystemSetEchoOff(&ucxHandle);
    printf("   Echo off: %s\n", (status == 0) ? "OK" : "ERROR");

    // Get UART settings
    printf("\n4. Reading UART settings...\n");
    uCxSystemGetUartSettings_t uartSettings;
    status = uCxSystemGetUartSettings(&ucxHandle, &uartSettings);
    if (status == 0) {
        printf("   UART: %d baud, flow control: %d\n", 
               uartSettings.baud_rate, uartSettings.flow_control);
    } else {
        printf("   UART settings query failed\n");
    }

    // WiFi test (if credentials provided)
    if (strlen(pWpaPsk) > 0) {
        printf("\n5. WiFi connection test...\n");
        printf("   Setting up WiFi security...\n");
        status = uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WPA_THRESHOLD_WPA2);
        printf("   WiFi security: %s\n", (status == 0) ? "OK" : "ERROR");

        if (status == 0) {
            printf("   Setting connection parameters...\n");
            status = uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
            printf("   Connection params: %s\n", (status == 0) ? "OK" : "ERROR");

            if (status == 0) {
                printf("   Connecting to WiFi (this may take a moment)...\n");
                status = uCxWifiStationConnect(&ucxHandle, 0);
                printf("   WiFi connect: %s\n", (status == 0) ? "OK" : "ERROR");

                if (status == 0) {
                    printf("   Waiting for network up event (10 seconds)...\n");
                    sleepMs(10000);
                }
            }
        }
    } else {
        printf("\n5. WiFi test skipped (no password provided)\n");
    }

    printf("\n=== Test completed ===\n");
    printf("Press any key to exit (or wait 5 seconds)...\n");
    
    if (!waitForKey(5000)) {
        printf("Timeout - exiting...\n");
    }

cleanup:
    printf("\nCleaning up...\n");
    uPortAtClose(&client);
    printf("Done.\n");
    
    return 0;
}