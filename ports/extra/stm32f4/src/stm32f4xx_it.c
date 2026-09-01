/**
  ******************************************************************************
  * @file    FreeRTOS/FreeRTOS_Semaphore/Src/stm32f4xx_it.c 
  * @author  MCD Application Team
  * @brief   Main Interrupt Service Routines.
  *          This file provides template for all exceptions handler and 
  *          peripherals interrupt service routine.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
#include "cmsis_os.h"
#include <stdio.h>

/* Forward declarations */
extern void uPortUart_IRQHandler(void);
extern void uPortUartDma_IRQHandler(void);
extern void exampleConsoleUart_IRQHandler(void);

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/******************************************************************************/
/*            Cortex-M4 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  * @param  None
  * @retval None
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  * @param  None
  * @retval None
  */
void HardFault_Handler(void)
{
  printf("\r\nFATAL: HardFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  /* Go to infinite loop when Hard Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  * @param  None
  * @retval None
  */
void MemManage_Handler(void)
{
  printf("\r\nFATAL: MemManage exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  /* Go to infinite loop when Memory Manage exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  * @param  None
  * @retval None
  */
void BusFault_Handler(void)
{
  printf("\r\nFATAL: BusFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  /* Go to infinite loop when Bus Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  * @param  None
  * @retval None
  */
void UsageFault_Handler(void)
{
  printf("\r\nFATAL: UsageFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  /* Go to infinite loop when Usage Fault exception occurs */
  while (1)
  {
  }
}

/**
  * @brief  This function handles Debug Monitor exception.
  * @param  None
  * @retval None
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  * @param  None
  * @retval None
  */
void SysTick_Handler(void)
{
  /* osSystickHandler() only feeds FreeRTOS's tick once the scheduler is
   * running, so HAL_IncTick() must be called unconditionally here or
   * HAL_GetTick()/HAL-internal timeouts stay frozen at 0 for all of main()
   * before vTaskStartScheduler() - turning bounded timeouts into infinite
   * hangs (e.g. SystemClock_Config()'s HSE/PLL ready waits). */
  HAL_IncTick();
  osSystickHandler();
}

/******************************************************************************/
/*                 STM32F4xx Peripherals Interrupt Handlers                   */
/*  Add here the Interrupt Handler for the used peripheral(s) (PPP), for the  */
/*  available peripheral interrupt handler's name please refer to the startup */
/*  file (startup_stm32f4xx.s).                                               */
/******************************************************************************/

/**
  * @brief NORA-W36 UART interrupt handler
  * - ODIN-W26/F439/NUCLEO-F439ZI: USART1 on PA9/PA10 (all define STM32F439xx)
  * - F407/F429: USART3 on PB10/PB11
  */
#if defined(ODIN_W26) || defined(STM32F439xx)

/**
  * @brief This function handles USART1 global interrupt (u-blox module).
  */
void USART1_IRQHandler(void)
{
  uPortUart_IRQHandler();
}

/**
  * @brief This function handles DMA2 Stream 2 global interrupt (USART1 RX DMA).
  */
void DMA2_Stream2_IRQHandler(void)
{
  uPortUartDma_IRQHandler();
}

#else /* STM32F407G-DISC1 (default) */

/**
  * @brief This function handles USART3 global interrupt.
  */
void USART3_IRQHandler(void)
{
  uPortUart_IRQHandler();
}

/**
  * @brief This function handles DMA1 Stream 1 global interrupt (USART3 RX DMA).
  */
void DMA1_Stream1_IRQHandler(void)
{
  uPortUartDma_IRQHandler();
}

#endif

/**
  * @brief Console UART interrupt handler (ST-LINK VCP).
  * - NUCLEO-F439ZI: USART3 (console) - module is on USART1, so USART3 is
  *   free to use for the console here.
  * - STM32F407G-DISC1: USART2 (console) - module is on USART3, so USART2 is
  *   free to use for the console here.
  *
  * Feeds the interrupt-driven RX ring buffer in main_stm32.c so
  * exampleConsoleUartRead() never overruns during sustained/binary transfers
  * (e.g. XMODEM via uart_bridge_example).
  */
#if defined(NUCLEO_F439ZI)
void USART3_IRQHandler(void)
{
  exampleConsoleUart_IRQHandler();
}
#else
void USART2_IRQHandler(void)
{
  exampleConsoleUart_IRQHandler();
}
#endif /* NUCLEO_F439ZI */
