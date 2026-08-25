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
 * - Circular DMA reception directly into a ring buffer (no per-byte
 *   interrupts, required for high baud rates such as 2 Mbaud+)
 * - Automatic recovery from UART errors (overrun, framing, noise)
 *
 * Target boards:
 * - STM32F407G-DISC1
 * - STM32F429I-DISC1
 * - STM32F439 (ODIN-W2 / NUCLEO-F439ZI)
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
#define U_PORT_UART_RX_BUFFER_SIZE  (8192)
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Structure representing a UART handle.
 */
typedef struct {
    UART_HandleTypeDef huart;
    DMA_HandleTypeDef hdmaRx;
    uint8_t rxBuffer[U_PORT_UART_RX_BUFFER_SIZE];
    uint32_t rxTotalRead;            // Total bytes consumed by reader (mod 2^32)
    volatile uint32_t rxWraps;       // DMA buffer wrap count (incremented in ISR)
    volatile bool rxResync;          // Set by error callback, handled by reader
    volatile uint32_t errorCount;    // UART errors (overrun/framing/noise)
    volatile uint32_t overflowCount; // Ring buffer overflows (reader too slow)
    bool isOpen;
} uPortUartHandle;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortUartHandle *gpUartHandle = NULL;

/* ----------------------------------------------------------------
 * STATIC FUNCTION PROTOTYPES
 * -------------------------------------------------------------- */

static uint32_t getDmaWriteCount(uPortUartHandle *pHandle);
static uint32_t getRxBufferAvailable(uPortUartHandle *pHandle);
static void startRxDma(uPortUartHandle *pHandle);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

/** Total bytes written to the ring buffer by DMA (mod 2^32).
 *  Reads wrap counter and DMA NDTR consistently (retries if a
 *  buffer wrap happens between the two reads).
 */
static uint32_t getDmaWriteCount(uPortUartHandle *pHandle)
{
    uint32_t wraps;
    uint32_t ndtr;

    do {
        wraps = pHandle->rxWraps;
        ndtr = __HAL_DMA_GET_COUNTER(pHandle->huart.hdmarx);
    } while (wraps != pHandle->rxWraps);

    return (wraps * U_PORT_UART_RX_BUFFER_SIZE) +
           (U_PORT_UART_RX_BUFFER_SIZE - ndtr);
}

static uint32_t getRxBufferAvailable(uPortUartHandle *pHandle)
{
    if (pHandle->rxResync) {
        // UART error occurred and DMA reception was restarted:
        // discard everything received before the error.
        pHandle->rxResync = false;
        pHandle->rxTotalRead = getDmaWriteCount(pHandle);
        return 0;
    }

    uint32_t available = getDmaWriteCount(pHandle) - pHandle->rxTotalRead;
    if (available > U_PORT_UART_RX_BUFFER_SIZE) {
        // Hardware reloads NDTR to full the instant a circular wrap
        // completes, but rxWraps is only incremented later inside the DMA
        // transfer-complete ISR. Sampling in that gap makes the computed
        // write count undershoot by one full buffer, which looks like a
        // huge unsigned "overflow" here but is not a real one. A genuine
        // reader-too-slow overflow persists; this race self-heals within
        // microseconds once the pending ISR runs, so retry first.
        for (int retry = 0; retry < 100 && available > U_PORT_UART_RX_BUFFER_SIZE; retry++) {
            available = getDmaWriteCount(pHandle) - pHandle->rxTotalRead;
        }
    }
    if (available > U_PORT_UART_RX_BUFFER_SIZE) {
        // Still bad after retries - DMA has genuinely lapped the reader and
        // buffer content is no longer coherent. Drop it all rather than
        // deliver corrupt data.
        pHandle->overflowCount++;
        pHandle->rxTotalRead = getDmaWriteCount(pHandle);
        return 0;
    }
    return available;
}

