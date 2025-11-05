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
#include "urc_handlers.h"
#include "settings.h"

// ----------------------------------------------------------------
// WiFi Functions
// ----------------------------------------------------------------

void showWifiStatus(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Status ---\n");
    
    // Get connection status
    uCxWifiStationStatus_t status;
    
    // Check if connected
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &status)) {
        int32_t connState = status.rspWifiStatusIdInt.int_val;
        uCxEnd(&gUcxHandle);
        
        if (connState == 2) {
            printf("Status: Connected\n");
            
            // Get SSID
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_SSID, &status)) {
                printf("SSID: %s\n", status.rspWifiStatusIdStr.ssid);
                uCxEnd(&gUcxHandle);
            }
            
            // Get RSSI
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &status)) {
                int32_t rssi = status.rspWifiStatusIdInt.int_val;
                if (rssi != -32768) {
                    printf("RSSI: %d dBm\n", rssi);
                }
                uCxEnd(&gUcxHandle);
            }
        } else {
            printf("Status: Not connected\n");
        }
    } else {
        printf("ERROR: Failed to get Wi-Fi status\n");
    }
}

void wifiScan(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Network Scan ---\n");
    printf("Scanning for networks... (this may take several seconds)\n\n");
    
    // Set 60 second timeout for scan command (scan can take 30-40 seconds for many networks)
    uCxAtClientSetCommandTimeout(gUcxHandle.pAtClient, 60000, false);
    
    // Start WiFi scan
    uCxWifiStationScanDefaultBegin(&gUcxHandle);
    
    int networkCount = 0;
    // Track both 2.4 GHz (channels 1-14) and 5 GHz channels (up to 165)
    int channelCount2_4[15] = {0}; // 2.4 GHz: channels 1-14
    int channelCount5[166] = {0};   // 5 GHz: channels up to 165
    uCxWifiStationScanDefault_t network;
    
    // Get scan results
    // Process responses until GetNext() returns false (no more responses or timeout/OK)
    // Note: Some firmware versions may send malformed URCs, so we continue even after errors
    bool gotResponse;
    do {
        gotResponse = uCxWifiStationScanDefaultGetNext(&gUcxHandle, &network);
        if (gotResponse) {
            networkCount++;
            printf("Network %d:\n", networkCount);
            // Display SSID (or "Hidden Network" if empty)
            if (network.ssid && network.ssid[0] != '\0') {
                printf("  SSID: %s\n", network.ssid);
            } else {
                printf("  SSID: <Hidden Network>\n");
            }
        printf("  BSSID: %02X:%02X:%02X:%02X:%02X:%02X\n",
               network.bssid.address[0], network.bssid.address[1],
               network.bssid.address[2], network.bssid.address[3],
               network.bssid.address[4], network.bssid.address[5]);
        printf("  Channel: %d", network.channel);
        // Indicate band
        if (network.channel >= 1 && network.channel <= 14) {
            printf(" (2.4 GHz)\n");
        } else if (network.channel >= 36) {
            printf(" (5 GHz)\n");
        } else {
            printf("\n");
        }
        printf("  RSSI: %d dBm\n", network.rssi);
        
        // Track channel usage
        if (network.channel >= 1 && network.channel <= 14) {
            channelCount2_4[network.channel]++;
        } else if (network.channel >= 36 && network.channel <= 165) {
            channelCount5[network.channel]++;
        }
        
        // Print security type based on authentication suites
        printf("  Security: ");
        if (network.authentication_suites == 0) {
            printf("Open\n");
        } else {
            if (network.authentication_suites & (1 << 5)) printf("WPA3 ");
            if (network.authentication_suites & (1 << 4)) printf("WPA2 ");
            if (network.authentication_suites & (1 << 3)) printf("WPA ");
            if (network.authentication_suites & (1 << 1)) printf("PSK ");
        }
        printf("\n");
        }
    } while (gotResponse);
    
    uCxEnd(&gUcxHandle);
    
    if (networkCount == 0) {
        printf("No networks found.\n");
    } else {
        printf("Found %d network(s).\n", networkCount);
        
        // Display channel usage summary
        printf("\n==============================================================\n");
        printf("                   CHANNEL USAGE SUMMARY\n");
        printf("==============================================================\n");
        
        // 2.4 GHz Band
        printf("\n2.4 GHz Band (Channels 1-14):\n");
        int maxNetworks2_4 = 0;
        int hasAny2_4 = 0;
        for (int i = 1; i <= 14; i++) {
            if (channelCount2_4[i] > 0) {
                hasAny2_4 = 1;
                if (channelCount2_4[i] > maxNetworks2_4) {
                    maxNetworks2_4 = channelCount2_4[i];
                }
            }
        }
        
        if (hasAny2_4) {
            for (int i = 1; i <= 14; i++) {
                if (channelCount2_4[i] > 0) {
                    printf("Channel %2d: %2d network(s) ", i, channelCount2_4[i]);
                    
                    // Visual bar graph
                    printf("[");
                    int barLen = (channelCount2_4[i] * 30) / (maxNetworks2_4 > 0 ? maxNetworks2_4 : 1);
                    for (int j = 0; j < barLen; j++) {
                        printf("=");
                    }
                    for (int j = barLen; j < 30; j++) {
                        printf(" ");
                    }
                    printf("]");
                    
                    // Congestion indicator
                    if (channelCount2_4[i] >= 5) {
                        printf(" - CONGESTED");
                    } else if (channelCount2_4[i] >= 3) {
                        printf(" - Busy");
                    } else if (channelCount2_4[i] == 1) {
                        printf(" - Clear");
                    }
                    printf("\n");
                }
            }
            
            // Recommend best 2.4 GHz channels (1, 6, 11 are non-overlapping)
            printf("\n** Best channel for creating a new 2.4 GHz access point **\n");
            printf("(Non-overlapping channels: 1, 6, 11)\n");
            int bestChannel = 0;
            int minCount = 999;
            int recommendedChannels[] = {1, 6, 11};
            for (int i = 0; i < 3; i++) {
                int ch = recommendedChannels[i];
                if (channelCount2_4[ch] < minCount) {
                    minCount = channelCount2_4[ch];
                    bestChannel = ch;
                }
            }
            if (bestChannel > 0) {
                printf("Least congested: Channel %d (%d existing network(s))\n", bestChannel, minCount);
            }
        } else {
            printf("No 2.4 GHz networks detected\n");
        }
        
        // 5 GHz Band
        printf("\n5 GHz Band:\n");
        int maxNetworks5 = 0;
        int hasAny5 = 0;
        for (int i = 36; i <= 165; i++) {
            if (channelCount5[i] > 0) {
                hasAny5 = 1;
                if (channelCount5[i] > maxNetworks5) {
                    maxNetworks5 = channelCount5[i];
                }
            }
        }
        
        if (hasAny5) {
            for (int i = 36; i <= 165; i++) {
                if (channelCount5[i] > 0) {
                    printf("Channel %3d: %2d network(s) ", i, channelCount5[i]);
                    
                    // Visual bar graph
                    printf("[");
                    int barLen = (channelCount5[i] * 30) / (maxNetworks5 > 0 ? maxNetworks5 : 1);
                    for (int j = 0; j < barLen; j++) {
                        printf("=");
                    }
                    for (int j = barLen; j < 30; j++) {
                        printf(" ");
                    }
                    printf("]");
                    
                    // Congestion indicator
                    if (channelCount5[i] >= 5) {
                        printf(" - CONGESTED");
                    } else if (channelCount5[i] >= 3) {
                        printf(" - Busy");
                    } else if (channelCount5[i] == 1) {
                        printf(" - Clear");
                    }
                    printf("\n");
                }
            }
            
            // Find best 5 GHz channel (only from valid channels that were detected)
            printf("\n** Best channel for creating a new 5 GHz access point **\n");
            printf("(5 GHz offers less interference and more bandwidth)\n");
            int best5GHz = 0;
            int min5Count = 999;
            
            // Valid 5 GHz channels (most common)
            int valid5GHzChannels[] = {36, 40, 44, 48, 52, 56, 60, 64, 
                                       100, 104, 108, 112, 116, 120, 124, 128, 132, 136, 140, 144,
                                       149, 153, 157, 161, 165};
            int numValid5GHz = sizeof(valid5GHzChannels) / sizeof(valid5GHzChannels[0]);
            
            for (int i = 0; i < numValid5GHz; i++) {
                int ch = valid5GHzChannels[i];
                if (channelCount5[ch] < min5Count) {
                    min5Count = channelCount5[ch];
                    best5GHz = ch;
                }
            }
            
            if (best5GHz > 0 && min5Count < 999) {
                printf("Least congested: Channel %d (%d existing network(s))\n", best5GHz, min5Count);
            }
        } else {
            printf("No 5 GHz networks detected\n");
        }
        
        printf("==============================================================\n");
    }
}

