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
 * @brief STM32H7 SPI port configuration for NUCLEO-H743ZI/H753ZI.
 *
 * SPI Configuration (uCX2 SPI transport, host = SPI master):
 * ------------------------------------------------------------
 * SPI1 (u-blox module communication):
 *   - PA5: SCK  (Arduino D13 / CN7 pin 10, AF5)
 *   - PA6: MISO (Arduino D12 / CN7 pin 12, AF5)
 *   - PA7: MOSI (Arduino D11 / CN7 pin 14, AF5)
 *   - PA4: CS   (Arduino A2  / CN7 pin 32, manual GPIO output - software
 *                chip-select, NOT the SPI1 hardware NSS function, since
 *                the protocol needs precise per-phase CS control)
 *   - PB0: DRDY (Arduino A3  / CN10 pin 31, GPIO input, active-high,
 *                polled - no EXTI in this first implementation)
 *
 * @warning PLACEHOLDER PINS - NOT YET VERIFIED AGAINST REAL HARDWARE.
 * Neither the STM32H7 <-> NORA-W36/W56 SPI wiring nor the module's own
 * SPI-slave pin mapping has been confirmed on a physical test rig at the
 * time this file was written. Verify against the actual EVK/module
 * datasheet before wiring real hardware - do not assume these pins are
 * correct. See project instructions "HARDWARE FACTS - VERIFY, NEVER GUESS".
 *
 * These pins are deliberately disjoint from the existing USART1 (PB6/PB7)
 * and USART3 console (PD8/PD9) pins used by the UART transport, so a
 * board could in principle support both transports being wired
 * simultaneously (only one is ever active in a given firmware build,
 * selected at CMake configure time via -DSTM32_TRANSPORT=spi).
 */

#ifndef U_PORT_SPI_STM32H7_H
#define U_PORT_SPI_STM32H7_H

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/** SPI instance used for u-blox module communication. */
#define U_PORT_SPI_INSTANCE      SPI1
#define U_PORT_SPI_CLK_ENABLE    __HAL_RCC_SPI1_CLK_ENABLE
#define U_PORT_SPI_CLK_DISABLE   __HAL_RCC_SPI1_CLK_DISABLE
#define U_PORT_SPI_AF            GPIO_AF5_SPI1

/** SCK/MISO/MOSI on GPIOA. */
#define U_PORT_SPI_GPIO_PORT       GPIOA
#define U_PORT_SPI_GPIO_CLK_ENABLE __HAL_RCC_GPIOA_CLK_ENABLE
#define U_PORT_SPI_SCK_PIN         GPIO_PIN_5
#define U_PORT_SPI_MISO_PIN        GPIO_PIN_6
#define U_PORT_SPI_MOSI_PIN        GPIO_PIN_7

/** Chip-select: manual GPIO output (software CS), active low. */
#define U_PORT_SPI_CS_GPIO_PORT       GPIOA
#define U_PORT_SPI_CS_GPIO_CLK_ENABLE __HAL_RCC_GPIOA_CLK_ENABLE
#define U_PORT_SPI_CS_PIN             GPIO_PIN_4

/** DRDY: GPIO input, active high, polled. */
#define U_PORT_SPI_DRDY_GPIO_PORT       GPIOB
#define U_PORT_SPI_DRDY_GPIO_CLK_ENABLE __HAL_RCC_GPIOB_CLK_ENABLE
#define U_PORT_SPI_DRDY_PIN             GPIO_PIN_0

/** Default SPI clock, Hz. Spec allows up to 20 MHz; start conservative. */
#ifndef U_PORT_SPI_DEFAULT_CLOCK_HZ
#define U_PORT_SPI_DEFAULT_CLOCK_HZ  (1000000U)
#endif

#ifdef __cplusplus
}
#endif

#endif // U_PORT_SPI_STM32H7_H
