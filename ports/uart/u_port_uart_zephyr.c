/*
 * Copyright 2025 u-blox
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
 * @brief Zephyr UART port implementation using Zephyr UART driver.
 *
 * This implementation uses static allocation and currently supports a single UART instance.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include "u_port_uart.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_RINGBUFFER_SIZE   128

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Structure representing a UART handle.
 */
typedef struct {
    const struct device *pUartDev;
    struct ring_buf rxRingBuf;
    struct k_sem rxSem;
    uint8_t rxBuffer[U_RINGBUFFER_SIZE];
} uPortUartHandle;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortUartHandle gUartHandle = {0};

/* ----------------------------------------------------------------
 * STATIC FUNCTION PROTOTYPES
 * -------------------------------------------------------------- */

static void uartIsr(const struct device *dev, void *user_data);

/* ----------------------------------------------------------------
 * FORWARD DECLARATIONS
 * -------------------------------------------------------------- */

// Forward declare the RX signal function from OS layer
extern void uPortRxSignal(void);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static void uartIsr(const struct device *dev, void *user_data)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)user_data;
    int ret = 1;
    bool signalRxWorker = false;

    while (uart_irq_update(dev) && uart_irq_rx_ready(dev) && (ret > 0)) {
        uint8_t *pData;
        signalRxWorker = true;
        ret = ring_buf_put_claim(&pHandle->rxRingBuf, &pData, UINT32_MAX);
        if (ret == 0) {
            // Ring buffer full
            uart_irq_rx_disable(dev);
        } else {
            // Read as many bytes as ring_buf_put_claim() returned
            ret = uart_fifo_read(dev, pData, ret);
        }
        ring_buf_put_finish(&pHandle->rxRingBuf, (ret > 0) ? ret : 0);
    }

    if (signalRxWorker) {
        k_sem_give(&pHandle->rxSem);
        uPortRxSignal();
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uPortUartHandle_t uPortUartOpen(const char *pDevName, int32_t baudRate, bool useFlowControl)
{
    if (pDevName == NULL) {
        return NULL;
    }

    // Use static handle - only supports one UART instance
    uPortUartHandle *pHandle = &gUartHandle;
    
    // Check if already in use
    if (pHandle->pUartDev != NULL) {
        return NULL;
    }

    memset(pHandle, 0, sizeof(uPortUartHandle));

    // Get UART device
    pHandle->pUartDev = device_get_binding(pDevName);
    if (pHandle->pUartDev == NULL) {
        return NULL;
    }

    if (!device_is_ready(pHandle->pUartDev)) {
        pHandle->pUartDev = NULL;
        return NULL;
    }

    // Configure UART
    struct uart_config config = {
        .baudrate = baudRate,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = useFlowControl ? UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE
    };

    if (uart_configure(pHandle->pUartDev, &config) < 0) {
        pHandle->pUartDev = NULL;
        return NULL;
    }

    // Initialize ring buffer and semaphore
    ring_buf_init(&pHandle->rxRingBuf, sizeof(pHandle->rxBuffer), pHandle->rxBuffer);
    k_sem_init(&pHandle->rxSem, 0, 1);

    // Set up interrupt callback
    if (uart_irq_callback_user_data_set(pHandle->pUartDev, uartIsr, pHandle) < 0) {
        pHandle->pUartDev = NULL;
        return NULL;
    }

    // Enable RX interrupts
    uart_irq_rx_enable(pHandle->pUartDev);

    return (uPortUartHandle_t)pHandle;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    if (handle != NULL) {
        uPortUartHandle *pHandle = (uPortUartHandle *)handle;
        uart_irq_rx_disable(pHandle->pUartDev);
        pHandle->pUartDev = NULL;
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
    return (int32_t)uart_fifo_fill(pHandle->pUartDev, pData, length);
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
    int32_t bytesRead = 0;

    if (timeoutMs > 0) {
        uint8_t *pDataBytes = (uint8_t *)pData;
        uint32_t startTimeMs = k_uptime_get_32();
        int32_t timePassedMs = 0;
        
        while (bytesRead < (int32_t)length) {
            if (ring_buf_is_empty(&pHandle->rxRingBuf)) {
                if (timePassedMs > timeoutMs) {
                    // Timeout
                    break;
                }
                int32_t remainingMs = timeoutMs - timePassedMs;
                k_sem_take(&pHandle->rxSem, K_MSEC(remainingMs));
            } else {
                if (pDataBytes != NULL) {
                    uart_irq_rx_disable(pHandle->pUartDev);
                    bytesRead += (int32_t)ring_buf_get(&pHandle->rxRingBuf, 
                                                        &pDataBytes[bytesRead], 
                                                        length - (size_t)bytesRead);
                    uart_irq_rx_enable(pHandle->pUartDev);
                } else {
                    // NULL buffer - just return 0
                    break;
                }
            }
            timePassedMs = (int32_t)(k_uptime_get_32() - startTimeMs);
        }
    } else {
        if (!ring_buf_is_empty(&pHandle->rxRingBuf) && (pData != NULL)) {
            uart_irq_rx_disable(pHandle->pUartDev);
            bytesRead = (int32_t)ring_buf_get(&pHandle->rxRingBuf, pData, length);
            uart_irq_rx_enable(pHandle->pUartDev);
        }
    }

    return bytesRead;
}
