<div align="center">

<img src="docs/images/logo.png" width="96" height="96" alt="GameScrcpy">

# GameScrcpy (macOS)

**在 macOS 上投屏控制安卓手机，支持蓝图与 TypeScript 自动化（移植进行中）**

[![CI](https://img.shields.io/github/actions/workflow/status/wyb-070110/GameScrcpy-macos/macos.yml?style=flat-square&label=macOS%20CI)](https://github.com/wyb-070110/GameScrcpy-macos/actions/workflows/macos.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/macOS%2014%2B-Apple%20Silicon-000000?style=flat-square)](#系统要求)

简体中文 · [English](README.en.md)

[移植进度](docs/macos-port.md) · [使用帮助](docs/README.md) · [更新日志](CHANGELOG.md) · [上游项目](https://github.com/NuoRan/GameScrcpy)

<img src="docs/images/keymap-editor.png" width="860" alt="在游戏画面上直接摆放键位">

</div>

本仓库是 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) 的 **Apple Silicon macOS 移植版（进行中）**。GameScrcpy 把安卓手机的画面低延迟地投到电脑上，支持在游戏画面上直接摆放键位，并用蓝图（节点和连线）或 TypeScript 编写识图、找色、OCR 与定时任务逻辑。手机端服务、通信协议和桌面端思路源自 [scrcpy](https://github.com/Genymobile/scrcpy)。

## 功能与 macOS 移植状态

### 已支持

- **低延迟画面与声音**：VideoToolbox 硬件解码（H.264、H.265，AV1 由 dav1d 解码）与 Qt RHI（Metal）直接绘制，支持 NV12、P010 与 YUV 4:2:0；SDL3 播放手机声音（Android 11 及以上）。
- **多种连接**：USB 数据线；无线调试（配对码或扫码配对）；WiFi 直连（UDP 加 Reed-Solomon 纠删码，断线自动续连）；手机伴侣应用经 BSD 套接字与 Bonjour 发现连接。
- **多台手机与管理**：独立投屏窗口或宫格同屏、触控板单指拖拽滑动、录像与 MP4 导出、文件管理、应用管理、独立虚拟屏幕启动应用、真实端到端延迟测量。
- **蓝图、脚本与 OCR**：蓝图与 TypeScript 双向转换，脚本运行在 `sandbox_init` 隔离的 `gs-scripthost` 进程中；接入 macOS Vision 框架实现中英文等文字识别；内置命令行工具 `gs`。
- **AI 与本地 MCP**：AI 小助手密钥保存在 macOS 钥匙串（Keychain）中；本地 MCP 服务通过 Unix 套接字连接 Claude、Cursor 等工具。
- **扩展系统**：支持 `.gsext` / `.gsbundle` 扩展包与 `.dylib` 原生扩展（Mach-O 导入检查、独立 `gs-exthost` 进程与 macOS 沙箱隔离），支持 `gamescrcpy://` 链接唤起。

### 尚未完成

- **全局键鼠与手柄捕获**：`InputCapture` 目前在 macOS 上仍为占位实现（投屏窗口内鼠标点击与触控板滑动可用，键位映射的全局键盘/鼠标锁定与手柄输入待接入）。
- **USB / AOA 与采集卡**：USB 插拔监听、AOA 触控通道、HDMI 采集卡与扩展系统级硬件枚举尚未移植。
- **扩展画面滤镜与语音命令**：Metal 渲染管线暂未接入扩展着色器滤镜；语音朗读已接入 macOS，语音命令识别尚未移植。

详见 [docs/macos-port.md](docs/macos-port.md)。

## 系统要求

| 项目 | 要求 |
|:--|:--|
| 电脑 |搭载 Apple Silicon（M 系列芯片）的 Mac，macOS 14 Sonoma 或更高版本 |
| 手机 | Android 9 或更高版本；声音与无线配对需要 Android 11 或更高版本 |
| 连接 | USB 数据线，或与 Mac 在同一局域网的 Wi-Fi / 有线网络 |

## 从源码构建

### 1. 安装依赖

```bash
xcode-select --install
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
```

同时通过 Qt 官方安装器或 `aqtinstall` 安装 **Qt 6.10.3（macOS）**，默认路径为 `~/Qt/6.10.3/macos`（若安装在其他位置，可通过 `-DCMAKE_PREFIX_PATH=<路径>` 指定）。

### 2. 编译与运行

```bash
# 构建完整 macOS 应用（GameScrcpy.app）
cmake --preset mac-app
cmake --build --preset mac-app

# 不连手机直接体验演示画面
./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --demo-device tests/data/video/emulator_h265.media
```

仅编译无 Qt 依赖的核心库与单元测试：

```bash
cmake --workflow --preset posix-core
```

### 3. 首次运行权限说明

- **本地网络权限（macOS 15 及以上）**：首次使用无线调试配对、WiFi 直连或手机伴侣时，macOS 会询问是否允许查找本地网络上的设备，请点击“允许”（也可在“系统设置 > 隐私与安全性 > 本地网络”中开启 `GameScrcpy`）。
- **配置与日志位置**：设置、键位方案、扩展与日志保存在 `~/Library/Application Support/GameScrcpy`。

## 快速开始

1. 在手机上打开“开发者选项”和“USB 调试”（或“无线调试”）。
2. 用数据线连接手机并在手机上允许调试，或在“设备”页通过无线调试配对连接。
3. 在“设备”页点击“投屏”。
4. 按 `F1` 打开内置图文帮助，查看投屏窗口、键位编辑器、方案、蓝图脚本与扩展说明。

## 文档

| 内容 | 位置 |
|:--|:--|
| macOS 移植进度与真机验收记录 | [docs/macos-port.md](docs/macos-port.md) |
| 使用帮助（与程序里 F1 打开的是同一份） | [docs/README.md](docs/README.md) |
| 脚本接口 | [sdk/docs](sdk/docs/README.md) |
| 扩展开发 | [docs/extensions](docs/extensions/README.md) |
| 架构概览 | [docs/architecture.md](docs/architecture.md) |
| 通信协议 | [protocol/SPEC.md](protocol/SPEC.md) |
| 参与开发规范 | [CONTRIBUTING.md](CONTRIBUTING.md) |

## 反馈与交流

- macOS 移植问题：[GameScrcpy-macos Issues](https://github.com/wyb-070110/GameScrcpy-macos/issues)（附上程序里“设置 > 关于 > 导出诊断包”生成的文件能更快定位问题）。
- 上游项目：[NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)。
- 安全问题请按 [SECURITY.md](SECURITY.md) 私下报告。

## 许可证与致谢

GameScrcpy 以 [Apache License 2.0](LICENSE) 发布。

- [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy)：本项目所基于的上游原版。
- [scrcpy](https://github.com/Genymobile/scrcpy)：安卓投屏的先驱，GameScrcpy 的思路源自它。
- [QtScrcpy](https://github.com/barry-ran/QtScrcpy)：GameScrcpy 1.x 的基础。
- 用到的第三方组件（Qt、FFmpeg、dav1d、SDL、libusb、QuickJS-ng、TypeScript、QWindowKit、nlohmann/json、miniz、QR Code generator、Lucide、doctest、tl::expected）及其版本与许可证见 [third_party/README.md](third_party/README.md)，程序里在“设置 > 关于 > 开源许可”。
