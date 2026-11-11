# OLED 驱动（GD32 移植）

来源：`lczhuigz/HAL_I2C_0.96_OLED_Driver`，提交 `fdb276408b26171692c0dc5deeff1d393b2de2d9`。
保留上游 `Hardware/Inc` 和 `Hardware/Src` 的四个驱动文件及 `License`（GPL-3.0）；未引入 STM32 示例工程和 HAL。

移植修改：用 GD32 标准外设库控制 PB8（SCL）、PB9（SDA）的开漏 GPIO，并使用 FreeRTOS 等待 OLED 上电；保留上游软件 I2C 和 SSD1306 绘图 API。头文件补充 C++ 链接支持，字符串接口接受常量字符串，`OLED_Printf` 使用有界格式化。

接线：OLED SCL → PB8、SDA → PB9、VCC → 3.3 V、GND → GND；SCL/SDA 需要上拉（模块自带或外接），设备使用上游固定的 7 位 I2C 地址 `0x3C`。必须在 FreeRTOS 调度器启动后从任务上下文调用 `OLED_Init()`，不要从中断调用；绘图后调用 `OLED_Update()` 才会显示。示例入口在 `App/Task/app_main.cpp`。如需更换引脚或地址，修改 `Src/OLED.c` 中对应配置。
