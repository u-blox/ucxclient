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
 * @brief Physical SPI + DRDY port abstraction layer.
 *
 * This header defines the low-level, protocol-agnostic interface that each
 * platform must implement in order to support the uCX2 SPI transport
 * (see u_cx_spi_transport_master.h for the protocol state machine that
 * sits on top of this).
 *
 * The uCX2 SPI transport is a split-phase protocol: every logical
 * transaction is one or two independently CS-bounded phases, and the
 * second phase for a READ needs to be split into "read the 8-byte
 * response header" then "read K more payload bytes" WITHOUT releasing
 * CS in between (K is only known after decoding the header). For this
 * reason CS control is exposed explicitly (uPortSpiCsAssert/Deassert)
 * rather than being folded into a single "transceive" call.
 *
 * DRDY is an active-high, module-driven handshake line (see protocol
 * spec section 4 for the full state machine). The master must be able
 * to both sample its current level and block-wait for a level change
 * with a timeout; see u_cx_spi_transport_master.c for how the "stale
 * high" / edge-arming logic built on top of these primitives works.
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET.
 */

#ifndef U_PORT_SPI_H
#define U_PORT_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

/** SPI handle - platform-specific implementation */
typedef void *uPortSpiHandle_t;

/** Configuration passed to uPortSpiOpen(). */
typedef struct {
    uint32_t clockHz; ///< Requested SCLK frequency; driver clamps to nearest supported divider (<= 20 MHz per spec).
} uPortSpiConfig_t;

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

/**
 * @brief Open/configure the SPI master peripheral and DRDY GPIO input.
 *
 * Configures SPI Mode 3 (CPOL=1, CPHA=1), MSB-first, 8-bit words, and the
 * CS/DRDY GPIOs, per the uCX2 SPI transport protocol spec.
 *
 * @param[in]  pConfig  Requested configuration (may be NULL for platform defaults).
 * @return              SPI handle on success, NULL on failure.
 */
uPortSpiHandle_t uPortSpiOpen(const uPortSpiConfig_t *pConfig);

/**
 * @brief Close the SPI peripheral previously opened with uPortSpiOpen().
 *
 * @param[in]  handle  SPI handle from uPortSpiOpen().
 */
void uPortSpiClose(uPortSpiHandle_t handle);

/**
 * @brief Assert (drive low) the chip-select line.
 *
 * @param[in]  handle  SPI handle from uPortSpiOpen().
 */
void uPortSpiCsAssert(uPortSpiHandle_t handle);

/**
 * @brief De-assert (drive high) the chip-select line.
 *
 * @param[in]  handle  SPI handle from uPortSpiOpen().
 */
void uPortSpiCsDeassert(uPortSpiHandle_t handle);

/**
 * @brief Full-duplex transfer of exactly `length` bytes.
 *
 * CS is NOT touched by this function - the caller must have already
 * called uPortSpiCsAssert() and will call uPortSpiCsDeassert() once all
 * phases of the current logical transaction are complete. This lets a
 * READ response header + payload be clocked as two back-to-back calls
 * to this function under a single continuous CS assertion, per spec.
 *
 * @param[in]  handle  SPI handle from uPortSpiOpen().
 * @param[in]  pTx     Bytes to transmit on MOSI, or NULL to clock dummy (0xFF) bytes.
 * @param[out] pRx     Buffer to store bytes received on MISO, or NULL to discard them.
 * @param      length  Number of bytes to transfer.
 * @return             0 on success, negative on error.
 */
int32_t uPortSpiTransfer(uPortSpiHandle_t handle, const uint8_t *pTx, uint8_t *pRx, size_t length);

/**
 * @brief Sample the current level of the DRDY input line.
 *
 * @param[in]  handle  SPI handle from uPortSpiOpen().
 * @return             true if DRDY is currently high.
 */
bool uPortSpiDrdyGetLevel(uPortSpiHandle_t handle);

/**
 * @brief Block (polling) until DRDY reaches the requested level, or timeout.
 *
 * @param[in]  handle     SPI handle from uPortSpiOpen().
 * @param      level      Level to wait for (true = high, false = low).
 * @param      timeoutMs  Timeout in milliseconds.
 * @return                true if the level was observed within the timeout, false otherwise.
 */
bool uPortSpiDrdyWaitLevel(uPortSpiHandle_t handle, bool level, int32_t timeoutMs);

#ifdef __cplusplus
}
#endif

#endif // U_PORT_SPI_H
