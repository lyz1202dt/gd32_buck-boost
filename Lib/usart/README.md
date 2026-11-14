# USART interrupt driver

`usart_handle_t` and the API are arranged like an STM32 HAL UART handle. The
driver uses the GD32 standard peripheral library and FreeRTOS binary semaphores:
the task-side transmit and receive calls enable the corresponding USART
interrupt and suspend until `usart_irq_handler()` signals completion.

```c
static usart_handle_t huart0;
static const usart_config_t config = {
    .instance = USART0,
    .baud_rate = 115200U,
    .data_bits = 8U,
    .parity = USART_PARITY_NONE,
    .stop_bits = USART_STOP_BITS_1,
};

void uart_task(void *argument)
{
    uint8_t tx[] = "hello\r\n";
    uint8_t rx[8];

    (void)argument;
    (void)usart_init(&huart0, &config);
    (void)usart_transmit_it(&huart0, tx, sizeof(tx) - 1U, portMAX_DELAY);
    (void)usart_receive_it(&huart0, rx, sizeof(rx), pdMS_TO_TICKS(100));
    vTaskDelete(NULL);
}
```

The IRQ entry is deliberately left to the application. It must call the
handler for the matching instance:

```c
void USART0_IRQHandler(void)
{
    usart_irq_handler(&huart0);
}
```

Configure GPIO alternate-function pins and the NVIC in an application override
of `usart_msp_init()`, just as `HAL_UART_MspInit()` is used with STM32 HAL. The
USART interrupt priority must be at or below the FreeRTOS syscall priority
(numerically `5` or higher, meaning equal or lower urgency, in the default
project configuration), since the handler calls `xSemaphoreGiveFromISR()`.

Only the byte-oriented 8-bit data path is implemented. `usart_get_rx_count()`
can be used after a timeout, and `usart_get_error()` returns the GD32 STAT0
error bits captured by the IRQ handler.

The handle cleanup function is named `usart_handle_deinit()` because the GD32
standard peripheral library already defines `usart_deinit(uint32_t)`.
