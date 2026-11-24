# GameScrcpy macOS 使用指南（G1 / G4 更新）

> **日期**：2026-11-24
> **结论**：GameScrcpy-macos 目前处于开发测试阶段，需通过 `cmake --preset mac-app` 从源码构建运行。最新进展：键鼠映射改为读取窗口内输入，**不再需要“输入监控”系统授权**；手柄通过 SDL3 原生支持 Xbox / PS4 / PS5 及震动；投屏打字由手机端自动安装的 **GameScrcpy Companion** 输入法接管，手机自带软键盘不再弹出；主界面新增工具栏四边停靠位置（上/下/左/右），并移除了赞助入口。无线调试下若遇 WiFi 直连失败（`DirectUnreachable`），可先切换为 **adb 隧道** 正常投屏。

---

## 一、安装与构建（中文）

### 1. 环境准备

- **系统**：macOS（推荐 Apple Silicon / 支持 Metal 的机型）。
- **工具与依赖**：
  - Xcode Command Line Tools、CMake（≥ 3.28）、Ninja
  - Qt **6.10.3**（默认安装路径 `~/Qt/6.10.3/macos`）
  - 通过 Homebrew 安装运行时依赖与工具：`ffmpeg`、`sdl3`、`libusb`、`pkg-config`、`android-platform-tools`（提供 `adb`）

### 2. 从源码构建与启动

```bash
# 1. 配置与编译
cmake --preset mac-app
cmake --build --preset mac-app

# 2. 启动应用
open build/mac-app/bin/GameScrcpy.app
```

无需连接手机即可用内置演示录像体验投屏界面：

```bash
./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy \
  --demo-device tests/data/video/emulator_h265.media
```

应用设置、方案与日志默认存放在 `~/Library/Application Support/GameScrcpy/`。

> 注：待后续打包脚本（`packaging/macos/`）与公证完成后，将提供直接拖入“应用程序（Applications）”的 `.dmg` 安装包。

---

## 二、首次打开时的系统授权

在 **系统设置 → 隐私与安全性** 中，GameScrcpy 涉及的权限如下：

| 权限 | 用途 | 操作建议 |
|---|---|---|
| **本地网络（Local Network）** | 无线调试配对、WiFi 直连（UDP）与手机伴侣局域网通信 | **必须允许**。首次弹窗请点“允许”；若未弹窗或已拒绝，请前往 **系统设置 → 隐私与安全性 → 本地网络** 开启 `GameScrcpy`。重编 `.app` 后签名变化可能导致权限重置，需重新确认。 |
| **输入监控（Input Monitoring）** | ~~全局键鼠监听~~ | **不再需要**。键鼠映射改为直接读取 GameScrcpy 窗口内的按键与鼠标事件，无需授予“输入监控”权限。 |
| **麦克风与语音识别（Microphone & Speech Recognition）** | 语音命令触发键位或脚本动作 | 仅在使用语音命令功能时需要；首次使用时按系统弹窗分别授予麦克风与语音识别权限（该模块仍在补全中）。 |
| **未签名应用提示（Gatekeeper）** | 源码自编或未公证构建首次启动 | 若系统提示无法验证开发者，可在 **隐私与安全性** 底部点“仍要打开”，或在终端执行 `xattr -dr com.apple.quarantine GameScrcpy.app`（仅限可信本地构建）。 |

---

## 三、USB 与无线调试连接

### 1. USB 有线连接

1. 在手机上打开 **开发者选项** 与 **USB 调试**（部分品牌如小米/OPPO/vivo 需额外开启“USB 调试（安全设置）”或“禁止权限监控”才能模拟点击）。
2. 用数据线将手机连到 Mac，在手机弹窗中勾选“一律允许使用这台计算机进行调试”并确定。
3. 在 GameScrcpy 主界面点击对应设备右侧的 **投屏** 按钮即可开始。首次投屏时会自动在手机上安装 **GameScrcpy Companion**，用于在投屏时接管电脑键盘输入。

### 2. 无线调试连接（Android 11+）

1. 确保手机与 Mac 连在**同一个 Wi-Fi 局域网**（路由器未开启 AP 隔离）。
2. 在手机上进入 **开发者选项 → 无线调试** 并开启。
3. 在 GameScrcpy 中点击无线配对：
   - **配对码配对**：在手机点击“使用配对码配对设备”，将 IP、端口与 6 位配对码填入 GameScrcpy。
   - **二维码配对**：用手机无线调试里的“使用二维码配对设备”扫描界面上的二维码。
4. 配对成功后设备会以 `adb-..._adb-tls-connect._tcp` 出现在列表中，点击 **投屏** 即可。

> **提示**：无线调试默认尝试 **WiFi 直连（UDP）**，若在“验证”阶段提示 **“手机没有回应 WiFi 直连”**，请点下方的 **改用 adb 隧道**，或在设备卡片上切换为 **adb 隧道**。

---

## 四、手柄、打字与界面小贴士

