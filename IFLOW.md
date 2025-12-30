# iFlow CLI 项目说明

## 项目概述

这是一个基于 PlatformIO 的 ESP32 智能球体控制系统，项目名为 "ball"。该项目使用 Arduino 框架开发，目标硬件平台为 ESP32 开发板（esp32dev）。项目实现了完整的LED灯带控制、多路按钮状态监控、MQTT通信、WebUI界面和OTA固件升级功能。

## 项目结构

```
ball/
├── .gitignore            # Git 忽略文件配置
├── IFLOW.md              # iFlow CLI 项目说明文件
├── platformio.ini        # PlatformIO 项目配置文件
├── README.md             # 项目详细说明文档
├── .git/                 # Git 版本控制目录
├── .pio/                 # PlatformIO 构建输出目录
├── .vscode/              # VSCode 配置文件
│   └── extensions.json   # VSCode 扩展配置
├── include/              # 项目头文件目录
│   ├── config.h          # 系统配置和常量定义
│   └── README
├── lib/                  # 项目私有库目录
│   └── README
├── src/                  # 源代码目录
│   ├── main.cpp          # 主程序文件
│   ├── config.cpp        # 配置文件实现
│   └── README
└── test/                 # 测试文件目录
    └── README
```

## 配置详情

### platformio.ini
- **平台**：Espressif 32 (ESP32)
- **开发板**：esp32dev
- **框架**：Arduino
- **依赖库**：
  - `fastled/FastLED@^3.6.0`：WS2812 LED控制库
  - `knolleary/PubSubClient@^2.8.0`：MQTT客户端库
  - `esphome/ESPAsyncWebServer-esphome@^2.0.0`：异步Web服务器
  - `esphome/AsyncTCP-esphome@^1.1.1`：异步TCP支持
- **串口速度**：115200

### main.cpp
- **核心功能**：完整的智能球体控制系统
- **主要模块**：
  - WiFi连接管理
  - MQTT通信处理
  - 7路按钮状态监控
  - WS2812 LED灯带控制（144个LED）
  - 异步Web服务器
  - WebSocket实时通信
  - OTA固件升级
- **LED效果**：红色呼吸、绿色呼吸、黄色频闪、关闭状态
- **按钮逻辑**：支持单按钮和组合按钮触发不同效果

### config.h / config.cpp
- **硬件配置**：LED引脚、按钮引脚定义
- **网络配置**：WiFi和MQTT服务器配置
- **颜色配置**：RGB颜色值定义
- **时间配置**：消抖延迟、更新间隔等
- **数据结构**：按钮状态、LED控制器、系统状态结构体

## 核心功能

### 🎨 LED灯带控制
- **WS2812灯带驱动**：支持144个LED灯珠的控制
- **多种显示效果**：
  - 红色呼吸：默认状态和P32触发状态
  - 绿色呼吸：P12、P14、P25、P26、P27组合触发（亮度根据按下按钮数量调整）
  - 黄色频闪：P13单独触发
  - 关闭状态：LED关闭模式

### 🎮 按钮状态监控
- **7路数字输入**：P13、P12、P14、P27、P26、P25、P32
- **消抖处理**：50ms消抖延迟，确保信号稳定性
- **实时状态检测**：支持多按钮组合状态判断
- **优先级逻辑**：P13 > P32 > 绿色组合 > 默认状态

### 📡 MQTT通信
- **服务器连接**：192.168.10.80:1883
- **主题订阅**：ball/triggered、ball/firstTriggered
- **消息发布**：
  - `ball/triggered`：当5个绿色按钮同时触发时发送
  - `ball/firstTriggered`：首次触发时发送
  - `btn/resetAll`：P32按钮触发时发送重置信号

### 🌐 WebUI界面
- **实时监控**：WebSocket实时显示所有按钮状态
- **响应式设计**：支持PC和移动设备访问
- **状态可视化**：按钮状态用不同颜色标识
- **系统状态显示**：WiFi、MQTT连接状态
- **OTA升级界面**：通过Web界面上传固件

### 🔄 OTA升级
- **Web界面升级**：通过浏览器上传.bin固件文件
- **自动重启**：升级完成后设备自动重启
- **进度反馈**：实时显示升级状态

## 硬件配置

### 引脚定义
| 功能 | 引脚 | 说明 |
|------|------|------|
| WS2812数据 | GPIO23 | LED灯带控制信号 |
| 按钮输入 | GPIO13 | P13按钮（黄色频闪） |
| 按钮输入 | GPIO12 | P12按钮（绿色组） |
| 按钮输入 | GPIO14 | P14按钮（绿色组） |
| 按钮输入 | GPIO27 | P27按钮（绿色组） |
| 按钮输入 | GPIO26 | P26按钮（绿色组） |
| 按钮输入 | GPIO25 | P25按钮（绿色组） |
| 按钮输入 | GPIO32 | P32按钮（红色呼吸/重置） |

