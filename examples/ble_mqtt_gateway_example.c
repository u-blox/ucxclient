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
 * @brief Reference example: MQTT -> BLE gateway using the uCx API.
 *
 * This example runs on a host PC connected to a NORA-W36 module over UART.
 * It mirrors the behaviour of the companion s-center AT script
 * `nora_w366_gateway_simple.script`:
 *
 *   1. Bring up Wi-Fi station
 *   2. Connect (plain TCP, no TLS) to an MQTT broker
 *   3. Scan for a BLE peripheral by name (default "NINA-B5 LED") and connect
 *   4. Subscribe to an MQTT topic
 *   5. Forever: on each received MQTT message, write the payload byte
 *      (`0` -> LED off, `1` -> LED on) to a GATT value handle on the peer
 *
 * The BLE peripheral must be running and advertising. Edit the GATT value
 * handle (`U_EXAMPLE_BLE_CMD_HANDLE`) and peer name to match your device.
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
#include "u_cx_gatt_client.h"
#include "u_cx_mqtt.h"
#include "u_cx_system.h"
#include "u_cx_general.h"
#include "example_utils.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_EXAMPLE_MQTT_BROKER
# define U_EXAMPLE_MQTT_BROKER       "broker.emqx.io"
#endif
#ifndef U_EXAMPLE_MQTT_PORT
# define U_EXAMPLE_MQTT_PORT         1883
#endif
#ifndef U_EXAMPLE_MQTT_CLIENT_ID
# define U_EXAMPLE_MQTT_CLIENT_ID    "ucx-ble-gw"
#endif
#ifndef U_EXAMPLE_MQTT_TOPIC
# define U_EXAMPLE_MQTT_TOPIC        "demo/led/state"
#endif
/* Topic to publish updated LED status (echo from peer notification).
 * Same topic as U_EXAMPLE_MQTT_TOPIC by design: the gateway is the only
 * publisher; echo from the broker is suppressed by the gKnownPeerState /
 * gLastPublished dedup logic below. */
#ifndef U_EXAMPLE_MQTT_STATUS_TOPIC
# define U_EXAMPLE_MQTT_STATUS_TOPIC "demo/led/state"
#endif

/* Name (or name prefix) of the BLE peripheral to find during scan. */
#ifndef U_EXAMPLE_BLE_PEER_NAME
# define U_EXAMPLE_BLE_PEER_NAME     "NINA-B5 LED"
#endif

/* GATT VALUE handle of the LED Command characteristic on the peer.
 * Discover with `AT+UBTGSCD=<conn>,1,65535` -- use the <value_handle>
 * column, not the <char_handle> column.
 */
#ifndef U_EXAMPLE_BLE_CMD_HANDLE
# define U_EXAMPLE_BLE_CMD_HANDLE    21
#endif

/* GATT VALUE handle of the LED State characteristic (Read+Notify) and the
 * matching CCCD descriptor (typically value_handle + 1). */
#ifndef U_EXAMPLE_BLE_STATE_HANDLE
# define U_EXAMPLE_BLE_STATE_HANDLE       18
#endif
#ifndef U_EXAMPLE_BLE_STATE_CCCD_HANDLE
# define U_EXAMPLE_BLE_STATE_CCCD_HANDLE  19
#endif

#define MQTT_ID                      0
#define BLE_SCAN_DURATION_MS         5000
#define MQTT_PAYLOAD_BUF_SIZE        128

#define URC_FLAG_NETWORK_UP          (1 << 0)
#define URC_FLAG_MQTT_CONNECTED      (1 << 1)
#define URC_FLAG_MQTT_DISCONNECTED   (1 << 2)
#define URC_FLAG_MQTT_DATA_AVAILABLE (1 << 3)
#define URC_FLAG_BLE_CONNECTED       (1 << 4)
#define URC_FLAG_BLE_DISCONNECTED    (1 << 5)
#define URC_FLAG_BLE_NOTIFY          (1 << 6)

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static volatile bool    gMqttConnected   = false;
static volatile bool    gBleConnected    = false;
static volatile int32_t gBleConnHandle   = -1;
static volatile bool    gNotifyPending   = false;
static volatile uint8_t gNotifyValue     = 0;
/* Last LED state we believe the peer is in (0 or 1). 0xFF = unknown.
 * Used to break the MQTT echo loop when the publish topic equals the
 * subscribe topic: we skip a BLE write that would just re-assert the
 * value we already know the peer to hold. */
