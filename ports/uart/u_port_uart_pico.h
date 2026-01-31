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
 * @brief Raspberry Pi Pico UART port configuration header
 *
 * Supports RP2040 (Pico), RP2350 (Pico 2)
 * Defines the default UART pins and instance for NORA-W36 communication.
 * Can be overridden by defining these before including this header.
 */

#ifndef U_PORT_UART_PICO_H
#define U_PORT_UART_PICO_H

#include "hardware/uart.h"

/* ----------------------------------------------------------------
 * DEFAULT PIN CONFIGURATION
 * 
 * Default: UART0 on GP0 (TX) and GP1 (RX)
 * This matches the typical Pico wiring for NORA-W36:
 *   Pico GP0 (TX) -> NORA-W36 RX
 *   Pico GP1 (RX) -> NORA-W36 TX
 * -------------------------------------------------------------- */

#ifndef U_PORT_UART_INSTANCE
#define U_PORT_UART_INSTANCE    uart0
#endif

#ifndef U_PORT_UART_TX_PIN
#define U_PORT_UART_TX_PIN      0   /* GP0 */
#endif

#ifndef U_PORT_UART_RX_PIN
#define U_PORT_UART_RX_PIN      1   /* GP1 */
#endif

#ifndef U_PORT_UART_CTS_PIN
#define U_PORT_UART_CTS_PIN     2   /* GP2 - optional, set to -1 to disable */
#endif

#ifndef U_PORT_UART_RTS_PIN
#define U_PORT_UART_RTS_PIN     3   /* GP3 - optional, set to -1 to disable */
#endif

#ifndef U_PORT_UART_RX_BUFFER_SIZE
#define U_PORT_UART_RX_BUFFER_SIZE  2048
#endif

#endif /* U_PORT_UART_PICO_H */
