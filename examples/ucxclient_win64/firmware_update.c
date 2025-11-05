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

#include "firmware_update.h"

// TODO: Extract from original ucxclient_win64.c lines ~3115-3265, plus firmware download functions

void firmwareUpdateProgress(size_t totalBytes, size_t bytesTransferred, 
                           int32_t percentComplete, void *pUserData)
{
    printf("TODO: firmwareUpdateProgress() - needs implementation\n");
    (void)totalBytes;
    (void)bytesTransferred;
    (void)percentComplete;
    (void)pUserData;
}

bool downloadFirmwareFromGitHub(const char *product, char *downloadedPath, size_t pathSize)
{
    printf("TODO: downloadFirmwareFromGitHub() - needs implementation\n");
    (void)product;
    if (downloadedPath && pathSize > 0) downloadedPath[0] = '\0';
    return false;
}

bool downloadFirmwareFromGitHubInteractive(char *downloadedPath, size_t pathSize)
{
    printf("TODO: downloadFirmwareFromGitHubInteractive() - needs implementation\n");
    if (downloadedPath && pathSize > 0) downloadedPath[0] = '\0';
    return false;
}

bool extractZipFile(const char *zipPath, const char *destFolder)
{
    printf("TODO: extractZipFile() - needs implementation\n");
    (void)zipPath;
    (void)destFolder;
    return false;
}

bool saveBinaryFile(const char *filepath, const char *data, size_t size)
{
    printf("TODO: saveBinaryFile() - needs implementation\n");
    (void)filepath;
    (void)data;
    (void)size;
    return false;
}
