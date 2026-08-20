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
 * @brief STM32H7 UART port configuration for NUCLEO-H743ZI/H753ZI
 *
 * UART Configuration:
 * -------------------
 * USART1 (u-blox module communication):
 *   - PB6: TX (Arduino D1 / same Morpho pins as the F439ZI Nucleo wiring)
 *   - PB7: RX (Arduino D0)
 *   - Baud: Configurable
 *
 * USART3 (Console/Debug):
 *   - PD8: TX, PD9: RX (ST-LINK VCP)
 *   - Baud: 115200
 *   - Used for printf() output - see main_stm32.c
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
 * UART instance for u-blox module communication (USART1 PB6/PB7).
 */
#define U_PORT_UART_INSTANCE    USART1
#define U_PORT_UART_IRQn        USART1_IRQn
#define U_PORT_UART_IRQHandler  USART1_IRQHandler
#define U_PORT_UART_CLK_ENABLE  __HAL_RCC_USART1_CLK_ENABLE
#define U_PORT_UART_CLK_DISABLE __HAL_RCC_USART1_CLK_DISABLE

/**
 * RX DMA configuration. On STM32H7 any DMA1/DMA2 stream can serve any
 * peripheral via DMAMUX; we use DMA1 Stream 0 with the USART1_RX request.
 * RX uses circular DMA into a ring buffer so that no per-byte interrupts
 * are needed - required for reliable operation at high baud rates (2 Mbaud+).
 *
 * NOTE: the ring buffer must be DMA-accessible. The linker script places
 * .bss/heap in RAM_D1 (AXI SRAM @ 0x24000000) which DMA1 can reach.
 * DTCM (0x20000000) is NOT DMA-accessible on H7.
 */
#define U_PORT_UART_DMA_CLK_ENABLE    __HAL_RCC_DMA1_CLK_ENABLE
#define U_PORT_UART_RX_DMA_STREAM     DMA1_Stream0
#define U_PORT_UART_RX_DMA_REQUEST    DMA_REQUEST_USART1_RX
#define U_PORT_UART_RX_DMA_IRQn       DMA1_Stream0_IRQn
#define U_PORT_UART_RX_DMA_IRQHandler DMA1_Stream0_IRQHandler

#ifdef __cplusplus
}
#endif

#endif // U_PORT_UART_STM32H7_H