void testConnectivity(const char *gateway, const char *ssid, int32_t rssi, int32_t channel)
{
    printf("\n--- Testing Network Connectivity ---\n");
    
    // Variables to store results for summary
    int32_t localPingAvg = 0;
    int32_t internetPingAvg = 0;
    bool localSuccess = false;
    bool internetSuccess = false;
    
    // Warm-up: Single ping to gateway (helps wake up Wi-Fi from power save mode)
    printf("\nPerforming warm-up ping to gateway...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    if (uCxDiagnosticsPing2(&gUcxHandle, gateway, 1) == 0) {
        waitEvent(URC_FLAG_PING_COMPLETE, 10);
        printf("   Warm-up complete (%d ms)\n", gPingAvgTime);
    }
    
    // Test 1: Ping gateway (local network) - 4 pings
    printf("\n1. Testing local network (gateway: %s)...\n", gateway);
    printf("   Sending 4 pings...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    if (uCxDiagnosticsPing2(&gUcxHandle, gateway, 4) == 0) {
        // Wait for ping complete URC event (max 15 seconds for 4 pings)
        if (waitEvent(URC_FLAG_PING_COMPLETE, 15)) {
            if (gPingSuccess > 0) {
                localPingAvg = gPingAvgTime;
                localSuccess = true;
                printf("   ✓ Local network OK: %d/%d packets, avg %d ms\n", 
                       gPingSuccess, gPingSuccess + gPingFailed, gPingAvgTime);
            } else {
                printf("   ✗ Local network FAILED: No response from gateway\n");
            }
        } else {
            printf("   ✗ Local network test TIMEOUT\n");
        }
    } else {
        printf("   ✗ Failed to start ping test\n");
    }
    
    // Reset counters for second test
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    // Test 2: Ping Google DNS (internet connectivity) - 4 pings
    printf("\n2. Testing internet connectivity (8.8.8.8)...\n");
    printf("   Sending 4 pings...\n");
    gPingSuccess = 0;
    gPingFailed = 0;
    gPingAvgTime = 0;
    
    if (uCxDiagnosticsPing2(&gUcxHandle, "8.8.8.8", 4) == 0) {
        // Wait for ping complete URC event (max 15 seconds for 4 pings)
        if (waitEvent(URC_FLAG_PING_COMPLETE, 15)) {
            if (gPingSuccess > 0) {
                internetPingAvg = gPingAvgTime;
                internetSuccess = true;
                printf("   ✓ Internet access OK: %d/%d packets, avg %d ms\n", 
                       gPingSuccess, gPingSuccess + gPingFailed, gPingAvgTime);
            } else {
                printf("   ✗ Internet access FAILED: No response\n");
            }
        } else {
            printf("   ✗ Internet test TIMEOUT\n");
        }
    } else {
        printf("   ✗ Failed to start ping test\n");
    }
    
    // Display connection summary
    printf("\n");
    printf("==============================================================\n");
    printf("                   CONNECTION SUMMARY\n");
    printf("==============================================================\n");
    printf("Network:           %s\n", ssid);
    printf("Channel:           %d\n", channel);
    
    // Signal strength assessment
    const char *signalQuality;
    if (rssi >= -50) {
        signalQuality = "Excellent";
    } else if (rssi >= -60) {
        signalQuality = "Very Good";
    } else if (rssi >= -70) {
        signalQuality = "Good";
    } else if (rssi >= -80) {
        signalQuality = "Fair";
    } else {
        signalQuality = "Poor";
    }
    printf("Signal Strength:   %d dBm (%s)\n", rssi, signalQuality);
    
    // Local network ping
    if (localSuccess) {
        const char *localQuality = localPingAvg < 10 ? "Excellent" : 
                                   localPingAvg < 50 ? "Good" : "Fair";
        printf("Local Ping:        %d ms (%s)\n", localPingAvg, localQuality);
    } else {
        printf("Local Ping:        FAILED\n");
    }
    
    // Internet ping
    if (internetSuccess) {
        const char *internetQuality = internetPingAvg < 20 ? "Excellent" : 
                                      internetPingAvg < 50 ? "Very Good" :
                                      internetPingAvg < 100 ? "Good" : "Fair";
        printf("Internet Ping:     %d ms (%s)\n", internetPingAvg, internetQuality);
    } else {
        printf("Internet Ping:     FAILED\n");
    }
    
    // Overall assessment
    printf("==============================================================\n");
    const char *overall;
    if (localSuccess && internetSuccess && rssi >= -70 && localPingAvg < 50 && internetPingAvg < 100) {
        overall = "EXCELLENT - Ready for all applications";
    } else if (localSuccess && internetSuccess && rssi >= -80) {
        overall = "GOOD - Suitable for most applications";
    } else if (localSuccess && rssi >= -80) {
        overall = "FAIR - Local network OK, check internet";
    } else if (localSuccess) {
        overall = "MARGINAL - Weak signal or connectivity issues";
    } else {
        overall = "POOR - Connection not stable";
    }
    printf("Overall:           %s\n", overall);
    printf("==============================================================\n");
}

void wifiConnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Connect ---\n");
    
    // Check if already connected and disconnect if necessary
    uCxWifiStationStatus_t connStatus;
    if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CONNECTION, &connStatus)) {
        int32_t connState = connStatus.rspWifiStatusIdInt.int_val;
        uCxEnd(&gUcxHandle);
        
        if (connState == 2) {  // 2 = Connected
            printf("Already connected to Wi-Fi. Disconnecting first...\n");
            
            if (uCxWifiStationDisconnect(&gUcxHandle) == 0) {
                printf("Disconnect command sent successfully.\n");
                Sleep(1000);  // Give module time to disconnect
            } else {
                printf("Warning: Disconnect command failed, attempting to connect anyway...\n");
            }
        }
    }
    
    char ssid[64];
    char password[64];
    
    // Show saved SSID if available
    if (strlen(gWifiSsid) > 0) {
        printf("Last SSID: %s\n", gWifiSsid);
    }
    
    printf("Enter SSID (or press Enter to use saved): ");
    if (fgets(ssid, sizeof(ssid), stdin)) {
        // Remove trailing newline
        char *end = strchr(ssid, '\n');
        if (end) *end = '\0';
        
        // If empty and we have saved SSID, use it
        if (strlen(ssid) == 0 && strlen(gWifiSsid) > 0) {
            strncpy(ssid, gWifiSsid, sizeof(ssid) - 1);
            ssid[sizeof(ssid) - 1] = '\0';
            strncpy(password, gWifiPassword, sizeof(password) - 1);
            password[sizeof(password) - 1] = '\0';
            printf("Using saved credentials for '%s'\n", ssid);
        } else {
            printf("Enter password (or press Enter for open network): ");
            if (fgets(password, sizeof(password), stdin)) {
                // Remove trailing newline
                end = strchr(password, '\n');
                if (end) *end = '\0';
            }
        }
        
        printf("Connecting to '%s'...\n", ssid);
        
        // Set connection parameters (wlan_handle = 0, default)
        if (uCxWifiStationSetConnectionParams(&gUcxHandle, 0, ssid) != 0) {
            printf("ERROR: Failed to set connection parameters\n");
            return;
        }
        
        // Set security based on password
        if (strlen(password) > 0) {
            // WPA2/WPA3 with password (threshold = WPA2 or higher)
            printf("Setting WPA2/WPA3 security...\n");
            if (uCxWifiStationSetSecurityWpa(&gUcxHandle, 0, password, U_WPA_THRESHOLD_WPA2) != 0) {
                printf("ERROR: Failed to set WPA security\n");
                return;
            }
        } else {
            // Open network (no password)
            printf("Setting open security (no password)...\n");
            if (uCxWifiStationSetSecurityOpen(&gUcxHandle, 0) != 0) {
                printf("ERROR: Failed to set open security\n");
                return;
            }
        }
        
        // Clear any pending network event flags before connecting
        U_CX_MUTEX_LOCK(gUrcMutex);
        gUrcEventFlags &= ~(URC_FLAG_NETWORK_UP | URC_FLAG_NETWORK_DOWN);
        U_CX_MUTEX_UNLOCK(gUrcMutex);
        
        // Initiate connection
        printf("Initiating connection...\n");
        if (uCxWifiStationConnect(&gUcxHandle, 0) != 0) {
            printf("ERROR: Failed to initiate connection\n");
            return;
        }
        
        // Wait for network up event (using URC handler)
        printf("Waiting for network up event...\n");
        if (waitEvent(URC_FLAG_NETWORK_UP, 20)) {
            printf("Successfully connected to '%s'\n", ssid);
            
            // Get RSSI
            int32_t rssi = -100;  // Default value
            uCxWifiStationStatus_t rssiStatus;
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_RSSI, &rssiStatus)) {
                rssi = rssiStatus.rspWifiStatusIdInt.int_val;
                if (rssi != -32768) {
                    printf("Signal strength: %d dBm\n", rssi);
                }
                uCxEnd(&gUcxHandle);
            }
            
            // Get IP address using WiFi Station Network Status (AT+UWSNST)
            uSockIpAddress_t ipAddr;
            char ipStr[40];  // Allow for IPv6
            char gatewayStr[40] = "";  // Store gateway for ping test
            
            if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_IPV4, &ipAddr) == 0) {
                if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                    printf("IP address: %s\n", ipStr);
                }
            }
            
            if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_SUBNET, &ipAddr) == 0) {
                if (uCxIpAddressToString(&ipAddr, ipStr, sizeof(ipStr)) > 0) {
                    printf("Subnet mask: %s\n", ipStr);
                }
            }
            
            if (uCxWifiStationGetNetworkStatus(&gUcxHandle, U_STATUS_ID_GATE_WAY, &ipAddr) == 0) {
                if (uCxIpAddressToString(&ipAddr, gatewayStr, sizeof(gatewayStr)) > 0) {
                    printf("Gateway: %s\n", gatewayStr);
                }
            }
            
            // Get channel number
            int32_t channel = 0;
            uCxWifiStationStatus_t channelStatus;
            if (uCxWifiStationStatusBegin(&gUcxHandle, U_WIFI_STATUS_ID_CHANNEL, &channelStatus)) {
                channel = channelStatus.rspWifiStatusIdInt.int_val;
                uCxEnd(&gUcxHandle);
            }
            
            // Save Wi-Fi credentials for next time
            strncpy(gWifiSsid, ssid, sizeof(gWifiSsid) - 1);
            gWifiSsid[sizeof(gWifiSsid) - 1] = '\0';
            strncpy(gWifiPassword, password, sizeof(gWifiPassword) - 1);
            gWifiPassword[sizeof(gWifiPassword) - 1] = '\0';
            saveSettings();
            
            // Test connectivity (ping gateway and internet) with connection summary
            if (strlen(gatewayStr) > 0) {
                testConnectivity(gatewayStr, ssid, rssi, channel);
            }
        } else {
            printf("Connection failed - timeout waiting for network up event (IP configuration)\n");
            printf("Wi-Fi link is established but network layer failed to initialize.\n");
        }
    }
}

