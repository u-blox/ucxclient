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
 * @brief STM32H7 physical SPI + DRDY port implementation using HAL.
 *
 * Implements u_port_spi.h for the uCX2 SPI transport (host = SPI master,
 * SPI Mode 3, MSB first, 8-bit words, software chip-select). DRDY is a
 * plain polled GPIO input in this first implementation - no EXTI - see
 * the DRDY polling note below.
 *
 * @warning EXPERIMENTAL / NOT HARDWARE-VALIDATED YET. See u_port_spi_stm32h7.h
 * for the pin assignments, which are PLACEHOLDERS pending verification
 * against real hardware.
 */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "stm32h7xx_hal.h"
#include "u_port.h"
#include "u_cx_log.h"
#include "u_port_spi.h"
#include "u_port_spi_stm32h7.h"

/* ----------------------------------------------------------------
 * COMPILE-TIME MACROS
 * -------------------------------------------------------------- */

/* SPI1 kernel clock, per SystemClock_Config() in main_stm32.c:
 * HSE(8MHz, bypass) / PLLM(1) * PLLN(120) = 960 MHz VCO, PLL1Q = VCO/PLLQ(4) = 240 MHz.
 * RCC_SPI123CLKSOURCE_PLL selects PLL1Q as the SPI1/2/3 kernel clock (it is
 * also the hardware reset default, but selected explicitly below for
 * clarity/robustness). If the PLL configuration in main_stm32.c ever
 * changes, this value must be updated to match. */
#define SPI_KERNEL_CLOCK_HZ  (240000000UL)

/* Chunk size used when clocking dummy TX bytes / discarding RX bytes so
 * that no protocol-layer buffer size assumptions leak into this file, and
 * so no large stack/static buffer is needed just for filler bytes. */
#define SPI_CHUNK_SIZE  64

/* ----------------------------------------------------------------
 * TYPES
 * -------------------------------------------------------------- */

typedef struct {
    SPI_HandleTypeDef hspi;
    bool isOpen;
} uPortSpiHandleStruct;

/* ----------------------------------------------------------------
 * STATIC VARIABLES
 * -------------------------------------------------------------- */

static uPortSpiHandleStruct gSpiHandle;
static uint8_t gDummyTx[SPI_CHUNK_SIZE];
static uint8_t gScratchRx[SPI_CHUNK_SIZE];

/* ----------------------------------------------------------------
 * STATIC PROTOTYPES
 * -------------------------------------------------------------- */

static uint32_t pickBaudRatePrescaler(uint32_t desiredHz);

/* ----------------------------------------------------------------
 * STATIC FUNCTIONS
 * -------------------------------------------------------------- */

static uint32_t pickBaudRatePrescaler(uint32_t desiredHz)
{
    uint32_t div = 2;

    if (desiredHz == 0) {
        desiredHz = U_PORT_SPI_DEFAULT_CLOCK_HZ;
    }
    while ((div < 256) && ((SPI_KERNEL_CLOCK_HZ / div) > desiredHz)) {
        div *= 2;
    }

    switch (div) {
        case 2:   return SPI_BAUDRATEPRESCALER_2;
        case 4:   return SPI_BAUDRATEPRESCALER_4;
        case 8:   return SPI_BAUDRATEPRESCALER_8;
        case 16:  return SPI_BAUDRATEPRESCALER_16;
        case 32:  return SPI_BAUDRATEPRESCALER_32;
        case 64:  return SPI_BAUDRATEPRESCALER_64;
        case 128: return SPI_BAUDRATEPRESCALER_128;
        default:  return SPI_BAUDRATEPRESCALER_256;
    }
}

/* ----------------------------------------------------------------
 * PUBLIC FUNCTIONS
 * -------------------------------------------------------------- */