- **手柄支持**：通过 SDL3 支持 **Xbox、PlayStation（PS4 / PS5）** 等主流手柄连接与震动反馈，可直接驱动方案中的键位。
- **电脑键盘打字**：首次投屏会自动在手机安装并启用 **GameScrcpy Companion** 输入法，投屏期间直接在电脑键盘打字即可输入文字，手机自带软键盘不再弹出挡住画面。
- **工具栏位置**：可在设置或投屏窗口中自由调整工具栏停靠位置（**上 / 下 / 左 / 右**）。

---

## 五、常见问题排查

1. **打字时手机键盘还弹出来怎么办？**
   - **原因**：投屏时由手机上的 **GameScrcpy Companion** 输入法接管电脑打字；如果首次自动安装被手机拦截、输入法未启用，或系统切回了默认输入法，点击输入框时手机软键盘就会继续弹出。
   - **解决**：
     1. 检查手机桌面是否已安装 **GameScrcpy Companion**；若未安装，请确认手机开发者选项里已允许 **“USB 安装”**（部分品牌如小米/OPPO/vivo/华为需要手动开启）后重新投屏。
     2. 打开手机 **设置 → 系统（或常规管理）→ 语言与输入法 → 屏幕键盘 / 管理键盘**，勾选启用 **GameScrcpy Companion**。
     3. 确认在 GameScrcpy 手机伴侣设置中已开启输入法接管开关；若仍弹出，可在输入时下拉手机通知栏点击“选择输入法”，手动切换到 **GameScrcpy Companion**。
2. **提示“手机没有回应 WiFi 直连”（`DirectUnreachable`）**
   - **原因**：UDP 握手超时（当前仅 1500 ms），可能由手机 Wi-Fi 休眠唤醒延迟、macOS 本地网络权限拦截或路由器 AP 隔离引起。
   - **解决**：先切到 **adb 隧道** 正常使用；确认 **系统设置 → 隐私与安全性 → 本地网络** 已允许 `GameScrcpy`；保持手机亮屏后再重试 WiFi 直连。排查日志见 `~/Library/Application Support/GameScrcpy/logs/GameScrcpy.log`。
3. **找不到 `adb` 或设备列表为空**
   - 确认已安装 `adb`（`which adb`），或已安装 Android SDK Platform-Tools / Homebrew `android-platform-tools`。
   - 无线调试设备若息屏后断连，重新在手机端开关一次“无线调试”。
4. **扩展画面滤镜无法开启**
   - 目前扩展滤镜（Direct3D HLSL 着色器）在 macOS Metal 后端尚未启用，属于已知限制。

---

## English Summary & Quick Guide

> **Date**: 2026-11-24  
> **Conclusion**: GameScrcpy-macos is currently a work-in-progress macOS port built from source (`cmake --preset mac-app`). Key/mouse mapping no longer requires macOS **Input Monitoring** permission; Xbox/PS4/PS5 controllers with rumble are supported via SDL3; typing is handled by the auto-installed **GameScrcpy Companion** IME so the phone's on-screen keyboard stays hidden; and the toolbar can now be docked on any edge (top/bottom/left/right).

### Build & Run
1. Install Xcode CLT, CMake (≥ 3.28), Ninja, Qt 6.10.3 (`~/Qt/6.10.3/macos`), and Homebrew packages (`ffmpeg`, `sdl3`, `libusb`, `pkg-config`, `android-platform-tools`).
2. Configure, build, and launch:
   ```bash
   cmake --preset mac-app
   cmake --build --preset mac-app
   open build/mac-app/bin/GameScrcpy.app
   ```
3. Settings, profiles, and logs are stored in `~/Library/Application Support/GameScrcpy/`.

### macOS Permissions (System Settings → Privacy & Security)
- **Local Network**: Required for wireless debugging, WiFi direct (UDP), and the companion app. Always allow when prompted (rebuilding `.app` may reset the grant).
- **Input Monitoring**: **No longer required** — key and mouse mapping reads input directly from GameScrcpy's own window.
- **Microphone & Speech Recognition**: Only required when using voice commands (still WIP).

### Connecting & Everyday Features
- **USB & Wireless debugging (Android 11+)**: Enable Developer options & USB/Wireless debugging, connect or pair via code/QR, and click **Mirror**. If **WiFi direct** fails with `DirectUnreachable`, switch to **adb tunnel**.
- **Gamepads**: Xbox, PS4, and PS5 controllers (including rumble) work via SDL3.
- **Toolbar position**: Dock the mirror toolbar to the **top, bottom, left, or right** edge.

### FAQ: Phone keyboard still pops up when typing?
Typing is routed through the **GameScrcpy Companion** IME (installed automatically on first mirror) so the phone's soft keyboard stays hidden. If the soft keyboard still appears:
1. Make sure **GameScrcpy Companion** was installed on the phone (enable **Install via USB** in Developer options if your phone blocks APK installs over adb, then reconnect).
2. On the phone, open **Settings → System / General management → Keyboard / On-screen keyboard** and enable **GameScrcpy Companion**.
3. If needed, switch the active input method to **GameScrcpy Companion** from the phone's keyboard switcher notification while mirroring.
