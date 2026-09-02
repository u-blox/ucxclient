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
 * @brief uCX2 SPI transport protocol - host-side codec implementation.
 */

#include <string.h>

#include "u_cx_spi_transport_protocol.h"

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static uint16_t readUint16Be(const uint8_t *pBuffer);
static void writeUint16Be(uint8_t *pBuffer, uint16_t value);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static uint16_t readUint16Be(const uint8_t *pBuffer)
{
    return (uint16_t)(((uint16_t)pBuffer[0] << 8) | pBuffer[1]);
}

static void writeUint16Be(uint8_t *pBuffer, uint16_t value)
{
    pBuffer[0] = (uint8_t)(value >> 8);
    pBuffer[1] = (uint8_t)(value & 0xFFU);
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

void uCxSpiTransportEncodeRequest(uCxSpiTransportOpcode_t opcode,
                                  uint16_t txLen, uint16_t rxLen,
                                  uint8_t *pBuffer)
{
    pBuffer[0] = U_CX_SPI_TRANSPORT_REQUEST_PREAMBLE;
    pBuffer[1] = (uint8_t)((U_CX_SPI_TRANSPORT_PROTOCOL_VERSION << 4) |
                          ((uint8_t)opcode & U_CX_SPI_TRANSPORT_STATUS_MASK));
    writeUint16Be(&pBuffer[2], txLen);
    writeUint16Be(&pBuffer[4], rxLen);
    pBuffer[6] = 0; // flags/reserved - must be 0x0000
    pBuffer[7] = 0;
}

bool uCxSpiTransportDecodeResponse(const uint8_t *pBuffer, size_t length,
                                   uCxSpiTransportResponse_t *pResponse)
{
    uCxSpiTransportStatus_t status;
    uint8_t stickyFlags;

    if ((pBuffer == NULL) || (pResponse == NULL) ||
        (length < U_CX_SPI_TRANSPORT_HEADER_SIZE) ||
        (pBuffer[0] != U_CX_SPI_TRANSPORT_RESPONSE_PREAMBLE)) {
        return false;
    }

    status = (uCxSpiTransportStatus_t)(pBuffer[1] & U_CX_SPI_TRANSPORT_STATUS_MASK);
    stickyFlags = (uint8_t)(pBuffer[1] & U_CX_SPI_TRANSPORT_STICKY_MASK);

    if ((pBuffer[2] != 0) || (pBuffer[3] != 0) ||
        (status > U_CX_SPI_TRANSPORT_STATUS_NOT_ACTIVE) ||
        ((stickyFlags & (uint8_t)~U_CX_SPI_TRANSPORT_OVERRUN_STICKY) != 0)) {
        return false;
    }

    pResponse->status = status;
    pResponse->stickyFlags = stickyFlags;
    pResponse->txAvailable = readUint16Be(&pBuffer[4]);
    pResponse->rxCredit = readUint16Be(&pBuffer[6]);

    return true;
}
