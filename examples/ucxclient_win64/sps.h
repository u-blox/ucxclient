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

#ifndef SPS_H
#define SPS_H

#include "common.h"

// ----------------------------------------------------------------
// Serial Port Service (SPS) Functions
// ----------------------------------------------------------------

/**
 * Enable SPS service
 */
void spsEnableService(void);

/**
 * Connect via SPS
 */
void spsConnect(void);

/**
 * Send data via SPS
 */
void spsSendData(void);

/**
 * Read data via SPS
 */
void spsReadData(void);

#endif // SPS_H
