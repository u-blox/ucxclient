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

#include "urc_handlers.h"

// ----------------------------------------------------------------
// Event Management Functions
// ----------------------------------------------------------------

bool waitEvent(uint32_t evtFlag, uint32_t timeoutS)
{
    int32_t timeoutMs = timeoutS * 1000;
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "waitEvent(%d, %d)", evtFlag, timeoutS);
    do {
        U_CX_MUTEX_TRY_LOCK(gUrcMutex, 100);
        if (gUrcEventFlags & evtFlag) {
            gUrcEventFlags &= ~evtFlag;  // Clear the flag
            U_CX_MUTEX_UNLOCK(gUrcMutex);
            return true;
        }
        U_CX_MUTEX_UNLOCK(gUrcMutex);
        
        // Sleep to allow RX thread and AT client to process URCs
        Sleep(50);  // Check every 50ms instead of spinning
    } while (U_CX_PORT_GET_TIME_MS() - startTime < timeoutMs);

    U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "Timeout waiting for: %d", evtFlag);
    return false;
}

void signalEvent(uint32_t evtFlag)
{
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags |= evtFlag;
    U_CX_MUTEX_UNLOCK(gUrcMutex);
}

// ----------------------------------------------------------------
// URC Callback Functions
// ----------------------------------------------------------------

void networkUpUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Network UP");
    signalEvent(URC_FLAG_NETWORK_UP);
}

void networkDownUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Network DOWN");
    signalEvent(URC_FLAG_NETWORK_DOWN);
}

void linkUpUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel)
{
    (void)wlan_handle;
    (void)bssid;
    (void)channel;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Link UP");
    signalEvent(URC_FLAG_WIFI_LINK_UP);
}

void linkDownUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, int32_t reason)
{
    (void)wlan_handle;
    (void)reason;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Wi-Fi Link DOWN");
    signalEvent(URC_FLAG_WIFI_LINK_DOWN);
}

void sockConnected(struct uCxHandle *puCxHandle, int32_t socket_handle)
{
    (void)puCxHandle;
    (void)socket_handle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Socket connected: %d", socket_handle);
    signalEvent(URC_FLAG_SOCK_CONNECTED);
}

void socketDataAvailable(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)socket_handle;
    (void)number_bytes;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Socket data available: %d bytes on socket %d", number_bytes, socket_handle);
    signalEvent(URC_FLAG_SOCK_DATA);
}

void spsDataAvailable(struct uCxHandle *puCxHandle, int32_t connection_handle, int32_t number_bytes)
{
    (void)puCxHandle;
    (void)connection_handle;
    (void)number_bytes;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "SPS data available: %d bytes on connection %d", number_bytes, connection_handle);
    signalEvent(URC_FLAG_SPS_DATA);
}

void spsConnected(struct uCxHandle *puCxHandle, int32_t connection_handle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** SPS Connection established! Connection handle: %d ***", connection_handle);
    signalEvent(URC_FLAG_SPS_CONNECTED);
}

void spsDisconnected(struct uCxHandle *puCxHandle, int32_t connection_handle)
{
    (void)puCxHandle;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** SPS Disconnected! Connection handle: %d ***", connection_handle);
    signalEvent(URC_FLAG_SPS_DISCONNECTED);
}

void startupUrc(struct uCxHandle *puCxHandle)
{
    (void)puCxHandle;
    // Record timestamp when STARTUP is received
    gStartupTimestamp = GetTickCount64();
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "*** Module STARTUP detected ***");
    signalEvent(URC_FLAG_STARTUP);
}

void pingResponseUrc(struct uCxHandle *puCxHandle, uPingResponse_t ping_response, int32_t response_time)
{
    (void)puCxHandle;
    if (ping_response == U_PING_RESPONSE_TRUE) {
        gPingSuccess++;
        // Store individual ping time
        if (gPingCount < MAX_PING_TIMES) {
            gPingTimes[gPingCount] = response_time;
            gPingCount++;
        }
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Ping response: %d ms", response_time);
    } else {
        gPingFailed++;
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, "Ping failed");
    }
}

void pingCompleteUrc(struct uCxHandle *puCxHandle, int32_t transmitted_packets, 
                     int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time)
{
    (void)puCxHandle;
    (void)packet_loss_rate;
    gPingSuccess = received_packets;
    gPingFailed = transmitted_packets - received_packets;
    gPingAvgTime = avg_response_time;
    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, puCxHandle->pAtClient->instance, 
                   "Ping complete: %d/%d packets, avg %d ms", 
                   received_packets, transmitted_packets, avg_response_time);
    signalEvent(URC_FLAG_PING_COMPLETE);
}
