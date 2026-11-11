#ifndef LIB_USART_H
#define LIB_USART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct usart_handle usart_handle_t;

typedef enum {
    USART_PORT_0 = 0
} usart_port_t;

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
    uint32_t baud_rate;
    uint8_t data_bits;
    usart_parity_t parity;
    usart_stop_bits_t stop_bits;
} usart_config_t;

bool usart_driver_install(usart_port_t port, const usart_config_t *config,
                          size_t rx_buffer_size, usart_handle_t **out_handle);
void usart_driver_delete(usart_handle_t *handle);

size_t usart_read_bytes(usart_handle_t *handle, uint8_t *data, size_t length,
                        TickType_t timeout);
size_t usart_write_bytes(usart_handle_t *handle, const uint8_t *data, size_t length);
bool usart_get_buffered_data_len(usart_handle_t *handle, size_t *length);
uint32_t usart_get_dropped_bytes(usart_handle_t *handle);

void usart_rx_idle_irq(void);
void usart_rx_dma_irq(void);
void usart_tx_dma_irq(void);

#ifdef __cplusplus
}
#endif

#endif
