#include "usart.h"

#include <string.h>

#include "gd32f30x_dma.h"
#include "gd32f30x_gpio.h"
#include "gd32f30x_rcu.h"
#include "gd32f30x_usart.h"
#include "semphr.h"
#include "stream_buffer.h"

#define USART_DMA_BUFFER_SIZE 256U
#define USART_IRQ_PRIORITY 5U

struct usart_handle {
    usart_port_t port;
    StreamBufferHandle_t rx_stream;
    SemaphoreHandle_t tx_mutex;
    SemaphoreHandle_t tx_complete;
    volatile uint32_t dropped_bytes;
    uint8_t rx_dma_buffer[USART_DMA_BUFFER_SIZE];
    uint8_t tx_dma_buffer[USART_DMA_BUFFER_SIZE];
};

static usart_handle_t * volatile active_handle;

static void usart_release_handle(usart_handle_t *handle)
{
    if (handle->tx_complete != NULL) {
        vSemaphoreDelete(handle->tx_complete);
    }
    if (handle->tx_mutex != NULL) {
        vSemaphoreDelete(handle->tx_mutex);
    }
    if (handle->rx_stream != NULL) {
        vStreamBufferDelete(handle->rx_stream);
    }
    vPortFree(handle);
}

static void usart_rx_drain(usart_handle_t *handle)
{
    size_t length;
    BaseType_t higher_priority_task_woken = pdFALSE;

    dma_channel_disable(DMA0, DMA_CH4);
    length = USART_DMA_BUFFER_SIZE - dma_transfer_number_get(DMA0, DMA_CH4);
    if (length != 0U) {
        size_t written = xStreamBufferSendFromISR(handle->rx_stream, handle->rx_dma_buffer,
                                                    length, &higher_priority_task_woken);
        handle->dropped_bytes += (uint32_t)(length - written);
    }
    dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_G);
    dma_transfer_number_config(DMA0, DMA_CH4, USART_DMA_BUFFER_SIZE);
    dma_channel_enable(DMA0, DMA_CH4);
    portYIELD_FROM_ISR(higher_priority_task_woken);
}

bool usart_driver_install(usart_port_t port, const usart_config_t *config,
                          size_t rx_buffer_size, usart_handle_t **out_handle)
{
    dma_parameter_struct dma_init_struct;
    usart_handle_t *handle;

    if (out_handle == NULL) {
        return false;
    }
    *out_handle = NULL;
    if ((port != USART_PORT_0) || (active_handle != NULL) || (config == NULL) ||
        (config->baud_rate == 0U) || (config->data_bits != 8U) ||
        ((config->parity != USART_PARITY_NONE) && (config->parity != USART_PARITY_EVEN) &&
         (config->parity != USART_PARITY_ODD)) ||
        ((config->stop_bits != USART_STOP_BITS_1) &&
         (config->stop_bits != USART_STOP_BITS_2)) ||
        (rx_buffer_size < USART_DMA_BUFFER_SIZE)) {
        return false;
    }

    handle = pvPortMalloc(sizeof(*handle));
    if (handle == NULL) {
        return false;
    }
    memset(handle, 0, sizeof(*handle));
    handle->port = port;
    handle->rx_stream = xStreamBufferCreate(rx_buffer_size, 1U);
    handle->tx_mutex = xSemaphoreCreateMutex();
    handle->tx_complete = xSemaphoreCreateBinary();
    if ((handle->rx_stream == NULL) || (handle->tx_mutex == NULL) ||
        (handle->tx_complete == NULL)) {
        usart_release_handle(handle);
        return false;
    }

    rcu_periph_clock_enable(RCU_AF);
    rcu_periph_clock_enable(RCU_GPIOB);
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_DMA0);
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    gpio_pin_remap_config(GPIO_USART0_REMAP, ENABLE);

    usart_deinit(USART0);
    usart_baudrate_set(USART0, config->baud_rate);
    usart_word_length_set(USART0, (config->parity == USART_PARITY_NONE) ?
                                      USART_WL_8BIT : USART_WL_9BIT);
    usart_stop_bit_set(USART0, (config->stop_bits == USART_STOP_BITS_1) ?
                                  USART_STB_1BIT : USART_STB_2BIT);
    if (config->parity == USART_PARITY_EVEN) {
        usart_parity_config(USART0, USART_PM_EVEN);
    } else if (config->parity == USART_PARITY_ODD) {
        usart_parity_config(USART0, USART_PM_ODD);
    } else {
        usart_parity_config(USART0, USART_PM_NONE);
    }
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);

    dma_deinit(DMA0, DMA_CH4);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)handle->rx_dma_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = USART_DMA_BUFFER_SIZE;
    dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH4, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH4);
    dma_memory_to_memory_disable(DMA0, DMA_CH4);
    dma_interrupt_enable(DMA0, DMA_CH4, DMA_INT_FTF);

    dma_deinit(DMA0, DMA_CH3);
    dma_struct_para_init(&dma_init_struct);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL;
    dma_init_struct.memory_addr = (uint32_t)handle->tx_dma_buffer;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = USART_DMA_BUFFER_SIZE;
    dma_init_struct.periph_addr = (uint32_t)&USART_DATA(USART0);
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    dma_init(DMA0, DMA_CH3, &dma_init_struct);
    dma_circulation_disable(DMA0, DMA_CH3);
    dma_memory_to_memory_disable(DMA0, DMA_CH3);
    dma_interrupt_enable(DMA0, DMA_CH3, DMA_INT_FTF);

    (void)USART_STAT0(USART0);
    (void)USART_DATA(USART0);
    dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_G);
    dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_G);
    active_handle = handle;
    nvic_irq_enable(USART0_IRQn, USART_IRQ_PRIORITY, 0U);
    nvic_irq_enable(DMA0_Channel3_IRQn, USART_IRQ_PRIORITY, 0U);
    nvic_irq_enable(DMA0_Channel4_IRQn, USART_IRQ_PRIORITY, 0U);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_ENABLE);
    usart_interrupt_enable(USART0, USART_INT_IDLE);
    dma_channel_enable(DMA0, DMA_CH4);
    *out_handle = handle;
    return true;
}

