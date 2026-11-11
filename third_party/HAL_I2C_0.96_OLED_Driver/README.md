# OLED 驱动（多实例）

来源：`lczhuigz/HAL_I2C_0.96_OLED_Driver`，提交 `fdb276408b26171692c0dc5deeff1d393b2de2d9`。保留上游四个驱动文件及 `License`（GPL-3.0）。

上游软件 I2C 已移除。驱动不依赖 GD32 或 FreeRTOS，通过 `OLED_RegisterCallbacks()` 注册实例级回调：`write(context, control, data, size)` 在**同一笔 I2C 写事务**内发送控制字节和数据（`control=0x00` 为命令，`0x40` 为显存数据），成功返回 `true`；可选的 `delay_ms(context, milliseconds)` 用于 `OLED_Init()` 的上电等待。`context` 由调用者维护，必须在句柄使用期间有效。

每块屏幕使用独立的 `OLED_Handle`（包含 1024 字节显存）、独立设备地址及回调上下文；初始化前注册回调，注册会清空该实例显存。所有绘图函数首参数为句柄；绘图后调用 `OLED_Update(&oled)` 或 `OLED_UpdateArea(&oled, ...)` 才写入屏幕。`OLED_Init`、`OLED_Update`、`OLED_UpdateArea` 返回 `bool`，写失败立即停止刷新；绘制同一实例的多个任务需由应用自行同步。不能在中断中调用硬件 I2C 回调。

`App/Task/screen.cpp` 示范与 `Lib/i2c` 结合：I2C0 重映射到 PB8/PB9、100 kHz、SSD1306 地址 `0x3C`。OLED 的 SCL/SDA 需要上拉（模块自带或外接）。更换地址应修改应用的设备配置，无需改动驱动源码；要连接另一块屏幕，创建另一设备和 OLED 句柄并分别注册回调。
