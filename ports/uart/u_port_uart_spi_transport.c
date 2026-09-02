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
 * @brief u_port_uart.h implemented on top of the uCX2 SPI transport.
 *
 * This is a "virtual UART" adapter: it implements the exact same
 * uPortUart*() API that u_cx_at_client.c already calls, but backs it with
 * the SPI transport (u_cx_spi_transport_master.c) instead of a real UART
 * peripheral. This lets the AT client, and every example built on top of
 * it, run unmodified over SPI - only the build's port source selection
 * changes (see cmake/stm32.cmake, -DSTM32_TRANSPORT=spi).
 *
 * pDevName and useFlowControl are not meaningful for SPI and are ignored;
 * baudRate is reinterpreted as the desired SPI clock frequency in Hz (0
 * selects the platform default - see u_port_spi_stm32h7.h).
 *
 * This file is portable: it only depends on u_port_spi.h and
 * u_cx_spi_transport_master.h, so it works with any platform that provides
 * a u_port_spi_xxx.c physical driver (currently: STM32H7 only).
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET.
 */

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#include "u_port_uart.h"
#include "u_port_spi.h"
#include "u_cx_spi_transport_master.h"

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uCxSpiTransportMaster_t gMaster;
static bool gInUse = false;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uPortUartHandle_t uPortUartOpen(const char *pDevName, int32_t baudRate, bool useFlowControl)
{
    uPortSpiConfig_t spiConfig;

    (void)pDevName;       // Not meaningful for SPI - single fixed physical port
    (void)useFlowControl; // Flow control is handled by the transport's rx_credit mechanism

    if (gInUse) {
        return NULL; // Only one instance supported
    }

    spiConfig.clockHz = (baudRate > 0) ? (uint32_t)baudRate : 0;

    if (!uCxSpiTransportMasterOpen(&gMaster, &spiConfig)) {
        return NULL;
    }

    gInUse = true;
    return (uPortUartHandle_t)&gMaster;
}

void uPortUartClose(uPortUartHandle_t handle)
{
    if ((handle == NULL) || !gInUse) {
        return;
    }

    uCxSpiTransportMasterClose((uCxSpiTransportMaster_t *)handle);
    gInUse = false;
}

int32_t uPortUartWrite(uPortUartHandle_t handle, const void *pData, size_t length)
{
    if (handle == NULL) {
        return -1;
    }
    return uCxSpiTransportMasterWrite((uCxSpiTransportMaster_t *)handle, pData, length);
}

int32_t uPortUartRead(uPortUartHandle_t handle, void *pData, size_t length, int32_t timeoutMs)
{
    if (handle == NULL) {
        return -1;
    }
    return uCxSpiTransportMasterRead((uCxSpiTransportMaster_t *)handle, pData, length, timeoutMs);
}

void uPortUartFlushRx(uPortUartHandle_t handle)
{
    if (handle == NULL) {
        return;
    }
    uCxSpiTransportMasterFlushRx((uCxSpiTransportMaster_t *)handle);
}

void uPortUartFlushTx(uPortUartHandle_t handle)
{
    // uCxSpiTransportMasterWrite() already blocks until all bytes are sent
    // (or an error/backoff-limit occurs), so there is no pending async TX
    // queue to flush here.
    (void)handle;
}