static volatile uint8_t gKnownPeerState  = 0xFF;
/* Last value we published to MQTT. 0xFF = nothing published yet. Used to
 * suppress notify-driven re-publishes when the value has not actually
 * changed -- otherwise a self-subscribed gateway loops forever. */
static uint8_t          gLastPublished   = 0xFF;

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

static void mqttDataAvailableUrc(struct uCxHandle *puCxHandle, int32_t mqtt_id, int32_t message_len)
{
    (void)puCxHandle;
    (void)mqtt_id;
    (void)message_len;
    exampleSignalEvent(URC_FLAG_MQTT_DATA_AVAILABLE);
}

static void bleConnectUrc(struct uCxHandle *puCxHandle, int32_t conn_handle, uBtLeAddress_t *bd_addr)
{
    (void)puCxHandle;
    (void)bd_addr;
    gBleConnHandle = conn_handle;
    gBleConnected  = true;
    exampleSignalEvent(URC_FLAG_BLE_CONNECTED);
}

static void bleDisconnectUrc(struct uCxHandle *puCxHandle, int32_t conn_handle)
{
    (void)puCxHandle;
    (void)conn_handle;
    gBleConnected  = false;
    gBleConnHandle = -1;
    exampleSignalEvent(URC_FLAG_BLE_DISCONNECTED);
}

