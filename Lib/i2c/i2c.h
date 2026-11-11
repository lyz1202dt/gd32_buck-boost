#ifndef LIB_I2C_H
#define LIB_I2C_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct i2c_master_bus *i2c_master_bus_handle_t;
typedef struct i2c_master_device *i2c_master_dev_handle_t;

typedef enum {
    I2C_PORT_0,
    I2C_PORT_1
} i2c_port_t;

typedef enum {
    I2C_OK,
    I2C_ERR_INVALID_ARG,
    I2C_ERR_NO_MEM,
    I2C_ERR_BUSY,
    I2C_ERR_TIMEOUT,
    I2C_ERR_NACK,
    I2C_ERR_BUS,
    I2C_ERR_STATE
} i2c_err_t;

typedef struct {
    i2c_port_t port;
    uint32_t clock_speed_hz;
} i2c_master_bus_config_t;

typedef struct {
    uint8_t device_address;
    uint32_t timeout_ms;
} i2c_device_config_t;

typedef struct {
    const uint8_t *data;
    size_t size;
} i2c_buffer_t;

i2c_err_t i2c_new_master_bus(const i2c_master_bus_config_t *config, i2c_master_bus_handle_t *bus);
i2c_err_t i2c_del_master_bus(i2c_master_bus_handle_t bus);
i2c_err_t i2c_master_bus_add_device(i2c_master_bus_handle_t bus, const i2c_device_config_t *config,
                                    i2c_master_dev_handle_t *device);
i2c_err_t i2c_master_bus_rm_device(i2c_master_dev_handle_t device);
i2c_err_t i2c_master_transmit(i2c_master_dev_handle_t device, const uint8_t *data, size_t size);
i2c_err_t i2c_master_transmitv(i2c_master_dev_handle_t device, const i2c_buffer_t *buffers, size_t count);
i2c_err_t i2c_master_receive(i2c_master_dev_handle_t device, uint8_t *data, size_t size);
i2c_err_t i2c_master_transmit_receive(i2c_master_dev_handle_t device, const uint8_t *tx_data,
                                       size_t tx_size, uint8_t *rx_data, size_t rx_size);

#ifdef __cplusplus
}
#endif

#endif
