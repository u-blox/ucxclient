/*
 * Copyright 2025 u-blox
 *
 * Main application for STM32F407VG running ucxclient http_example
 */

#include <stdio.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include "u_port.h"
#include "u_cx_log.h"

/* Forward declarations */
void SystemClock_Config(void);
static void GPIO_Init(void);

/* External application main function from examples */
extern int app_main(int argc, char *argv[]);

/* FreeRTOS application task */
static void ucx_task(void *pvParameters)
{
    (void)pvParameters;

    /* Initialize port layer */
    uPortInit();

    /* Prepare argc/argv for example
     * Configure these based on your WiFi network */
    char *argv[] = {
        "http_example",
        NULL,  /* UART device - not used on STM32 */
        "YourSSID",
        "YourPassword"
    };
    int argc = 4;

    /* Run the application */
    app_main(argc, argv);

    /* Should not reach here */
    uPortDeinit();
    vTaskDelete(NULL);
}

int main(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    HAL_Init();

    /* Configure the system clock to 168 MHz */
    SystemClock_Config();

    /* Initialize GPIO for UART */
    GPIO_Init();

    /* Create the main application task */
    xTaskCreate(ucx_task, "ucx", 4096, NULL, 5, NULL);

    /* Start scheduler */
    vTaskStartScheduler();

    /* We should never get here as control is now taken by the scheduler */
    while (1);
}

/**
 * @brief  System Clock Configuration
 *         Configures the system clock to 168 MHz using HSE (8 MHz external crystal)
 */
void SystemClock_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    RCC_OscInitTypeDef RCC_OscInitStruct;

    /* Enable Power Control clock */
    __HAL_RCC_PWR_CLK_ENABLE();

    /* The voltage scaling allows optimizing the power consumption when the device is 
       clocked below the maximum system frequency, to update the voltage scaling value 
       regarding system frequency refer to product datasheet.  */
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
    
    /* Enable HSE Oscillator and activate PLL with HSE as source */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 8;  /* Changed from 25 for 8MHz crystal */
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
    RCC_OscInitStruct.PLL.PLLQ = 7;
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

/**
 * @brief  Initialize GPIO pins for UART2
 * UART2 TX: PA2
 * UART2 RX: PA3
 */
static void GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* Enable GPIOA clock */
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /* Configure UART2 GPIO pins (PA2 = TX, PA3 = RX) */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* If using hardware flow control, configure CTS/RTS pins here:
     * PA0 = CTS, PA1 = RTS */
}

/* HAL MSP Init */
void HAL_MspInit(void)
{
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_RCC_PWR_CLK_ENABLE();

    /* System interrupt init*/
    HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

/* FreeRTOS Hooks */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    /* Stack overflow detected */
    while (1);
}

void vApplicationMallocFailedHook(void)
{
    /* Malloc failed */
    while (1);
}

/* Error handler */
void Error_Handler(void)
{
    /* User may add their own implementation to report the error */
    __disable_irq();
    while (1);
}

/* HAL assert handler */
void assert_failed(uint8_t* file, uint32_t line)
{
    (void)file;
    (void)line;
    while (1);
}

/* Retarget printf to nothing (or implement for debugging via SWO/UART) */
int _write(int file, char *ptr, int len)
{
    (void)file;
    (void)ptr;
    (void)len;
    return len;
}
