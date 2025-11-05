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

#include "settings.h"

// ----------------------------------------------------------------
// Password Obfuscation Helper Functions
// ----------------------------------------------------------------

// Simple XOR-based obfuscation (not cryptographically secure, but better than plaintext)
#define OBFUSCATION_KEY "uBloxUcxClient"

void obfuscatePassword(const char *input, char *output, size_t outputSize)
{
    size_t inputLen = strlen(input);
    size_t keyLen = strlen(OBFUSCATION_KEY);
    
    if (inputLen == 0) {
        output[0] = '\0';
        return;
    }
    
    // Convert to hex with XOR
    size_t outIdx = 0;
    for (size_t i = 0; i < inputLen && outIdx < outputSize - 3; i++) {
        unsigned char c = input[i] ^ OBFUSCATION_KEY[i % keyLen];
        snprintf(&output[outIdx], outputSize - outIdx, "%02x", c);
        outIdx += 2;
    }
    output[outIdx] = '\0';
}

void deobfuscatePassword(const char *input, char *output, size_t outputSize)
{
    size_t inputLen = strlen(input);
    size_t keyLen = strlen(OBFUSCATION_KEY);
    
    if (inputLen == 0 || inputLen % 2 != 0) {
        output[0] = '\0';
        return;
    }
    
    // Convert from hex with XOR
    size_t outIdx = 0;
    for (size_t i = 0; i < inputLen && outIdx < outputSize - 1; i += 2) {
        char hexByte[3] = {input[i], input[i+1], '\0'};
        unsigned char c = (unsigned char)strtol(hexByte, NULL, 16);
        output[outIdx] = c ^ OBFUSCATION_KEY[outIdx % keyLen];
        outIdx++;
    }
    output[outIdx] = '\0';
}

// ----------------------------------------------------------------
// Firmware Path Management
// ----------------------------------------------------------------

const char* getProductFirmwarePath(const char *productName)
{
    for (int i = 0; i < gProductFirmwarePathCount; i++) {
        if (strcmp(gProductFirmwarePaths[i].productName, productName) == 0) {
            return gProductFirmwarePaths[i].lastFirmwarePath;
        }
    }
    return ""; // Not found
}

void setProductFirmwarePath(const char *productName, const char *firmwarePath)
{
    // Check if product already exists
    for (int i = 0; i < gProductFirmwarePathCount; i++) {
        if (strcmp(gProductFirmwarePaths[i].productName, productName) == 0) {
            // Update existing entry
            strncpy(gProductFirmwarePaths[i].lastFirmwarePath, firmwarePath, 
                    sizeof(gProductFirmwarePaths[i].lastFirmwarePath) - 1);
            gProductFirmwarePaths[i].lastFirmwarePath[sizeof(gProductFirmwarePaths[i].lastFirmwarePath) - 1] = '\0';
            return;
        }
    }
    
    // Add new entry if we have space
    if (gProductFirmwarePathCount < MAX_PRODUCT_PATHS) {
        strncpy(gProductFirmwarePaths[gProductFirmwarePathCount].productName, productName,
                sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].productName) - 1);
        gProductFirmwarePaths[gProductFirmwarePathCount].productName[sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].productName) - 1] = '\0';
        
        strncpy(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath, firmwarePath,
                sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath) - 1);
        gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath[sizeof(gProductFirmwarePaths[gProductFirmwarePathCount].lastFirmwarePath) - 1] = '\0';
        
        gProductFirmwarePathCount++;
    }
}

// ----------------------------------------------------------------
// Settings Load/Save Functions
// ----------------------------------------------------------------

