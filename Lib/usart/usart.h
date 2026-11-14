#ifndef LIB_USART_H
#define LIB_USART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "gd32f30x_usart.h"
#include "semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * The driver follows the shape of the STM32 HAL UART driver, while using the
 * GD32 standard peripheral library underneath. The handle is owned by the
 * application and must remain alive until usart_handle_deinit() returns.
 */
typedef enum {
    USART_STATUS_OK = 0,
    USART_STATUS_ERROR,
    USART_STATUS_BUSY,
    USART_STATUS_TIMEOUT,
    USART_STATUS_INVALID_ARGUMENT,
    USART_STATUS_NOT_READY,
    USART_STATUS_INVALID_CONTEXT
} usart_status_t;

typedef enum {
    USART_PARITY_NONE = 0,
    USART_PARITY_EVEN,
    USART_PARITY_ODD
} usart_parity_t;

typedef enum {
    USART_STOP_BITS_1 = 1,
    USART_STOP_BITS_2 = 2
} usart_stop_bits_t;

typedef struct {
    uint32_t instance; /* USART0, USART1, USART2, UART3 or UART4. */
    uint32_t baud_rate;
    uint8_t data_bits; /* The byte API currently supports 8 data bits. */
    usart_parity_t parity;
    usart_stop_bits_t stop_bits;
} usart_config_t;

typedef struct usart_handle {
    uint32_t instance;
    usart_config_t init;

    SemaphoreHandle_t tx_mutex;
    SemaphoreHandle_t rx_mutex;
    SemaphoreHandle_t tx_complete;
    SemaphoreHandle_t rx_complete;

    const uint8_t *tx_buffer;
    size_t tx_size;
    volatile size_t tx_count;
    volatile bool tx_active;
    volatile usart_status_t tx_status;

    uint8_t *rx_buffer;
    size_t rx_size;
    volatile size_t rx_count;
    volatile bool rx_active;
    volatile usart_status_t rx_status;

    volatile uint32_t error_code;
    bool initialized;
} usart_handle_t;


usart_status_t usart_init(usart_handle_t *handle, const usart_config_t *config);
usart_status_t usart_handle_deinit(usart_handle_t *handle);

/*
 * These APIs use the USART peripheral interrupts internally, then block the
 * calling task until the IRQ handler gives the corresponding semaphore.
 * timeout is used for both mutex acquisition and transfer completion.
 */
usart_status_t usart_transmit_it(usart_handle_t *handle, const uint8_t *data,
                                 size_t length, TickType_t timeout);
usart_status_t usart_receive_it(usart_handle_t *handle, uint8_t *data,
                                size_t length, TickType_t timeout);

/* Blocking aliases with an explicit interrupt based implementation. */
usart_status_t usart_transmit(usart_handle_t *handle, const uint8_t *data,
                              size_t length, TickType_t timeout);
usart_status_t usart_receive(usart_handle_t *handle, uint8_t *data,
                             size_t length, TickType_t timeout);

/*
 * Call this function from the matching USARTx_IRQHandler(). It contains all
 * interrupt flag checks and data movement, like HAL_UART_IRQHandler().
 */
void usart_irq_handler(usart_handle_t *handle);

size_t usart_get_rx_count(const usart_handle_t *handle);
uint32_t usart_get_error(const usart_handle_t *handle);

/* Compatibility helpers for code that uses read/write byte counts. */
size_t usart_read_bytes(usart_handle_t *handle, uint8_t *data, size_t length,
                        TickType_t timeout);
size_t usart_write_bytes(usart_handle_t *handle, const uint8_t *data, size_t length);

#ifdef __cplusplus
}
#endif

#endif /* LIB_USART_H */
