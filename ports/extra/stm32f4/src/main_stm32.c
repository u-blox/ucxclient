/*
 * Copyright 2025 u-blox
 *
 * Main application for STM32F407VG running ucxclient examples
 */

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include "u_port.h"
#include "u_cx_log.h"

/* Forward declarations */
static void SystemClock_Config(void);
static void GPIO_Init(void);
void Error_Handler(void);

/* External application main function from examples */
extern int app_main(int argc, char *argv[]);

/* FreeRTOS application task */
static void ucx_task(void *pvParameters)
{
    (void)pvParameters;

    printf("Starting application...\r\n");

    /* Run the application */
    int result = app_main(0, NULL);

    printf("app_main returned %d\r\n", result);

    /* Should not reach here */
    uPortDeinit();
    vTaskDelete(NULL);
}

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock */
    SystemClock_Config();

    /* Initialize all configured peripherals */
    GPIO_Init();

    /* Send early test message */
    printf("\r\n===========================================\r\n");
#if defined(NUCLEO_F439ZI)
    printf("NUCLEO-F439ZI ucxclient - Starting...\r\n");
#else
    printf("STM32F407 ucxclient - Starting...\r\n");
#endif
    printf("===========================================\r\n");
    printf("Creating FreeRTOS task...\r\n");


    /* Create the main application task */
    BaseType_t result = xTaskCreate(ucx_task, "ucx", 2048, NULL, 5, NULL);

    if (result == pdPASS) {
        printf("Task created successfully\r\n");
    } else {
        printf("ERROR: Failed to create task (err=%ld)\r\n", (long)result);
        printf("FATAL: System halted\r\n");
        while (1);
    }

    printf("Starting FreeRTOS scheduler...\r\n");

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    printf("ERROR: Scheduler returned!\r\n");
    printf("FATAL: System halted\r\n");
    while (1);
}

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow :
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 168000000
  *            HCLK(Hz)                       = 168000000
  *            AHB Prescaler                  = 1
  *            APB1 Prescaler                 = 4
  *            APB2 Prescaler                 = 2
  *            HSE Frequency(Hz)              = 25000000
  *            PLL_M                          = 25
  *            PLL_N                          = 336
  *            PLL_P                          = 2
  *            PLL_Q                          = 7
  *            VDD(V)                         = 3.3
  *            Main regulator output voltage  = Scale1 mode
  *            Flash Latency(WS)              = 5
  * @param  None
  * @retval None
  */
static void SystemClock_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct;
  RCC_OscInitTypeDef RCC_OscInitStruct;

  /* Enable Power Control clock */
  __HAL_RCC_PWR_CLK_ENABLE();

  /* The voltage scaling allows optimizing the power consumption when the device is
     clocked below the maximum system frequency, to update the voltage scaling value
     regarding system frequency refer to product datasheet.  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

#if defined(NUCLEO_F439ZI)
  /* NUCLEO-F439ZI: 8 MHz clock from ST-LINK MCO on HSE input (bypass mode).
   * PLL: 8 MHz / M=8 * N=336 / P=2 = 168 MHz SYSCLK, Q=7 -> 48 MHz. */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
#else
  /* Enable HSE Oscillator and activate PLL with HSE as source */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
#endif
  HAL_RCC_OscConfig(&RCC_OscInitStruct);

  /* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2
  clocks dividers */
  RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;
  HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5);

  /* STM32F405x/407x/415x/417x Revision Z and upper devices: prefetch is supported  */
  if (HAL_GetREVID() >= 0x1001)
  {
    /* Enable the Flash prefetch */
    __HAL_FLASH_PREFETCH_BUFFER_ENABLE();
  }
}

/* UART handle for console output (printf) */
static UART_HandleTypeDef gConsoleUart;

#ifndef U_EXAMPLE_CONSOLE_RX_BUFFER_SIZE
#define U_EXAMPLE_CONSOLE_RX_BUFFER_SIZE (2048)
#endif

/* Interrupt-driven RX ring buffer for the console UART.
 *
 * uart_bridge_example.c polls exampleConsoleUartRead() from a task-context
 * loop, which can be delayed (FreeRTOS preemption, time spent blocked on
 * the module UART write) for longer than one byte period at 115200 baud
 * (~87us). The console USART's data register is only a single byte deep,
 * so a plain blocking HAL_UART_Receive() poll silently drops bytes under
 * sustained continuous streaming (e.g. an XMODEM firmware transfer over
 * TeraTerm) - this is what made large/binary transfers get stuck. Feeding
 * this ring buffer directly from the RXNE interrupt means no byte is ever
 * lost between polls, regardless of how promptly the bridge loop drains it. */