void wifiDisconnect(void)
{
    if (!gConnected) {
        printf("ERROR: Not connected to device\n");
        return;
    }
    
    printf("\n--- Wi-Fi Disconnect ---\n");
    printf("Disconnecting from Wi-Fi...\n");
    
    // Clear any pending disconnect event flags
    U_CX_MUTEX_LOCK(gUrcMutex);
    gUrcEventFlags &= ~(URC_FLAG_NETWORK_DOWN | URC_FLAG_WIFI_LINK_DOWN);
    U_CX_MUTEX_UNLOCK(gUrcMutex);
    
    if (uCxWifiStationDisconnect(&gUcxHandle) == 0) {
        // Wait for Wi-Fi link down URC event (max 3 seconds)
        if (waitEvent(URC_FLAG_WIFI_LINK_DOWN, 3)) {
            printf("Disconnected successfully.\n");
        } else {
            printf("Disconnect command sent (waiting for confirmation timed out).\n");
        }
    } else {
        printf("ERROR: Failed to disconnect\n");
    }
}

void wifiFunctionsMenu(void)
{
    printf("--- Wi-Fi Functions ---\n");
    printf("  NOTE: Requires Active Wi-Fi connection!\n");
    printf("  [1] Socket menu (TCP/UDP)\n");
    printf("  [2] MQTT (publish/subscribe)\n");
    printf("  [3] HTTP Client (GET/POST/PUT)\n");
    printf("  [4] Security/TLS (certificates)\n");
    printf("  [0] Back to main menu  [q] Quit\n");
    
    // This is a menu display function, actual handling is in ui_menus.c handleUserInput()
    // Just set state back to main for now
    gMenuState = MENU_MAIN;
}
