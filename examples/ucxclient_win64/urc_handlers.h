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

#ifndef URC_HANDLERS_H
#define URC_HANDLERS_H

#include "common.h"

// ----------------------------------------------------------------
// Event Management Functions
// ----------------------------------------------------------------

/**
 * Wait for a specific URC event flag with timeout
 * 
 * @param evtFlag Event flag to wait for
 * @param timeoutS Timeout in seconds
 * @return true if event occurred, false if timeout
 */
bool waitEvent(uint32_t evtFlag, uint32_t timeoutS);

/**
 * Signal an event flag
 * 
 * @param evtFlag Event flag to set
 */
void signalEvent(uint32_t evtFlag);

// ----------------------------------------------------------------
// URC Callback Functions
// ----------------------------------------------------------------

void networkUpUrc(struct uCxHandle *puCxHandle);
void networkDownUrc(struct uCxHandle *puCxHandle);
void linkUpUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, uMacAddress_t *bssid, int32_t channel);
void linkDownUrc(struct uCxHandle *puCxHandle, int32_t wlan_handle, int32_t reason);
void sockConnected(struct uCxHandle *puCxHandle, int32_t socket_handle);
void socketDataAvailable(struct uCxHandle *puCxHandle, int32_t socket_handle, int32_t number_bytes);
void spsDataAvailable(struct uCxHandle *puCxHandle, int32_t connection_handle, int32_t number_bytes);
void spsConnected(struct uCxHandle *puCxHandle, int32_t connection_handle);
void spsDisconnected(struct uCxHandle *puCxHandle, int32_t connection_handle);
void startupUrc(struct uCxHandle *puCxHandle);
void pingResponseUrc(struct uCxHandle *puCxHandle, uPingResponse_t ping_response, int32_t response_time);
void pingCompleteUrc(struct uCxHandle *puCxHandle, int32_t transmitted_packets, 
                     int32_t received_packets, int32_t packet_loss_rate, int32_t avg_response_time);

#endif // URC_HANDLERS_H
