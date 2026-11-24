# GameScrcpy macOS 版更新说明草稿（G7）

> **日期**：2026-11-24
> **结论**：本草稿基于本分支自 macOS 移植启动（`970f851`）至最新提交（`2015ae7`）的完整变更整理，面向最终用户（中英双语），隐藏内部实现与重构细节，标注了 macOS 首个测试版已可用能力与当前已知限制。

---

## 中文版（Release Notes Draft）

### GameScrcpy macOS 预览版更新说明

GameScrcpy 现已登陆 macOS！你可以直接在 Mac 上低延迟投屏安卓手机，并使用键盘、鼠标、触控板与手柄畅玩手游或管理设备。

#### 新增与亮点
- **macOS 原生投屏与渲染**：支持在 macOS 上通过硬件加速进行低延迟解码与画面渲染，支持 H.264、H.265 与 AV1。
- **有线与无线连接**：支持 USB 数据线连接与 Android 11+ 无线调试（配对码 / 二维码配对），自动查找已安装的 `adb`。
- **键鼠与手柄映射**：
  - 在投屏窗口内直接响应键盘与鼠标映射，**无需授予系统“输入监控”权限**。
  - 原生支持 Xbox、PlayStation（PS4 / PS5）手柄及震动反馈。
  - 支持 Mac 触控板双指滑动直接操作手机屏幕。
- **无打扰电脑打字**：首次投屏自动安装手机端 **GameScrcpy Companion** 输入法，投屏期间可直接用 Mac 键盘向手机输入文字，手机自带软键盘不再弹出遮挡画面。
- **macOS 系统深度适配**：
  - 采用原生透明标题栏，支持标题栏拖拽移动窗口，并适配 macOS 系统字体。
  - 投屏期间自动防止系统休眠，支持菜单栏图标、系统通知与朗读功能。
  - AI 助手密钥安全存储在 macOS 钥匙串（Keychain）中。
- **界面与工具增强**：
  - 新增**工具栏停靠位置**设置，支持将投屏工具栏固定在窗口的**上、下、左、右**任意一侧。
  - 支持录像与 MP4 导出、蓝图与 TypeScript 自动化脚本（内置 `gs` 命令行工具与脚本沙盒），以及 macOS 原生扩展（`.dylib`）与本地 MCP 服务。
  - 移除了应用内的赞助入口。

#### 已知限制（开发中）
- **无线调试 WiFi 直连**：部分环境下可能出现“手机没有回应 WiFi 直连”，可先切换为 **adb 隧道** 正常使用。
- **扩展画面滤镜**：目前暂未在 macOS 上启用，正在推进 Metal 适配。
- **部分硬件与语音能力**：语音命令、AOA/触控盒 USB 监控与采集卡功能仍在移植完善中。

---

## English Version (Release Notes Draft)

### GameScrcpy for macOS (Preview) Release Notes

GameScrcpy is now available on macOS, bringing low-latency Android screen mirroring with keyboard, mouse, trackpad, and gamepad controls to your Mac.

#### What's New
- **Native macOS Mirroring**: Hardware-accelerated video decoding and Metal rendering with support for H.264, H.265, and AV1.
- **USB & Wireless Debugging**: Connect over USB or pair wirelessly on Android 11+ (pairing code or QR code), with automatic `adb` discovery.
- **Keyboard, Mouse, Trackpad & Gamepad Controls**:
  - Key and mouse mapping works directly inside the mirror window — **no macOS Input Monitoring permission required**.
  - Supports Xbox and PlayStation (PS4/PS5) controllers with rumble.
  - Mac trackpad two-finger swipes map directly to touch gestures on the phone.
- **Seamless Desktop Typing**: Automatically installs the **GameScrcpy Companion** IME on first mirror so you can type with your Mac keyboard without the phone's on-screen keyboard popping up.
- **macOS Integration**:
  - Native transparent title bar with window dragging and native macOS typography.
  - Prevents system sleep while mirroring, with menu bar controls, system notifications, speech output, and macOS Keychain storage for AI API keys.
- **Interface & Tools**:
  - Added **toolbar position** options so you can dock the mirror toolbar to the **top, bottom, left, or right** edge.
  - Includes screen recording with MP4 export, Blueprints and TypeScript automation (with bundled `gs` CLI and sandboxed execution), macOS native extensions (`.dylib`), and local MCP support.
  - Removed the in-app sponsoring entry.

#### Known Limitations (Work in Progress)
- **WiFi Direct over Wireless Debugging**: May report `DirectUnreachable` in some network setups; switch the connection mode to **adb tunnel** as a workaround.
- **Extension Picture Filters**: Not yet enabled on macOS (Metal support is in progress).
- **Hardware & Voice Features**: Voice commands, USB/AOA/touch-box watchers, and capture card support are still being ported.
