#include "screen.h"

#include "FreeRTOS.h"
#include "OLED.h"
#include "i2c.h"
#include "task.h"

static OLED_Handle oled;

static bool oled_write(void *context, uint8_t control, const uint8_t *data, size_t size)
{
    const i2c_buffer_t buffers[] = {{&control, 1U}, {data, size}};
    return i2c_master_transmitv((i2c_master_dev_handle_t)context, buffers, 2U) == I2C_OK;
}

static void oled_delay(void *context, uint32_t milliseconds)
{
    (void)context;
    vTaskDelay(pdMS_TO_TICKS(milliseconds));
}

void ScreenTask(void *param)
{
    i2c_master_bus_handle_t bus;
    i2c_master_dev_handle_t device;
    const i2c_master_bus_config_t bus_config = {I2C_PORT_0, 100000U};
    const i2c_device_config_t device_config = {0x3CU, 100U};
    const OLED_Callbacks callbacks = {oled_write, oled_delay};
    TickType_t previous_wake_time;

    (void)param;
    if (i2c_new_master_bus(&bus_config, &bus) != I2C_OK) {
        vTaskDelete(NULL);
        return;
    }

    if (i2c_master_bus_add_device(bus, &device_config, &device) != I2C_OK) {
        i2c_del_master_bus(bus);
        vTaskDelete(NULL);
        return;
    }

    if (!OLED_RegisterCallbacks(&oled, &callbacks, device) || !OLED_Init(&oled)) {
        i2c_master_bus_rm_device(device);
        i2c_del_master_bus(bus);
        vTaskDelete(NULL);
        return;
    }

    OLED_ShowString(&oled, 0, 0, "GD32 OLED", OLED_8X16);
    if (!OLED_Update(&oled)) {
        i2c_master_bus_rm_device(device);
        i2c_del_master_bus(bus);
        vTaskDelete(NULL);
        return;
    }

    previous_wake_time = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&previous_wake_time, pdMS_TO_TICKS(50U));
    }
}
