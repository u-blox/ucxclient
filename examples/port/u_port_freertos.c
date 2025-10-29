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
 * @brief FreeRTOS port layer implementation
 *
 * This port provides integration with FreeRTOS for embedded platforms like STM32.
 * 
 * IMPORTANT: You need to implement the UART functions for your specific hardware:
 * - uPortUartOpen()
 * - uPortUartClose()
 * - uPortUartWrite()
 * - uPortUartRead()
 * - uPortUartFlush()
 *
 * Examples are provided below for STM32 HAL, but you should adapt them
 * to your specific hardware and UART driver.
 */

#include <string.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "u_cx_log.h"
#include "u_port.h"

/* If using STM32 HAL, uncomment this and include your specific STM32 HAL header */
/* #include "stm32f4xx_hal.h" */  // Adjust for your STM32 family (F4, F7, H7, etc.)

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    void *pUartHandle;  // Pointer to hardware UART handle (e.g., UART_HandleTypeDef for STM32)
    uCxAtClient_t *pClient;
} uPortContext_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length);
static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs);

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uCxAtClientConfig_t *gPConfig = NULL;

/* ----------------------------------------------------------------
 * HARDWARE-SPECIFIC UART FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * Open and configure UART for the specified device
 * 
 * YOU MUST IMPLEMENT THIS FOR YOUR HARDWARE!
 * 
 * For STM32 HAL example:
 * - Parse pDevName to determine which UART to use (e.g., "UART1", "UART2")
 * - Configure the UART handle with the specified baudrate
 * - Enable hardware flow control if useFlowControl is true
 * - Return a handle to the UART (e.g., &huart1)
 *
 * @param[in] pDevName:        Device name (e.g., "UART1")
 * @param[in] baudRate:        UART baud rate
 * @param[in] useFlowControl:  Enable CTS/RTS flow control
 * @return Pointer to UART handle on success, NULL on failure
 */
static void *uPortUartOpen(const char *pDevName, int baudRate, bool useFlowControl)
{
    /* EXAMPLE IMPLEMENTATION FOR STM32 HAL (you need to adapt this):
    
    UART_HandleTypeDef *pHuart = NULL;
    
    // Select UART instance based on device name
    if (strcmp(pDevName, "UART1") == 0) {
        pHuart = &huart1;  // Extern handle from main.c or MX init
    } else if (strcmp(pDevName, "UART2") == 0) {
        pHuart = &huart2;
    } else {
        return NULL;
    }
    
    // Configure baud rate
    pHuart->Init.BaudRate = baudRate;
    pHuart->Init.WordLength = UART_WORDLENGTH_8B;
    pHuart->Init.StopBits = UART_STOPBITS_1;
    pHuart->Init.Parity = UART_PARITY_NONE;
    pHuart->Init.Mode = UART_MODE_TX_RX;
    
    if (useFlowControl) {
        pHuart->Init.HwFlowCtl = UART_HWCONTROL_RTS_CTS;
    } else {
        pHuart->Init.HwFlowCtl = UART_HWCONTROL_NONE;
    }
    
    pHuart->Init.OverSampling = UART_OVERSAMPLING_16;
    
    if (HAL_UART_Init(pHuart) != HAL_OK) {
        return NULL;
    }
    
    return pHuart;
    */
    
    /* PLACEHOLDER IMPLEMENTATION - REPLACE WITH YOUR HARDWARE CODE */
    (void)pDevName;
    (void)baudRate;
    (void)useFlowControl;
    
    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "uPortUartOpen() not implemented for your hardware!");
    U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "Please implement UART functions in u_port_freertos.c");
    
    return NULL;  // Return NULL to indicate failure
}

/**
 * Close UART
 * 
 * @param[in] pUartHandle:  Handle returned by uPortUartOpen()
 */
static void uPortUartClose(void *pUartHandle)
{
    /* EXAMPLE FOR STM32 HAL:
    
    UART_HandleTypeDef *pHuart = (UART_HandleTypeDef *)pUartHandle;
    HAL_UART_DeInit(pHuart);
    */
    
    (void)pUartHandle;
}

/**
 * Write data to UART
 * 
 * @param[in] pUartHandle:  Handle returned by uPortUartOpen()
 * @param[in] pData:        Data to write
 * @param[in] length:       Number of bytes to write
 * @return Number of bytes written, or negative on error
 */
static int32_t uPortUartWrite(void *pUartHandle, const void *pData, size_t length)
{
    /* EXAMPLE FOR STM32 HAL:
    
    UART_HandleTypeDef *pHuart = (UART_HandleTypeDef *)pUartHandle;
    
    if (HAL_UART_Transmit(pHuart, (uint8_t *)pData, length, 1000) == HAL_OK) {
        return (int32_t)length;
    }
    return -1;
    */
    
    (void)pUartHandle;
    (void)pData;
    (void)length;
    return -1;
}

