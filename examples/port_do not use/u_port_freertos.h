/*
 * Copyright 2024 u-blox
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
 * @brief FreeRTOS port example
 *
 * This port layer provides integration with FreeRTOS for STM32 and other
 * embedded platforms. It uses FreeRTOS semaphores for mutex support and
 * FreeRTOS tick count for timing.
 *
 * Supported Platforms:
 * - STM32 (all families: F0/F1/F2/F3/F4/F7/G0/G4/H7/L0/L1/L4/L5/U5/WB/WL)
 * - ESP32/ESP32-S/ESP32-C series
 * - NXP Kinetis, LPC, and i.MX RT series
 * - Microchip/Atmel SAM D/E/C/S/L series
 * - Texas Instruments MSP432 and Tiva C series
 * - Silicon Labs EFM32 and EFR32 (Gecko)
 * - Renesas RA series
 * - ARM Cortex-A with FreeRTOS+POSIX (i.MX, Zynq, etc.)
 * - RISC-V cores (SiFive, GigaDevice, Nuclei, etc.)
 *
 * To use this port:
 * 1. Define your platform (e.g., U_PORT_STM32_HAL, U_PORT_ESP32)
 * 2. Define U_PORT_FREERTOS in your build
 * 3. Add u_port_freertos.c to your build
 * 4. Configure UART pins and clocks in your hardware initialization
 * 5. Include "u_port.h" in your application code
 *
 * Platform Selection (define one):
 * - U_PORT_STM32_HAL    : STM32 using HAL library
 * - U_PORT_ESP32        : ESP32/ESP-IDF
 * - U_PORT_NXP_SDK      : NXP MCUXpresso SDK
 * - U_PORT_ATMEL_ASF    : Microchip/Atmel ASF
 * - U_PORT_TI_DRIVERLIB : Texas Instruments DriverLib
 * - U_PORT_SILABS_GECKO : Silicon Labs Gecko SDK
 * - U_PORT_RENESAS_FSP  : Renesas FSP
 * - U_PORT_ARM_CORTEXA  : ARM Cortex-A with FreeRTOS+POSIX
 * - U_PORT_RISCV        : RISC-V with memory-mapped UART
 */

#ifndef U_PORT_FREERTOS_H
#define U_PORT_FREERTOS_H

#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "semphr.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_MUTEX_HANDLE                     SemaphoreHandle_t
#define U_CX_MUTEX_CREATE(mutex)              mutex = xSemaphoreCreateMutex()
#define U_CX_MUTEX_DELETE(mutex)              vSemaphoreDelete(mutex)
#define U_CX_MUTEX_LOCK(mutex)                xSemaphoreTake(mutex, portMAX_DELAY)
#define U_CX_MUTEX_TRY_LOCK(mutex, timeoutMs) uPortMutexTryLock(mutex, timeoutMs)
#define U_CX_MUTEX_UNLOCK(mutex)              xSemaphoreGive(mutex)

#define U_CX_PORT_GET_TIME_MS()               uPortGetTickTimeMs()

/* Example configuration - adjust these for your hardware */
#ifndef U_EXAMPLE_UART
# define U_EXAMPLE_UART "UART1"  // Adjust for your UART instance
#endif

#ifndef U_EXAMPLE_SSID
# define U_EXAMPLE_SSID "ubx"
#endif

#ifndef U_EXAMPLE_WPA_PSK
# define U_EXAMPLE_WPA_PSK ""
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
  * @brief Get current tick time in milliseconds
  *
  * @return Time in milliseconds since system start
  */
int32_t uPortGetTickTimeMs(void);

/**
  * @brief FreeRTOS implementation of U_CX_MUTEX_TRY_LOCK()
  *
  * @param[in] mutex:      FreeRTOS mutex handle
  * @param[in] timeoutMs:  Timeout in milliseconds
  * @return 0 on success, negative value on timeout
  */
int32_t uPortMutexTryLock(SemaphoreHandle_t mutex, int32_t timeoutMs);

#endif
