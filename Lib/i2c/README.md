# GD32F30x 硬件 I2C 主机

借鉴 ESP-IDF 的总线 / 设备两级句柄结构。`i2c_new_master_bus()` 创建总线，`i2c_master_bus_add_device()` 为不同的 7 位地址创建设备句柄；各设备可配置独立超时。使用后先 `i2c_master_bus_rm_device()`，再 `i2c_del_master_bus()`。同一总线上的传输由 FreeRTOS mutex 串行化；`transmitv` 将多个缓冲区组成**同一笔 START → 地址 → 数据 → STOP**事务，适用于 SSD1306 控制字节 + 数据。

支持 I2C0（重映射 PB8=SCL、PB9=SDA）和 I2C1（PB10=SCL、PB11=SDA），100 kHz / 400 kHz 范围内由 `clock_speed_hz` 设定，使用开漏复用输出，线路须有上拉。I2C0 的默认 PB6/PB7 当前用于 USART0，不受此驱动影响。传输是轮询方式，使用任务 tick 限时并在地址 NACK、总线错误、超时后返回错误，不在中断中使用。`timeout_ms` 分别用于等待互斥锁及一笔总线事务；持续被外部设备拉低的总线可能需要外部断电或 GPIO 解锁，不会无限等待。

```c
i2c_master_bus_handle_t bus;
i2c_master_dev_handle_t device;
i2c_master_bus_config_t bus_config = {I2C_PORT_0, 100000};
i2c_device_config_t device_config = {0x3C, 100};
i2c_new_master_bus(&bus_config, &bus);
i2c_master_bus_add_device(bus, &device_config, &device);
uint8_t command[] = {0x00, 0xAE};
i2c_err_t result = i2c_master_transmit(device, command, sizeof(command));
```

接口还提供 `i2c_master_receive()` 和重复起始的 `i2c_master_transmit_receive()`（发寄存器地址后读数据）。总线/设备只能在调度器启动后的任务中创建和使用；删除设备之前必须确保没有其他任务正在调用该设备，关闭总线之前必须释放所有设备。错误状态见 `i2c.h`，调用方应检查返回值。
