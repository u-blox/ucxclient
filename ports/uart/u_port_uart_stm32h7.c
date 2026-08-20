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
 * Same design as the STM32F4 port (u_port_uart_stm32f4.c):
 * - Circular DMA reception directly into a ring buffer (no per-byte
 *   interrupts, required for high baud rates such as 2 Mbaud+)
 * - Automatic recovery from UART errors (overrun, framing, noise)
 *
 * H7 specifics:
 * - DMA request routing via DMAMUX (Init.Request instead of Init.Channel)
 * - Ring buffer lives in the malloc heap (RAM_D1 / AXI SRAM) which is
 *   DMA-accessible; DTCM is not. D-cache is left disabled by the example.
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
        // DMA has lapped the reader - buffer content is no longer coherent.
        // Drop it all rather than deliver corrupt data.
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
    pHandle->huart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&pHandle->huart) != HAL_OK) {
        free(pHandle);
        return NULL;
    }

    // Configure circular DMA for RX (writes directly into the ring buffer)
    U_PORT_UART_DMA_CLK_ENABLE();
    pHandle->hdmaRx.Instance = U_PORT_UART_RX_DMA_STREAM;
    pHandle->hdmaRx.Init.Request = U_PORT_UART_RX_DMA_REQUEST;
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
    // Using priority 6 to ensure it's lower priority than FreeRTOS syscalls
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

/**
 * @brief DMA transfer complete callback (circular mode = buffer wrap)
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
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
        __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_FEF);
        HAL_UART_DMAStop(huart);
        startRxDma(gpUartHandle);
    }
}

/* ----------------------------------------------------------------
 * UART INTERRUPT HANDLERS
 * -------------------------------------------------------------- */

/**
 * @brief UART interrupt handler
 *
 * This function must be called from your UART IRQ handler in your
 * main application code (e.g., in stm32h7xx_it.c):
 *
 * void USART1_IRQHandler(void)
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
 * main application code (e.g., in stm32h7xx_it.c):
 *
 * void DMA1_Stream0_IRQHandler(void)
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
