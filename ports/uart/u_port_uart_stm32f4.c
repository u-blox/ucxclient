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
 * @brief STM32F4 UART port implementation using HAL.
 *
 * This implementation uses STM32F4 HAL library and supports:
 * - Configurable UART instance (USART1-6, UART4-5)
 * - Hardware flow control (RTS/CTS)
 * - Interrupt-driven reception with circular buffer
 * - DMA support (optional, can be enabled via defines)
 *
 * Target boards:
 * - STM32F407G-DISC1
 * - STM32F429I-DISC1
 * - STM32F439 (ODIN-W2)
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

#include <stdio.h>  // For debug printf

uPortUartHandle_t uPortUartOpen(const char *pDevice, int32_t baudRate, bool useFlowControl)
{
    (void)pDevice;  // Device name not used on embedded systems

    printf("[UART] Open: %ld baud, flow=%d\r\n", (long)baudRate, useFlowControl);

    if (gpUartHandle != NULL) {
        printf("[UART] ERROR: Already open!\r\n");
        return NULL;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)malloc(sizeof(uPortUartHandle));
    if (pHandle == NULL) {
        printf("[UART] ERROR: malloc failed!\r\n");
        return NULL;
    }

    memset(pHandle, 0, sizeof(uPortUartHandle));

    // Enable GPIO and UART clocks
#if defined(NUCLEO_F439ZI)
    // NUCLEO-F439ZI: USART1 on PB6/PB7 (same as H753 Nucleo)
    __HAL_RCC_GPIOB_CLK_ENABLE();
#elif defined(ODIN_W26) || defined(STM32F439xx)
    // ODIN-W26/ODIN-W2: USART1 on PA9/PA10 (SPA UART)
    __HAL_RCC_GPIOA_CLK_ENABLE();
#else
    // F407/F429: USART3 on PB10/PB11
    __HAL_RCC_GPIOB_CLK_ENABLE();
#endif
    U_PORT_UART_CLK_ENABLE();

    // Configure GPIO pins for UART
    // Use NOPULL and LOW speed to match original ODIN-W26 firmware settings
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = U_PORT_UART_TX_PIN | U_PORT_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;           // Match original firmware
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;  // Match original firmware
    GPIO_InitStruct.Alternate = U_PORT_UART_GPIO_AF;
    HAL_GPIO_Init(U_PORT_UART_TX_PORT, &GPIO_InitStruct);

    // Configure flow control pins if enabled
#if defined(U_PORT_UART_CTS_PORT) && defined(U_PORT_UART_RTS_PORT)
    if (useFlowControl) {
        GPIO_InitStruct.Pin = U_PORT_UART_CTS_PIN;
        HAL_GPIO_Init(U_PORT_UART_CTS_PORT, &GPIO_InitStruct);
        GPIO_InitStruct.Pin = U_PORT_UART_RTS_PIN;
        HAL_GPIO_Init(U_PORT_UART_RTS_PORT, &GPIO_InitStruct);
    }
#else
    (void)useFlowControl;
#endif

    // Configure UART
    pHandle->huart.Instance = U_PORT_UART_INSTANCE;
    pHandle->huart.Init.BaudRate = (uint32_t)baudRate;
    pHandle->huart.Init.WordLength = UART_WORDLENGTH_8B;
    pHandle->huart.Init.StopBits = UART_STOPBITS_1;
    pHandle->huart.Init.Parity = UART_PARITY_NONE;
    pHandle->huart.Init.Mode = UART_MODE_TX_RX;

#if defined(U_PORT_UART_CTS_PORT) && defined(U_PORT_UART_RTS_PORT)
    if (useFlowControl) {
        pHandle->huart.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    } else {
        pHandle->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
#else
    pHandle->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
#endif

    pHandle->huart.Init.OverSampling = UART_OVERSAMPLING_16;

    HAL_StatusTypeDef halStatus = HAL_UART_Init(&pHandle->huart);
    if (halStatus != HAL_OK) {
        printf("[UART] ERROR: HAL_UART_Init failed: %d\r\n", halStatus);
        free(pHandle);
        return NULL;
    }
    printf("[UART] Init OK\r\n");

    // Enable UART interrupt
    // Priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) for FreeRTOS compatibility
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

// Forward declarations for debug UART console input
extern void ConsoleInput_ProcessByte(uint8_t byte);
extern uint8_t* ConsoleInput_GetRxByteBuffer(void);

// Debug UART handle from debug_uart.c
// - ODIN-W26/F439 (ODIN-W2): USART3 on PD8/PD9
// - F407/F429: USART2 on PA2/PA3
#if defined(ODIN_W26) || defined(STM32F439xx)
extern UART_HandleTypeDef huart3;  // ODIN-W26/ODIN-W2: Debug on USART3
#define DEBUG_UART_INSTANCE USART3
#define DEBUG_UART_HANDLE huart3
#else
extern UART_HandleTypeDef huart2;  // F407/F429: Debug on USART2
#define DEBUG_UART_INSTANCE USART2
#define DEBUG_UART_HANDLE huart2
#endif

/**
 * @brief UART RX complete callback
 *
 * This function is called by HAL when a byte is received.
 * Handles both NORA-W36 UART and Debug UART.
 * 
 * UART assignments:
 * - ODIN-W26/F439 (ODIN-W2): USART1=NORA-W36, USART3=Debug
 * - F407/F429: USART3=NORA-W36, USART2=Debug
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // NORA-W36 UART (USART1 on ODIN-W26/F439, USART3 on F407/F429)
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
    // Debug UART - for keyboard input
    else if (huart->Instance == DEBUG_UART_INSTANCE) {
        uint8_t* rxBuf = ConsoleInput_GetRxByteBuffer();
        if (rxBuf) {
            ConsoleInput_ProcessByte(*rxBuf);
            // Re-enable reception
            HAL_UART_Receive_IT(&DEBUG_UART_HANDLE, rxBuf, 1);
        }
    }
}

/* ----------------------------------------------------------------
 * UART INTERRUPT HANDLER
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

    // Disable UART RX interrupt while resetting buffer
    __HAL_UART_DISABLE_IT(&pHandle->huart, UART_IT_RXNE);
    pHandle->rxHead = 0;
    pHandle->rxTail = 0;
    __HAL_UART_ENABLE_IT(&pHandle->huart, UART_IT_RXNE);
}
