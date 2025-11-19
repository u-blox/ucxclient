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
 * @brief Zephyr port example
 *
 * The current implementation only support one instance of the AT client.
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <string.h>

#include "u_cx_log.h"
#include "u_cx_at_client.h"
#include "u_port.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_RINGBUFFER_SIZE   128

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    struct k_work rxWork;
    struct ring_buf rxRingBuf;
    struct k_sem rxSem;
    uint8_t rxBuffer[U_RINGBUFFER_SIZE];
    const struct device *pUartDev;
    uCxAtClient_t *pClient;
} uPortContext_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uCxAtClientConfig_t *gPConfig = NULL;
bool gDisableRxWorker = 0; // Used during test

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

void uartIsr(const struct device *dev, void *user_data)
{
    uPortContext_t *pCtx = (uPortContext_t *)user_data;
    int ret = 1;
    bool signalRxWorker = false;
    while (uart_irq_update(dev) && uart_irq_rx_ready(dev) && (ret > 0)) {
        uint8_t *pData;
        signalRxWorker = true;
        ret = ring_buf_put_claim(&pCtx->rxRingBuf, &pData, UINT32_MAX);
        if (ret == 0) {
            // Ring buffer full
            uart_irq_rx_disable(dev);
        } else {
            // Read as many bytes as ring_buf_put_claim() returned
            ret = uart_fifo_read(dev, pData, ret);
        }
        ring_buf_put_finish(&pCtx->rxRingBuf, (ret > 0) ? ret : 0);
    }
    if (signalRxWorker) {
        k_sem_give(&pCtx->rxSem);
        if (!k_work_is_pending(&pCtx->rxWork) && !gDisableRxWorker) {
            k_work_submit(&pCtx->rxWork);
        }
    }
}

void rxTask(struct k_work *pItem)
{
    uPortContext_t *pCtx = CONTAINER_OF(pItem, uPortContext_t, rxWork);
    uCxAtClientHandleRx(pCtx->pClient);
}

static int32_t uartRead(uCxAtClient_t *pClient, void *pStreamHandle, void *pData, size_t length, int32_t timeoutMs)
{
    (void)pClient;
    int32_t bytesRead = 0;
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;

    if (timeoutMs > 0) {
        uint8_t *pDataBytes = (uint8_t *)pData;
        uint32_t startTimeMs = k_uptime_get_32();
        int32_t timePassedMs = 0;
        while (bytesRead < length) {
            if (ring_buf_is_empty(&pCtx->rxRingBuf)) {
                if (timePassedMs > timeoutMs) {
                    // Timeout
                    break;
                }
                int32_t remainingMs = timeoutMs - timePassedMs;
                k_sem_take(&pCtx->rxSem, K_MSEC(remainingMs));
            } else {
                uart_irq_rx_disable(pCtx->pUartDev);
                bytesRead += ring_buf_get(&pCtx->rxRingBuf, &pDataBytes[bytesRead], length - bytesRead);
                uart_irq_rx_enable(pCtx->pUartDev);
            }
            timePassedMs = (int32_t)(k_uptime_get_32() - startTimeMs);
        }
    } else {
        if (!ring_buf_is_empty(&pCtx->rxRingBuf)) {
            uart_irq_rx_disable(pCtx->pUartDev);
            bytesRead = ring_buf_get(&pCtx->rxRingBuf, pData, length);
            uart_irq_rx_enable(pCtx->pUartDev);
        }
    }

    return bytesRead;
}

static int32_t uartWrite(uCxAtClient_t *pClient, void *pStreamHandle, const void *pData, size_t length)
{
    (void)pClient;
    uPortContext_t *pCtx = (uPortContext_t *)pStreamHandle;
    return (int32_t)uart_fifo_fill(pCtx->pUartDev, pData, length);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uPortAtInit(uCxAtClient_t *pClient)
{
    static uPortContext_t context;
    static char rxBuf[CONFIG_UCXCLIENT_RX_BUFFER_SIZE];
#if U_CX_USE_URC_QUEUE == 1
    static char urcBuf[CONFIG_UCXCLIENT_URC_QUEUE_SIZE];
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
    ring_buf_init(&context.rxRingBuf, sizeof(context.rxBuffer),
                  context.rxBuffer);
    k_work_init(&context.rxWork, rxTask);
    k_sem_init(&context.rxSem, 0, 1);

    // Current implementation of this port only support one instance
    U_CX_AT_PORT_ASSERT(gPConfig == NULL);
    gPConfig = &config;

    uCxAtClientInit(&config, pClient);
}

bool uPortAtOpen(uCxAtClient_t *pClient, const char *pDevName, int baudRate, bool useFlowControl)
{
    struct uart_config config = {
        .baudrate = baudRate,
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = useFlowControl ?
                     UART_CFG_FLOW_CTRL_RTS_CTS : UART_CFG_FLOW_CTRL_NONE
    };
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    U_CX_AT_PORT_ASSERT(pClient->pConfig != NULL);
    U_CX_AT_PORT_ASSERT(pCtx != NULL);
    U_CX_AT_PORT_ASSERT(pCtx->pUartDev == NULL);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Opening %s at %d with %s flow control",
                  pDevName, baudRate, useFlowControl ? "CTS/RTS" : "no");

    pCtx->pUartDev = device_get_binding(pDevName);
    if (pCtx->pUartDev == NULL) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to open UART %s", pDevName);
        return false;
    }
    if (!device_is_ready(pCtx->pUartDev)) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "UART %s is not ready", pDevName);
        return false;
    }
    if (uart_irq_callback_user_data_set(pCtx->pUartDev, uartIsr, pCtx) < 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to set UART callback");
        return false;
    }
    if (uart_configure(pCtx->pUartDev, &config) < 0) {
        U_CX_LOG_LINE_I(U_CX_LOG_CH_ERROR, pClient->instance, "Failed to configure UART");
        return false;
    }

    ring_buf_reset(&pCtx->rxRingBuf);
    uart_irq_rx_enable(pCtx->pUartDev);

    return true;
}

void uPortAtClose(uCxAtClient_t *pClient)
{
    uPortContext_t *pCtx = pClient->pConfig->pStreamHandle;
    U_CX_AT_PORT_ASSERT(pCtx->pUartDev != NULL);

    U_CX_LOG_LINE_I(U_CX_LOG_CH_DBG, pClient->instance, "Closing UART");

    uart_irq_rx_disable(pCtx->pUartDev);
    k_work_cancel(&pCtx->rxWork);
    pCtx->pUartDev = NULL;
}
