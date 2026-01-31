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
 * @brief Raspberry Pi Pico UART port implementation
 *
 * Supports RP2040 (Pico), RP2350 (Pico 2)
 * This implementation uses Pico SDK and supports:
 * - UART0 or UART1 instances
 * - Hardware flow control (RTS/CTS) - optional
 * - Interrupt-driven reception with circular buffer
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/sync.h"
#include "hardware/uart.h"
#include "hardware/gpio.h"
#include "hardware/irq.h"

#include "u_port_uart.h"
#include "u_port_uart_pico.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Structure representing a UART handle */
typedef struct {
    uart_inst_t *uart;
    uint8_t rxBuffer[U_PORT_UART_RX_BUFFER_SIZE];
    volatile uint32_t rxHead;
    volatile uint32_t rxTail;
    bool isOpen;
    bool useFlowControl;
} uPortUartHandle;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortUartHandle *gpUartHandle = NULL;

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

/**
 * @brief UART RX interrupt handler for UART0
 */
static void uart0_rx_isr(void)
{
    if (gpUartHandle == NULL || gpUartHandle->uart != uart0) {
        return;
    }

    while (uart_is_readable(uart0)) {
        uint8_t ch = uart_getc(uart0);
        uint32_t nextHead = (gpUartHandle->rxHead + 1) % U_PORT_UART_RX_BUFFER_SIZE;
        
        if (nextHead != gpUartHandle->rxTail) {
            gpUartHandle->rxBuffer[gpUartHandle->rxHead] = ch;
            gpUartHandle->rxHead = nextHead;
        }
        // else: buffer full, drop byte
    }
}

/**
 * @brief UART RX interrupt handler for UART1
 */
static void uart1_rx_isr(void)
{
    if (gpUartHandle == NULL || gpUartHandle->uart != uart1) {
        return;
    }

    while (uart_is_readable(uart1)) {
        uint8_t ch = uart_getc(uart1);
        uint32_t nextHead = (gpUartHandle->rxHead + 1) % U_PORT_UART_RX_BUFFER_SIZE;
        
        if (nextHead != gpUartHandle->rxTail) {
            gpUartHandle->rxBuffer[gpUartHandle->rxHead] = ch;
            gpUartHandle->rxHead = nextHead;
        }
        // else: buffer full, drop byte
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uPortUartHandle_t uPortUartOpen(const char *pDevice, int32_t baudRate, bool useFlowControl)
{
    (void)pDevice;  // Device name not used on embedded systems - using configured pins

    if (gpUartHandle != NULL) {
        // Only one UART instance supported at a time
        printf("[u_port_uart_rp2040] Error: UART already open\n");
        return NULL;
    }

    uPortUartHandle *pHandle = (uPortUartHandle *)malloc(sizeof(uPortUartHandle));
    if (pHandle == NULL) {
        printf("[u_port_uart_rp2040] Error: Failed to allocate handle\n");
        return NULL;
    }

    memset(pHandle, 0, sizeof(uPortUartHandle));
    pHandle->uart = U_PORT_UART_INSTANCE;
    pHandle->useFlowControl = useFlowControl;

    // Initialize UART
    uart_init(pHandle->uart, (uint32_t)baudRate);

    // Set GPIO functions for TX and RX
    gpio_set_function(U_PORT_UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(U_PORT_UART_RX_PIN, GPIO_FUNC_UART);

    // Configure flow control if requested
    if (useFlowControl && U_PORT_UART_CTS_PIN >= 0 && U_PORT_UART_RTS_PIN >= 0) {
        gpio_set_function(U_PORT_UART_CTS_PIN, GPIO_FUNC_UART);
        gpio_set_function(U_PORT_UART_RTS_PIN, GPIO_FUNC_UART);
        uart_set_hw_flow(pHandle->uart, true, true);
    } else {
        uart_set_hw_flow(pHandle->uart, false, false);
    }

    // Configure UART format: 8N1
    uart_set_format(pHandle->uart, 8, 1, UART_PARITY_NONE);
    uart_set_fifo_enabled(pHandle->uart, true);

    // Setup interrupt handler
    int irqNum;
    irq_handler_t handler;
    
    if (pHandle->uart == uart0) {
        irqNum = UART0_IRQ;
        handler = uart0_rx_isr;
    } else {
        irqNum = UART1_IRQ;
        handler = uart1_rx_isr;
    }

    irq_set_exclusive_handler(irqNum, handler);
    irq_set_enabled(irqNum, true);
    uart_set_irq_enables(pHandle->uart, true, false);

    pHandle->isOpen = true;
    gpUartHandle = pHandle;

    printf("[u_port_uart_rp2040] UART opened @ %ld baud, flow=%d\n", 
           baudRate, useFlowControl);

    return (uPortUartHandle_t)pHandle;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle == NULL || !pHandle->isOpen) {
        return;
    }

    // Disable interrupts
    int irqNum = (pHandle->uart == uart0) ? UART0_IRQ : UART1_IRQ;
    uart_set_irq_enables(pHandle->uart, false, false);
    irq_set_enabled(irqNum, false);

    // Deinit UART
    uart_deinit(pHandle->uart);

    pHandle->isOpen = false;
    
    if (gpUartHandle == pHandle) {
        gpUartHandle = NULL;
    }

    free(pHandle);

    printf("[u_port_uart_rp2040] UART closed\n");
}

int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle == NULL || !pHandle->isOpen || pData == NULL || length == 0) {
        return -1;
    }

    uart_write_blocking(pHandle->uart, (const uint8_t *)pData, length);

    return (int32_t)length;
}

int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle == NULL || !pHandle->isOpen || pData == NULL || length == 0) {
        return -1;
    }

    uint8_t *pBuffer = (uint8_t *)pData;
    size_t bytesRead = 0;
    absolute_time_t deadline;

    if (timeoutMs < 0) {
        // Blocking: wait forever
        deadline = at_the_end_of_time;
    } else if (timeoutMs == 0) {
        // Non-blocking: return immediately with available data
        deadline = get_absolute_time();
    } else {
        deadline = make_timeout_time_ms(timeoutMs);
    }

    while (bytesRead < length) {
        // Check if data is available
        if (pHandle->rxTail != pHandle->rxHead) {
            pBuffer[bytesRead++] = pHandle->rxBuffer[pHandle->rxTail];
            pHandle->rxTail = (pHandle->rxTail + 1) % U_PORT_UART_RX_BUFFER_SIZE;
        } else {
            // No data available
            if (timeoutMs == 0) {
                // Non-blocking mode - return what we have
                break;
            }
            
            if (time_reached(deadline)) {
                // Timeout reached
                break;
            }
            
            // Small sleep to avoid busy-waiting
            sleep_us(100);
        }
    }

    return (int32_t)bytesRead;
}

void uPortUartFlushRx(uPortUartHandle_t handle)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle == NULL || !pHandle->isOpen) {
        return;
    }

    // Disable interrupts briefly while clearing
    uint32_t save = save_and_disable_interrupts();
    pHandle->rxHead = 0;
    pHandle->rxTail = 0;
    restore_interrupts(save);

    // Also drain hardware FIFO
    while (uart_is_readable(pHandle->uart)) {
        uart_getc(pHandle->uart);
    }
}

void uPortUartFlushTx(uPortUartHandle_t handle)
{
    uPortUartHandle *pHandle = (uPortUartHandle *)handle;

    if (pHandle == NULL || !pHandle->isOpen) {
        return;
    }

    // Wait for TX FIFO to empty
    uart_tx_wait_blocking(pHandle->uart);
}
