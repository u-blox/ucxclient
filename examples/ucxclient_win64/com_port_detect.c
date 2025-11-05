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

#include "com_port_detect.h"

// TODO: Extract from original ucxclient_win64.c lines ~3625-4252

void listAvailableComPorts(char *recommendedPort, size_t recommendedPortSize, 
                           char *recommendedDevice, size_t recommendedDeviceSize)
{
    printf("TODO: listAvailableComPorts() - needs implementation\n");
    if (recommendedPort && recommendedPortSize > 0) recommendedPort[0] = '\0';
    if (recommendedDevice && recommendedDeviceSize > 0) recommendedDevice[0] = '\0';
}

char* selectComPortFromList(const char *recommendedPort)
{
    printf("TODO: selectComPortFromList() - needs implementation\n");
    (void)recommendedPort;
    return NULL;
}

bool initFtd2xxLibrary(void)
{
    printf("TODO: initFtd2xxLibrary() - needs implementation\n");
    return false;
}
