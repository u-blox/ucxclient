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
 * @brief STM32F4 UART port implementation using HAL - interrupt-driven (default).
 *
 * This is the original, proven implementation: single-byte RX interrupts
 * into a circular buffer. Lower throughput than the DMA variant
 * (u_port_uart_stm32f4_dma.c) but has no DMA setup/error-recovery surface.
 * Select the DMA variant instead with -DSTM32_UART_USE_DMA=ON if you need
 * higher baud rates (2 Mbaud+).
 *
 * GPIO/clock configuration for the u-blox module UART is done in
 * HAL_UART_MspInit() (see main_stm32.c), which is called automatically by
 * HAL_UART_Init() below.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "stm32f4xx_hal.h"
#include "u_port_uart.h"
#include "u_port_uart_stm32f4.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#ifndef U_PORT_UART_RX_BUFFER_SIZE
#define U_PORT_UART_RX_BUFFER_SIZE  (2048)
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Structure representing a UART handle.
 */
typedef struct {
    UART_HandleTypeDef huart;
    uint8_t rxBuffer[U_PORT_UART_RX_BUFFER_SIZE];
    volatile uint32_t rxHead;
    volatile uint32_t rxTail;
    uint8_t rxByte;  // Single byte for interrupt RX
    bool isOpen;
} uPortUartHandle;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortUartHandle *gpUartHandle = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTION PROTOTYPES
 * -------------------------------------------------------------- */

static uint32_t getRxBufferAvailable(uPortUartHandle *pHandle);
static void startRxInterrupt(uPortUartHandle *pHandle);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static uint32_t getRxBufferAvailable(uPortUartHandle *pHandle)
{
    uint32_t head = pHandle->rxHead;
    uint32_t tail = pHandle->rxTail;

    if (head >= tail) {
        return head - tail;
    } else {
        return U_PORT_UART_RX_BUFFER_SIZE - tail + head;
    }
}

static void startRxInterrupt(uPortUartHandle *pHandle)
{
    HAL_UART_Receive_IT(&pHandle->huart, &pHandle->rxByte, 1);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uPortUartHandle_t uPortUartOpen(const char *pDevice, int32_t baudRate, bool useFlowControl)
{
    (void)pDevice;  // Device name not used on embedded systems

    if (gpUartHandle != NULL) {
        // Only one UART instance supported
        return NULL;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)malloc(sizeof(uPortUartHandle));
    if (pHandle == NULL) {
        return NULL;
    }

    memset(pHandle, 0, sizeof(uPortUartHandle));

    // Enable UART clock (GPIO clock/pins are configured in HAL_UART_MspInit())
    U_PORT_UART_CLK_ENABLE();

    // Configure UART
    pHandle->huart.Instance = U_PORT_UART_INSTANCE;
    pHandle->huart.Init.BaudRate = (uint32_t)baudRate;
    pHandle->huart.Init.WordLength = UART_WORDLENGTH_8B;
    pHandle->huart.Init.StopBits = UART_STOPBITS_1;
    pHandle->huart.Init.Parity = UART_PARITY_NONE;
    pHandle->huart.Init.Mode = UART_MODE_TX_RX;
    pHandle->huart.Init.HwFlowCtl = useFlowControl ? UART_HWCONTROL_RTS_CTS : UART_HWCONTROL_NONE;
    pHandle->huart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&pHandle->huart) != HAL_OK) {
        free(pHandle);
        return NULL;
    }

    // Enable UART interrupt
    // Priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) for FreeRTOS compatibility
    // Using priority 6 to ensure it's lower priority than FreeRTOS syscalls
    HAL_NVIC_SetPriority(U_PORT_UART_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(U_PORT_UART_IRQn);

    pHandle->isOpen = true;
    gpUartHandle = pHandle;

    // Start receiving
    startRxInterrupt(pHandle);

    return (uPortUartHandle_t)pHandle;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    if (handle != NULL) {
        uPortUartHandle *pHandle = (uPortUartHandle *)handle;

        if (pHandle->isOpen) {
            HAL_NVIC_DisableIRQ(U_PORT_UART_IRQn);
            HAL_UART_DeInit(&pHandle->huart);
            U_PORT_UART_CLK_DISABLE();
            pHandle->isOpen = false;
        }

        if (gpUartHandle == pHandle) {
            gpUartHandle = NULL;
        }

        free(pHandle);
    }
}

