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
 * This header defines the UART instances available on STM32F407VG.
 * Users can configure which UART instance to use via CMake defines.
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
 * UART instance selection
 * Define one of these before including this header:
 * - U_PORT_UART_USE_USART1
 * - U_PORT_UART_USE_USART2
 * - U_PORT_UART_USE_USART3
 * - U_PORT_UART_USE_UART4
 * - U_PORT_UART_USE_UART5
 * - U_PORT_UART_USE_USART6
 *
 * Default is USART2 if none specified.
 */

#if defined(U_PORT_UART_USE_USART1)
#define U_PORT_UART_INSTANCE    USART1
#define U_PORT_UART_IRQn        USART1_IRQn
#define U_PORT_UART_IRQHandler  USART1_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART1_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART1_CLK_DISABLE

#elif defined(U_PORT_UART_USE_USART3)
#define U_PORT_UART_INSTANCE    USART3
#define U_PORT_UART_IRQn        USART3_IRQn
#define U_PORT_UART_IRQHandler  USART3_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART3_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART3_CLK_DISABLE

#elif defined(U_PORT_UART_USE_UART4)
#define U_PORT_UART_INSTANCE    UART4
#define U_PORT_UART_IRQn        UART4_IRQn
#define U_PORT_UART_IRQHandler  UART4_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_UART4_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_UART4_CLK_DISABLE

#elif defined(U_PORT_UART_USE_UART5)
#define U_PORT_UART_INSTANCE    UART5
#define U_PORT_UART_IRQn        UART5_IRQn
#define U_PORT_UART_IRQHandler  UART5_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_UART5_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_UART5_CLK_DISABLE

#elif defined(U_PORT_UART_USE_USART6)
#define U_PORT_UART_INSTANCE    USART6
#define U_PORT_UART_IRQn        USART6_IRQn
#define U_PORT_UART_IRQHandler  USART6_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART6_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART6_CLK_DISABLE

#else
/* Default to USART2 */
#define U_PORT_UART_USE_USART2
#define U_PORT_UART_INSTANCE    USART2
#define U_PORT_UART_IRQn        USART2_IRQn
#define U_PORT_UART_IRQHandler  USART2_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART2_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART2_CLK_DISABLE
#endif

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_STM32F4_H
