#include "usart.h"

#include <string.h>

#include "gd32f30x_rcu.h"
#include "task.h"

static bool usart_is_valid_instance(uint32_t instance)
{
    return (instance == USART0) || (instance == USART1) || (instance == USART2) ||
           (instance == UART3) || (instance == UART4);
}

static bool usart_is_in_interrupt(void)
{
    return __get_IPSR() != 0U;
}

static void usart_enable_clock(uint32_t instance)
{
    switch (instance) {
    case USART0:
        rcu_periph_clock_enable(RCU_USART0);
        break;
    case USART1:
        rcu_periph_clock_enable(RCU_USART1);
        break;
    case USART2:
        rcu_periph_clock_enable(RCU_USART2);
        break;
    case UART3:
        rcu_periph_clock_enable(RCU_UART3);
        break;
    case UART4:
        rcu_periph_clock_enable(RCU_UART4);
        break;
    default:
        break;
    }
}

static void usart_drain_semaphore(SemaphoreHandle_t semaphore)
{
    while (xSemaphoreTake(semaphore, 0U) == pdTRUE) {
    }
}

static void usart_disable_transfer_interrupts(usart_handle_t *handle)
{
    usart_interrupt_disable(handle->instance, USART_INT_TBE);
    usart_interrupt_disable(handle->instance, USART_INT_TC);
    usart_interrupt_disable(handle->instance, USART_INT_RBNE);
}

static void usart_clear_error_flags(usart_handle_t *handle, uint32_t stat0)
{
    /* Reading STAT0 followed by DATA clears the GD32 error/RBNE condition. */
    (void)stat0;
    (void)USART_STAT0(handle->instance);
    (void)usart_data_receive(handle->instance);
}

static void usart_finish_tx_from_isr(usart_handle_t *handle, usart_status_t status,
                                     BaseType_t *higher_priority_task_woken)
{
    if (!handle->tx_active) {
        return;
    }

    usart_interrupt_disable(handle->instance, USART_INT_TBE);
    usart_interrupt_disable(handle->instance, USART_INT_TC);
    handle->tx_status = status;
    handle->tx_active = false;
    (void)xSemaphoreGiveFromISR(handle->tx_complete, higher_priority_task_woken);
}

static void usart_finish_rx_from_isr(usart_handle_t *handle, usart_status_t status,
                                     BaseType_t *higher_priority_task_woken)
{
    if (!handle->rx_active) {
        return;
    }

    usart_interrupt_disable(handle->instance, USART_INT_RBNE);
    handle->rx_status = status;
    handle->rx_active = false;
    (void)xSemaphoreGiveFromISR(handle->rx_complete, higher_priority_task_woken);
}

usart_status_t usart_init(usart_handle_t *handle, const usart_config_t *config)
{
    if ((handle == NULL) || (config == NULL) || !usart_is_valid_instance(config->instance) ||
        (config->baud_rate == 0U) || (config->data_bits != 8U) ||
        ((config->parity != USART_PARITY_NONE) && (config->parity != USART_PARITY_EVEN) &&
         (config->parity != USART_PARITY_ODD)) ||
        ((config->stop_bits != USART_STOP_BITS_1) &&
         (config->stop_bits != USART_STOP_BITS_2))) {
        return USART_STATUS_INVALID_ARGUMENT;
    }
    if (usart_is_in_interrupt()) {
        return USART_STATUS_INVALID_CONTEXT;
    }
    if (handle->initialized) {
        return USART_STATUS_BUSY;
    }

    /* A zero-initialized handle is the HAL-style reset state. */
    handle->instance = config->instance;
    handle->init = *config;
    handle->tx_mutex = xSemaphoreCreateMutex();
    handle->rx_mutex = xSemaphoreCreateMutex();
    handle->tx_complete = xSemaphoreCreateBinary();
    handle->rx_complete = xSemaphoreCreateBinary();
    if ((handle->tx_mutex == NULL) || (handle->rx_mutex == NULL) ||
        (handle->tx_complete == NULL) || (handle->rx_complete == NULL)) {
        if (handle->tx_complete != NULL) {
            vSemaphoreDelete(handle->tx_complete);
        }
        if (handle->rx_complete != NULL) {
            vSemaphoreDelete(handle->rx_complete);
        }
        if (handle->tx_mutex != NULL) {
            vSemaphoreDelete(handle->tx_mutex);
        }
        if (handle->rx_mutex != NULL) {
            vSemaphoreDelete(handle->rx_mutex);
        }
        handle->tx_complete = NULL;
        handle->rx_complete = NULL;
        handle->tx_mutex = NULL;
        handle->rx_mutex = NULL;
        return USART_STATUS_ERROR;
    }

    handle->tx_buffer = NULL;
    handle->tx_size = 0U;
    handle->tx_count = 0U;
    handle->tx_active = false;
    handle->tx_status = USART_STATUS_OK;
    handle->rx_buffer = NULL;
    handle->rx_size = 0U;
    handle->rx_count = 0U;
    handle->rx_active = false;
    handle->rx_status = USART_STATUS_OK;
    handle->error_code = 0U;

    usart_enable_clock(handle->instance);
    usart_deinit(handle->instance);
    usart_baudrate_set(handle->instance, config->baud_rate);
    usart_word_length_set(handle->instance,
                          config->parity == USART_PARITY_NONE ? USART_WL_8BIT : USART_WL_9BIT);
    usart_stop_bit_set(handle->instance,
                       config->stop_bits == USART_STOP_BITS_1 ? USART_STB_1BIT : USART_STB_2BIT);
    if (config->parity == USART_PARITY_EVEN) {
        usart_parity_config(handle->instance, USART_PM_EVEN);
    } else if (config->parity == USART_PARITY_ODD) {
        usart_parity_config(handle->instance, USART_PM_ODD);
    } else {
        usart_parity_config(handle->instance, USART_PM_NONE);
    }
    usart_receive_config(handle->instance, USART_RECEIVE_ENABLE);
    usart_transmit_config(handle->instance, USART_TRANSMIT_ENABLE);
    usart_enable(handle->instance);

    /* Error interrupt is left enabled so an active task cannot wait forever. */
    usart_interrupt_enable(handle->instance, USART_INT_ERR);
    handle->initialized = true;
    return USART_STATUS_OK;
}

