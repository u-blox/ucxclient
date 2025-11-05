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

#ifndef WIFI_H
#define WIFI_H

#include "common.h"

// ----------------------------------------------------------------
// WiFi Functions
// ----------------------------------------------------------------

/**
 * Show WiFi status
 */
void showWifiStatus(void);

/**
 * Perform WiFi scan
 */
void wifiScan(void);

/**
 * Connect to WiFi network
 */
void wifiConnect(void);

/**
 * Disconnect from WiFi network
 */
void wifiDisconnect(void);

/**
 * Test connectivity with ping tests
 * 
 * @param gateway Gateway IP address
 * @param ssid SSID of connected network
 * @param rssi Signal strength (RSSI)
 * @param channel WiFi channel
 */
void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel);

/**
 * WiFi functions submenu
 */
void wifiFunctionsMenu(void);

#endif // WIFI_H
