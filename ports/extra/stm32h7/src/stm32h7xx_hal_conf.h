/**
  ******************************************************************************
  * @file    stm32h7xx_hal_conf.h
  * @brief   HAL configuration file for STM32H753 (NUCLEO-H753ZI)
  *
  * Extends the H743 base configuration with hardware crypto peripherals:
  *   - CRYP (AES-128/192/256 in ECB/CBC/CTR/GCM/CCM modes)
  *   - HASH (SHA-1, SHA-224, SHA-256, MD5, HMAC)
  *   - RNG  (True Random Number Generator)
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025-2026 u-blox
  * SPDX-License-Identifier: Apache-2.0
  *
  ******************************************************************************
  */

#ifndef __STM32H7xx_HAL_CONF_H
#define __STM32H7xx_HAL_CONF_H

#ifdef __cplusplus
extern "C" {
#endif

/* ########################## Module Selection ############################## */
#define HAL_MODULE_ENABLED
#define HAL_CORTEX_MODULE_ENABLED
#define HAL_DMA_MODULE_ENABLED
#define HAL_FLASH_MODULE_ENABLED
#define HAL_GPIO_MODULE_ENABLED
#define HAL_PWR_MODULE_ENABLED
#define HAL_RCC_MODULE_ENABLED
#define HAL_TIM_MODULE_ENABLED
#define HAL_UART_MODULE_ENABLED

/* STM32H753 hardware crypto accelerator modules */
#define HAL_CRYP_MODULE_ENABLED
#define HAL_HASH_MODULE_ENABLED
#define HAL_RNG_MODULE_ENABLED
#define HAL_IWDG_MODULE_ENABLED      /* Independent watchdog */

/* SPI HAL module: needed for SD card via SPI (UCM_ENABLE_SD_CARD feature flag)
 * and/or the uCX2 SPI transport (UCX_SPI_TRANSPORT, set by -DSTM32_TRANSPORT=spi) */
#if defined(UCM_ENABLE_SD_CARD) || defined(UCX_SPI_TRANSPORT)
#define HAL_SPI_MODULE_ENABLED
#endif

/* ########################## Oscillator Values adaptation ################### */
#if !defined(HSE_VALUE)
#define HSE_VALUE    ((uint32_t)8000000U) /*!< 8MHz HSE from ST-Link MCO on Nucleo board */
#endif

#if !defined(HSE_STARTUP_TIMEOUT)
#define HSE_STARTUP_TIMEOUT    ((uint32_t)100U)
#endif

#if !defined(CSI_VALUE)
#define CSI_VALUE    ((uint32_t)4000000U)
#endif

#if !defined(HSI_VALUE)
#define HSI_VALUE    ((uint32_t)64000000U)
#endif

#if !defined(LSI_VALUE)
#define LSI_VALUE  ((uint32_t)32000U)
#endif

#if !defined(LSE_VALUE)
#define LSE_VALUE    ((uint32_t)32768U)
#endif

#if !defined(LSE_STARTUP_TIMEOUT)
#define LSE_STARTUP_TIMEOUT    ((uint32_t)5000U)
#endif

#if !defined(EXTERNAL_CLOCK_VALUE)
#define EXTERNAL_CLOCK_VALUE    12288000U
#endif

/* ########################### System Configuration ######################### */
#define  VDD_VALUE                    ((uint32_t)3300U)
#define  TICK_INT_PRIORITY            ((uint32_t)0U)
#define  USE_RTOS                     0U

/* ########################## Assert Selection ############################## */
/* #define USE_FULL_ASSERT    1U */

/* ################## SPI peripheral configuration ########################## */
#define USE_SPI_CRC                     0U

/* Includes ------------------------------------------------------------------*/

#ifdef HAL_RCC_MODULE_ENABLED
  #include "stm32h7xx_hal_rcc.h"
#endif

#ifdef HAL_GPIO_MODULE_ENABLED
  #include "stm32h7xx_hal_gpio.h"
#endif

#ifdef HAL_DMA_MODULE_ENABLED
  #include "stm32h7xx_hal_dma.h"
#endif

#ifdef HAL_CORTEX_MODULE_ENABLED
  #include "stm32h7xx_hal_cortex.h"
#endif

#ifdef HAL_FLASH_MODULE_ENABLED
  #include "stm32h7xx_hal_flash.h"
#endif

#ifdef HAL_PWR_MODULE_ENABLED
  #include "stm32h7xx_hal_pwr.h"
#endif

#ifdef HAL_TIM_MODULE_ENABLED
  #include "stm32h7xx_hal_tim.h"
#endif

#ifdef HAL_UART_MODULE_ENABLED
  #include "stm32h7xx_hal_uart.h"
#endif

#ifdef HAL_CRYP_MODULE_ENABLED
  #include "stm32h7xx_hal_cryp.h"
#endif

#ifdef HAL_HASH_MODULE_ENABLED
  #include "stm32h7xx_hal_hash.h"
#endif

#ifdef HAL_RNG_MODULE_ENABLED
  #include "stm32h7xx_hal_rng.h"
#endif

#ifdef HAL_IWDG_MODULE_ENABLED
  #include "stm32h7xx_hal_iwdg.h"
#endif

#ifdef HAL_SPI_MODULE_ENABLED
  #include "stm32h7xx_hal_spi.h"
#endif

/* Exported macro ------------------------------------------------------------*/
#ifdef  USE_FULL_ASSERT
  #define assert_param(expr) ((expr) ? (void)0U : assert_failed((uint8_t *)__FILE__, __LINE__))
  void assert_failed(uint8_t* file, uint32_t line);
#else
  #define assert_param(expr) ((void)0U)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STM32H7xx_HAL_CONF_H */
