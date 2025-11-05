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

#ifndef DEVICE_CONNECTION_H
#define DEVICE_CONNECTION_H

#include "common.h"

// ----------------------------------------------------------------
// Device Connection Functions
// ----------------------------------------------------------------

/**
 * Connect to device on specified COM port
 * 
 * @param comPort COM port name (e.g., "COM31")
 * @return true if successful, false otherwise
 */
bool connectDevice(const char *comPort);

/**
 * Quick connect to last used device
 * 
 * @return true if successful, false otherwise
 */
bool quickConnectToLastDevice(void);

/**
 * Disconnect from device
 */
void disconnectDevice(void);

/**
 * Execute AT test command
 */
void executeAtTest(void);

/**
 * Execute ATI9 command (product info)
 */
void executeAti9(void);

/**
 * Execute module reboot
 */
void executeModuleReboot(void);

/**
 * Get executable directory path
 * 
 * @param buffer Buffer for directory path
 * @param bufferSize Size of buffer
 */
void getExecutableDirectory(char *buffer, size_t bufferSize);

#endif // DEVICE_CONNECTION_H