uPortSpiHandle_t uPortSpiOpen(const uPortSpiConfig_t *pConfig)
{
    GPIO_InitTypeDef gpioInit = {0};
    RCC_PeriphCLKInitTypeDef periphClkInit = {0};
    uint32_t clockHz = (pConfig != NULL) ? pConfig->clockHz : U_PORT_SPI_DEFAULT_CLOCK_HZ;

    if (gSpiHandle.isOpen) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] uPortSpiOpen: only one SPI instance supported");
        return NULL;
    }

    memset(&gDummyTx, 0xFF, sizeof(gDummyTx));

    /* Explicitly select PLL1Q as the SPI1/2/3 kernel clock source (also the
     * hardware reset default - see SPI_KERNEL_CLOCK_HZ comment above). */
    periphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI123;
    periphClkInit.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
    if (HAL_RCCEx_PeriphCLKConfig(&periphClkInit) != HAL_OK) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] Failed to configure SPI123 kernel clock");
        return NULL;
    }

    /* SCK/MISO/MOSI GPIOs (AF5) */
    U_PORT_SPI_GPIO_CLK_ENABLE();
    gpioInit.Pin = U_PORT_SPI_SCK_PIN | U_PORT_SPI_MISO_PIN | U_PORT_SPI_MOSI_PIN;
    gpioInit.Mode = GPIO_MODE_AF_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpioInit.Alternate = U_PORT_SPI_AF;
    HAL_GPIO_Init(U_PORT_SPI_GPIO_PORT, &gpioInit);

    /* CS: manual GPIO output, idle high (de-asserted) */
    U_PORT_SPI_CS_GPIO_CLK_ENABLE();
    HAL_GPIO_WritePin(U_PORT_SPI_CS_GPIO_PORT, U_PORT_SPI_CS_PIN, GPIO_PIN_SET);
    gpioInit.Pin = U_PORT_SPI_CS_PIN;
    gpioInit.Mode = GPIO_MODE_OUTPUT_PP;
    gpioInit.Pull = GPIO_NOPULL;
    gpioInit.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(U_PORT_SPI_CS_GPIO_PORT, &gpioInit);

    /* DRDY: input, polled (no EXTI in this first implementation) */
    U_PORT_SPI_DRDY_GPIO_CLK_ENABLE();
    gpioInit.Pin = U_PORT_SPI_DRDY_PIN;
    gpioInit.Mode = GPIO_MODE_INPUT;
    gpioInit.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(U_PORT_SPI_DRDY_GPIO_PORT, &gpioInit);

    /* SPI1 peripheral: Mode 3 (CPOL=1, CPHA=1), MSB first, 8-bit, software NSS */
    U_PORT_SPI_CLK_ENABLE();
    memset(&gSpiHandle.hspi, 0, sizeof(gSpiHandle.hspi));
    gSpiHandle.hspi.Instance = U_PORT_SPI_INSTANCE;
    gSpiHandle.hspi.Init.Mode = SPI_MODE_MASTER;
    gSpiHandle.hspi.Init.Direction = SPI_DIRECTION_2LINES;
    gSpiHandle.hspi.Init.DataSize = SPI_DATASIZE_8BIT;
    gSpiHandle.hspi.Init.CLKPolarity = SPI_POLARITY_HIGH;
    gSpiHandle.hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
    gSpiHandle.hspi.Init.NSS = SPI_NSS_SOFT;
    gSpiHandle.hspi.Init.BaudRatePrescaler = pickBaudRatePrescaler(clockHz);
    gSpiHandle.hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
    gSpiHandle.hspi.Init.TIMode = SPI_TIMODE_DISABLE;
    gSpiHandle.hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    gSpiHandle.hspi.Init.CRCPolynomial = 0;
    gSpiHandle.hspi.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
    gSpiHandle.hspi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    gSpiHandle.hspi.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    gSpiHandle.hspi.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    gSpiHandle.hspi.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    gSpiHandle.hspi.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    gSpiHandle.hspi.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    gSpiHandle.hspi.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    gSpiHandle.hspi.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    gSpiHandle.hspi.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_ENABLE;
    gSpiHandle.hspi.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    if (HAL_SPI_Init(&gSpiHandle.hspi) != HAL_OK) {
        U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] HAL_SPI_Init failed");
        U_PORT_SPI_CLK_DISABLE();
        return NULL;
    }

    gSpiHandle.isOpen = true;
    U_CX_LOG_LINE(U_CX_LOG_CH_DBG, "[SPI] Opened, kernel=%luHz prescaler_reg=0x%08lX",
                 (unsigned long)SPI_KERNEL_CLOCK_HZ, (unsigned long)gSpiHandle.hspi.Init.BaudRatePrescaler);
    return (uPortSpiHandle_t)&gSpiHandle;
}

void uPortSpiClose(uPortSpiHandle_t handle)
{
    uPortSpiHandleStruct *pH = (uPortSpiHandleStruct *)handle;

    if ((pH == NULL) || !pH->isOpen) {
        return;
    }

    HAL_SPI_DeInit(&pH->hspi);
    U_PORT_SPI_CLK_DISABLE();
    pH->isOpen = false;
}

void uPortSpiCsAssert(uPortSpiHandle_t handle)
{
    (void)handle;
    HAL_GPIO_WritePin(U_PORT_SPI_CS_GPIO_PORT, U_PORT_SPI_CS_PIN, GPIO_PIN_RESET);
}

void uPortSpiCsDeassert(uPortSpiHandle_t handle)
{
    (void)handle;
    HAL_GPIO_WritePin(U_PORT_SPI_CS_GPIO_PORT, U_PORT_SPI_CS_PIN, GPIO_PIN_SET);
}

int32_t uPortSpiTransfer(uPortSpiHandle_t handle, const uint8_t *pTx, uint8_t *pRx, size_t length)
{
    uPortSpiHandleStruct *pH = (uPortSpiHandleStruct *)handle;
    size_t offset = 0;

    if ((pH == NULL) || !pH->isOpen) {
        return -1;
    }

    while (offset < length) {
        size_t chunk = length - offset;
        if (chunk > SPI_CHUNK_SIZE) {
            chunk = SPI_CHUNK_SIZE;
        }

        uint8_t *pTxChunk = (pTx != NULL) ? (uint8_t *)&pTx[offset] : gDummyTx;
        uint8_t *pRxChunk = (pRx != NULL) ? &pRx[offset] : gScratchRx;

        if (HAL_SPI_TransmitReceive(&pH->hspi, pTxChunk, pRxChunk, (uint16_t)chunk, HAL_MAX_DELAY) != HAL_OK) {
            U_CX_LOG_LINE(U_CX_LOG_CH_ERROR, "[SPI] HAL_SPI_TransmitReceive failed");
            return -1;
        }
        offset += chunk;
    }

    return 0;
}

bool uPortSpiDrdyGetLevel(uPortSpiHandle_t handle)
{
    (void)handle;
    return HAL_GPIO_ReadPin(U_PORT_SPI_DRDY_GPIO_PORT, U_PORT_SPI_DRDY_PIN) == GPIO_PIN_SET;
}

bool uPortSpiDrdyWaitLevel(uPortSpiHandle_t handle, bool level, int32_t timeoutMs)
{
    int32_t startTime = U_CX_PORT_GET_TIME_MS();

    for (;;) {
        if (uPortSpiDrdyGetLevel(handle) == level) {
            return true;
        }
        if ((U_CX_PORT_GET_TIME_MS() - startTime) >= timeoutMs) {
            return uPortSpiDrdyGetLevel(handle) == level; // one last check
        }
    }
}
