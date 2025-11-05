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

#ifndef FIRMWARE_UPDATE_H
#define FIRMWARE_UPDATE_H

#include "common.h"

// ----------------------------------------------------------------
// Firmware Update Functions
// ----------------------------------------------------------------

/**
 * Firmware update progress callback
 * 
 * @param totalBytes Total bytes to transfer
 * @param bytesTransferred Bytes transferred so far
 * @param percentComplete Percentage complete
 * @param pUserData User data pointer
 */
void firmwareUpdateProgress(size_t totalBytes, size_t bytesTransferred, 
                           int32_t percentComplete, void *pUserData);

/**
 * Download firmware from GitHub
 * 
 * @param product Product name
 * @param downloadedPath Buffer for downloaded path
 * @param pathSize Size of downloadedPath buffer
 * @return true if successful, false otherwise
 */
bool downloadFirmwareFromGitHub(const char *product, char *downloadedPath, size_t pathSize);

/**
 * Download firmware from GitHub interactively
 * 
 * @param downloadedPath Buffer for downloaded path
 * @param pathSize Size of downloadedPath buffer
 * @return true if successful, false otherwise
 */
bool downloadFirmwareFromGitHubInteractive(char *downloadedPath, size_t pathSize);

/**
 * Extract ZIP file
 * 
 * @param zipPath Path to ZIP file
 * @param destFolder Destination folder
 * @return true if successful, false otherwise
 */
bool extractZipFile(const char *zipPath, const char *destFolder);

/**
 * Save binary file
 * 
 * @param filepath File path
 * @param data Data buffer
 * @param size Data size
 * @return true if successful, false otherwise
 */
bool saveBinaryFile(const char *filepath, const char *data, size_t size);

#endif // FIRMWARE_UPDATE_H