static void startRxDma(uPortUartHandle *pHandle)
{
    pHandle->rxWraps = 0;
    HAL_UART_Receive_DMA(&pHandle->huart, pHandle->rxBuffer,
                         U_PORT_UART_RX_BUFFER_SIZE);
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

    // Configure circular DMA for RX (writes directly into the ring buffer)
    U_PORT_UART_DMA_CLK_ENABLE();
    pHandle->hdmaRx.Instance = U_PORT_UART_RX_DMA_STREAM;
    pHandle->hdmaRx.Init.Channel = U_PORT_UART_RX_DMA_CHANNEL;
    pHandle->hdmaRx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    pHandle->hdmaRx.Init.PeriphInc = DMA_PINC_DISABLE;
    pHandle->hdmaRx.Init.MemInc = DMA_MINC_ENABLE;
    pHandle->hdmaRx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    pHandle->hdmaRx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    pHandle->hdmaRx.Init.Mode = DMA_CIRCULAR;
    pHandle->hdmaRx.Init.Priority = DMA_PRIORITY_HIGH;
    pHandle->hdmaRx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;

    if (HAL_DMA_Init(&pHandle->hdmaRx) != HAL_OK) {
        HAL_UART_DeInit(&pHandle->huart);
        free(pHandle);
        return NULL;
    }
    __HAL_LINKDMA(&pHandle->huart, hdmarx, pHandle->hdmaRx);

    // Enable UART + DMA interrupts
    // Priority must be >= configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY (5) for FreeRTOS compatibility
    HAL_NVIC_SetPriority(U_PORT_UART_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(U_PORT_UART_IRQn);
    HAL_NVIC_SetPriority(U_PORT_UART_RX_DMA_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(U_PORT_UART_RX_DMA_IRQn);

    pHandle->isOpen = true;
    gpUartHandle = pHandle;

    // Start receiving
    startRxDma(pHandle);

    return (uPortUartHandle_t)pHandle;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    if (handle != NULL) {
        uPortUartHandle *pHandle = (uPortUartHandle *)handle;

        if (pHandle->isOpen) {
            HAL_UART_DMAStop(&pHandle->huart);
            HAL_NVIC_DisableIRQ(U_PORT_UART_RX_DMA_IRQn);
            HAL_NVIC_DisableIRQ(U_PORT_UART_IRQn);
            HAL_DMA_DeInit(&pHandle->hdmaRx);
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

    // Read data from circular buffer (may need two copies at wrap point)
    uint32_t bytesToRead = (length < available) ? length : available;
    uint32_t tailIdx = pHandle->rxTotalRead % U_PORT_UART_RX_BUFFER_SIZE;
    uint32_t firstChunk = U_PORT_UART_RX_BUFFER_SIZE - tailIdx;
    if (firstChunk > bytesToRead) {
        firstChunk = bytesToRead;
    }
    memcpy(pData, &pHandle->rxBuffer[tailIdx], firstChunk);
    if (bytesToRead > firstChunk) {
        memcpy((uint8_t *)pData + firstChunk, &pHandle->rxBuffer[0],
               bytesToRead - firstChunk);
    }

    pHandle->rxTotalRead += bytesToRead;

    return (int32_t)bytesToRead;
}

/* ----------------------------------------------------------------
 * UART INTERRUPT CALLBACKS
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
/**
 * @brief DMA transfer complete callback (circular mode = buffer wrap)
 *
 * UART assignments:
 * - ODIN-W26/F439 (ODIN-W2): USART1=NORA-W36, USART3=Debug
 * - F407/F429: USART3=NORA-W36, USART2=Debug
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    // NORA-W36 UART (USART1 on ODIN-W26/F439, USART3 on F407/F429)
    if (gpUartHandle != NULL && huart->Instance == gpUartHandle->huart.Instance) {
        gpUartHandle->rxWraps++;
    }
}

/**
 * @brief UART error callback (overrun, framing, noise, DMA error)
 *
 * Without this callback a single overrun error would abort DMA reception
 * permanently and the UART would go silently deaf. Restart reception and
 * let the reader resynchronize.
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (gpUartHandle != NULL && huart->Instance == gpUartHandle->huart.Instance) {
        gpUartHandle->errorCount++;
        gpUartHandle->rxResync = true;
        // HAL has already aborted the transfer at this point; clear any
        // remaining error flags and restart circular DMA reception.
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        HAL_UART_DMAStop(huart);
        startRxDma(gpUartHandle);
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
 * @brief RX DMA stream interrupt handler
 *
 * This function must be called from the RX DMA stream IRQ handler in your
 * main application code (e.g., in stm32f4xx_it.c):
 *
 * void DMA1_Stream1_IRQHandler(void)
 * {
 *     uPortUartDma_IRQHandler();
 * }
 */
void uPortUartDma_IRQHandler(void)
{
    if (gpUartHandle != NULL) {
        HAL_DMA_IRQHandler(&gpUartHandle->hdmaRx);
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

    // Discard anything currently sitting in the DMA ring buffer by
    // fast-forwarding the read position to the current DMA write count.
    pHandle->rxTotalRead = getDmaWriteCount(pHandle);
}
