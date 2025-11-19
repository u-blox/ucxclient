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

/** @file
 * @brief Example port layer header
 */

#ifndef U_PORT_H
#define U_PORT_H

#include "u_cx_at_client.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Port Initialization
  *
  * Must be called before any other port function is called.
  * The supplied pClient will be initialized by this function, so
  * user should not call uCxAtClientInit() when using this port layer.
  *
  * @param[out] pClient:  pointer to a uCxAtClient_t struct to initialize.
  */
void uPortAtInit(uCxAtClient_t *pClient);

/**
  * @brief Open AT UART interface
  *
  * After this function has been called you may use any of the uCxAtClientXxx
  * functions. You may also want to initialize the uCx API by calling uCxInit().
  *
  * @param[in] pClient:         pointer to the uCxAtClient_t struct initialized with uPortAtInit().
  * @param[in] pDevName:        UART device name (example: "/dev/ttyUSB0").
  * @param     baudRate:        UART baudrate.
  * @param     useFlowControl:  set to true to use CTS/RTS flow control.
  * @return:                    true on success, false on failure.
  */
bool uPortAtOpen(uCxAtClient_t *pClient, const char *pDevName, int baudRate, bool useFlowControl);

/**
  * @brief Close AT UART interface
  *
  * Call this to close an AT UART interface opened with uPortAtOpen().
  *
  * @param[in] pClient:  pointer to the uCxAtClient_t struct initialized with uPortAtInit().
  */
void uPortAtClose(uCxAtClient_t *pClient);

#endif