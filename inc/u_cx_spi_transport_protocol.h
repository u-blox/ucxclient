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
 * @brief uCX2 SPI transport protocol - wire format constants and codec
 *
 * Implements the HOST (SPI master) side of the framed, flow-controlled,
 * DRDY-synchronized transport described in the uCX2 SPI Transport Protocol
 * specification (v1.0). The module (NORA-W36/NORA-W56) is always the SPI
 * slave; this codec only ever needs to ENCODE requests and DECODE responses.
 *
 * This file is purely a byte-buffer <-> struct codec: it has no dependency
 * on any SPI peripheral, GPIO, or OS primitive, so it is safe to compile
 * on every platform/port and is unit-testable in isolation.
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET. Neither the NORA-W36
 * nor the NORA-W56 SPI-slave firmware implementation has been verified
 * against this host-side codec on real hardware at the time this file was
 * written. Treat all field sizes/behaviour as "per spec", not "per test".
 */

#ifndef U_CX_SPI_TRANSPORT_PROTOCOL_H
#define U_CX_SPI_TRANSPORT_PROTOCOL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

#define U_CX_SPI_TRANSPORT_PROTOCOL_VERSION   1
#define U_CX_SPI_TRANSPORT_REQUEST_PREAMBLE   0xA5
#define U_CX_SPI_TRANSPORT_RESPONSE_PREAMBLE  0x5A
#define U_CX_SPI_TRANSPORT_HEADER_SIZE        8

#define U_CX_SPI_TRANSPORT_STATUS_MASK        0x0F
#define U_CX_SPI_TRANSPORT_STICKY_MASK        0xF0
#define U_CX_SPI_TRANSPORT_OVERRUN_STICKY      0x10

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** Request opcodes. Only STATUS/WRITE/READ/FACTORY_RESTORE are used by
 * the host; EXCHANGE and ACTIVATE are reserved/unsupported per spec.
 */
typedef enum {
    U_CX_SPI_TRANSPORT_OPCODE_STATUS = 0x0,
    U_CX_SPI_TRANSPORT_OPCODE_WRITE = 0x1,
    U_CX_SPI_TRANSPORT_OPCODE_READ = 0x2,
    U_CX_SPI_TRANSPORT_OPCODE_FACTORY_RESTORE = 0xE
} uCxSpiTransportOpcode_t;

/** Response status codes (low nibble of response header byte 1). */
typedef enum {
    U_CX_SPI_TRANSPORT_STATUS_OK = 0x00,
    U_CX_SPI_TRANSPORT_STATUS_BUSY = 0x01,
    U_CX_SPI_TRANSPORT_STATUS_INVALID_REQUEST = 0x02,
    U_CX_SPI_TRANSPORT_STATUS_NOT_ACTIVE = 0x03
} uCxSpiTransportStatus_t;

/** Decoded 8-byte response header. */
typedef struct {
    uCxSpiTransportStatus_t status;
    uint8_t stickyFlags;   ///< High nibble, e.g. U_CX_SPI_TRANSPORT_OVERRUN_STICKY
    uint16_t txAvailable;  ///< STATUS: queued TX bytes. READ: exact payload length K.
    uint16_t rxCredit;     ///< Slave's currently-guaranteed write-accept capacity.
} uCxSpiTransportResponse_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Encode an 8-byte request header (host -> module, MOSI).
 *
 * @param[in]  opcode   Request opcode.
 * @param      txLen    Payload length the host intends to WRITE (0 otherwise).
 * @param      rxLen    Payload length the host wants to READ (0 otherwise).
 * @param[out] pBuffer  Buffer of at least U_CX_SPI_TRANSPORT_HEADER_SIZE bytes.
 */
void uCxSpiTransportEncodeRequest(uCxSpiTransportOpcode_t opcode,
                                  uint16_t txLen, uint16_t rxLen,
                                  uint8_t *pBuffer);

/**
 * @brief Decode an 8-byte response header (module -> host, MISO).
 *
 * Validates the response preamble, reserved-zero bytes, status range and
 * sticky-flag mask before accepting the response.
 *
 * @param[in]  pBuffer    Response bytes received on MISO.
 * @param      length     Number of bytes available in pBuffer.
 * @param[out] pResponse  Decoded response, only valid if this function returns true.
 * @return                true if the response is well-formed, false otherwise.
 */
bool uCxSpiTransportDecodeResponse(const uint8_t *pBuffer, size_t length,
                                   uCxSpiTransportResponse_t *pResponse);

#ifdef __cplusplus
}
#endif

#endif // U_CX_SPI_TRANSPORT_PROTOCOL_H
