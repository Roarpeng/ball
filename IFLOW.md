# iFlow CLI 项目说明

## 项目概述

这是一个基于 PlatformIO 的 ESP32 项目，项目名为 "ball"。该项目使用 Arduino 框架开发，目标硬件平台为 ESP32 开发板（esp32dev）。目前项目包含一个基本的 PlatformIO 模板代码，实现了简单的加法函数。

## 项目结构

```
ball/
├── .gitignore            # Git 忽略文件配置
├── platformio.ini        # PlatformIO 项目配置文件
├── .pio/                 # PlatformIO 构建输出目录
├── .vscode/              # VSCode 配置文件
│   ├── c_cpp_properties.json
│   ├── extensions.json
│   └── launch.json
├── include/              # 项目头文件目录
│   └── README
├── lib/                  # 项目私有库目录
│   └── README
├── src/                  # 源代码目录
│   └── main.cpp          # 主程序文件
└── test/                 # 测试文件目录
    └── README
```

## 配置详情

### platformio.ini
- 平台：Espressif 32 (ESP32)
- 开发板：esp32dev
- 框架：Arduino

### main.cpp
- 包含基本的 Arduino 代码结构（setup 和 loop 函数）
- 实现了一个简单的 myFunction 函数，执行两个整数相加
- 目前代码功能非常基础，作为项目模板

## 开发约定

### 架构
- 使用 Arduino 框架进行 ESP32 开发
- 遵循 Arduino 标准的 setup/loop 架构模式

### 代码结构
- 主要源代码放在 `src/` 目录
- 头文件放在 `include/` 目录
- 项目特定库放在 `lib/` 目录
- 测试代码放在 `test/` 目录

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

- 推荐使用 PlatformIO IDE（VSCode 扩展）
- 已配置调试环境（launch.json）
- 包含 C/C++ 智能提示配置（c_cpp_properties.json）

## 测试

- 测试框架：PlatformIO Unit Testing
- 测试文件应放在 `test/` 目录
- 可使用 `pio test` 命令运行单元测试

## 项目用途推测

从项目名称 "ball" 推测，这可能是一个与球体相关的项目，可能涉及：
- 球体追踪/检测
- 球体运动控制
- 传感器数据处理
- 物理仿真

但目前的代码仅为模板，具体功能需要进一步开发。