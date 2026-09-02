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
 * @brief uCX2 SPI transport - portable master (host) state machine implementation.
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET.
 */

#include <string.h>

#include "u_port.h"
#include "u_cx_log.h"
#include "u_cx_spi_transport_protocol.h"
#include "u_cx_spi_transport_master.h"

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef enum {
    TXN_OK,
    TXN_NOT_READY,  ///< DRDY handshake timed out - transport not responding.
    TXN_BUSY,       ///< Slave returned STATUS_BUSY.
    TXN_INVALID,    ///< Slave returned STATUS_INVALID_REQUEST.
    TXN_NOT_ACTIVE, ///< Slave returned STATUS_NOT_ACTIVE.
    TXN_IO_ERROR    ///< Malformed/undecodable response or SPI transfer error.
} txnResult_t;

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static txnResult_t doTransaction(uCxSpiTransportMaster_t *pM,
                                 uCxSpiTransportOpcode_t opcode,
                                 const uint8_t *pTxPayload, uint16_t txLen, uint16_t rxLen,
                                 uint8_t *pRxPayloadOut, uint16_t *pRxPayloadLenOut,
                                 uCxSpiTransportResponse_t *pResponseOut);
static txnResult_t doStatus(uCxSpiTransportMaster_t *pM, uCxSpiTransportResponse_t *pResponseOut);
static size_t minSize(size_t a, size_t b);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static size_t minSize(size_t a, size_t b)
{
    return (a < b) ? a : b;
}

/** Runs one full request/response transaction, including the DRDY handshake.
 *
 * Per spec section 4.1: if DRDY was already high before this request was
 * issued ("stale high"), the master must not treat that as readiness for
 * the upcoming phase - it must observe DRDY go low, then high, before
 * clocking the second phase. If DRDY was low beforehand, simply waiting
 * for it to go high is sufficient.
 */
