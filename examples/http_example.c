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
 * @brief Example of how to do a HTTP GET request using the uCx API
 *
 * This example will:
 * - Setup WiFi
 * - Create a TCP socket and connect to EXAMPLE_URL
 * - Send "GET /"
 * - Print server response output
 *
 * This example works with both OS and no-OS configurations.
 * The build system creates two executables from this same source:
 * - http_example: Uses POSIX port (with background RX task)
 * - http_example_no_os: Uses no-OS port (manual RX polling)
 *
 * Execute with following args:
 * http_example [uart_device] [wifi_ssid] [wifi_psk]
 *
 * All arguments are optional and default to U_EXAMPLE_UART, U_EXAMPLE_SSID,
 * and U_EXAMPLE_WPA_PSK from config.local.h.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_http.h"
#include "u_cx_system.h"
#include "u_cx_general.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* HTTP download target - override in config.local.h to change */
#ifndef U_EXAMPLE_HTTP_URL
# define U_EXAMPLE_HTTP_URL  "https://staging.ampnet.autometer.com"
#endif
#ifndef U_EXAMPLE_HTTP_PATH
# define U_EXAMPLE_HTTP_PATH "/uploads/firmware_app_load_mod/bct_468/bct_468_firmware_app_load_mod_2.05-alpha.fw"
#endif

/* UART speed used after the initial 115200 handshake, and whether to use
 * CTS/RTS flow control. NOTE: flow control requires RTS/CTS to be WIRED
 * (not available on the plain PB6/PB7 Nucleo hookup). Override in
 * config.local.h, e.g. 3000000 + true on a Windows FTDI setup. */
#ifndef U_EXAMPLE_UART_BAUD
# define U_EXAMPLE_UART_BAUD 921600
#endif
#ifndef U_EXAMPLE_UART_FLOW
# define U_EXAMPLE_UART_FLOW false
#endif

/* Output file for the downloaded body (hosts with a filesystem only) */
#ifndef U_EXAMPLE_HTTP_OUT_FILE
# define U_EXAMPLE_HTTP_OUT_FILE "http_download.bin"
#endif

#define URC_FLAG_NETWORK_UP         (1 << 0)
#define URC_FLAG_HTTP_RESPONSE      (1 << 1)

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "networkUpUrc");
    exampleSignalEvent(URC_FLAG_NETWORK_UP);
}

static void httpRequestStatus(struct uCxHandle *puCxHandle, int32_t session_id, int32_t status_code, const char *description)
{
    (void)puCxHandle;
    (void)session_id;
    (void)description;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "HTTP response: %" PRId32, status_code);
    exampleSignalEvent(URC_FLAG_HTTP_RESPONSE);
}

