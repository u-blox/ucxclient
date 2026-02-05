/*
 * Copyright 2025 u-blox
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/** @file
 * @brief STM32F7 UART port configuration for NUCLEO-F767ZI
 *
 * This port is configured for the NUCLEO-F767ZI development board.
 *
 * UART Configuration:
 * -------------------
 * USART3 (Console/Debug via ST-Link VCP):
 *   - PD8: TX
 *   - PD9: RX
 *   - Baud: 115200
 *   - Used for printf() output and debugging
 *
 * USART6 (u-blox NORA-W36 module communication):
 *   - PC6: TX
 *   - PC7: RX
 *   - PG8: CTS (when hardware flow control enabled)
 *   - PG7: RTS (when hardware flow control enabled)
 *   - Baud: Configurable (typically 115200)
 *   - Used for AT command communication with NORA-W36
 */

#ifndef U_PORT_UART_STM32F7_H
#define U_PORT_UART_STM32F7_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/**
 * RX buffer size for ucxclient AT command parsing.
 */
#ifndef U_PORT_UART_RX_BUFFER_SIZE
#define U_PORT_UART_RX_BUFFER_SIZE  (2048)
#endif

/**
 * UART instance for u-blox NORA-W36 module communication
 * Using USART6 on NUCLEO-F767ZI (PC6/PC7 pins available on CN9)
 */
#define U_PORT_UART_INSTANCE    USART6
#define U_PORT_UART_IRQn        USART6_IRQn
#define U_PORT_UART_IRQHandler  USART6_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART6_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART6_CLK_DISABLE

#ifdef __cplusplus
}
#endif

#endif /* U_PORT_UART_STM32F7_H */
