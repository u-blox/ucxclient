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
 *   - USART1 (PB6/PB7): Arduino D0/D1 pins - used for NORA-W36
 *
 * USART1 Pins (Arduino header, no soldering needed):
 *   - PB6: USART1_TX → D1 (CN10)
 *   - PB7: USART1_RX → D0 (CN10)
 */
#define U_PORT_UART_INSTANCE    USART1
#define U_PORT_UART_IRQn        USART1_IRQn
#define U_PORT_UART_IRQHandler  USART1_IRQHandler
#define U_PORT_UART_CLK_ENABLE()  __HAL_RCC_USART1_CLK_ENABLE()
#define U_PORT_UART_CLK_DISABLE() __HAL_RCC_USART1_CLK_DISABLE()

/**
 * UART GPIO configuration for USART1 on PB6/PB7 (Arduino D1/D0).
 */
#ifndef U_PORT_UART_TX_PORT
#define U_PORT_UART_TX_PORT     GPIOB
#define U_PORT_UART_TX_PIN      GPIO_PIN_6
#endif

#ifndef U_PORT_UART_RX_PORT
#define U_PORT_UART_RX_PORT     GPIOB
#define U_PORT_UART_RX_PIN      GPIO_PIN_7
#endif

#ifndef U_PORT_UART_AF
#define U_PORT_UART_AF          GPIO_AF7_USART1  // AF7 for USART1 on PB6/PB7
#endif

// Hardware flow control not used
#ifndef U_PORT_UART_USE_HW_FLOW_CONTROL
#define U_PORT_UART_USE_HW_FLOW_CONTROL 0
#endif

#define U_PORT_UART_GPIO_AF     GPIO_AF7_USART1

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
