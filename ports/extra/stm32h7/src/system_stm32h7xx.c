/**
  ******************************************************************************
  * @file    system_stm32h7xx.c
  * @brief   CMSIS Cortex-M7 Device Peripheral Access Layer System Source File
  *          for STM32H743 Matter application
  *
  *   This file provides two functions and one global variable:
  *    - SystemInit(): Initializes the system (FPU, vector table, etc.)
  *    - SystemCoreClockUpdate(): Updates SystemCoreClock variable
  *    - SystemCoreClock: Contains the core clock (HCLK) frequency
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 u-blox
  * SPDX-License-Identifier: Apache-2.0
  *
  ******************************************************************************
  */

#include "stm32h7xx.h"

#if !defined  (HSE_VALUE)
#define HSE_VALUE    ((uint32_t)8000000U) /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (CSI_VALUE)
#define CSI_VALUE    ((uint32_t)4000000U) /*!< Value of the Internal oscillator in Hz*/
#endif /* CSI_VALUE */

#if !defined  (HSI_VALUE)
#define HSI_VALUE    ((uint32_t)64000000U) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

/* Note: Following vector table addresses must be defined in line with linker
         configuration. */
/*!< Uncomment the following line if you need to relocate the vector table
     anywhere in Flash or Sram, else the vector table is kept at the automatic
     remap of boot address selected */
/* #define USER_VECT_TAB_ADDRESS */

#if defined(USER_VECT_TAB_ADDRESS)
/*!< Uncomment the following line if you need to relocate your vector Table
     in Sram else user rance data memory */
/* #define VECT_TAB_SRAM */
#if defined(VECT_TAB_SRAM)
#define VECT_TAB_BASE_ADDRESS   SRAM1_BASE      /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#else
#define VECT_TAB_BASE_ADDRESS   FLASH_BANK1_BASE /*!< Vector Table base address field.
                                                     This value must be a multiple of 0x200. */
#define VECT_TAB_OFFSET         0x00000000U     /*!< Vector Table base offset field.
                                                     This value must be a multiple of 0x200. */
#endif /* VECT_TAB_SRAM */
#endif /* USER_VECT_TAB_ADDRESS */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use initialized data in D2 domain SRAM (AHB SRAM)  */
/* #define DATA_IN_D2_SRAM */

/******************************************************************************/

/**
  * @brief  System Clock Configuration to 480MHz
  *         The system Clock is configured as follows:
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 480000000
  *            HCLK(Hz)                       = 240000000
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2
  *            D2 APB1 Prescaler              = 2
  *            D2 APB2 Prescaler              = 2
  *            D3 APB4 Prescaler              = 2
  *            HSE Frequency(Hz)              = 8000000
  *            PLL_M                          = 1
  *            PLL_N                          = 120
  *            PLL_P                          = 2
  *            PLL_Q                          = 4
  *            PLL_R                          = 2
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  */

  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
         Note: If you use this function to configure the system clock; then there
               is no need to call the 2 first functions listed above, since SystemCoreClock
               variable is updated automatically.
  */
uint32_t SystemCoreClock = 64000000U; /* Will be updated after clock config */
uint32_t SystemD2Clock = 64000000U;
const  uint8_t D1CorePrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting and vector table location
  *         configuration.
  * @param  None
  * @retval None
  */
