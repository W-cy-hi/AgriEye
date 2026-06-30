# STM32 固件 - 传感器采集与本地显示

## 开发环境
- **MCU**: STM32F103C8T6
- **IDE**: Keil MDK 5 或 STM32CubeIDE
- **库**: 正点原子 SYSTM 库

## 硬件连接

| 外设 | 引脚 |
|------|------|
| DHT22 | PA0 |
| BH1750 | I2C1 (PB6 SCL, PB7 SDA) |
| TDS 传感器 | PA1 (ADC) |
| LCD 显示屏 | FSMC 接口 |
| KEY0 | PE4 |
| UART1 (与 ESP32 通信) | PA9 (TX), PA10 (RX) |

## 编译与烧录

1. 使用 Keil 打开工程文件
2. 选择芯片型号：STM32F103C8
3. 编译（F7）
4. 使用 ST-Link 烧录（F8）

## 串口协议

### STM32 → ESP32 发送数据格式
