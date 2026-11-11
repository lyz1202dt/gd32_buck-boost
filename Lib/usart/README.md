# USART0 DMA 驱动

接口参照 ESP-IDF UART 驱动的「配置 → 安装 → 读写 → 删除」形式。ESP-IDF 使用端口号定位已安装的驱动；本项目额外返回不透明的 `usart_handle_t *` 句柄，后续接口必须传入此句柄。当前仅支持 USART0，板级引脚固定为 PB6 (TX)、PB7 (RX)，DMA0 CH3/CH4 分别用于发送/接收。

```c
const usart_config_t config = {
    .baud_rate = 115200U,
    .data_bits = 8U,
    .parity = USART_PARITY_NONE,
    .stop_bits = USART_STOP_BITS_1,
};
usart_handle_t *handle = NULL;

if (!usart_driver_install(USART_PORT_0, &config, 1024U, &handle)) {
    return;
}

uint8_t data[64];
size_t received = usart_read_bytes(handle, data, sizeof(data), pdMS_TO_TICKS(100));
size_t sent = usart_write_bytes(handle, data, received);

size_t buffered = 0U;
bool valid = usart_get_buffered_data_len(handle, &buffered);
uint32_t dropped = usart_get_dropped_bytes(handle);

usart_driver_delete(handle);
```

本工程已在 `init_task` 安装驱动，并将句柄作为 `app_main(void *param)` 的参数传入；使用时可转换为 `usart_handle_t *`。安装/删除只能在任务中操作；删除前必须确保没有任务正在读写或将继续使用该句柄。`usart_driver_install` 对无效配置、重复安装或分配失败返回 `false`，并将输出句柄设为 `NULL`。数据位目前支持 8，校验支持无/奇/偶，停止位支持 1/2。

`usart_read_bytes` 只返回当前可读的数据，可能少于请求长度；超时为 FreeRTOS tick，仅允许一个任务读取。`usart_write_bytes` 支持多任务调用，并阻塞到 DMA 取走所有数据，返回发送给 DMA 的字节数；返回时最后一个字节可能仍在 USART 硬件中。两个读写接口均不得在中断中调用。接收流缓冲的大小由安装参数指定（至少 256 字节）；缓冲已满时丢弃新字节，并可查询累计丢弃数。`usart_get_buffered_data_len` 只计入已从 DMA 转入流缓冲的数据，不含正在接收的 DMA 数据。

接收采用 DMA 满缓冲及 USART 空闲中断，发送采用 DMA 完成中断。中断入口已接到 `usart_rx_idle_irq`、`usart_rx_dma_irq`、`usart_tx_dma_irq`。使用 FreeRTOS ISR API，安装前需将 NVIC 设置为 `NVIC_PRIGROUP_PRE4_SUB0`；本工程已设置，中断优先级为 5。