int32_t uPortUartWrite(uPortUartHandle_t handle,
                       const void *pData,
                       size_t length)
{
    if ((handle == NULL) || (pData == NULL) || (length == 0)) {
        return -1;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (!pHandle->isOpen) {
        return -1;
    }

    HAL_StatusTypeDef status = HAL_UART_Transmit(&pHandle->huart, (uint8_t *)pData, (uint16_t)length, HAL_MAX_DELAY);

    if (status != HAL_OK) {
        return -1;
    }

    return (int32_t)length;
}

int32_t uPortUartRead(uPortUartHandle_t handle,
                      void *pData,
                      size_t length,
                      int32_t timeoutMs)
{
    if ((handle == NULL) || (length == 0)) {
        return -1;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (!pHandle->isOpen) {
        return -1;
    }

    // Check available data
    uint32_t available = getRxBufferAvailable(pHandle);

    if (timeoutMs == 0) {
        // Non-blocking: return immediately
        if (available == 0) {
            return 0;
        }
    }

    // If pData is NULL, just return 0 (test case)
    if (pData == NULL) {
        return 0;
    }

    // Wait for data if blocking
    if (timeoutMs > 0 && available == 0) {
        uint32_t startTime = HAL_GetTick();
        while (available == 0) {
            available = getRxBufferAvailable(pHandle);
            if ((HAL_GetTick() - startTime) >= (uint32_t)timeoutMs) {
                return 0;  // Timeout
            }
        }
    }

    // Read data from circular buffer
    uint32_t bytesToRead = (length < available) ? length : available;
    uint8_t *pBytes = (uint8_t *)pData;
    uint32_t tail = pHandle->rxTail;

    for (uint32_t i = 0; i < bytesToRead; i++) {
        pBytes[i] = pHandle->rxBuffer[tail];
        tail = (tail + 1) % U_PORT_UART_RX_BUFFER_SIZE;
    }

    pHandle->rxTail = tail;

    return (int32_t)bytesToRead;
}

/* ----------------------------------------------------------------
 * UART INTERRUPT CALLBACK
 * -------------------------------------------------------------- */

/**
 * @brief UART RX complete callback
 *
 * This function is called by HAL when a byte is received.
 * It stores the byte in the circular buffer and restarts reception.
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (gpUartHandle != NULL && huart->Instance == gpUartHandle->huart.Instance) {
        // Store received byte in circular buffer
        uint32_t nextHead = (gpUartHandle->rxHead + 1) % U_PORT_UART_RX_BUFFER_SIZE;

        if (nextHead != gpUartHandle->rxTail) {
            // Buffer not full
            gpUartHandle->rxBuffer[gpUartHandle->rxHead] = gpUartHandle->rxByte;
            gpUartHandle->rxHead = nextHead;
        }
        // If buffer full, drop the byte (could add overflow handling here)

        // Restart reception
        startRxInterrupt(gpUartHandle);
    }
}

/* ----------------------------------------------------------------
 * UART INTERRUPT HANDLERS
 * -------------------------------------------------------------- */

/**
 * @brief UART interrupt handler
 *
 * This function must be called from your UART IRQ handler in your
 * main application code (e.g., in stm32f4xx_it.c):
 *
 * void USART3_IRQHandler(void)
 * {
 *     uPortUart_IRQHandler();
 * }
 */
void uPortUart_IRQHandler(void)
{
    if (gpUartHandle != NULL) {
        HAL_UART_IRQHandler(&gpUartHandle->huart);
    }
}

/**
 * @brief RX DMA stream interrupt handler stub.
 *
 * stm32f4xx_it.c always wires up a DMA stream IRQ handler for the
 * u-blox module UART regardless of build mode. This mode doesn't use DMA,
 * so the DMA interrupt is never actually enabled/fired - this stub just
 * satisfies the link.
 */
void uPortUartDma_IRQHandler(void)
{
}

/* ----------------------------------------------------------------
 * UART FLUSH FUNCTIONS
 * -------------------------------------------------------------- */

void uPortUartFlushRx(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (!pHandle->isOpen) {
        return;
    }

    // Discard anything currently sitting in the ring buffer. Disable the
    // UART interrupt briefly so rxHead can't move under us while we snapshot
    // it into rxTail (HAL_UART_RxCpltCallback above writes both).
    HAL_NVIC_DisableIRQ(U_PORT_UART_IRQn);
    pHandle->rxTail = pHandle->rxHead;
    HAL_NVIC_EnableIRQ(U_PORT_UART_IRQn);
}
