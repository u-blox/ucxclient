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

#include "device_connection.h"

// TODO: Extract from original ucxclient_win64.c lines ~3189-3356, ~4515-4667

bool connectDevice(const char *comPort)
{
    printf("TODO: connectDevice() - needs implementation\n");
    (void)comPort;
    return false;
}

bool quickConnectToLastDevice(void)
{
    printf("TODO: quickConnectToLastDevice() - needs implementation\n");
    return false;
}

void disconnectDevice(void)
{
    printf("TODO: disconnectDevice() - needs implementation\n");
}

void executeAtTest(void)
{
    printf("TODO: executeAtTest() - needs implementation\n");
}

void executeAti9(void)
{
    printf("TODO: executeAti9() - needs implementation\n");
}

void executeModuleReboot(void)
{
    printf("TODO: executeModuleReboot() - needs implementation\n");
}

void getExecutableDirectory(char *buffer, size_t bufferSize)
{
    // Minimal implementation for Phase 1
    GetModuleFileName(NULL, buffer, (DWORD)bufferSize);
    char *lastSlash = strrchr(buffer, '\\');
    if (lastSlash) {
        *(lastSlash + 1) = '\0';
    }
}