void usart_driver_delete(usart_handle_t *handle)
{
    if ((handle == NULL) || (handle != active_handle)) {
        return;
    }

    nvic_irq_disable(USART0_IRQn);
    nvic_irq_disable(DMA0_Channel3_IRQn);
    nvic_irq_disable(DMA0_Channel4_IRQn);
    usart_interrupt_disable(USART0, USART_INT_IDLE);
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_DISABLE);
    usart_dma_transmit_config(USART0, USART_TRANSMIT_DMA_DISABLE);
    dma_channel_disable(DMA0, DMA_CH3);
    dma_channel_disable(DMA0, DMA_CH4);
    dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_G);
    dma_interrupt_flag_clear(DMA0, DMA_CH4, DMA_INT_FLAG_G);
    usart_deinit(USART0);
    active_handle = NULL;
    usart_release_handle(handle);
}

size_t usart_read_bytes(usart_handle_t *handle, uint8_t *data, size_t length,
                        TickType_t timeout)
{
    if ((handle == NULL) || (handle != active_handle) || (data == NULL) ||
        (length == 0U)) {
        return 0U;
    }
    return xStreamBufferReceive(handle->rx_stream, data, length, timeout);
}

size_t usart_write_bytes(usart_handle_t *handle, const uint8_t *data, size_t length)
{
    size_t written = 0U;

    if ((handle == NULL) || (handle != active_handle) || (data == NULL) ||
        (length == 0U)) {
        return 0U;
    }
    xSemaphoreTake(handle->tx_mutex, portMAX_DELAY);
    while (written < length) {
        size_t chunk = length - written;
        if (chunk > USART_DMA_BUFFER_SIZE) {
            chunk = USART_DMA_BUFFER_SIZE;
        }
        memcpy(handle->tx_dma_buffer, data + written, chunk);
        dma_channel_disable(DMA0, DMA_CH3);
        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_G);
        dma_transfer_number_config(DMA0, DMA_CH3, (uint32_t)chunk);
        dma_channel_enable(DMA0, DMA_CH3);
        xSemaphoreTake(handle->tx_complete, portMAX_DELAY);
        written += chunk;
    }
    xSemaphoreGive(handle->tx_mutex);
    return written;
}

bool usart_get_buffered_data_len(usart_handle_t *handle, size_t *length)
{
    if ((handle == NULL) || (handle != active_handle) || (length == NULL)) {
        return false;
    }
    *length = xStreamBufferBytesAvailable(handle->rx_stream);
    return true;
}

uint32_t usart_get_dropped_bytes(usart_handle_t *handle)
{
    return ((handle == NULL) || (handle != active_handle)) ? 0U :
           handle->dropped_bytes;
}

void usart_rx_idle_irq(void)
{
    if (RESET != usart_interrupt_flag_get(USART0, USART_INT_FLAG_IDLE)) {
        (void)USART_STAT0(USART0);
        (void)USART_DATA(USART0);
        if (active_handle != NULL) {
            usart_rx_drain(active_handle);
        }
    }
}

void usart_rx_dma_irq(void)
{
    if ((active_handle != NULL) &&
        (RESET != dma_interrupt_flag_get(DMA0, DMA_CH4, DMA_INT_FLAG_FTF))) {
        usart_rx_drain(active_handle);
    }
}

void usart_tx_dma_irq(void)
{
    if ((active_handle != NULL) &&
        (RESET != dma_interrupt_flag_get(DMA0, DMA_CH3, DMA_INT_FLAG_FTF))) {
        BaseType_t higher_priority_task_woken = pdFALSE;

        dma_interrupt_flag_clear(DMA0, DMA_CH3, DMA_INT_FLAG_G);
        dma_channel_disable(DMA0, DMA_CH3);
        xSemaphoreGiveFromISR(active_handle->tx_complete, &higher_priority_task_woken);
        portYIELD_FROM_ISR(higher_priority_task_woken);
    }
}