static txnResult_t doTransaction(uCxSpiTransportMaster_t *pM,
                                 uCxSpiTransportOpcode_t opcode,
                                 const uint8_t *pTxPayload, uint16_t txLen, uint16_t rxLen,
                                 uint8_t *pRxPayloadOut, uint16_t *pRxPayloadLenOut,
                                 uCxSpiTransportResponse_t *pResponseOut)
{
    uint8_t headerBuf[U_CX_SPI_TRANSPORT_HEADER_SIZE];
    bool drdyWasHigh = uPortSpiDrdyGetLevel(pM->spiHandle);
    txnResult_t result = TXN_OK;

    /* Phase 1: request header */
    uCxSpiTransportEncodeRequest(opcode, txLen, rxLen, headerBuf);
    uPortSpiCsAssert(pM->spiHandle);
    uPortSpiTransfer(pM->spiHandle, headerBuf, NULL, U_CX_SPI_TRANSPORT_HEADER_SIZE);
    uPortSpiCsDeassert(pM->spiHandle);

    /* DRDY handshake before the second phase */
    if (drdyWasHigh) {
        if (!uPortSpiDrdyWaitLevel(pM->spiHandle, false, U_CX_SPI_TRANSPORT_DRDY_TIMEOUT_MS)) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] DRDY stuck high (stale) - no LOW edge before opcode 0x%X", (unsigned)opcode);
            return TXN_NOT_READY;
        }
    }
    if (!uPortSpiDrdyWaitLevel(pM->spiHandle, true, U_CX_SPI_TRANSPORT_DRDY_TIMEOUT_MS)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] DRDY handshake timeout (opcode 0x%X)", (unsigned)opcode);
        return TXN_NOT_READY;
    }

    /* Phase 2: WRITE payload, or response header (+ payload for READ) */
    uPortSpiCsAssert(pM->spiHandle);
    if (opcode == U_CX_SPI_TRANSPORT_OPCODE_WRITE) {
        uPortSpiTransfer(pM->spiHandle, pTxPayload, NULL, txLen);
    } else {
        uint8_t rspHeader[U_CX_SPI_TRANSPORT_HEADER_SIZE];
        uCxSpiTransportResponse_t rsp;

        uPortSpiTransfer(pM->spiHandle, NULL, rspHeader, U_CX_SPI_TRANSPORT_HEADER_SIZE);

        if (!uCxSpiTransportDecodeResponse(rspHeader, sizeof(rspHeader), &rsp)) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] Malformed response header (opcode 0x%X)", (unsigned)opcode);
            result = TXN_IO_ERROR;
        } else {
            if (rsp.stickyFlags & U_CX_SPI_TRANSPORT_OVERRUN_STICKY) {
                U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "[SPI] Slave reports OVERRUN_STICKY - a prior WRITE was dropped");
            }
            if ((opcode == U_CX_SPI_TRANSPORT_OPCODE_READ) &&
                (rsp.status == U_CX_SPI_TRANSPORT_STATUS_OK)) {
                uint16_t k = rsp.txAvailable;
                if (k > rxLen) {
                    k = rxLen; // defensive clamp - slave should never report more than requested
                }
                if ((k > 0) && (pRxPayloadOut != NULL)) {
                    uPortSpiTransfer(pM->spiHandle, NULL, pRxPayloadOut, k);
                }
                if (pRxPayloadLenOut != NULL) {
                    *pRxPayloadLenOut = k;
                }
            }
            if (pResponseOut != NULL) {
                *pResponseOut = rsp;
            }
            switch (rsp.status) {
                case U_CX_SPI_TRANSPORT_STATUS_OK:
                    result = TXN_OK;
                    break;
                case U_CX_SPI_TRANSPORT_STATUS_BUSY:
                    result = TXN_BUSY;
                    break;
                case U_CX_SPI_TRANSPORT_STATUS_INVALID_REQUEST:
                    result = TXN_INVALID;
                    break;
                case U_CX_SPI_TRANSPORT_STATUS_NOT_ACTIVE:
                default:
                    result = TXN_NOT_ACTIVE;
                    break;
            }
        }
    }
    uPortSpiCsDeassert(pM->spiHandle);

    /* Completion: slave should drop DRDY low once it has processed the phase.
     * Best-effort only - a timeout here is logged but does not fail the
     * transaction, since the response/payload has already been captured. */
    if (!uPortSpiDrdyWaitLevel(pM->spiHandle, false, U_CX_SPI_TRANSPORT_DRDY_TIMEOUT_MS)) {
        U_CX_LOG_LINE(U_CX_LOG_CH_WARN, "[SPI] DRDY did not go low after completion (opcode 0x%X)", (unsigned)opcode);
    }

    return result;
}

static txnResult_t doStatus(uCxSpiTransportMaster_t *pM, uCxSpiTransportResponse_t *pResponseOut)
{
    return doTransaction(pM, U_CX_SPI_TRANSPORT_OPCODE_STATUS, NULL, 0, 0, NULL, NULL, pResponseOut);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

bool uCxSpiTransportMasterOpen(uCxSpiTransportMaster_t *pMaster, const uPortSpiConfig_t *pSpiConfig)
{
    uCxSpiTransportResponse_t rsp;

    memset(pMaster, 0, sizeof(*pMaster));

    pMaster->spiHandle = uPortSpiOpen(pSpiConfig);
    if (pMaster->spiHandle == NULL) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] uPortSpiOpen() failed");
        return false;
    }

    /* Sync with the slave via an initial STATUS query (spec section 5.1) to
     * learn the starting rx_credit before any WRITE is attempted. */
    if (doStatus(pMaster, &rsp) != TXN_OK) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] Initial STATUS sync failed");
        uPortSpiClose(pMaster->spiHandle);
        pMaster->spiHandle = NULL;
        return false;
    }

    pMaster->rxCredit = rsp.rxCredit;
    pMaster->isOpen = true;
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "[SPI] Transport synced, initial rx_credit=%u", (unsigned)pMaster->rxCredit);
    return true;
}

void uCxSpiTransportMasterClose(uCxSpiTransportMaster_t *pMaster)
{
    if (pMaster->spiHandle != NULL) {
        uPortSpiClose(pMaster->spiHandle);
        pMaster->spiHandle = NULL;
    }
    pMaster->isOpen = false;
}

