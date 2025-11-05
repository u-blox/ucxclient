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

#ifndef COM_PORT_DETECT_H
#define COM_PORT_DETECT_H

#include "common.h"

// ----------------------------------------------------------------
// COM Port Detection Functions
// ----------------------------------------------------------------

/**
 * List available COM ports and recommend one if possible
 * 
 * @param recommendedPort Buffer for recommended port
 * @param recommendedPortSize Size of recommendedPort buffer
 * @param recommendedDevice Buffer for recommended device description
 * @param recommendedDeviceSize Size of recommendedDevice buffer
 */
void listAvailableComPorts(char *recommendedPort, size_t recommendedPortSize, 
                           char *recommendedDevice, size_t recommendedDeviceSize);

/**
 * Select COM port from list interactively
 * 
 * @param recommendedPort Recommended port to highlight
 * @return Selected COM port (caller must free), or NULL if cancelled
 */
char* selectComPortFromList(const char *recommendedPort);

/**
 * Initialize FTDI library (dynamic loading)
 * 
 * @return true if successful, false otherwise
 */
bool initFtd2xxLibrary(void);

#endif // COM_PORT_DETECT_H
