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
 * @brief STM32F4 UART port configuration for STM32F407G-DISC1
 *
 * This port is configured for the STM32F407G-DISC1 development board.
 *
 * UART Configuration:
 * -------------------
 * USART2 (Console/Debug):
 *   - PA2: TX
 *   - PA3: RX
 *   - Baud: 115200
 *   - Used for printf() output and debugging
 *
 * USART3 (u-blox module communication):
 *   - PB10: TX
 *   - PB11: RX
 *   - PB13: CTS (when hardware flow control enabled)
 *   - PB14: RTS (when hardware flow control enabled)
 *   - Baud: Configurable (typically 115200)
 *   - Used for AT command communication with u-blox modules
 */

#ifndef U_PORT_UART_STM32F4_H
#define U_PORT_UART_STM32F4_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/**
 * UART instance for u-blox module communication
 * Hardcoded to USART3 for STM32F407G-DISC1 board compatibility.
 * See file header for complete pin assignment documentation.
 */
#define U_PORT_UART_INSTANCE    USART3
#define U_PORT_UART_IRQn        USART3_IRQn
#define U_PORT_UART_IRQHandler  USART3_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART3_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART3_CLK_DISABLE

/**
 * RX DMA configuration (USART3_RX = DMA1 Stream 1, Channel 4 on STM32F4).
 * RX uses circular DMA into a ring buffer so that no per-byte interrupts
 * are needed - required for reliable operation at high baud rates (2 Mbaud+).
 */
#define U_PORT_UART_DMA_CLK_ENABLE    __HAL_RCC_DMA1_CLK_ENABLE
#define U_PORT_UART_RX_DMA_STREAM     DMA1_Stream1
#define U_PORT_UART_RX_DMA_CHANNEL    DMA_CHANNEL_4
#define U_PORT_UART_RX_DMA_IRQn       DMA1_Stream1_IRQn
#define U_PORT_UART_RX_DMA_IRQHandler DMA1_Stream1_IRQHandler

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_STM32F4_H
