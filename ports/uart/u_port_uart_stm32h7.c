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
 * @brief STM32H7 UART port implementation using HAL.
 *
 * This implementation uses STM32H7 HAL library and supports:
 * - Configurable UART instance (USART1-6, UART4-8)
 * - Hardware flow control (RTS/CTS) - REQUIRED for NORA-W36
 * - Interrupt-driven reception with circular buffer
 * - Higher baud rates (up to 921600 for faster AT commands)
 *
 * Based on STM32F4 implementation with H7-specific adaptations.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "stm32h7xx_hal.h"
#include "u_port_uart.h"
#include "u_port_uart_stm32h7.h"

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
static void gpioInit(bool useFlowControl);
static void gpioDeinit(void);

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

static void gpioInit(bool useFlowControl)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    // Enable GPIO clocks for UART pins
    __HAL_RCC_GPIOA_CLK_ENABLE();  // PA0/PA1 for UART4
    __HAL_RCC_GPIOD_CLK_ENABLE();  // PD8/PD9 for USART3 (debug)

    // Configure TX pin
    GPIO_InitStruct.Pin = U_PORT_UART_TX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    GPIO_InitStruct.Alternate = U_PORT_UART_GPIO_AF;
    HAL_GPIO_Init(U_PORT_UART_TX_PORT, &GPIO_InitStruct);

    // Configure RX pin
    GPIO_InitStruct.Pin = U_PORT_UART_RX_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(U_PORT_UART_RX_PORT, &GPIO_InitStruct);

    if (useFlowControl) {
#if U_PORT_UART_USE_HW_FLOW_CONTROL
        // Configure CTS pin (input from module)
        GPIO_InitStruct.Pin = U_PORT_UART_CTS_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(U_PORT_UART_CTS_PORT, &GPIO_InitStruct);

        // Configure RTS pin (output to module)
        GPIO_InitStruct.Pin = U_PORT_UART_RTS_PIN;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        HAL_GPIO_Init(U_PORT_UART_RTS_PORT, &GPIO_InitStruct);
#endif
    }
}

static void gpioDeinit(void)
{
    HAL_GPIO_DeInit(U_PORT_UART_TX_PORT, U_PORT_UART_TX_PIN);
    HAL_GPIO_DeInit(U_PORT_UART_RX_PORT, U_PORT_UART_RX_PIN);
#if U_PORT_UART_USE_HW_FLOW_CONTROL
    HAL_GPIO_DeInit(U_PORT_UART_CTS_PORT, U_PORT_UART_CTS_PIN);
    HAL_GPIO_DeInit(U_PORT_UART_RTS_PORT, U_PORT_UART_RTS_PIN);
#endif
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

    // Initialize GPIO
    gpioInit(useFlowControl);

    // Enable UART clock
    U_PORT_UART_CLK_ENABLE();

    // Configure UART
    pHandle->huart.Instance = U_PORT_UART_INSTANCE;
    pHandle->huart.Init.BaudRate = (uint32_t)baudRate;
    pHandle->huart.Init.WordLength = UART_WORDLENGTH_8B;
    pHandle->huart.Init.StopBits = UART_STOPBITS_1;
    pHandle->huart.Init.Parity = UART_PARITY_NONE;
    pHandle->huart.Init.Mode = UART_MODE_TX_RX;

    if (useFlowControl) {
        pHandle->huart.Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    } else {
        pHandle->huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }

    pHandle->huart.Init.OverSampling = UART_OVERSAMPLING_16;
    pHandle->huart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    pHandle->huart.Init.ClockPrescaler = UART_PRESCALER_DIV1;

    // STM32H7 specific advanced features
    pHandle->huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&pHandle->huart) != HAL_OK) {
        gpioDeinit();
        free(pHandle);
        return NULL;
    }

    // Disable FIFO mode for simpler interrupt handling
    if (HAL_UARTEx_SetTxFifoThreshold(&pHandle->huart, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK) {
        // Non-fatal, continue
    }
    if (HAL_UARTEx_SetRxFifoThreshold(&pHandle->huart, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK) {
        // Non-fatal, continue
    }
    if (HAL_UARTEx_DisableFifoMode(&pHandle->huart) != HAL_OK) {
        // Non-fatal, continue
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
            gpioDeinit();
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
    if ((handle == NULL) || (pData == NULL) || (length == 0)) {
        return -1;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (!pHandle->isOpen) {
        return -1;
    }

    uint8_t *pBuffer = (uint8_t *)pData;
    uint32_t bytesRead = 0;
    uint32_t startTime = HAL_GetTick();

    while (bytesRead < length) {
        if (getRxBufferAvailable(pHandle) > 0) {
            pBuffer[bytesRead++] = pHandle->rxBuffer[pHandle->rxTail];
            pHandle->rxTail = (pHandle->rxTail + 1) % U_PORT_UART_RX_BUFFER_SIZE;
        } else if (timeoutMs >= 0) {
            if ((HAL_GetTick() - startTime) >= (uint32_t)timeoutMs) {
                break;  // Timeout
            }
            // Small delay to avoid busy loop
            HAL_Delay(1);
        } else {
            // Non-blocking mode, return what we have
            break;
        }
    }

    return (int32_t)bytesRead;
}

int32_t uPortUartAvailable(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return -1;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (!pHandle->isOpen) {
        return -1;
    }

    return (int32_t)getRxBufferAvailable(pHandle);
}

/* ----------------------------------------------------------------
 * INTERRUPT HANDLERS
 * -------------------------------------------------------------- */

// Forward declarations for debug UART console input
extern void ConsoleInput_ProcessByte(uint8_t byte);
extern uint8_t* ConsoleInput_GetRxByteBuffer(void);
extern UART_HandleTypeDef huart3;  // Debug UART from debug_uart.c

/**
 * @brief HAL UART RX complete callback.
 *
 * Called when a byte is received via interrupt.
 * Handles both NORA-W36 UART (UART4) and Debug UART (USART3).
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // NORA-W36 UART (UART4)
    if (gpUartHandle != NULL && huart == &gpUartHandle->huart) {
        // Store received byte in circular buffer
        uint32_t nextHead = (gpUartHandle->rxHead + 1) % U_PORT_UART_RX_BUFFER_SIZE;
        if (nextHead != gpUartHandle->rxTail) {
            gpUartHandle->rxBuffer[gpUartHandle->rxHead] = gpUartHandle->rxByte;
            gpUartHandle->rxHead = nextHead;
        }
        // Buffer overflow: oldest byte is lost

        // Continue receiving
        startRxInterrupt(gpUartHandle);
    }
    // Debug UART (USART3) - for keyboard input
    else if (huart->Instance == USART3) {
        uint8_t* rxBuf = ConsoleInput_GetRxByteBuffer();
        if (rxBuf) {
            ConsoleInput_ProcessByte(*rxBuf);
            // Re-enable reception
            HAL_UART_Receive_IT(&huart3, rxBuf, 1);
        }
    }
}

/**
 * @brief HAL UART error callback.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (gpUartHandle != NULL && huart == &gpUartHandle->huart) {
        // Clear error flags and restart reception
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);
        startRxInterrupt(gpUartHandle);
    }
}

/**
 * @brief UART interrupt handler.
 *
 * This is called from the vector table and dispatches to HAL.
 */
void U_PORT_UART_IRQHandler(void)
{
    if (gpUartHandle != NULL) {
        HAL_UART_IRQHandler(&gpUartHandle->huart);
    }
}
