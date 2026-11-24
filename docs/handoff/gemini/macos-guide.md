# GameScrcpy macOS 使用指南（G1）

> **日期**：2026-11-24
> **结论**：当前仓库处于 macOS 移植开发阶段（WIP），尚未提供签名公证的 `.dmg` 安装包，需通过 `cmake --preset mac-app` 从源码构建运行；首次在无线调试下使用“WiFi 直连”若遇 `DirectUnreachable`（当前已知待修问题），可先切换为 **adb 隧道** 正常投屏；系统授权中“本地网络”对无线调试至关重要，“输入监控”与“语音识别/麦克风”对应功能仍在开发中。

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
# 1. 配置与编译（需要约几分钟）
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

在 **系统设置 → 隐私与安全性** 中，GameScrcpy 会涉及以下权限：

| 权限 | 用途 | 当前状态与操作建议 |
|---|---|---|
| **本地网络（Local Network）** | 无线调试配对、WiFi 直连（UDP）与手机伴侣局域网通信 | **必须允许**。首次弹窗请点“允许”；若未弹窗或已拒绝，请前往 **系统设置 → 隐私与安全性 → 本地网络** 开启 `GameScrcpy`。重编 `.app` 后签名变化可能导致权限失效，需要重新确认。 |
| **输入监控（Input Monitoring）** | 键位映射的全局键盘/鼠标原始输入捕获 | 当前 macOS 端 `InputCapture` 仍为占位实现（开发任务 C1 进行中）。后续启用后，需在 **隐私与安全性 → 输入监控** 中勾选 `GameScrcpy`。 |
| **麦克风与语音识别（Microphone & Speech Recognition）** | 语音命令触发键位或脚本动作 | 当前 macOS 端语音识别仍在补全中（开发任务 A1 进行中）。后续启用后，首次使用需分别授予麦克风与语音识别权限。 |
| **未签名应用提示（Gatekeeper）** | 源码自编或未公证构建首次启动 | 若系统提示无法验证开发者，可在 **隐私与安全性** 底部点“仍要打开”，或在终端执行 `xattr -dr com.apple.quarantine GameScrcpy.app`（仅限可信本地构建）。 |

---

## 三、USB 与无线调试连接

### 1. USB 有线连接

1. 在手机上打开 **开发者选项** 与 **USB 调试**（部分品牌如小米/OPPO/vivo 需额外开启“USB 调试（安全设置）”或“禁止权限监控”才能模拟点击）。
2. 用数据线将手机连到 Mac，在手机弹窗中勾选“一律允许使用这台计算机进行调试”并确定。
3. 在 GameScrcpy 主界面点击对应设备右侧的 **投屏** 按钮即可开始。

### 2. 无线调试连接（Android 11+）

1. 确保手机与 Mac 连在**同一个 Wi-Fi 局域网**（路由器未开启 AP 隔离）。
2. 在手机上进入 **开发者选项 → 无线调试** 并开启。
3. 在 GameScrcpy 中点击无线配对：
   - **配对码配对**：在手机点击“使用配对码配对设备”，将 IP、端口与 6 位配对码填入 GameScrcpy。
   - **二维码配对**：用手机无线调试里的“使用二维码配对设备”扫描界面上的二维码。
4. 配对成功后设备会以 `adb-..._adb-tls-connect._tcp` 出现在列表中，点击 **投屏** 即可。

> **重要提示**：当前无线调试默认尝试 **WiFi 直连（UDP）**，若在“验证”阶段提示 **“手机没有回应 WiFi 直连”**，请直接点击提示下方的 **改用 adb 隧道**，或在设备卡片上手动切换为 **adb 隧道**。

---

## 四、常见问题排查

1. **提示“手机没有回应 WiFi 直连”（`DirectUnreachable`）**
   - **原因**：UDP 握手超时（当前仅 1500 ms），可能由手机 Wi-Fi 休眠唤醒延迟、macOS 本地网络权限拦截或路由器 AP 隔离引起。
   - **解决**：先切到 **adb 隧道** 正常使用；确认 **系统设置 → 隐私与安全性 → 本地网络** 已允许 `GameScrcpy`；保持手机亮屏后再重试 WiFi 直连。排查日志可查看 `~/Library/Application Support/GameScrcpy/logs/GameScrcpy.log`。
2. **找不到 `adb` 或设备列表为空**
   - 确认已安装 `adb`（`which adb`），或已安装 Android SDK Platform-Tools / Homebrew `android-platform-tools`。
   - 无线调试设备若长期息屏断连，重新在手机端开关一次“无线调试”。
3. **点击画面或键盘按键没有反应**
   - 确认手机端已开启模拟输入相关选项（如“USB 调试（安全设置）”），或在连接设置中尝试切换触控方式。
   - 注意 macOS 全局输入捕获（`InputCapture`）目前仍在移植中，完整键鼠捕获将在后续提交完善。
4. **扩展画面滤镜无法开启**
   - 目前扩展滤镜（Direct3D HLSL 着色器）在 macOS Metal 后端尚未启用，属于已知限制。

---

## English Summary & Quick Guide

> **Date**: 2026-11-24  
> **Conclusion**: GameScrcpy-macos is currently a work-in-progress macOS port. Prebuilt `.dmg` packages are not available yet; build from source with `cmake --preset mac-app`. For wireless debugging, switch to **adb tunnel** if **WiFi direct** fails with `DirectUnreachable`.

### Build & Run
1. Install Xcode CLT, CMake (≥ 3.28), Ninja, Qt 6.10.3 (`~/Qt/6.10.3/macos`), and Homebrew packages (`ffmpeg`, `sdl3`, `libusb`, `pkg-config`, `android-platform-tools`).
2. Configure and build:
   ```bash
   cmake --preset mac-app
   cmake --build --preset mac-app
   open build/mac-app/bin/GameScrcpy.app
   ```
3. Data and logs live in `~/Library/Application Support/GameScrcpy/`.

### macOS Permissions (System Settings → Privacy & Security)
- **Local Network**: Required for wireless debugging, WiFi direct (UDP), and the companion app. Always allow when prompted (note that rebuilding `.app` may reset the permission).
- **Input Monitoring**: Needed for global keyboard/mouse key-mapping capture once `InputCapture` on macOS is completed (WIP).
- **Microphone & Speech Recognition**: Needed for voice commands once `Speech` recognition on macOS is completed (WIP).

### Connecting a Phone
- **USB**: Enable Developer options & USB debugging (plus security-settings input toggles on Xiaomi/OPPO/vivo), plug in the cable, allow debugging, and click **Mirror**.
- **Wireless debugging (Android 11+)**: Keep Mac and phone on the same LAN, pair via pairing code or QR code, and click **Mirror**. If **WiFi direct** fails at the verification step (`The phone did not answer over WiFi direct`), switch the connection mode to **adb tunnel**.
