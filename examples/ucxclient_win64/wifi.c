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

#include "wifi.h"

// TODO: Extract from original ucxclient_win64.c lines ~4723-5464

void showWifiStatus(void)
{
    printf("TODO: showWifiStatus() - needs implementation\n");
}

void wifiScan(void)
{
    printf("TODO: wifiScan() - needs implementation\n");
}

void wifiConnect(void)
{
    printf("TODO: wifiConnect() - needs implementation\n");
}

void wifiDisconnect(void)
{
    printf("TODO: wifiDisconnect() - needs implementation\n");
}

void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel)
{
    printf("TODO: testConnectivity() - needs implementation\n");
    (void)gateway;
    (void)ssid;
    (void)rssi;
    (void)channel;
}

void wifiFunctionsMenu(void)
{
    printf("TODO: wifiFunctionsMenu() - needs implementation\n");
    gMenuState = MENU_MAIN;
}