int32_t uCxSpiTransportMasterWrite(uCxSpiTransportMaster_t *pMaster, const void *pData, size_t length)
{
    const uint8_t *pBytes = (const uint8_t *)pData;
    size_t sent = 0;
    int creditRetries = 0;

    if (!pMaster->isOpen) {
        return -1;
    }

    while (sent < length) {
        if (pMaster->rxCredit == 0) {
            uCxSpiTransportResponse_t rsp;

            if (creditRetries >= U_CX_SPI_TRANSPORT_CREDIT_RETRY_LIMIT) {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] Write: gave up waiting for rx_credit after %d retries", creditRetries);
                break;
            }
            creditRetries++;

            if (doStatus(pMaster, &rsp) != TXN_OK) {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] Write: STATUS refresh failed");
                break;
            }
            pMaster->rxCredit = rsp.rxCredit;

            if (pMaster->rxCredit == 0) {
                U_CX_PORT_SLEEP_MS(U_CX_SPI_TRANSPORT_CREDIT_BACKOFF_MS);
                continue;
            }
        }

        {
            size_t remaining = length - sent;
            size_t chunk = minSize(minSize(remaining, pMaster->rxCredit), U_CX_SPI_TRANSPORT_MAX_PAYLOAD);
            txnResult_t result = doTransaction(pMaster, U_CX_SPI_TRANSPORT_OPCODE_WRITE,
                                               &pBytes[sent], (uint16_t)chunk, 0,
                                               NULL, NULL, NULL);
            if (result != TXN_OK) {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] WRITE transaction failed (result=%d)", (int)result);
                break;
            }
            /* Chunk was <= previously-known credit, so per the protocol's
             * atomic-write semantics it cannot have been dropped for lack
             * of credit. Optimistically consume it locally; credit is
             * refreshed from the slave again next time it reaches zero. */
            pMaster->rxCredit = (uint16_t)(pMaster->rxCredit - chunk);
            sent += chunk;
            creditRetries = 0;
        }
    }

    return (sent > 0) ? (int32_t)sent : (length == 0 ? 0 : -1);
}

int32_t uCxSpiTransportMasterRead(uCxSpiTransportMaster_t *pMaster, void *pData, size_t length, int32_t timeoutMs)
{
    int32_t startTime = U_CX_PORT_GET_TIME_MS();
    uint16_t rxLen;

    if (!pMaster->isOpen) {
        return -1;
    }

    rxLen = (uint16_t)minSize(length, U_CX_SPI_TRANSPORT_MAX_PAYLOAD);
    if (rxLen == 0) {
        return 0;
    }

    for (;;) {
        if (uPortSpiDrdyGetLevel(pMaster->spiHandle)) {
            uCxSpiTransportResponse_t rsp;
            uint16_t got = 0;
            txnResult_t result = doTransaction(pMaster, U_CX_SPI_TRANSPORT_OPCODE_READ,
                                               NULL, 0, rxLen,
                                               (uint8_t *)pData, &got, &rsp);
            if (result == TXN_OK) {
                pMaster->rxCredit = rsp.rxCredit; // opportunistic credit refresh
                if (got > 0) {
                    return (int32_t)got;
                }
                /* DRDY was high but nothing was actually queued for us - fall
                 * through to the timeout/poll logic below. */
            } else if (result != TXN_BUSY) {
                U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] READ transaction failed (result=%d)", (int)result);
                return -1;
            }
        }

        if (timeoutMs == 0) {
            return 0; // non-blocking: nothing available right now
        }
        if ((timeoutMs > 0) && ((U_CX_PORT_GET_TIME_MS() - startTime) >= timeoutMs)) {
            return 0;
        }
        U_CX_PORT_SLEEP_MS(U_CX_SPI_TRANSPORT_POLL_INTERVAL_MS);
    }
}

void uCxSpiTransportMasterFlushRx(uCxSpiTransportMaster_t *pMaster)
{
    if (!pMaster->isOpen) {
        return;
    }

    /* Drain repeatedly (non-blocking) until the module reports nothing left. */
    for (;;) {
        int32_t got = uCxSpiTransportMasterRead(pMaster, pMaster->payloadBuf,
                                                sizeof(pMaster->payloadBuf), 0);
        if (got <= 0) {
            break;
        }
    }
}