static void bleNotifyUrc(struct uCxHandle *puCxHandle, int32_t conn_handle,
                         int32_t value_handle, uByteArray_t *hex_data)
{
    (void)puCxHandle;
    (void)conn_handle;
    if (value_handle != U_EXAMPLE_BLE_STATE_HANDLE) {
        return;     /* notification from some other characteristic */
    }
    if (hex_data != NULL && hex_data->length >= 1 && hex_data->pData != NULL) {
        gNotifyValue    = hex_data->pData[0];
        gKnownPeerState = hex_data->pData[0];
        gNotifyPending  = true;
        exampleSignalEvent(URC_FLAG_BLE_NOTIFY);
    }
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
 * Run a single BLE discovery round and return the first device that the
 * module's own data filter (AD type 0x09 = Complete Local Name) lets through.
 * The address is copied to @p pOutAddr. Returns true on match.
 *
 * The filter uses the first up-to-10 bytes of @p pTargetName as a prefix
 * match (firmware limit, mirrors the byte_array filter in
 * `nora_w366_gateway_simple.script`).
 */
static bool scanForPeer(uCxHandle_t *pUcx, const char *pTargetName,
                        uBtLeAddress_t *pOutAddr)
{
    bool found = false;

    /* Configure data filter 0: AD type 0x09 (Complete Local Name) = name prefix. */
    size_t nameLen = strlen(pTargetName);
    if (nameLen > 10) {
        nameLen = 10;       /* firmware byte_array filter caps at 10 bytes */
    }
    int32_t fRet = uCxBluetoothSetDiscoveryDataFilter(pUcx, 0, 0x09,
                                                      (const uint8_t *)pTargetName,
                                                      (int32_t)nameLen);
    printf("  uCxBluetoothSetDiscoveryDataFilter(idx=0, ad_type=0x09, len=%u)"
           " -> %" PRId32 "%s\n",
           (unsigned)nameLen, fRet,
           (fRet == 0) ? " (OK)" : " (FAILED - continuing without filter)");

    uCxBluetoothDiscovery3Begin(pUcx,
                                U_BT_DISCOVERY_TYPE_DATA_FILTER_NO_DUPLICATES,
                                U_BT_DISCOVERY_MODE_ACTIVE,
                                BLE_SCAN_DURATION_MS);

    uCxBtDiscovery_t dev;
    while (uCxBluetoothDiscovery3GetNext(pUcx, &dev)) {
        const char *pName = (dev.device_name != NULL) ? dev.device_name : "";
        char addr[18];
        formatBdAddr(&dev.bd_addr, addr, sizeof(addr));
        printf("  match %s  rssi=%-4" PRId32 "  name=\"%s\"\n",
               addr, dev.rssi, pName);

        if (!found) {
            *pOutAddr = dev.bd_addr;
            found = true;
            /* Keep draining results so the AT response is fully consumed. */
        }
    }
    uCxEnd(pUcx);
    return found;
}

/**
 * Write a single byte to the LED Command characteristic on the peer.
 * Uses Write-Without-Response (mirrors AT+UBTGWNR in the script).
 */
static int32_t writeLedState(uCxHandle_t *pUcx, int32_t connHandle, uint8_t state)
{
    return uCxGattClientWriteNoRsp(pUcx, connHandle,
                                   U_EXAMPLE_BLE_CMD_HANDLE,
                                   &state, 1);
}

/**
 * Drain one MQTT message from the module and act on it. Payload is treated
 * as ASCII: "1" -> LED on, "0" -> LED off, anything else -> ignored.
 */
static void handleMqttMessage(uCxHandle_t *pUcx)
{
    uint8_t      payload[MQTT_PAYLOAD_BUF_SIZE];
    const char *pTopic = NULL;

    int32_t len = uCxMqttReadBegin(pUcx, MQTT_ID, payload,
                                   sizeof(payload) - 1, &pTopic);
    if (len < 0) {
        printf("uCxMqttReadBegin() failed: %" PRId32 "\n", len);
        uCxEnd(pUcx);
        return;
    }
    payload[len] = 0;
    printf("MQTT rx on \"%s\" (%" PRId32 " bytes): \"%s\"\n",
           pTopic ? pTopic : "?", len, (char *)payload);
    uCxEnd(pUcx);

    if (!gBleConnected) {
        printf("  BLE not connected -- ignoring.\n");
        return;
    }

    uint8_t desired;
    if (len >= 1 && payload[0] == '1') {
        desired = 0x01;
    } else if (len >= 1 && payload[0] == '0') {
        desired = 0x00;
    } else {
        printf("  Unknown payload -- ignored.\n");
        return;
    }

    /* Break the echo loop: if the peer already holds this value (either
     * from a notification or from our own previous write), don't write
     * again -- otherwise we'd re-publish on the next notify and bounce
     * forever. */
    if (gKnownPeerState == desired) {
        printf("  Peer already at %u -- skipping BLE write.\n",
               (unsigned)desired);
        return;
    }

    printf("  -> Writing LED %s to peer (handle %d)\n",
           desired ? "ON" : "OFF", U_EXAMPLE_BLE_CMD_HANDLE);
    int32_t r = writeLedState(pUcx, gBleConnHandle, desired);
    if (r != 0) {
        printf("     uCxGattClientWriteNoRsp() failed: %" PRId32 "\n", r);
    } else {
        /* Latch optimistically; the imminent notification will confirm. */
        gKnownPeerState = desired;
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int U_EXAMPLE_MAIN(int argc, char **argv)
{
    /* Make stdout line-buffered so logs survive a forced Stop-Process kill. */
    setvbuf(stdout, NULL, _IONBF, 0);

    exampleCheckHelp(argc, argv, "ble_mqtt_gateway_example",
        "Reference MQTT -> BLE gateway example.\n"
        "Connects to Wi-Fi, connects to an MQTT broker, scans for a named BLE\n"
        "peripheral, connects to it, subscribes to an MQTT topic and forwards\n"
        "received payloads (\"0\"/\"1\") as Write-Without-Response to a GATT\n"
        "value handle on the peer.",
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
    printf("MQTT -> BLE Gateway Example\n");
    printf("  Wi-Fi SSID   : %s\n", pSsid);
    printf("  MQTT broker  : %s:%" PRId32 "\n", pBroker, port);
    printf("  MQTT topic   : %s\n", U_EXAMPLE_MQTT_TOPIC);
    printf("  BLE peer name: %s\n", U_EXAMPLE_BLE_PEER_NAME);
    printf("  BLE cmd hnd  : %d\n", U_EXAMPLE_BLE_CMD_HANDLE);
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
    uCxMqttRegisterDataAvailable(&ucx, mqttDataAvailableUrc);
    uCxBluetoothRegisterConnect(&ucx, bleConnectUrc);
    uCxBluetoothRegisterDisconnect(&ucx, bleDisconnectUrc);
    uCxGattClientRegisterNotification(&ucx, bleNotifyUrc);

    /* Clean module state. */
    printf("Rebooting module...\n");
    uCxSystemReboot(&ucx);
    U_CX_PORT_SLEEP_MS(4000);
    uCxSystemSetEchoOff(&ucx);

    /* --- BLE side: central role, ready to scan. --- */
    if (uCxBluetoothSetMode(&ucx, U_BT_MODE_CENTRAL_PERIPHERAL) != 0) {
        printf("Warning: uCxBluetoothSetMode() failed (may already be set).\n");
    }

    /* --- Step 1: Wi-Fi. --- */
    printf("\nStep 1: Connecting Wi-Fi to \"%s\"...\n", pSsid);
    uCxWifiStationSetSecurityWpa(&ucx, 0, pWpaPsk, U_WIFI_WPA_THRESHOLD_WPA2);
    uCxWifiStationSetConnectionParams(&ucx, 0, pSsid);
    uCxWifiStationConnect(&ucx, 0);
    if (!exampleWaitEvent(URC_FLAG_NETWORK_UP, 30)) {
        printf("Timed out waiting for Wi-Fi network up.\n");
        goto fail;
    }
    printf("  Wi-Fi up.\n");

    /* --- Step 2: MQTT broker. --- */
    printf("\nStep 2: Connecting MQTT to %s:%" PRId32 " as \"%s\"...\n",
           pBroker, port, U_EXAMPLE_MQTT_CLIENT_ID);
    int32_t ret = uCxMqttSetConnectionParams4(&ucx, MQTT_ID, pBroker, port,
                                              U_EXAMPLE_MQTT_CLIENT_ID);
    if (ret != 0) {
        printf("uCxMqttSetConnectionParams4() failed: %" PRId32 "\n", ret);
        goto fail;
    }
    if ((ret = uCxMqttConnect(&ucx, MQTT_ID)) != 0) {
        printf("uCxMqttConnect() failed: %" PRId32 "\n", ret);
        goto fail;
    }
    if (!exampleWaitEvent(URC_FLAG_MQTT_CONNECTED, 20)) {
        printf("Timed out waiting for MQTT connect.\n");
        goto fail;
    }
    printf("  MQTT connected.\n");

    /* --- Step 3: BLE scan + connect to named peer. --- */
    printf("\nStep 3: Scanning for BLE peer \"%s\"...\n", U_EXAMPLE_BLE_PEER_NAME);
    uBtLeAddress_t peerAddr;
    if (!scanForPeer(&ucx, U_EXAMPLE_BLE_PEER_NAME, &peerAddr)) {
        printf("Peer \"%s\" not found in scan window (%d ms).\n",
               U_EXAMPLE_BLE_PEER_NAME, BLE_SCAN_DURATION_MS);
        goto fail;
    }
    {
        char addr[18];
        formatBdAddr(&peerAddr, addr, sizeof(addr));
        printf("  Found peer at %s -- connecting...\n", addr);
    }
    if ((ret = uCxBluetoothConnect(&ucx, &peerAddr)) != 0) {
        printf("uCxBluetoothConnect() failed: %" PRId32 "\n", ret);
        goto fail;
    }
    if (!exampleWaitEvent(URC_FLAG_BLE_CONNECTED, 15)) {
        printf("Timed out waiting for BLE connect.\n");
        goto fail;
    }
    printf("  BLE connected. conn_handle = %" PRId32 "\n", gBleConnHandle);

    /* Enable notifications on the LED State characteristic by writing the
     * Client Characteristic Configuration Descriptor (CCCD). */
    printf("  Enabling notifications on LED State (cccd handle %d)...\n",
           U_EXAMPLE_BLE_STATE_CCCD_HANDLE);
    ret = uCxGattClientConfigWrite(&ucx, gBleConnHandle,
                                   U_EXAMPLE_BLE_STATE_CCCD_HANDLE,
                                   U_GATT_CLIENT_CONFIG_ENABLE_NOTIFICATIONS);
    if (ret != 0) {
        printf("  uCxGattClientConfigWrite() failed: %" PRId32
               " -- notifications disabled\n", ret);
    }

    /* --- Step 4: subscribe to MQTT topic. --- */
    printf("\nStep 4: Subscribing to \"%s\"...\n", U_EXAMPLE_MQTT_TOPIC);
    ret = uCxMqttSubscribe4(&ucx, MQTT_ID,
                            U_MQTT_SUBSCRIBE_ACTION_SUBSCRIBE,
                            U_EXAMPLE_MQTT_TOPIC,
                            U_MQTT_QOS_AT_MOST_ONCE);
    if (ret != 0) {
        printf("uCxMqttSubscribe4() failed: %" PRId32 "\n", ret);
        goto fail;
    }

    /* --- Step 5: forward MQTT payloads to BLE peer. --- */
    printf("\n=== Gateway ready ===\n");
    printf("Publish '0' or '1' to MQTT topic \"%s\" to control the LED.\n",
           U_EXAMPLE_MQTT_TOPIC);
    printf("LED state changes will be published to \"%s\".\n\n",
           U_EXAMPLE_MQTT_STATUS_TOPIC);

    while (gMqttConnected && gBleConnected) {
        if (exampleWaitEventEx(URC_FLAG_MQTT_DATA_AVAILABLE, 1, true)) {
            handleMqttMessage(&ucx);
        }
        if (gNotifyPending) {
            gNotifyPending = false;
            uint8_t v = gNotifyValue;
            if (v == gLastPublished) {
                /* Already announced this value; stay quiet so we don't
                 * keep echoing through the broker. */
                continue;
            }
            const char *pMsg = (v == 0) ? "0" : "1";
            printf("BLE notify: LED state = %u -> MQTT publish \"%s\" to \"%s\"\n",
                   (unsigned)v, pMsg, U_EXAMPLE_MQTT_STATUS_TOPIC);
            int32_t pubRet = uCxMqttPublish(&ucx, MQTT_ID,
                                            U_MQTT_QOS_AT_MOST_ONCE,
                                            U_MQTT_RETAIN_NO,
                                            U_EXAMPLE_MQTT_STATUS_TOPIC,
                                            (const uint8_t *)pMsg, 1);
            if (pubRet < 0) {
                printf("  uCxMqttPublish() failed: %" PRId32 "\n", pubRet);
            } else {
                gLastPublished = v;
            }
        }
    }

    if (!gMqttConnected) {
        printf("\nMQTT disconnected -- exiting.\n");
    } else {
        printf("\nBLE disconnected -- exiting.\n");
    }

    if (gBleConnected) {
        uCxBluetoothDisconnect(&ucx, gBleConnHandle);
    }
    uCxMqttDisconnect(&ucx, MQTT_ID);
    uCxAtClientClose(pClient);
    uCxAtClientDeinit(pClient);
    uPortDeinit();
    return 0;

fail:
    if (gBleConnected) {
        uCxBluetoothDisconnect(&ucx, gBleConnHandle);
    }
    if (pClient != NULL) {
        uCxAtClientClose(pClient);
        uCxAtClientDeinit(pClient);
        uPortDeinit();
    }
    return 1;
}