usart_status_t usart_handle_deinit(usart_handle_t *handle)
{
    if ((handle == NULL) || !handle->initialized) {
        return USART_STATUS_INVALID_ARGUMENT;
    }
    if (usart_is_in_interrupt()) {
        return USART_STATUS_INVALID_CONTEXT;
    }
    if (xSemaphoreTake(handle->tx_mutex, 0U) != pdTRUE) {
        return USART_STATUS_BUSY;
    }
    if (xSemaphoreTake(handle->rx_mutex, 0U) != pdTRUE) {
        (void)xSemaphoreGive(handle->tx_mutex);
        return USART_STATUS_BUSY;
    }

    taskENTER_CRITICAL();
    usart_disable_transfer_interrupts(handle);
    usart_interrupt_disable(handle->instance, USART_INT_ERR);
    handle->tx_active = false;
    handle->rx_active = false;
    taskEXIT_CRITICAL();
    usart_disable(handle->instance);

    xSemaphoreGive(handle->tx_mutex);
    xSemaphoreGive(handle->rx_mutex);
    vSemaphoreDelete(handle->tx_complete);
    vSemaphoreDelete(handle->rx_complete);
    vSemaphoreDelete(handle->tx_mutex);
    vSemaphoreDelete(handle->rx_mutex);
    memset(handle, 0, sizeof(*handle));
    return USART_STATUS_OK;
}

usart_status_t usart_transmit_it(usart_handle_t *handle, const uint8_t *data,
                                 size_t length, TickType_t timeout)
{
    if ((handle == NULL) || (data == NULL) || (length == 0U)) {
        return USART_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return USART_STATUS_NOT_READY;
    }
    if (usart_is_in_interrupt()) {
        return USART_STATUS_INVALID_CONTEXT;
    }
    if (xSemaphoreTake(handle->tx_mutex, timeout) != pdTRUE) {
        return USART_STATUS_TIMEOUT;
    }

    usart_drain_semaphore(handle->tx_complete);
    taskENTER_CRITICAL();
    handle->tx_buffer = data;
    handle->tx_size = length;
    handle->tx_count = 0U;
    handle->tx_status = USART_STATUS_BUSY;
    handle->tx_active = true;
    usart_interrupt_disable(handle->instance, USART_INT_TC);
    usart_interrupt_enable(handle->instance, USART_INT_TBE);
    taskEXIT_CRITICAL();

    if (xSemaphoreTake(handle->tx_complete, timeout) != pdTRUE) {
        taskENTER_CRITICAL();
        handle->tx_active = false;
        handle->tx_status = USART_STATUS_TIMEOUT;
        usart_interrupt_disable(handle->instance, USART_INT_TBE);
        usart_interrupt_disable(handle->instance, USART_INT_TC);
        taskEXIT_CRITICAL();
        usart_drain_semaphore(handle->tx_complete);
        xSemaphoreGive(handle->tx_mutex);
        return USART_STATUS_TIMEOUT;
    }

    usart_status_t status = handle->tx_status;
    xSemaphoreGive(handle->tx_mutex);
    return status;
}

