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
 */

/** @file
 * @brief Reference example: BLE-scan -> MQTT-publish gateway using the uCx API.
 *
 * This example runs on a host PC connected to a NORA-W36 module over UART.
 * It does:
 *   1. Bring up Wi-Fi station
 *   2. Connect (plain TCP, no TLS) to an MQTT broker
 *   3. Forever:
 *        - Run a short BLE discovery
 *        - For each discovered device, publish a small JSON payload to MQTT
 *
 * It is intentionally small and blocking. Topics, payload schema, filtering,
 * GATT services and TLS are placeholders -- swap them for your real ones.
 *
 * Execute with following args (all optional, defaults from config.local.h):
 *   ble_mqtt_gateway_example [uart_device] [wifi_ssid] [wifi_psk] [broker] [port]
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include "u_cx_log.h"
#include "u_cx.h"
#include "u_cx_wifi.h"
#include "u_cx_bluetooth.h"
#include "u_cx_mqtt.h"
#include "u_cx_system.h"
#include "u_cx_general.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_EXAMPLE_MQTT_BROKER
# define U_EXAMPLE_MQTT_BROKER     "test.mosquitto.org"
#endif
#ifndef U_EXAMPLE_MQTT_PORT
# define U_EXAMPLE_MQTT_PORT       1883
#endif
#ifndef U_EXAMPLE_MQTT_CLIENT_ID
# define U_EXAMPLE_MQTT_CLIENT_ID  "ucx-ble-gw"
#endif
#ifndef U_EXAMPLE_MQTT_TOPIC
# define U_EXAMPLE_MQTT_TOPIC      "ucx/gateway/scan"
#endif

#define MQTT_ID                    0
#define BLE_SCAN_DURATION_MS       3000
#define SCAN_INTERVAL_PAUSE_MS     2000

#define URC_FLAG_NETWORK_UP        (1 << 0)
#define URC_FLAG_MQTT_CONNECTED    (1 << 1)
#define URC_FLAG_MQTT_DISCONNECTED (1 << 2)

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static volatile bool gMqttConnected = false;

/* ----------------------------------------------------------------
 * URC CALLBACKS (run in AT RX thread -- keep them short!)
 * -------------------------------------------------------------- */

static void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    exampleSignalEvent(URC_FLAG_NETWORK_UP);
}

static void mqttConnectUrc(struct uCxHandle *puCxHandle, int32_t mqtt_id)
{
    (void)puCxHandle;
    (void)mqtt_id;
    gMqttConnected = true;
    exampleSignalEvent(URC_FLAG_MQTT_CONNECTED);
}

static void mqttDisconnectUrc(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t reason)
{
    (void)puCxHandle;
    (void)mqtt_id;
    (void)reason;
    gMqttConnected = false;
    exampleSignalEvent(URC_FLAG_MQTT_DISCONNECTED);
}

/* ----------------------------------------------------------------
 * HELPERS
 * -------------------------------------------------------------- */

static void formatBdAddr(const uBtLeAddress_t *pAddr, char *pOut, size_t outLen)
{
    snprintf(pOut, outLen, "%02X:%02X:%02X:%02X:%02X:%02X",
             pAddr->address[0], pAddr->address[1], pAddr->address[2],
             pAddr->address[3], pAddr->address[4], pAddr->address[5]);
}

/**
 * Build a minimal JSON payload for one BLE scan result.
 * Placeholder format -- replace with your real schema.
 */
static int buildJsonPayload(const uCxBtDiscovery_t *pDev, char *pOut, size_t outLen)
{
    char addr[18];
    formatBdAddr(&pDev->bd_addr, addr, sizeof(addr));
    const char *pName = (pDev->device_name[0] != 0) ? pDev->device_name : "";
    return snprintf(pOut, outLen,
                    "{\"addr\":\"%s\",\"rssi\":%" PRId32 ",\"name\":\"%s\"}",
                    addr, pDev->rssi, pName);
}

/**
 * Publish one BLE scan result to MQTT.
 * Called from the main loop -- NOT from a URC callback.
 */