/**
 * Read data from UART
 * 
 * @param[in]  pUartHandle:  Handle returned by uPortUartOpen()
 * @param[out] pData:        Buffer to read into
 * @param[in]  length:       Maximum bytes to read
 * @param[in]  timeoutMs:    Timeout in milliseconds (0 = non-blocking)
 * @return Number of bytes read, 0 if no data available, or negative on error
 */
static int32_t uPortUartRead(void *pUartHandle, void *pData, size_t length, int32_t timeoutMs)
{
    /* EXAMPLE FOR STM32 HAL:
    
    UART_HandleTypeDef *pHuart = (UART_HandleTypeDef *)pUartHandle;
    
    if (timeoutMs == 0) {
        // Non-blocking read - check if data is available
        if (__HAL_UART_GET_FLAG(pHuart, UART_FLAG_RXNE) == RESET) {
            return 0;  // No data available
        }
    }
    
    HAL_StatusTypeDef status = HAL_UART_Receive(pHuart, (uint8_t *)pData, 1, timeoutMs);
    if (status == HAL_OK) {
        return 1;
    } else if (status == HAL_TIMEOUT) {
        return 0;
    }
    return -1;
    */
    
    (void)pUartHandle;
    (void)pData;
    (void)length;
    (void)timeoutMs;
    return 0;  // No data
}

/**
 * Flush UART buffers
 * 
 * @param[in] pUartHandle:  Handle returned by uPortUartOpen()
 */
static void uPortUartFlush(void *pUartHandle)
{
    /* EXAMPLE FOR STM32 HAL:
    
    UART_HandleTypeDef *pHuart = (UART_HandleTypeDef *)pUartHandle;
    
    // Abort any ongoing transfers
    HAL_UART_AbortReceive(pHuart);
    HAL_UART_AbortTransmit(pHuart);
    
    // Clear any pending data in hardware FIFO
    __HAL_UART_FLUSH_DRREGISTER(pHuart);
    */
    
    (void)pUartHandle;
}

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    (void)pClient;
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    
    return uPortUartWrite(pCtx->pUartHandle, pData, length);
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    (void)pClient;
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    
    return uPortUartRead(pCtx->pUartHandle, pData, length, timeoutMs);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

int32_t uPortGetTickTimeMs(void)
{
    TickType_t ticks = xTaskGetTickCount();
    return (int32_t)(ticks * portTICK_PERIOD_MS);
}

int32_t uPortMutexTryLock(SemaphoreHandle_t mutex, int32_t timeoutMs)
{
    TickType_t ticksToWait;
    
    if (timeoutMs == 0) {
        ticksToWait = 0;
    } else if (timeoutMs < 0) {
        ticksToWait = portMAX_DELAY;
    } else {
        ticksToWait = pdMS_TO_TICKS(timeoutMs);
    }
    
    if (xSemaphoreTake(mutex, ticksToWait) == pdTRUE) {
        return 0;  // Success
    }
    return -1;  // Timeout
}

void uPortAtInit(uCxAtClient_t *pClient)
{
    static uPortContext_t context;
    static char rxBuf[1024];
#if U_CX_USE_URC_QUEUE == 1
    static char urcBuf[1024];
#endif
    static uCxAtClientConfig_t config = {
        .pRxBuffer = &rxBuf[0],
        .rxBufferLen = sizeof(rxBuf),
#if U_CX_USE_URC_QUEUE == 1
        .pUrcBuffer = &urcBuf[0],
        .urcBufferLen = sizeof(urcBuf),
#endif
        .pStreamHandle = &context,
        .write = uartWrite,
        .read = uartRead
    };

    memset(&context, 0, sizeof(context));
    context.pClient = pClient;
    context.pUartHandle = NULL;

    // Current implementation of this port only supports one instance
    configASSERT(gPConfig == NULL);
    gPConfig = &config;

    uCxAtClientInit(&config, pClient);
}

bool uPortAtOpen(uCxAtClient_t *pClient, const char *pDevName, int baudRate, bool useFlowControl)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    configASSERT(pClient->pConfig != NULL);
    configASSERT(pCtx != NULL);
    configASSERT(pCtx->pUartHandle == NULL);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Opening %s at %d with %s flow control",
                    pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    pCtx->pUartHandle = uPortUartOpen(pDevName, baudRate, useFlowControl);
    if (pCtx->pUartHandle == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open UART");
        return false;
    }

    return true;
}

void uPortAtClose(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    configASSERT(pCtx->pUartHandle != NULL);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Closing UART");

    uPortUartClose(pCtx->pUartHandle);
    pCtx->pUartHandle = NULL;
}

void uPortAtFlush(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    
    if (pCtx->pUartHandle != NULL) {
        // Flush hardware UART buffers
        uPortUartFlush(pCtx->pUartHandle);
        
        // Clear the AT client's internal RX buffer
        if (pClient->pConfig->pRxBuffer != NULL) {
            memset(pClient->pConfig->pRxBuffer, 0, pClient->pConfig->rxBufferLen);
        }
        
        U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Serial buffers flushed");
    }
}
