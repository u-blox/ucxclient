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
 * @brief Example of using TCP sockets with the uCx API
 *
 * This example will:
 * - Connect to WiFi
 * - Create a TCP socket
 * - Connect to a remote server
 * - Send a simple HTTP GET request
 * - Read and display the response
 * - Close the socket
 *
 * Execute with following args:
 * socket_example [uart_device] [wifi_ssid] [wifi_psk]
 *
 * All arguments are optional and default to U_EXAMPLE_UART, U_EXAMPLE_SSID,
 * and U_EXAMPLE_WPA_PSK from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_socket.h"
#include "u_cx_system.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define EXAMPLE_HOST "httpbin.org"
#define EXAMPLE_PORT 80

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_SOCKET_CONNECTED   (1 << 1)
#define URC_FLAG_SOCKET_DATA        (1 << 2)
#define URC_FLAG_SOCKET_CLOSED      (1 << 3)

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static int32_t gSocketHandle = -1;
static int32_t gBytesAvailable = 0;

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    printf("WiFi network is up\n");
    exampleSignalEvent(URC_FLAG_NETWORK_UP);
}

static void socketConnectUrc(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    (void)puCxHandle;
    printf("Socket %d connected\n", (int)socket_handle);
    exampleSignalEvent(URC_FLAG_SOCKET_CONNECTED);
}

static void socketDataAvailableUrc(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)socket_handle;
    gBytesAvailable = number_bytes;
    printf("Socket data available: %d bytes\n", (int)number_bytes);
    exampleSignalEvent(URC_FLAG_SOCKET_DATA);
}

static void socketClosedUrc(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    (void)puCxHandle;
    printf("Socket %d closed\n", (int)socket_handle);
    exampleSignalEvent(URC_FLAG_SOCKET_CLOSED);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "socket_example",
        "Example of using TCP sockets with the uCx API.\n"
        "Connects to WiFi, opens a TCP socket, and sends an HTTP GET request.",
        "[uart_device] [wifi_ssid] [wifi_psk]");

    uCxHandle_t ucxHandle;
    int32_t ret;
    const char *pDevice = U_EXAMPLE_UART;
    const char *pSsid = U_EXAMPLE_SSID;
    const char *pWpaPsk = U_EXAMPLE_WPA_PSK;

    if (argc >= 2) {
        pDevice = argv[1];
    }
    if (argc >= 3) {
        pSsid = argv[2];
    }
    if (argc >= 4) {
        pWpaPsk = argv[3];
    }

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - connection will not work");
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN,
            "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.");
    }

    printf("===========================================\n");
    printf("TCP Socket Example\n");
    printf("===========================================\n");

    // Initialize example utilities and AT client
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, true);
    if (pClient == NULL) {
        return 1;
    }

    uCxInit(pClient, &ucxHandle);

    // Register URC callbacks
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxSocketRegisterConnect(&ucxHandle, socketConnectUrc);
    uCxSocketRegisterDataAvailable(&ucxHandle, socketDataAvailableUrc);
    uCxSocketRegisterClosed(&ucxHandle, socketClosedUrc);

    // Reboot module for clean state
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucxHandle);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Connect to WiFi
    printf("Connecting to WiFi: %s\n", pSsid);
    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);

    if (!exampleWaitEvent(URC_FLAG_NETWORK_UP, 20)) {
        printf("ERROR: Failed to connect to WiFi\n");
        goto cleanup;
    }

    // Create TCP socket
    printf("Creating TCP socket...\n");
    ret = uCxSocketCreate1(&ucxHandle, U_SOCKET_PROTOCOL_TCP, &gSocketHandle);
    if (ret != 0) {
        printf("ERROR: Failed to create socket: %" PRId32 "\n", ret);
        goto cleanup;
    }
    printf("Socket created: %d\n", (int)gSocketHandle);

    // Connect to remote server
    printf("Connecting to %s:%d...\n", EXAMPLE_HOST, EXAMPLE_PORT);
    ret = uCxSocketConnect(&ucxHandle, gSocketHandle, EXAMPLE_HOST, EXAMPLE_PORT);
    if (ret != 0) {
        printf("ERROR: Failed to initiate connection: %" PRId32 "\n", ret);
        goto cleanup;
    }

    // Wait for connection
    if (!exampleWaitEvent(URC_FLAG_SOCKET_CONNECTED, 10)) {
        printf("ERROR: Connection timeout\n");
        goto cleanup;
    }

    // Send HTTP GET request
    const char *httpRequest = "GET /get HTTP/1.1\r\n"
                              "Host: " EXAMPLE_HOST "\r\n"
                              "Connection: close\r\n"
                              "\r\n";

    printf("Sending HTTP request...\n");
    ret = uCxSocketWrite(&ucxHandle, gSocketHandle,
                         (const uint8_t *)httpRequest, (int32_t)strlen(httpRequest));
    if (ret < 0) {
        printf("ERROR: Failed to send data: %" PRId32 "\n", ret);
        goto cleanup;
    }
    printf("Sent %d bytes\n", (int)ret);

    // Read response
    printf("\n--- HTTP Response ---\n");
    uint8_t rxBuffer[512];
    bool socketOpen = true;

    while (socketOpen) {
        // Wait for data or socket close
        if (exampleWaitEventEx(URC_FLAG_SOCKET_DATA | URC_FLAG_SOCKET_CLOSED, 5, true)) {
            // Check if socket was closed
            if (gBytesAvailable == 0) {
                socketOpen = false;
                break;
            }

            // Read available data
            while (gBytesAvailable > 0) {
                int32_t toRead = gBytesAvailable;
                if (toRead > (int32_t)sizeof(rxBuffer) - 1) {
                    toRead = sizeof(rxBuffer) - 1;
                }

                ret = uCxSocketRead(&ucxHandle, gSocketHandle, toRead, rxBuffer);
                if (ret > 0) {
                    rxBuffer[ret] = '\0';
                    printf("%s", rxBuffer);
                    gBytesAvailable -= ret;
                } else {
                    break;
                }
            }
        } else {
            // Timeout - check if socket is still open
            uCxSocketGetStatus_t status;
            if (uCxSocketGetStatus(&ucxHandle, gSocketHandle, &status) == 0) {
                if (status.status != 1) {  // Not connected
                    socketOpen = false;
                }
            } else {
                socketOpen = false;
            }
        }
    }
    printf("\n--- End of Response ---\n");

cleanup:
    // Close socket if open
    if (gSocketHandle >= 0) {
        printf("Closing socket...\n");
        uCxSocketClose(&ucxHandle, gSocketHandle);
    }

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    printf("Done.\n");
    return 0;
}