static int32_t publishScanResult(uCxHandle_t *pUcx, const uCxBtDiscovery_t *pDev)
{
    char payload[160];
    int len = buildJsonPayload(pDev, payload, sizeof(payload));
    if (len <= 0) {
        return -1;
    }
    return uCxMqttPublish(pUcx,
                          MQTT_ID,
                          U_MQTT_QOS_AT_MOST_ONCE,
                          U_MQTT_RETAIN_NO,
                          U_EXAMPLE_MQTT_TOPIC,
                          (const uint8_t *)payload,
                          (int32_t)len);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    exampleCheckHelp(argc, argv, "ble_mqtt_gateway_example",
        "Reference BLE -> MQTT gateway example.\n"
        "Connects to Wi-Fi, connects to an MQTT broker, then forever scans for\n"
        "BLE devices and publishes each result as JSON to an MQTT topic.",
        "[uart_device] [wifi_ssid] [wifi_psk] [broker] [port]");

    const char *pDevice  = U_EXAMPLE_UART;
    const char *pSsid    = U_EXAMPLE_SSID;
    const char *pWpaPsk  = U_EXAMPLE_WPA_PSK;
    const char *pBroker  = U_EXAMPLE_MQTT_BROKER;
    int32_t     port     = U_EXAMPLE_MQTT_PORT;

    if (argc >= 2) { pDevice = argv[1]; }
    if (argc >= 3) { pSsid   = argv[2]; }
    if (argc >= 4) { pWpaPsk = argv[3]; }
    if (argc >= 5) { pBroker = argv[4]; }
    if (argc >= 6) { port    = (int32_t)atoi(argv[5]); }

    printf("===========================================\n");
    printf("BLE -> MQTT Gateway Example\n");
    printf("  Wi-Fi SSID : %s\n", pSsid);
    printf("  MQTT broker: %s:%" PRId32 "\n", pBroker, port);
    printf("  MQTT topic : %s\n", U_EXAMPLE_MQTT_TOPIC);
    printf("===========================================\n");

    if (*pWpaPsk == 0) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Wi-Fi not configured - set U_EXAMPLE_WPA_PSK in config.local.h");
    }

    uCxHandle_t   ucx;
    uCxAtClient_t *pClient = exampleInit(pDevice, 115200, false);
    if (pClient == NULL) {
        return 1;
    }
    uCxInit(pClient, &ucx);

    /* Register URCs BEFORE issuing the commands that trigger them. */
    uCxWifiRegisterStationNetworkUp(&ucx, networkUpUrc);
    uCxMqttRegisterConnect(&ucx, mqttConnectUrc);
    uCxMqttRegisterDisconnect(&ucx, mqttDisconnectUrc);

    /* Clean module state. */
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucx);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucx);

    /* --- BLE side: central role, ready to scan. --- */
    if (uCxBluetoothSetMode(&ucx, U_BT_MODE_CENTRAL_PERIPHERAL) != 0) {
        printf("Warning: uCxBluetoothSetMode() failed (may already be set).\n");
    }

    /* --- Wi-Fi side: connect to AP. --- */
    printf("Connecting Wi-Fi to \"%s\"...\n", pSsid);
    uCxWifiStationSetSecurityWpa(&ucx, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucx, 0, pSsid);
    uCxWifiStationConnect(&ucx, 0);
    if (!exampleWaitEvent(URC_FLAG_NETWORK_UP, 30)) {
        printf("Timed out waiting for Wi-Fi network up.\n");
        goto fail;
    }
    printf("Wi-Fi up.\n");

    /* --- MQTT side: connect to broker. --- */
    printf("Connecting MQTT to %s:%" PRId32 " as \"%s\"...\n",
           pBroker, port, U_EXAMPLE_MQTT_CLIENT_ID);
    int32_t ret = uCxMqttSetConnectionParams4(&ucx, MQTT_ID, pBroker, port,
                                              U_EXAMPLE_MQTT_CLIENT_ID);
    if (ret != 0) {
        printf("uCxMqttSetConnectionParams4() failed: %" PRId32 "\n", ret);
        goto fail;
    }
    ret = uCxMqttConnect(&ucx, MQTT_ID);
    if (ret != 0) {
        printf("uCxMqttConnect() failed: %" PRId32 "\n", ret);
        goto fail;
    }
    if (!exampleWaitEvent(URC_FLAG_MQTT_CONNECTED, 20)) {
        printf("Timed out waiting for MQTT connect.\n");
        goto fail;
    }
    printf("MQTT connected.\n");

    /* --- Gateway main loop. --- */
    int32_t scanRound = 0;
    int32_t totalPublished = 0;

    while (gMqttConnected) {
        scanRound++;
        printf("\n--- Scan #%" PRId32 " (%d ms) ---\n",
               scanRound, BLE_SCAN_DURATION_MS);

        uCxBluetoothDiscovery3Begin(&ucx,
                                    U_BT_DISCOVERY_TYPE_ALL_NO_DUPLICATES,
                                    U_BT_DISCOVERY_MODE_ACTIVE,
                                    BLE_SCAN_DURATION_MS);

        uCxBtDiscovery_t dev;
        int found = 0;
        while (uCxBluetoothDiscovery3GetNext(&ucx, &dev)) {
            found++;
            char addr[18];
            formatBdAddr(&dev.bd_addr, addr, sizeof(addr));
            printf("  [%d] %s  rssi=%-4" PRId32 "  name=\"%s\"\n",
                   found, addr, dev.rssi,
                   dev.device_name[0] ? dev.device_name : "");

            /* TODO: filter here (manufacturer data, name prefix, addr, ...) */

            int32_t pubRet = publishScanResult(&ucx, &dev);
            if (pubRet == 0) {
                totalPublished++;
            } else {
                printf("       publish failed: %" PRId32 "\n", pubRet);
            }
        }
        uCxEnd(&ucx);

        printf("Round %" PRId32 ": %d device(s) seen, %" PRId32 " published in total.\n",
               scanRound, found, totalPublished);

        U_CX_PORT_SLEEP_MS(SCAN_INTERVAL_PAUSE_MS);
    }

    printf("\nMQTT disconnected -- exiting.\n");

    uCxMqttDisconnect(&ucx, MQTT_ID);
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
