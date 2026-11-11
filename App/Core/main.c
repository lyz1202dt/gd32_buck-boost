#include "FreeRTOS.h"
#include "app_main.hpp"
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#include "task.h"
#include "usart.h"

static void init_task(void* param);
static void halt(void);

int main(void) {
    if (xTaskCreate(
            init_task, "app_main", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1U, NULL)
        != pdPASS) {
        halt();
    }

    vTaskStartScheduler();

    halt();
}

static void init_task(void* param) {
    usart_handle_t *usart_handle;
    const usart_config_t usart_config = {
        .baud_rate = 115200U,
        .data_bits = 8U,
        .parity = USART_PARITY_NONE,
        .stop_bits = USART_STOP_BITS_1,
    };

    (void)param;
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
    gpio_bit_write(GPIOA, GPIO_PIN_1, SET);

    nvic_priority_group_set(NVIC_PRIGROUP_PRE4_SUB0);
    if (!usart_driver_install(USART_PORT_0, &usart_config, 1024U, &usart_handle)) {
        halt();
    }
    app_main(usart_handle);

    for (;;) {
        gpio_bit_write(GPIOA, GPIO_PIN_1, RESET);
        gpio_bit_write(GPIOA, GPIO_PIN_2, SET);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_bit_write(GPIOA, GPIO_PIN_1, SET);
        gpio_bit_write(GPIOA, GPIO_PIN_2, RESET);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static void halt(void) {
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
