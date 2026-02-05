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
 * @brief STM32H7 UART port configuration for Matter application
 *
 * This port is configured for STM32H743/H753 MCUs.
 *
 * UART Configuration:
 * -------------------
 * USART3 (u-blox NORA-W36 module communication):
 *   - PD8: TX
 *   - PD9: RX
 *   - PD11: CTS (when hardware flow control enabled)
 *   - PD12: RTS (when hardware flow control enabled)
 *   - Baud: Configurable (typically 115200, can go up to 921600)
 *   - Used for AT command communication with NORA-W36
 *
 * USART2 (Console/Debug - optional):
 *   - PA2: TX
 *   - PA3: RX
 *   - Baud: 115200
 *   - Used for ChipLog output and debugging
 *
 * Note: Pin assignments can be changed by modifying HAL_UART_MspInit()
 * in the board-specific initialization code.
 */

#ifndef U_PORT_UART_STM32H7_H
#define U_PORT_UART_STM32H7_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/**
 * UART instance for NORA-W36 module communication.
 * 
 * STM32H743/H753 Nucleo-144 board:
 *   - USART3 (PD8/PD9): ST-Link VCP (debug console) - DO NOT USE for NORA!
 *   - UART4 (PA0/PA1): Available for NORA-W36 module
 * 
 * UART4 Pins:
 *   - PA0: UART4_TX (CN11 pin 28)
 *   - PA1: UART4_RX (CN11 pin 30)
 */
#define U_PORT_UART_INSTANCE    UART4
#define U_PORT_UART_IRQn        UART4_IRQn
#define U_PORT_UART_IRQHandler  UART4_IRQHandler
#define U_PORT_UART_CLK_ENABLE()  __HAL_RCC_UART4_CLK_ENABLE()
#define U_PORT_UART_CLK_DISABLE() __HAL_RCC_UART4_CLK_DISABLE()

/**
 * UART GPIO configuration for UART4 on PA0/PA1.
 */
#ifndef U_PORT_UART_TX_PORT
#define U_PORT_UART_TX_PORT     GPIOA
#define U_PORT_UART_TX_PIN      GPIO_PIN_0
#endif

#ifndef U_PORT_UART_RX_PORT
#define U_PORT_UART_RX_PORT     GPIOA
#define U_PORT_UART_RX_PIN      GPIO_PIN_1
#endif

#ifndef U_PORT_UART_AF
#define U_PORT_UART_AF          GPIO_AF8_UART4  // AF8 for UART4 on PA0/PA1
#endif

// Hardware flow control is not used for UART4 <-> NORA-W36
// Define empty macros to allow conditional compilation
#ifndef U_PORT_UART_USE_HW_FLOW_CONTROL
#define U_PORT_UART_USE_HW_FLOW_CONTROL 0
#endif

#if U_PORT_UART_USE_HW_FLOW_CONTROL
#ifndef U_PORT_UART_CTS_PORT
#define U_PORT_UART_CTS_PORT    GPIOB
#define U_PORT_UART_CTS_PIN     GPIO_PIN_15  // UART4_CTS (if needed)
#endif

#ifndef U_PORT_UART_RTS_PORT
#define U_PORT_UART_RTS_PORT    GPIOA
#define U_PORT_UART_RTS_PIN     GPIO_PIN_15  // UART4_RTS (if needed)
#endif
#endif

#define U_PORT_UART_GPIO_AF     GPIO_AF8_UART4

/**
 * Receive buffer size.
 * Increased for Matter protocol which can have larger payloads.
 */
#ifndef U_PORT_UART_RX_BUFFER_SIZE
#define U_PORT_UART_RX_BUFFER_SIZE  (4096)
#endif

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_STM32H7_H