/* Parse the Content-Length header value (case-insensitive). Returns -1 if absent. */
static int32_t parseContentLength(const char *pHeaders)
{
    const char *pTag = "content-length:";
    for (const char *p = pHeaders; *p != 0; p++) {
        size_t i = 0;
        while ((pTag[i] != 0) && (tolower((unsigned char)p[i]) == pTag[i])) {
            i++;
        }
        if (pTag[i] == 0) {
            return (int32_t)strtol(p + i, NULL, 10);
        }
    }
    return -1;
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "http_example",
        "Example of how to do a HTTP GET request using the uCx API.\n"
        "Connects to WiFi, downloads U_EXAMPLE_HTTP_URL + U_EXAMPLE_HTTP_PATH\n"
        "(binary-safe, saved to a file on hosts with a filesystem).\n"
        "This example will also illustrate how to change UART baud rate on the module.",
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
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "- You need to define U_EXAMPLE_UART, U_EXAMPLE_SSID & U_EXAMPLE_WPA_PSK.");
    }

    // Initialize example utilities and AT client
    int currentBaud = 115200;
    uCxAtClient_t *pClient = exampleInit(pDevice, currentBaud, false);
    if (pClient == NULL) {
        goto fail;
    }

    uCxInit(pClient, &ucxHandle);

    // Check communication with device - try different baud rates if needed
    uCxAtClientSetCommandTimeout(pClient, 500, false);
    if (uCxGeneralAttention(&ucxHandle) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "No response from device at 115200 baud - try %d baud",
                        (int)U_EXAMPLE_UART_BAUD);
        uCxAtClientClose(pClient);
        currentBaud = U_EXAMPLE_UART_BAUD;
        uCxAtClientOpen(pClient, currentBaud, U_EXAMPLE_UART_FLOW);
        if (uCxGeneralAttention(&ucxHandle) != 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "No response from device");
            goto fail;
        }
    }

    // Register URC callbacks
    uCxWifiRegisterStationNetworkUp(&ucxHandle, networkUpUrc);
    uCxHttpRegisterRequestStatus(&ucxHandle, httpRequestStatus);

    // Reboot the module to ensure a clean state
    uCxSystemReboot(&ucxHandle);
    if (currentBaud != 115200) {
        // Module returns to its default 115200 (no flow control) after reboot
        U_CX_PORT_SLEEP_MS(200);
        uCxAtClientClose(pClient);
        currentBaud = 115200;
        uCxAtClientOpen(pClient, currentBaud, false);
    }
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucxHandle);

    // Increase UART speed for better throughput
    ret = uCxSystemSetUartSettings3(&ucxHandle, U_EXAMPLE_UART_BAUD, U_EXAMPLE_UART_FLOW ? 1 : 0, 1);
    if (ret < 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "uCxSystemSetUartSettings3() failed");
        goto fail;
    }
    // Re-open AT client at new speed
    uCxAtClientClose(pClient);
    U_CX_PORT_SLEEP_MS(200);
    uCxAtClientOpen(pClient, U_EXAMPLE_UART_BAUD, U_EXAMPLE_UART_FLOW);
    if (uCxGeneralAttention(&ucxHandle) != 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "No response from device at %d baud",
                        (int)U_EXAMPLE_UART_BAUD);
        goto fail;
    }

    uCxWifiStationSetSecurityWpa(&ucxHandle, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucxHandle, 0, pSsid);
    uCxWifiStationConnect(&ucxHandle, 0);
    exampleWaitEvent(URC_FLAG_NETWORK_UP, 20);

    const int32_t sessionId = 0;

    // Configure HTTP connection
    ret = uCxHttpSetConnectionParams2(&ucxHandle, sessionId, U_EXAMPLE_HTTP_URL);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetConnectionParams2() returned %" PRId32, ret);

    // Set request path
    ret = uCxHttpSetRequestPath(&ucxHandle, sessionId, U_EXAMPLE_HTTP_PATH);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpSetRequestPath() returned %" PRId32, ret);

    // Send GET request
    ret = uCxHttpGetRequest(&ucxHandle, sessionId);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "uCxHttpGetRequest() returned %" PRId32, ret);

    // Wait for response
    exampleWaitEvent(URC_FLAG_HTTP_RESPONSE, 10);

    // Read response headers in chunks, capturing them so we can find Content-Length
    // (static: too large for embedded task stacks - 8 KB on STM32)
    static char headerBuf[4096];
    size_t headerLen = 0;
    uCxHttpGetHeader_t headerRsp;
    printf("HTTP Headers:\n");
    do {
        if (uCxHttpGetHeader2Begin(&ucxHandle, sessionId, 512, &headerRsp)) {
            int chunk = (int)headerRsp.byte_array_data.length;
            printf("%.*s", chunk, headerRsp.byte_array_data.pData);
            if ((chunk > 0) && (headerLen + (size_t)chunk < sizeof(headerBuf))) {
                memcpy(&headerBuf[headerLen], headerRsp.byte_array_data.pData, (size_t)chunk);
                headerLen += (size_t)chunk;
            }
            uCxEnd(&ucxHandle);
        } else {
            break;
        }
    } while (headerRsp.more_to_read);
    headerBuf[headerLen] = 0;
    printf("\n");

    // The module streams the body from the network as we read it, so more_to_read
    // only means "nothing buffered right now" - NOT "download complete". Use the
    // Content-Length header to know how many bytes to expect.
    int32_t contentLength = parseContentLength(headerBuf);
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Content-Length: %" PRId32, contentLength);

    // Read response body (1000 bytes = module max per AT+UHTCGBB read).
    // The body may be binary (e.g. a firmware image) so it must NOT be treated
    // as a C string. On hosts with a filesystem the exact bytes are written to
    // U_EXAMPLE_HTTP_OUT_FILE; embedded targets just count and verify length.
#if !defined(U_PORT_FREERTOS)
    const char *pOutFile = U_EXAMPLE_HTTP_OUT_FILE;
    FILE *pOut = fopen(pOutFile, "wb");
    if (pOut == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open output file %s", pOutFile);
        goto fail;
    }
#else
    const char *pOutFile = "(discarded - no filesystem)";
#endif
    static uint8_t rxData[1000];  // static: keep off the embedded task stack
    int32_t moreToRead = 0;
    int32_t totalBytes = 0;
    int32_t stallCount = 0;
    int32_t nextProgress = 100 * 1024;
    while (true) {
        ret = uCxHttpGetBody(&ucxHandle, sessionId, sizeof(rxData), rxData, &moreToRead);
        if (ret < 0) {
            U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "uCxHttpGetBody() failed: %" PRId32, ret);
            break;
        }
        if (ret > 0) {
#if !defined(U_PORT_FREERTOS)
            fwrite(rxData, 1, (size_t)ret, pOut);
#endif
            totalBytes += ret;
            stallCount = 0;
            if (totalBytes >= nextProgress) {
                U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Downloaded %" PRId32 " of %" PRId32 " bytes",
                                totalBytes, contentLength);
                nextProgress += 100 * 1024;
            }
        }
        // Complete once we've received the full Content-Length.
        if ((contentLength >= 0) && (totalBytes >= contentLength)) {
            break;
        }
        // Nothing buffered right now: if the length is unknown assume we're done,
        // otherwise the rest is still streaming in - wait briefly and retry.
        if ((ret == 0) && !moreToRead) {
            if (contentLength < 0) {
                break;
            }
            if (++stallCount > 500) {  // ~10s with no progress => give up
                U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Body download stalled");
                break;
            }
            U_CX_PORT_SLEEP_MS(20);
        }
    }
#if !defined(U_PORT_FREERTOS)
    fclose(pOut);
#endif
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance,
                    "Downloaded %" PRId32 " of %" PRId32 " bytes to %s",
                    totalBytes, contentLength, pOutFile);
    if ((contentLength >= 0) && (totalBytes != contentLength)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_WARN, pClient->instance, "Incomplete download!");
    }

    // Disconnect HTTP session
    uCxHttpDisconnect(&ucxHandle, sessionId);

    // Reboot module to restore default UART settings
    uCxSystemReboot(&ucxHandle);

    // Clean up
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();

    return 0;

fail:
    if (pClient != NULL) {
        uCxAtClientClose(pClient);
        uCxAtClientDeinit(pClient);
        uPortDeinit();
    }
    return 1;
}
