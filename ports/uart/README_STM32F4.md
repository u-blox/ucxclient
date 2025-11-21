# STM32F407VG UART Port

This document contains some details for the `u_port_uart_stm32f4` port targeted for `STM32F407VG` using the STM32 HAL library.

## STM32F407VG Pin Configuration

The UART pins must be configured in your application before calling `uPortUartOpen()`. Here are the default pins for each UART instance:

### USART1 (APB2 - up to 84 MHz)
- **TX**: PA9 or PB6
- **RX**: PA10 or PB7
- **CTS**: PA11 (if flow control enabled)
- **RTS**: PA12 (if flow control enabled)

### USART2 (APB1 - up to 42 MHz) - **Default**
- **TX**: PA2 or PD5
- **RX**: PA3 or PD6
- **CTS**: PA0 or PD3
- **RTS**: PA1 or PD4

### USART3 (APB1)
- **TX**: PB10 or PC10 or PD8
- **RX**: PB11 or PC11 or PD9
- **CTS**: PB13 or PD11
- **RTS**: PB14 or PD12

### UART4 (APB1)
- **TX**: PA0 or PC10
- **RX**: PA1 or PC11
- No hardware flow control

### UART5 (APB1)
- **TX**: PC12
- **RX**: PD2
- No hardware flow control

### USART6 (APB2)
- **TX**: PC6 or PG14
- **RX**: PC7 or PG9
- **CTS**: PG13 or PG15
- **RTS**: PG12 or PG8

## Configuration

### 1. Select UART Instance

Define one of these macros in your project:

```c
U_PORT_UART_USE_USART1
U_PORT_UART_USE_USART2  // Default if none specified
U_PORT_UART_USE_USART3
U_PORT_UART_USE_UART4
U_PORT_UART_USE_UART5
U_PORT_UART_USE_USART6
```

**CMake example:**
```cmake
target_compile_definitions(your_target PRIVATE
    U_PORT_UART_USE_USART1
)
```

### 2. Configure RX Buffer Size (Optional)

Default is 2048 bytes. To change:

```cmake
target_compile_definitions(your_target PRIVATE
    U_PORT_UART_RX_BUFFER_SIZE=4096
)
```

### 3. GPIO Initialization

**Important**: You must configure the GPIO pins in your application before opening the UART. 

Example using STM32CubeMX generated code in `MX_GPIO_Init()`:

```c
void MX_USART2_UART_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    PA0     ------> USART2_CTS (optional, if flow control)
    PA1     ------> USART2_RTS (optional, if flow control)
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* Optional: Configure CTS/RTS pins if using flow control */
    // GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    // HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}
```

**Note**: The `uPortUartOpen()` function will call `HAL_UART_Init()` internally, but **you** are responsible for GPIO configuration.

### 4. Interrupt Handler

Add the UART interrupt handler to your `stm32f4xx_it.c`:

```c
void USART2_IRQHandler(void)
{
    U_PORT_UART_IRQHandler();
}
```

For other UART instances, use the corresponding IRQ handler name (e.g., `USART1_IRQHandler`, `UART4_IRQHandler`).