usart_status_t usart_receive_it(usart_handle_t *handle, uint8_t *data,
                                size_t length, TickType_t timeout)
{
    if ((handle == NULL) || (data == NULL) || (length == 0U)) {
        return USART_STATUS_INVALID_ARGUMENT;
    }
    if (!handle->initialized) {
        return USART_STATUS_NOT_READY;
    }
    if (usart_is_in_interrupt()) {
        return USART_STATUS_INVALID_CONTEXT;
    }
    if (xSemaphoreTake(handle->rx_mutex, timeout) != pdTRUE) {
        return USART_STATUS_TIMEOUT;
    }

    usart_drain_semaphore(handle->rx_complete);
    taskENTER_CRITICAL();
    handle->rx_buffer = data;
    handle->rx_size = length;
    handle->rx_count = 0U;
    handle->rx_status = USART_STATUS_BUSY;
    handle->rx_active = true;
    usart_interrupt_enable(handle->instance, USART_INT_RBNE);
    taskEXIT_CRITICAL();

    if (xSemaphoreTake(handle->rx_complete, timeout) != pdTRUE) {
        taskENTER_CRITICAL();
        handle->rx_active = false;
        handle->rx_status = USART_STATUS_TIMEOUT;
        usart_interrupt_disable(handle->instance, USART_INT_RBNE);
        taskEXIT_CRITICAL();
        usart_drain_semaphore(handle->rx_complete);
        xSemaphoreGive(handle->rx_mutex);
        return USART_STATUS_TIMEOUT;
    }

    usart_status_t status = handle->rx_status;
    xSemaphoreGive(handle->rx_mutex);
    return status;
}

usart_status_t usart_transmit(usart_handle_t *handle, const uint8_t *data,
                              size_t length, TickType_t timeout)
{
    return usart_transmit_it(handle, data, length, timeout);
}

usart_status_t usart_receive(usart_handle_t *handle, uint8_t *data,
                             size_t length, TickType_t timeout)
{
    return usart_receive_it(handle, data, length, timeout);
}

void usart_irq_handler(usart_handle_t *handle)
{
    BaseType_t higher_priority_task_woken = pdFALSE;
    uint32_t stat0;

    if ((handle == NULL) || !handle->initialized) {
        return;
    }

    stat0 = USART_STAT0(handle->instance);
    if ((stat0 & (USART_STAT0_PERR | USART_STAT0_FERR | USART_STAT0_NERR |
                  USART_STAT0_ORERR)) != 0U) {
        handle->error_code = stat0 &
                             (USART_STAT0_PERR | USART_STAT0_FERR | USART_STAT0_NERR |
                              USART_STAT0_ORERR);
        usart_clear_error_flags(handle, stat0);
        usart_finish_tx_from_isr(handle, USART_STATUS_ERROR, &higher_priority_task_woken);
        usart_finish_rx_from_isr(handle, USART_STATUS_ERROR, &higher_priority_task_woken);
    }

    if (handle->tx_active &&
        (RESET != usart_interrupt_flag_get(handle->instance, USART_INT_FLAG_TBE))) {
        if (handle->tx_count < handle->tx_size) {
            usart_data_transmit(handle->instance, handle->tx_buffer[handle->tx_count]);
            ++handle->tx_count;
        }
        if (handle->tx_count >= handle->tx_size) {
            usart_interrupt_disable(handle->instance, USART_INT_TBE);
            usart_interrupt_enable(handle->instance, USART_INT_TC);
        }
    }

    if (handle->tx_active &&
        (RESET != usart_interrupt_flag_get(handle->instance, USART_INT_FLAG_TC))) {
        usart_finish_tx_from_isr(handle, USART_STATUS_OK, &higher_priority_task_woken);
    }

    if (handle->rx_active &&
        (RESET != usart_interrupt_flag_get(handle->instance, USART_INT_FLAG_RBNE))) {
        if (handle->rx_count < handle->rx_size) {
            handle->rx_buffer[handle->rx_count] = (uint8_t)usart_data_receive(handle->instance);
            ++handle->rx_count;
        } else {
            (void)usart_data_receive(handle->instance);
        }
        if (handle->rx_count >= handle->rx_size) {
            usart_finish_rx_from_isr(handle, USART_STATUS_OK, &higher_priority_task_woken);
        }
    }

    portYIELD_FROM_ISR(higher_priority_task_woken);
}

size_t usart_get_rx_count(const usart_handle_t *handle)
{
    return (handle == NULL) ? 0U : handle->rx_count;
}

uint32_t usart_get_error(const usart_handle_t *handle)
{
    return (handle == NULL) ? 0U : handle->error_code;
}

size_t usart_read_bytes(usart_handle_t *handle, uint8_t *data, size_t length,
                        TickType_t timeout)
{
    return usart_receive_it(handle, data, length, timeout) == USART_STATUS_OK ? length : 0U;
}

size_t usart_write_bytes(usart_handle_t *handle, const uint8_t *data, size_t length)
{
    return usart_transmit_it(handle, data, length, portMAX_DELAY) == USART_STATUS_OK ? length : 0U;
}