void loadSettings(void)
{
    FILE *f = fopen(gSettingsFilePath, "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f)) {
            // Remove trailing newline/whitespace
            char *end = strchr(line, '\n');
            if (end) *end = '\0';
            end = strchr(line, '\r');
            if (end) *end = '\0';
            
            if (strncmp(line, "last_port=", 10) == 0) {
                strncpy(gComPort, line + 10, sizeof(gComPort) - 1);
                gComPort[sizeof(gComPort) - 1] = '\0';
                printf("Loaded last port from settings: %s\n", gComPort);
            }
            else if (strncmp(line, "last_device=", 12) == 0) {
                strncpy(gLastDeviceModel, line + 12, sizeof(gLastDeviceModel) - 1);
                gLastDeviceModel[sizeof(gLastDeviceModel) - 1] = '\0';
                if (strlen(gLastDeviceModel) > 0) {
                    printf("Loaded last device from settings: %s\n", gLastDeviceModel);
                }
            }
            else if (strncmp(line, "wifi_ssid=", 10) == 0) {
                strncpy(gWifiSsid, line + 10, sizeof(gWifiSsid) - 1);
                gWifiSsid[sizeof(gWifiSsid) - 1] = '\0';
            }
            else if (strncmp(line, "wifi_password=", 14) == 0) {
                // Deobfuscate password from hex
                char obfuscated[128];
                strncpy(obfuscated, line + 14, sizeof(obfuscated) - 1);
                obfuscated[sizeof(obfuscated) - 1] = '\0';
                deobfuscatePassword(obfuscated, gWifiPassword, sizeof(gWifiPassword));
            }
            else if (strncmp(line, "remote_address=", 15) == 0) {
                strncpy(gRemoteAddress, line + 15, sizeof(gRemoteAddress) - 1);
                gRemoteAddress[sizeof(gRemoteAddress) - 1] = '\0';
            }
            else if (strncmp(line, "firmware_path_", 14) == 0) {
                // Dynamic firmware path: firmware_path_<PRODUCT>=<path>
                // e.g., "firmware_path_NORA-W36=/path/to/firmware.bin"
                char *equals = strchr(line + 14, '=');
                if (equals) {
                    // Extract product name (between "firmware_path_" and "=")
                    size_t productNameLen = equals - (line + 14);
                    if (productNameLen > 0 && productNameLen < 64) {
                        char productName[64];
                        strncpy(productName, line + 14, productNameLen);
                        productName[productNameLen] = '\0';
                        
                        // Convert underscores back to hyphens (NORA_W36 -> NORA-W36)
                        for (size_t i = 0; i < productNameLen; i++) {
                            if (productName[i] == '_') productName[i] = '-';
                        }
                        
                        // Store the firmware path for this product
                        setProductFirmwarePath(productName, equals + 1);
                    }
                }
            }
        }
        fclose(f);
    }
}

void saveSettings(void)
{
    FILE *f = fopen(gSettingsFilePath, "w");
    if (f) {
        fprintf(f, "last_port=%s\n", gComPort);
        fprintf(f, "last_device=%s\n", gLastDeviceModel);
        fprintf(f, "wifi_ssid=%s\n", gWifiSsid);
        
        // Obfuscate password before saving
        char obfuscatedPassword[128];
        obfuscatePassword(gWifiPassword, obfuscatedPassword, sizeof(obfuscatedPassword));
        fprintf(f, "wifi_password=%s\n", obfuscatedPassword);
        
        fprintf(f, "remote_address=%s\n", gRemoteAddress);
        
        // Save dynamic per-product firmware paths
        for (int i = 0; i < gProductFirmwarePathCount; i++) {
            if (gProductFirmwarePaths[i].productName[0] != '\0' && 
                gProductFirmwarePaths[i].lastFirmwarePath[0] != '\0') {
                // Convert hyphens to underscores for INI file compatibility
                // (NORA-W36 -> NORA_W36)
                char productKey[64];
                strncpy(productKey, gProductFirmwarePaths[i].productName, sizeof(productKey) - 1);
                productKey[sizeof(productKey) - 1] = '\0';
                for (size_t j = 0; j < strlen(productKey); j++) {
                    if (productKey[j] == '-') productKey[j] = '_';
                }
                fprintf(f, "firmware_path_%s=%s\n", productKey, gProductFirmwarePaths[i].lastFirmwarePath);
            }
        }
        
        fclose(f);
    }
}
