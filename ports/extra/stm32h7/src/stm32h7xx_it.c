/**
  ******************************************************************************
  * @file    stm32h7xx_it.c
  * @brief   Main Interrupt Service Routines for the ucxclient STM32H7 example.
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32h7xx_it.h"
#include "cmsis_os.h"
#include <stdio.h>

/* Forward declarations */
extern void uPortUart_IRQHandler(void);
extern void uPortUartDma_IRQHandler(void);
extern void exampleConsoleUart_IRQHandler(void);
extern TIM_HandleTypeDef htim6;

/******************************************************************************/
/*            Cortex-M7 Processor Exceptions Handlers                         */
/******************************************************************************/

/**
  * @brief  This function handles NMI exception.
  */
void NMI_Handler(void)
{
}

/**
  * @brief  This function handles Hard Fault exception.
  */
void HardFault_Handler(void)
{
  printf("\r\nFATAL: HardFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  while (1)
  {
  }
}

/**
  * @brief  This function handles Memory Manage exception.
  */
void MemManage_Handler(void)
{
  printf("\r\nFATAL: MemManage exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  while (1)
  {
  }
}

/**
  * @brief  This function handles Bus Fault exception.
  */
void BusFault_Handler(void)
{
  printf("\r\nFATAL: BusFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  while (1)
  {
  }
}

/**
  * @brief  This function handles Usage Fault exception.
  */
void UsageFault_Handler(void)
{
  printf("\r\nFATAL: UsageFault exception occurred\r\n");
  printf("FATAL: System halted\r\n");
  while (1)
  {
  }
}

/**
  * @brief  This function handles Debug Monitor exception.
  */
void DebugMon_Handler(void)
{
}

/**
  * @brief  This function handles SysTick Handler.
  */
void SysTick_Handler(void)
{
  /* osSystickHandler() only feeds FreeRTOS's tick once the scheduler is
   * running, so HAL_IncTick() must be called unconditionally here or
   * HAL_GetTick()/HAL-internal timeouts stay frozen at 0 for all of main()
   * before vTaskStartScheduler(), including SystemClock_Config()'s HSE/PLL
   * ready waits - turning a bounded timeout into an infinite hang. */
  HAL_IncTick();
  osSystickHandler();
}

/******************************************************************************/
/*                 STM32H7xx Peripherals Interrupt Handlers                   */
/******************************************************************************/

/**
  * @brief This function handles USART1 global interrupt (u-blox module).
  *
  * Not used in SPI transport mode (UCX_SPI_TRANSPORT) - USART1 is not
  * initialized there, the u-blox module is polled over SPI1/DRDY instead -
  * but the vector table still needs this symbol to exist.
  */
void USART1_IRQHandler(void)
{
#if !defined(UCX_SPI_TRANSPORT)
  uPortUart_IRQHandler();
#endif
}

/**
  * @brief This function handles DMA1 Stream 0 global interrupt (USART1 RX DMA).
  *
  * Not used in SPI transport mode (UCX_SPI_TRANSPORT) - see USART1_IRQHandler.
  */
void DMA1_Stream0_IRQHandler(void)
{
#if !defined(UCX_SPI_TRANSPORT)
  uPortUartDma_IRQHandler();
#endif
}

/**
  * @brief This function handles USART3 global interrupt (console/ST-LINK VCP).
  *
  * Feeds the interrupt-driven RX ring buffer in main_stm32.c so
  * exampleConsoleUartRead() never overruns during sustained/binary transfers
  * (e.g. XMODEM via uart_bridge_example).
  */
void USART3_IRQHandler(void)
{
  exampleConsoleUart_IRQHandler();
}

/**
  * @brief This function handles TIM6 global interrupt (HAL timebase).
  */
void TIM6_DAC_IRQHandler(void)
{
  HAL_TIM_IRQHandler(&htim6);
}
