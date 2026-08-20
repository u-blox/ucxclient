/*
 * Copyright 2025 u-blox
 *
 * Main application for STM32H7 Nucleo boards (NUCLEO-H743ZI/H753ZI)
 * running ucxclient examples.
 *
 * Clock: 480 MHz from 8 MHz HSE bypass (ST-LINK MCO) - proven config from
 * the u-connect-matter STM32 port. D-cache is intentionally left DISABLED
 * so that circular DMA RX into the ring buffer needs no cache maintenance.
 */

#include <stdio.h>
#include <string.h>
#include "stm32h7xx_hal.h"
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
    printf("NUCLEO-H7 ucxclient - Starting...\r\n");
    printf("===========================================\r\n");
    printf("SYSCLK: %lu MHz\r\n", (unsigned long)(HAL_RCC_GetSysClockFreq() / 1000000U));
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
  * @brief  System Clock Configuration for NUCLEO-H743ZI/H753ZI
  *         SYSCLK = 480 MHz: 8 MHz HSE bypass / M=1 * N=120 / P=2
  *         HCLK 240 MHz, APB1/2/3/4 120 MHz
  */
static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    /* Supply configuration (LDO on Nucleo boards) */
    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);
    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;  /* ST-LINK MCO outputs 8 MHz */
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 1;     /* VCO input = 8/1 = 8 MHz */
    RCC_OscInitStruct.PLL.PLLN = 120;   /* VCO output = 8 * 120 = 960 MHz */
    RCC_OscInitStruct.PLL.PLLP = 2;     /* SYSCLK = 960/2 = 480 MHz */
    RCC_OscInitStruct.PLL.PLLQ = 4;
    RCC_OscInitStruct.PLL.PLLR = 2;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;  /* VCO input: 8-16 MHz */
    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLFRACN = 0;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 |
                                  RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
    {
        Error_Handler();
    }

    /* USART3 (console) kernel clock from D2PCLK1 */
    RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    /* USART1 (u-blox module) kernel clock from D2PCLK2 */
    memset(&PeriphClkInitStruct, 0, sizeof(PeriphClkInitStruct));
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
        Error_Handler();
    }
}

/* UART handle for console output (printf) */
static UART_HandleTypeDef gConsoleUart;

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
    gConsoleUart.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
    gConsoleUart.Init.ClockPrescaler = UART_PRESCALER_DIV1;
    gConsoleUart.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;

    if (HAL_UART_Init(&gConsoleUart) != HAL_OK) {
        Error_Handler();
    }
}

/**
 * @brief  Legacy GPIO init kept for compatibility
 */
static void GPIO_Init(void)
{
    Console_UART_Init();
}

/* HAL MSP Init */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();

    /* System interrupt init */
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/* HAL UART MSP Init for the u-blox module UART */
void HAL_UART_MspInit(UART_HandleTypeDef* huart)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    if (huart->Instance == USART1)
    {
        /* USART1 GPIO Configuration on NUCLEO-H743ZI/H753ZI:
         * PB6: USART1_TX (Arduino D1)
         * PB7: USART1_RX (Arduino D0)
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
