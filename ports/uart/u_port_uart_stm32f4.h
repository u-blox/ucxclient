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
 * @brief STM32F4 UART port configuration
 *
 * Board-specific UART configurations:
 *
 * STM32F407G-DISC1 / STM32F429I-DISC1:
 * -------------------------------------
 * USART2 (Console/Debug): PA2=TX, PA3=RX @ 115200
 * USART3 (NORA-W36):      PB10=TX, PB11=RX, PB13=CTS, PB14=RTS
 *
 * ODIN-W2 EVK (STM32F439):
 * ------------------------
 * USART3 (Console/Debug): PD8=TX, PD9=RX @ 115200
 * USART1 (NORA-W36/SPA):  PA9=TX, PA10=RX, PA11=CTS, PA12=RTS
 *   - Full HW flow control supported
 *   - DMA available but not used
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
 * - NUCLEO-F439ZI: USART1 on PB6/PB7 (same as H753 Nucleo-144 wiring)
 * - ODIN-W26: USART1 on PA9/PA10 (SPA UART, 168MHz clock)
 * - STM32F439 (ODIN-W2): USART1 on PA9/PA10 (SPA UART)
 * - STM32F407/F429: USART3 on PB10/PB11
 */
#if defined(NUCLEO_F439ZI)
// NUCLEO-F439ZI: USART1 on PB6/PB7 (same Morpho pins as NUCLEO-H753ZI)
#define U_PORT_UART_INSTANCE    USART1
#define U_PORT_UART_IRQn        USART1_IRQn
#define U_PORT_UART_IRQHandler  USART1_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART1_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART1_CLK_DISABLE
// GPIO: PB6=TX, PB7=RX (AF7) - same as H753 Nucleo
#define U_PORT_UART_TX_PORT     GPIOB
#define U_PORT_UART_TX_PIN      GPIO_PIN_6
#define U_PORT_UART_RX_PORT     GPIOB
#define U_PORT_UART_RX_PIN      GPIO_PIN_7
#define U_PORT_UART_GPIO_AF     GPIO_AF7_USART1
#elif defined(ODIN_W26) || defined(STM32F439xx)
// ODIN-W2 EVK: USART1 for NORA-W36 (SPA UART)
#define U_PORT_UART_INSTANCE    USART1
#define U_PORT_UART_IRQn        USART1_IRQn
#define U_PORT_UART_IRQHandler  USART1_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART1_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART1_CLK_DISABLE
// GPIO: PA9=TX, PA10=RX, PA11=CTS, PA12=RTS (AF7)
#define U_PORT_UART_TX_PORT     GPIOA
#define U_PORT_UART_TX_PIN      GPIO_PIN_9
#define U_PORT_UART_RX_PORT     GPIOA
#define U_PORT_UART_RX_PIN      GPIO_PIN_10
#define U_PORT_UART_CTS_PORT    GPIOA
#define U_PORT_UART_CTS_PIN     GPIO_PIN_11
#define U_PORT_UART_RTS_PORT    GPIOA
#define U_PORT_UART_RTS_PIN     GPIO_PIN_12
#define U_PORT_UART_GPIO_AF     GPIO_AF7_USART1
#else
// STM32F407/F429: USART3 for NORA-W36
#define U_PORT_UART_INSTANCE    USART3
#define U_PORT_UART_IRQn        USART3_IRQn
#define U_PORT_UART_IRQHandler  USART3_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART3_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART3_CLK_DISABLE
// GPIO: PB10=TX, PB11=RX (AF7)
#define U_PORT_UART_TX_PORT     GPIOB
#define U_PORT_UART_TX_PIN      GPIO_PIN_10
#define U_PORT_UART_RX_PORT     GPIOB
#define U_PORT_UART_RX_PIN      GPIO_PIN_11
#define U_PORT_UART_CTS_PORT    GPIOB
#define U_PORT_UART_CTS_PIN     GPIO_PIN_13
#define U_PORT_UART_RTS_PORT    GPIOB
#define U_PORT_UART_RTS_PIN     GPIO_PIN_14
#define U_PORT_UART_GPIO_AF     GPIO_AF7_USART3
#endif

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_STM32F4_H