void SystemInit(void)
{
#if defined (DATA_IN_D2_SRAM)
  __IO uint32_t tmpreg;
#endif /* DATA_IN_D2_SRAM */

  /* FPU settings ------------------------------------------------------------*/
  #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10*2))|(3UL << (11*2)));  /* set CP10 and CP11 Full Access */
  #endif

  /* Reset the RCC clock configuration to the default reset state ------------*/
  /* Set HSION bit */
  RCC->CR |= RCC_CR_HSION;

  /* Reset CFGR register */
  RCC->CFGR = 0x00000000;

  /* Reset HSEON, CSSON , CSION,RC48ON, CSIKERON PLL1ON, PLL2ON and PLL3ON bits */
  RCC->CR &= 0xEAF6ED7FU;

  /* Reset D1CFGR register */
  RCC->D1CFGR = 0x00000000;

  /* Reset D2CFGR register */
  RCC->D2CFGR = 0x00000000;

  /* Reset D3CFGR register */
  RCC->D3CFGR = 0x00000000;

  /* Reset PLLCKSELR register */
  RCC->PLLCKSELR = 0x00000000;

  /* Reset PLLCFGR register */
  RCC->PLLCFGR = 0x00000000;
  /* Reset PLL1DIVR register */
  RCC->PLL1DIVR = 0x00000000;
  /* Reset PLL1FRACR register */
  RCC->PLL1FRACR = 0x00000000;

  /* Reset PLL2DIVR register */
  RCC->PLL2DIVR = 0x00000000;

  /* Reset PLL2FRACR register */
  RCC->PLL2FRACR = 0x00000000;

  /* Reset PLL3DIVR register */
  RCC->PLL3DIVR = 0x00000000;

  /* Reset PLL3FRACR register */
  RCC->PLL3FRACR = 0x00000000;

  /* Reset HSEBYP bit */
  RCC->CR &= 0xFFFBFFFFU;

  /* Disable all interrupts */
  RCC->CIER = 0x00000000;

#if defined (DATA_IN_D2_SRAM)
  /* in case of initialized data in D2 SRAM (AHB SRAM), enable the D2 SRAM clock (AHB SRAM clock) */
  RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN | RCC_AHB2ENR_D2SRAM3EN);
  tmpreg = RCC->AHB2ENR;
  (void) tmpreg;
#endif /* DATA_IN_D2_SRAM */

  /* Change the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) */
  if((DBGMCU->IDCODE & 0xFFFF0000U) < 0x20000000U)
  {
    /* if stm32h743 silicon revision Y, apply workaround */
    /* On rev Y, AXI SRAM is used, so apply workaround for read corruption */
    *((__IO uint32_t*)0x51008108) = 0x00000001U;
  }

  /*
   * Disable the FMC bank1 (enabled after reset).
   * This, prevents CPU speculation access on this bank which blocks the use of FMC during
   * 24us. During this time the others FMC master (such as LTDC) cannot use it!
   */
  FMC_Bank1_R->BTCR[0] = 0x000030D2;

#if defined (USER_VECT_TAB_ADDRESS)
  /* Configure the Vector Table location add offset address ------------------*/
  SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal D1 AXI-RAM or FLASH */
#endif /* USER_VECT_TAB_ADDRESS */

}

