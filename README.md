<div align="center">

# 🌾 AgriEye · 智农云眼

**一套硬件成本 < 500 元的开源智慧大棚监控系统**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform](https://img.shields.io/badge/Platform-ESP32%20%7C%20STM32-blue)](https://github.com)
[![Version](https://img.shields.io/badge/Version-v1.0.0-green)](https://github.com)

</div>

---

## 📖 项目简介

智农云眼（AgriEye）是一套面向**中小农户**的智慧大棚环境监控系统，旨在以极低成本解决传统大棚管理的痛点。

### 我们解决了什么问题？

| 传统痛点 | 我们的方案 |
|----------|------------|
| 💸 市面系统动辄几千上万元 | ✅ 硬件总成本 **< 500 元** |
| 🔧 操作复杂，需要专业人员维护 | ✅ **零安装**，浏览器扫码即用 |
| 📊 数据只看当下，没有智能预警 | ✅ **AI + 规则双引擎**，智能决策 |
| 🏠 只看棚内，忽略室外环境 | ✅ **室内外联动分析**，科学全面 |

---

## ✨ 核心特性

| 特性 | 说明 |
|------|------|
| 💰 **极致低成本** | BOM 总价 < 500 元，附完整物料清单 |
| 🧠 **AI + 规则双引擎** | 规则引擎实时阈值响应 + AI 引擎异常趋势预测 |
| 🔗 **室内外联动分析** | 对比棚内传感器与室外天气数据，科学决策 |
| 🔌 **双 MCU 异构架构** | ESP32（联网/云端）+ STM32（采集/容错） |
| 📱 **零安装远程访问** | 手机/电脑/平板浏览器直接打开，无需 App |
| 🌐 **Web 实时仪表盘** | 响应式设计，数据可视化大屏 |

---

## 🏗️ 系统架构图
┌─────────────────────────────────────────────────────┐
│ 用户访问层 │
│ 手机浏览器 │ 电脑浏览器 │ 平板浏览器 │
└─────────────────────┬───────────────────────────────┘
│ HTTPS
┌─────────────────────▼───────────────────────────────┐
│ 云端服务层 │
│ 数据存储 │ AI 推理 │ Web 服务器 │
└─────────────────────┬───────────────────────────────┘
│ MQTT / WebSocket
┌─────────────────────▼───────────────────────────────┐
│ ESP32（主控 + 联网） │
│ WiFi 通信 │ 数据上传 │
└─────────────────────┬───────────────────────────────┘
│ UART 串口通信
┌─────────────────────▼───────────────────────────────┐
│ STM32（采集 + 容错） │
│ 温湿度 │ 光照 │ CO₂ │ 土壤湿度 │
└─────────────────────────────────────────────────────┘

### 为什么用双 MCU？

- **ESP32**：负责 WiFi 联网、Web 服务、云端数据上报
- **STM32**：负责传感器采集、本地规则引擎、硬件容错

> 双芯片各司其职，即使 ESP32 网络异常，STM32 仍能独立运行规则引擎，保障大棚安全。

---

## 🚀 快速上手

### 1. 硬件清单

| 组件 | 型号 | 参考价格 |
|------|------|----------|
| 主控（联网） | ESP32 | ¥35 |
| 采集（容错） | STM32F103C8T6 | ¥25 |
| 温湿度传感器 | DHT22 | ¥20 |
| 光照传感器 | BH1750 | ¥15 |
| CO₂ 传感器 | MH-Z19B | ¥80 |
| 土壤湿度传感器 | 电容式（TL555I） | ¥10 |
| 继电器模块 | 4 路 | ¥30 |
| 电源适配器 | 12V / 2A | ¥40 |
| 外壳、杜邦线、PCB 等 | - | ¥150 |
| **总计** | | **< ¥500** |

> 📋 详细 BOM 清单（含购买链接）见 [`hardware/BOM.md`](./hardware/BOM.md)

---

### 2. 环境准备

| 工具 | 用途 |
|------|------|
| Arduino IDE 或 PlatformIO | ESP32 固件烧录 |
| STM32CubeIDE 或 Keil | STM32 固件烧录 |
| ST-Link 下载器 | STM32 程序下载 |
| USB 转 TTL 模块 | 串口调试 |

---

### 3. 部署步骤

```bash
# 1. 克隆项目到本地
git clone https://github.com/W-cy-hi/AgriEye.git

# 2. 进入项目目录
cd AgriEye

# 3. 修改 ESP32 WiFi 配置
# 编辑 firmware/esp32/wifi_config.h，填入你的 WiFi 名称和密码

# 4. 使用 Arduino IDE / PlatformIO 打开 ESP32 工程并烧录

# 5. 使用 STM32CubeIDE 打开 STM32 工程并烧录

# 6. 按 wiring.md 接线图连接各硬件模块

# 7. 上电启动，在浏览器输入 ESP32 的 IP 地址即可访问

📖 完整图文部署指南见 docs/deploy-guide.md

AgriEye/
├── README.md                 # 项目说明（本文件）
├── LICENSE                   # MIT 许可证
├── firmware/
│   ├── esp32/                # ESP32 固件源码
│   │   ├── esp32.ino
│   │   ├── wifi_config.h
│   │   ├── web_server.h
│   │   ├── mqtt_client.h
│   │   └── ...
│   └── stm32/                # STM32 固件源码
│       ├── Core/
│       │   ├── main.c
│       │   └── ...
│       ├── Drivers/
│       │   ├── sensor_drv.c
│       │   ├── rule_engine.c
│       │   └── ...
│       └── ...
├── hardware/
│   ├── BOM.md                # 详细物料清单
│   ├── wiring.md             # 接线图与引脚定义
│   └── schematic/            # 原理图文件
├── docs/
│   ├── deploy-guide.md       # 完整部署指南
│   ├── api.md                # API 接口文档
│   └── ai-model.md           # AI 模型说明
└── images/                   # 展示图片
    ├── dashboard.png         # 仪表盘截图
    ├── hardware.jpg          # 硬件实物照片
    └── architecture.png      # 架构图
🛠️ 技术栈
层级	技术
前端	HTML5 + CSS3 + JavaScript + ECharts
后端（ESP32）	C++ / Arduino Framework
采集端（STM32）	C / HAL 库
通信协议	UART（双 MCU 间）、HTTP/WebSocket（Web 端）、MQTT（云端）
云端（可选）	阿里云 / 腾讯云 IoT Hub
AI 框架	TensorFlow Lite（部署于 ESP32）
🤝 参与贡献
我们欢迎所有形式的贡献！

贡献方式	操作
🐛 报告 Bug	提交 Issue
💡 功能建议	提交 Issue
🔧 提交代码	Fork → 开发 → Pull Request
📖 完善文档	直接修改后提 PR
🌟 分享项目	给个 Star 并推荐给朋友
PR 流程
Fork 本项目

创建你的特性分支：git checkout -b feature/AmazingFeature

提交你的改动：git commit -m 'Add some AmazingFeature'

推送到分支：git push origin feature/AmazingFeature

提交 Pull Request

📄 许可证
本项目采用 MIT License，你可以自由使用、修改、分发，甚至用于商业项目。

https://img.shields.io/badge/License-MIT-yellow.svg

👥 团队
成员	角色	简介
@W-cy-hi	项目负责人 / 嵌入式开发	系统架构、ESP32/STM32 固件开发
欢迎更多志同道合的伙伴加入！感兴趣请通过 Issue 或 PR 联系我们。

📚 相关资源
项目 Wiki（开发中）

演示视频（待上传）

硬件购买清单

⭐ 支持我们
如果这个项目对你有帮助，请给我们一个 Star ⭐ —— 这是对我们最大的鼓励！

也欢迎 Fork 和 分享 给更多有需要的朋友。

<div align="center">
科技的价值在于让生活更美好 🌱

报告 Bug · 提出建议 · Star 项目

</div> ```
