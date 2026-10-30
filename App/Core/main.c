#include "FreeRTOS.h"
#include "app_main.hpp"
#include "gd32f30x_dma.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_usart.h"
#include "main.h"
#include "task.h"

#define USART0_DATA_ADDRESS ((uint32_t)&USART_DATA(USART0))

uint8_t usart0_rx_buffer[USART0_RX_BUFFER_SIZE];
volatile uint16_t usart0_rx_length;
volatile bool usart0_rx_ready;
volatile bool usart0_tx_busy;

static void init_task(void* param);
static void usart0_dma_init(void);
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
    (void)param;
    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_DMA0);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1 | GPIO_PIN_2);
    gpio_bit_write(GPIOA, GPIO_PIN_1, SET);

    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, 115200U);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    usart0_dma_init();
    nvic_irq_enable(USART0_IRQn, 0, 0);
    nvic_irq_enable(DMA0_Channel3_IRQn, 0, 1);
    app_main(NULL);

    for (;;) {
        gpio_bit_write(GPIOA, GPIO_PIN_1, RESET);
        gpio_bit_write(GPIOA, GPIO_PIN_2, SET);
        vTaskDelay(pdMS_TO_TICKS(300));
        gpio_bit_write(GPIOA, GPIO_PIN_1, SET);
        gpio_bit_write(GPIOA, GPIO_PIN_2, RESET);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static void usart0_dma_init(void) {
    dma_parameter_struct dma_init_struct;

    dma_deinit(DMA0, DMA_CH3);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = 0U;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 1U;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_memory_to_memory_disable(DMA0, DMA_CH3);
    dma_interrupt_enable(DMA0, DMA_CH3, DMA_INT_FTF);

    dma_deinit(DMA0, DMA_CH4);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)usart0_rx_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = USART0_RX_BUFFER_SIZE;
    dma_init_struct.periph_addr = USART0_DATA_ADDRESS;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);

    usart0_rx_length = 0U;
    usart0_rx_ready = false;
    usart0_tx_busy = false;

    usart_flag_clear(USART0, USART_FLAG_RBNE);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
    dma_channel_enable(DMA0, DMA_CH4);
}

bool usart0_dma_transmit(const uint8_t* data, uint16_t length) {
    if ((data == NULL) || (length == 0U) || usart0_tx_busy) {
        return false;
    }

    usart0_tx_busy = true;
    dma_channel_disable(DMA0, DMA_CH3);
    dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_G);
    dma_memory_address_config(DMA0, DMA_CH3, (uint32_t)data);
    dma_transfer_number_config(DMA0, DMA_CH3, length);
    dma_channel_enable(DMA0, DMA_CH3);

    return true;
}

static void halt(void) {
    taskDISABLE_INTERRUPTS();
    for (;;) {
    }
}