/**
   * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock (HCLK), it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock (HCLK) changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the CSI_VALUE(*)
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the HSE_VALUE(***)
  *             or HSI_VALUE(**) or CSI_VALUE(*) multiplied/divided by the PLL factors.
  *
  *         (*) CSI_VALUE is a constant defined in stm32h7xx_hal_conf.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *         (**) HSI_VALUE is a constant defined in stm32h7xx_hal_conf.h file (default value
  *             64 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *         (***) HSE_VALUE is a constant defined in stm32h7xx_hal_conf.h file (default value
  *              8 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  *
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate(void)
{
  uint32_t pllp, pllsource, pllm, pllfracen, hsivalue, tmp;
  uint32_t common_system_clock;
  float_t fracn1, pllvco;

  /* Get SYSCLK source -------------------------------------------------------*/
  switch (RCC->CFGR & RCC_CFGR_SWS)
  {
  case RCC_CFGR_SWS_HSI:  /* HSI used as system clock source */
    common_system_clock = (uint32_t) (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3));
    break;

  case RCC_CFGR_SWS_CSI:  /* CSI used as system clock  source */
    common_system_clock = CSI_VALUE;
    break;

  case RCC_CFGR_SWS_HSE:  /* HSE used as system clock  source */
    common_system_clock = HSE_VALUE;
    break;

  case RCC_CFGR_SWS_PLL1:  /* PLL1 used as system clock  source */
    /* PLL_VCO = (HSE_VALUE or HSI_VALUE or CSI_VALUE/ PLLM) * PLLN
    SYSCLK = PLL_VCO / PLLP
    */
    pllsource = (RCC->PLLCKSELR & RCC_PLLCKSELR_PLLSRC);
    pllm = ((RCC->PLLCKSELR & RCC_PLLCKSELR_DIVM1)>> 4)  ;
    pllfracen = ((RCC->PLLCFGR & RCC_PLLCFGR_PLL1FRACEN)>>RCC_PLLCFGR_PLL1FRACEN_Pos);
    fracn1 = (float_t)(uint32_t)(pllfracen* ((RCC->PLL1FRACR & RCC_PLL1FRACR_FRACN1)>> 3));

    if (pllm != 0U)
    {
      switch (pllsource)
      {
        case RCC_PLLCKSELR_PLLSRC_HSI:  /* HSI used as PLL clock source */

        hsivalue = (HSI_VALUE >> ((RCC->CR & RCC_CR_HSIDIV)>> 3)) ;
        pllvco = ( (float_t)hsivalue / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );

        break;

        case RCC_PLLCKSELR_PLLSRC_CSI:  /* CSI used as PLL clock source */
          pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
        break;

        case RCC_PLLCKSELR_PLLSRC_HSE:  /* HSE used as PLL clock source */
          pllvco = ((float_t)HSE_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
        break;

      default:
          pllvco = ((float_t)CSI_VALUE / (float_t)pllm) * ((float_t)(uint32_t)(RCC->PLL1DIVR & RCC_PLL1DIVR_N1) + (fracn1/(float_t)0x2000) +(float_t)1 );
        break;
      }
      pllp = (((RCC->PLL1DIVR & RCC_PLL1DIVR_P1) >>9) + 1U ) ;
      common_system_clock =  (uint32_t)(float_t)(pllvco/(float_t)pllp);
    }
    else
    {
      common_system_clock = 0U;
    }
    break;

  default:
    common_system_clock = CSI_VALUE;
    break;
  }

  /* Compute SystemClock frequency --------------------------------------------------*/
  tmp = D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_D1CPRE)>> RCC_D1CFGR_D1CPRE_Pos];

  /* common_system_clock frequency : CM7 CPU frequency  */
  common_system_clock >>= tmp;

  /* SystemD2Clock frequency : CM4 CPU, AXI and AHBs Clock frequency  */
  SystemD2Clock = (common_system_clock >> ((D1CorePrescTable[(RCC->D1CFGR & RCC_D1CFGR_HPRE)>> RCC_D1CFGR_HPRE_Pos]) & 0x1FU));

  SystemCoreClock = common_system_clock;
}

/**
  * @brief  Exit Run0 mode - Configure power supply for STM32H753
  *         Called from startup code before SystemInit.
  *         NUCLEO-H753ZI uses LDO power supply.
  * @param  None
  * @retval None
  */
void ExitRun0Mode(void)
{
#if defined(USE_PWR_LDO_SUPPLY)
  #if defined(SMPS)
    /* Exit Run* mode by disabling SMPS and enabling LDO */
    PWR->CR3 = (PWR->CR3 & ~PWR_CR3_SMPSEN) | PWR_CR3_LDOEN;
  #else
    /* Enable LDO mode */
    PWR->CR3 |= PWR_CR3_LDOEN;
  #endif /* SMPS */
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_EXTERNAL_SOURCE_SUPPLY)
  #if defined(SMPS)
    /* Exit Run* mode */
    PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_SMPSEN | PWR_CR3_LDOEN)) | PWR_CR3_BYPASS;
  #else
    PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_LDOEN)) | PWR_CR3_BYPASS;
  #endif /* SMPS */
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_DIRECT_SMPS_SUPPLY) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 &= ~(PWR_CR3_LDOEN);
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#else
  /* Default: LDO supply (NUCLEO-H753ZI) */
  PWR->CR3 |= PWR_CR3_LDOEN;
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#endif /* USE_PWR_LDO_SUPPLY */
}