static volatile uint8_t gConsoleRxBuffer[U_EXAMPLE_CONSOLE_RX_BUFFER_SIZE];
static volatile uint32_t gConsoleRxHead = 0;
static volatile uint32_t gConsoleRxTail = 0;

#if defined(NUCLEO_F439ZI)
/**
 * @brief  Initialize USART3 for console output (ST-LINK VCP)
 * USART3 TX: PD8, RX: PD9 (AF7)
 * Baud: 115200, 8N1
 */
static void Console_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clocks */
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_USART3_CLK_ENABLE();

    /* Configure USART3 GPIO pins (PD8 = TX, PD9 = RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    gConsoleUart.Instance = USART3;
    gConsoleUart.Init.BaudRate = 115200;
    gConsoleUart.Init.WordLength = UART_WORDLENGTH_8B;
    gConsoleUart.Init.StopBits = UART_STOPBITS_1;
    gConsoleUart.Init.Parity = UART_PARITY_NONE;
    gConsoleUart.Init.Mode = UART_MODE_TX_RX;
    gConsoleUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    gConsoleUart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&gConsoleUart) != HAL_OK) {
        Error_Handler();
    }

    /* Interrupt-driven RX (see gConsoleRxBuffer comment above) so large or
     * binary transfers relayed by uart_bridge_example don't overrun the
     * single-byte data register between polls. */
    HAL_NVIC_SetPriority(USART3_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART3_IRQn);
    __HAL_UART_ENABLE_IT(&gConsoleUart, UART_IT_RXNE);
}
#else
/**
 * @brief  Initialize UART2 for console output
 * UART2 TX: PA2, RX: PA3
 * Baud: 115200, 8N1
 */
static void Console_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable clocks */
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_USART2_CLK_ENABLE();

    /* Configure UART2 GPIO pins (PA2 = TX, PA3 = RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Configure UART2 - with HSI clock (16MHz), APB1 should be 16MHz */
    gConsoleUart.Instance = USART2;
    gConsoleUart.Init.BaudRate = 115200;
    gConsoleUart.Init.WordLength = UART_WORDLENGTH_8B;
    gConsoleUart.Init.StopBits = UART_STOPBITS_1;
    gConsoleUart.Init.Parity = UART_PARITY_NONE;
    gConsoleUart.Init.Mode = UART_MODE_TX_RX;
    gConsoleUart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    gConsoleUart.Init.OverSampling = UART_OVERSAMPLING_16;

    if (HAL_UART_Init(&gConsoleUart) != HAL_OK) {
        /* If HAL init fails, manually set baud rate for 16MHz APB1 clock */
        /* BRR = APB1_Clock / BaudRate = 16000000 / 115200 = 138.88 ≈ 139 (0x8B) */
        USART2->BRR = 139;
        /* Enable UART: UE, TE, RE */
        USART2->CR1 |= USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
    }

    /* Interrupt-driven RX (see gConsoleRxBuffer comment above) so large or
     * binary transfers relayed by uart_bridge_example don't overrun the
     * single-byte data register between polls. */
    HAL_NVIC_SetPriority(USART2_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    __HAL_UART_ENABLE_IT(&gConsoleUart, UART_IT_RXNE);
}
#endif /* NUCLEO_F439ZI */

/**
 * @brief  Legacy GPIO init kept for compatibility
 * Now calls Console_UART_Init instead
 */
static void GPIO_Init(void)
{
    Console_UART_Init();
}

/* HAL MSP Init */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* System interrupt init*/
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/* HAL UART MSP Init for the u-blox module UART */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

#if defined(NUCLEO_F439ZI)
    if (huart->Instance == USART1)
    {
        /* USART1 GPIO Configuration on NUCLEO-F439ZI:
         * PB6: USART1_TX
         * PB7: USART1_RX
         * (same Morpho pins as the NUCLEO-H753ZI wiring convention)
         */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART1_CLK_ENABLE();

        GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Configure CTS/RTS if hardware flow control is enabled
         * (USART1 CTS = PA11, RTS = PA12) */
        if (huart->Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS)
        {
            __HAL_RCC_GPIOA_CLK_ENABLE();
            GPIO_InitStruct.Pin = GPIO_PIN_11 | GPIO_PIN_12;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
            HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
        }
    }
    else if (huart->Instance == USART3)
    {
        /* USART3 (console) already initialized in Console_UART_Init */
    }