### 硬件连接
- WS2812灯带数据线连接至GPIO23
- 所有按钮引脚配置为输入模式，启用内部上拉电阻
- 按钮触发逻辑：低电平触发（按下时为LOW）

## 软件架构

### 依赖库
- `FastLED@^3.6.0`：高性能WS2812 LED控制库
- `PubSubClient@^2.8.0`：轻量级MQTT客户端
- `ESPAsyncWebServer-esphome@^2.0.0`：异步Web服务器
- `AsyncTCP-esphome@^1.1.1`：异步TCP支持库

### 核心模块
1. **WiFi连接模块**：自动连接配置的WiFi网络
2. **MQTT通信模块**：处理MQTT连接和消息收发
3. **LED控制模块**：管理LED灯带的各种显示效果
4. **按钮检测模块**：多路按钮状态监测和消抖处理
5. **Web服务器模块**：提供HTTP服务和WebSocket通信
6. **OTA升级模块**：处理固件在线升级

### 系统状态管理
- **SystemStatus结构体**：跟踪WiFi、MQTT连接状态和按钮触发状态
- **LEDController结构体**：管理LED模式、亮度、动画状态
- **ButtonState数组**：存储7个按钮的状态信息

## 构建和运行

### 构建项目
```bash
pio run
```

### 上传代码到 ESP32
```bash
pio run --target upload
```

### 清理构建文件
```bash
pio run --target clean
```

### 监视串口输出
```bash
pio device monitor
```

### 调试
项目配置了 PlatformIO 调试支持，可通过 VSCode 的调试功能进行调试。

## 开发环境

- **推荐IDE**：PlatformIO IDE（VSCode 扩展）
- **已配置调试环境**：launch.json
- **C/C++智能提示**：c_cpp_properties.json 配置

## 测试

- **测试框架**：PlatformIO Unit Testing
- **测试文件位置**：test/ 目录
- **测试命令**：pio test

## 配置说明

### WiFi配置
在 `config.cpp` 中修改：
```cpp
const char* WIFI_SSID = "YOUR_WIFI_SSID";
const char* WIFI_PASSWORD = "YOUR_WIFI_PASSWORD";
```

### MQTT配置
在 `config.cpp` 中修改：
```cpp
const char* MQTT_SERVER = "192.168.10.80";
const uint16_t MQTT_PORT = 1883;
const char* MQTT_USER = "ball";
```

### LED配置
在 `config.h` 中修改：
```cpp
#define LED_PIN 23
#define NUM_LEDS 144
#define LED_BRIGHTNESS 50
```

## 使用场景

### 按钮组合逻辑
- **P13单独按下**：触发黄色频闪效果（错误/警告状态）
- **P32按下**：强制红色呼吸效果并发送重置信号
- **绿色按钮组合**（P12+P14+P25+P26+P27）：
  - 部分按下：绿色呼吸，亮度根据按下数量调整
  - 全部按下：最大亮度绿色呼吸并发送触发消息

### WebUI访问
1. 设备启动后自动连接WiFi
2. 串口监视器显示设备IP地址
3. 浏览器访问 `http://[设备IP地址]` 打开控制面板

## 故障排除

### 常见问题
1. **WiFi连接失败**：检查WiFi凭据和信号强度
2. **MQTT连接失败**：检查服务器地址和网络连通性
3. **LED不亮**：检查GPIO23连接和LED供电
4. **WebUI无法访问**：检查设备IP和网络连通性

### 调试方法
- 使用串口监视器查看实时日志
- 通过WebUI监控按钮状态
- 检查硬件连接和配置参数

## 开发扩展

### 添加新功能
1. 在 `config.h` 中定义新的配置参数
2. 在 `setup()` 函数中初始化新功能
3. 在 `loop()` 函数中添加主循环逻辑
4. 更新WebUI界面以支持新功能

### 代码结构约定
- **初始化函数**：`initializeXXX()` 格式
- **更新函数**：`updateXXX()` 格式
- **处理函数**：`processXXX()` 或 `handleXXX()` 格式
- **常量定义**：使用 `#define` 或 `const`
- **枚举类型**：用于状态和模式定义

## 项目特色

1. **模块化设计**：代码结构清晰，功能模块分离
2. **实时响应**：WebSocket实现毫秒级状态更新
3. **稳定可靠**：完善的消抖处理和错误恢复机制
4. **易于维护**：配置集中管理，便于定制和扩展
5. **用户友好**：提供直观的WebUI界面

## 许可证和贡献

本项目采用开源许可证，欢迎提交Issue和Pull Request来改进项目功能。