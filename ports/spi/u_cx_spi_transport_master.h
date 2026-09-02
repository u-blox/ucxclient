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
 * @brief uCX2 SPI transport - portable master (host) state machine.
 *
 * Drives the STATUS / WRITE / READ transactions, the DRDY handshake and
 * the credit-based flow control described in the uCX2 SPI Transport
 * Protocol specification (section 4/5), on top of the physical
 * uPortSpi*() primitives (see u_port_spi.h). This file has no platform
 * dependency of its own - only u_port_spi.h and u_port.h (time/sleep) -
 * so it is reusable by any platform that implements a u_port_spi_xxx.c
 * physical driver.
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET.
 */

#ifndef U_CX_SPI_TRANSPORT_MASTER_H
#define U_CX_SPI_TRANSPORT_MASTER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "u_port_spi.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/**
 * Largest WRITE payload / READ response payload the transport will ever
 * request in a single transaction.
 *
 * This is a PRODUCT transfer-profile limit, not a protocol limit (the
 * wire format's txLen/rxLen fields are 16-bit). The spec documents
 * 4380 bytes as the NORA-W56x profile value; NORA-W36's own profile has
 * not been confirmed yet - revisit this once it is.
 */
#ifndef U_CX_SPI_TRANSPORT_MAX_PAYLOAD
#define U_CX_SPI_TRANSPORT_MAX_PAYLOAD  4380
#endif

/** Timeout waiting for a DRDY level/edge during a single transaction phase. */
#ifndef U_CX_SPI_TRANSPORT_DRDY_TIMEOUT_MS
#define U_CX_SPI_TRANSPORT_DRDY_TIMEOUT_MS  1000
#endif

/** Poll interval used while blocking in uCxSpiTransportMasterRead(). */
#ifndef U_CX_SPI_TRANSPORT_POLL_INTERVAL_MS
#define U_CX_SPI_TRANSPORT_POLL_INTERVAL_MS  2
#endif

/** Backoff between STATUS retries when the slave reports zero rx_credit. */
#ifndef U_CX_SPI_TRANSPORT_CREDIT_BACKOFF_MS
#define U_CX_SPI_TRANSPORT_CREDIT_BACKOFF_MS  5
#endif

/** Max number of zero-credit backoff retries in a single Write() call before giving up. */
#ifndef U_CX_SPI_TRANSPORT_CREDIT_RETRY_LIMIT
#define U_CX_SPI_TRANSPORT_CREDIT_RETRY_LIMIT  50
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Master transport instance state. Caller owns the storage (no malloc). */
typedef struct {
    uPortSpiHandle_t spiHandle;
    uint16_t rxCredit;    ///< Last known slave write-accept capacity (snapshot, see spec 4.3).
    bool isOpen;
    uint8_t payloadBuf[U_CX_SPI_TRANSPORT_MAX_PAYLOAD]; ///< Scratch RX buffer for READ transactions.
} uCxSpiTransportMaster_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Open the transport: opens the physical SPI port and performs an
 * initial STATUS transaction to learn the slave's starting rx_credit.
 *
 * @param[in,out] pMaster   Instance storage, provided by caller (e.g. static).
 * @param[in]     pSpiConfig Physical SPI config (clock rate), or NULL for defaults.
 * @return                  true on success, false if the physical port or the
 *                          initial STATUS sync failed.
 */
bool uCxSpiTransportMasterOpen(uCxSpiTransportMaster_t *pMaster, const uPortSpiConfig_t *pSpiConfig);

/**
 * @brief Close the transport and the underlying physical SPI port.
 *
 * @param[in,out] pMaster  Instance previously opened with uCxSpiTransportMasterOpen().
 */
void uCxSpiTransportMasterClose(uCxSpiTransportMaster_t *pMaster);

/**
 * @brief Write bytes to the module, honouring credit-based flow control.
 *
 * Splits the request into WRITE transactions no larger than the last known
 * rx_credit, refreshing credit via STATUS transactions as needed. Blocks
 * until all bytes are sent or a transport error/backoff-limit occurs.
 *
 * @param[in,out] pMaster  Open transport instance.
 * @param[in]     pData    Bytes to write.
 * @param         length   Number of bytes to write.
 * @return                 Number of bytes actually written, or negative on error.
 */
int32_t uCxSpiTransportMasterWrite(uCxSpiTransportMaster_t *pMaster, const void *pData, size_t length);

/**
 * @brief Read bytes from the module, blocking up to timeoutMs for data to arrive.
 *
 * @param[in,out] pMaster    Open transport instance.
 * @param[out]    pData      Buffer to store received data.
 * @param         length     Maximum number of bytes to read.
 * @param         timeoutMs  Timeout in milliseconds (0 = non-blocking, -1 = blocking).
 * @return                   Number of bytes actually read, or negative on error.
 */
int32_t uCxSpiTransportMasterRead(uCxSpiTransportMaster_t *pMaster, void *pData, size_t length, int32_t timeoutMs);

/**
 * @brief Discard any data currently queued on the module (drains via READ).
 *
 * @param[in,out] pMaster  Open transport instance.
 */
void uCxSpiTransportMasterFlushRx(uCxSpiTransportMaster_t *pMaster);

#ifdef __cplusplus
}
#endif

#endif // U_CX_SPI_TRANSPORT_MASTER_H