#else
    if (huart->Instance == USART3)
    {
        /* USART3 GPIO Configuration on STM32F407G-DISC1:
         * PB10: USART3_TX
         * PB11: USART3_RX
         * PB13: USART3_CTS (only if HW flow control enabled)
         * PB14: USART3_RTS (only if HW flow control enabled)
         */
        __HAL_RCC_GPIOB_CLK_ENABLE();
        __HAL_RCC_USART3_CLK_ENABLE();

        /* Always configure TX and RX */
        GPIO_InitStruct.Pin = GPIO_PIN_10 | GPIO_PIN_11;
        GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
        GPIO_InitStruct.Pull = GPIO_NOPULL;
        GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
        GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

        /* Configure CTS/RTS if hardware flow control is enabled */
        if (huart->Init.HwFlowCtl == UART_HWCONTROL_RTS_CTS)
        {
            GPIO_InitStruct.Pin = GPIO_PIN_13 | GPIO_PIN_14;
            GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
            GPIO_InitStruct.Pull = GPIO_NOPULL;
            GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
            GPIO_InitStruct.Alternate = GPIO_AF7_USART3;
            HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
        }
    }
    else if (huart->Instance == USART2)
    {
        /* USART2 already initialized in Console_UART_Init */
    }
#endif /* NUCLEO_F439ZI */
}

/* FreeRTOS Hooks */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    printf("\r\nFATAL: Stack overflow in task '%s'\r\n", pcTaskName);
    printf("FATAL: System halted\r\n");
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    printf("\r\nFATAL: Malloc failed - out of heap memory\r\n");
    printf("FATAL: System halted\r\n");
    while (1);
}

/* Error handler */
void Error_Handler(void)
{
    printf("\r\nFATAL: Error_Handler() called\r\n");
    printf("FATAL: System halted\r\n");
    __disable_irq();
    while (1);
}

/* HAL assert handler */
void assert_failed(uint8_t* file, uint32_t line)
{
    printf("\r\nFATAL: Assert failed at %s:%lu\r\n", (char*)file, line);
    printf("FATAL: System halted\r\n");
    while (1);
}

/* Retarget printf to the console UART via __io_putchar (used by syscalls.c _write) */
int __io_putchar(int ch)
{
    /* Use HAL_UART_Transmit with a reasonable timeout */
    uint8_t c = (uint8_t)ch;
    if (HAL_UART_Transmit(&gConsoleUart, &c, 1, 1000) == HAL_OK) {
        return ch;
    }
    return -1;
}

/**
 * @brief Console UART RX interrupt handler - called from USART3_IRQHandler
 * (NUCLEO-F439ZI) or USART2_IRQHandler (STM32F407G-DISC1) - see
 * stm32f4xx_it.c. Reads the raw data register directly instead of going
 * through HAL_UART_IRQHandler()/HAL_UART_RxCpltCallback(), since that
 * callback name is already defined (for the module UART instance) by the
 * generic port driver (u_port_uart_stm32f4_irq.c/_dma.c) and cannot be
 * redefined here.
 */
void exampleConsoleUart_IRQHandler(void)
{
    if (__HAL_UART_GET_FLAG(&gConsoleUart, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)gConsoleUart.Instance->DR;
        uint32_t nextHead = (gConsoleRxHead + 1) % U_EXAMPLE_CONSOLE_RX_BUFFER_SIZE;
        if (nextHead != gConsoleRxTail) {
            gConsoleRxBuffer[gConsoleRxHead] = byte;
            gConsoleRxHead = nextHead;
        }
        /* else: buffer full, drop the byte (reader too slow) */
    }
    if (__HAL_UART_GET_FLAG(&gConsoleUart, UART_FLAG_ORE)) {
        /* Overrun can only happen if RXNE wasn't serviced for >1 byte time;
         * clear it so reception keeps going. */
        __HAL_UART_CLEAR_OREFLAG(&gConsoleUart);
    }
}

/*
 * Raw console UART access for uart_bridge_example. Bypasses the printf
 * retarget above so the example can both read and write the ST-Link VCP
 * UART directly (needed to bridge it with the u-blox module UART).
 *
 * exampleConsoleUartRead() pops from the interrupt-fed ring buffer above
 * instead of calling HAL_UART_Receive() directly - see gConsoleRxBuffer
 * comment for why.
 */
int32_t exampleConsoleUartRead(uint8_t *pByte, int32_t timeoutMs)
{
    uint32_t startTime = HAL_GetTick();

    do {
        uint32_t tail = gConsoleRxTail;
        if (gConsoleRxHead != tail) {
            *pByte = gConsoleRxBuffer[tail];
            gConsoleRxTail = (tail + 1) % U_EXAMPLE_CONSOLE_RX_BUFFER_SIZE;
            return 1;
        }
    } while ((timeoutMs > 0) && ((int32_t)(HAL_GetTick() - startTime) < timeoutMs));

    return 0;
}

int32_t exampleConsoleUartWrite(const void *pData, size_t length)
{
    if (HAL_UART_Transmit(&gConsoleUart, (uint8_t *)pData, (uint16_t)length, 1000) == HAL_OK) {
        return (int32_t)length;
    }
    return -1;
}
