# GameScrcpy-macos 多 AI 协同交接文档 (HANDOFF)

> 最后更新：2026-11-24 | 当前分支：`ccr-e32722c2-75w11u` | 最新提交：`2ddac7d`

本文档用于多个 AI 助手在 `GameScrcpy-macos` 仓库中协同开发与排错，记录当前状态、已解决与待办事项、构建测试命令及项目约束。

---

## 1. 项目现状总览

本仓库是 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) 的 **macOS 移植版（WIP）**，基于 **C++20 + Qt 6.10.3 (Qt Quick) + Metal (`QRhi`) + VideoToolbox**，已移除 Windows CI 与专属实现。

### 已可用模块
- **视频解码与渲染**：VideoToolbox 硬解（NV12 / P010 / YUV 4:2:0）+ `QRhi` Metal 渲染管线（`src/render/`）。
- **连接与辅助服务**：USB / 无线调试（`adb` 从 Android SDK 或 Homebrew 查找）、`adb` 隧道、WiFi 直连（UDP + RS 纠删码）、手机伴侣（BSD Socket / Bonjour `_gs-companion._tcp`）、本地 MCP（Unix Socket）、`gs` 命令行工具、`gs-scripthost`（含 `kevent64` 微秒级高精度脚本定时器）、TypeScript 编译器。
- **原生扩展**：`.dylib` 扩展加载、Mach-O 安全检查、`gs-exthost` 独立进程与 macOS `sandbox_init` 隔离。
- **macOS 平台服务与窗口交互**：原生透明标题栏与红绿灯、标题栏拖拽移动窗口（`performWindowDragWithEvent:`）与双击缩放/最小化（遵循系统偏好）、Vision 框架中英文 OCR、Keychain 凭据存储、光标与前台窗口监听、键盘布局、辅助功能检测、投屏防休眠（禁用 App Nap）、菜单栏图标与通知、Speech 朗读、触控板滑动手势、合规 `.app` Bundle 签名（`Contents/Resources` 资源分离 + `Info.plist` 本地网络权限绑定）。

### 待办事项清单（剩余 P2）

| 优先级 | 模块 / 现象 | 关键文件 | 现状与下一步 |
|---|---|---|---|
| **P2** | **全局输入捕获与手柄支持** | `src/platform/src/mac/InputCapture.mm` | 目前仍为占位实现（投屏窗口内鼠标点击与触控板滑动可用）；键位映射所需的全局键盘/鼠标相对移动捕获（`CGEventTap` / `IOHIDManager` / GameController）待实现。 |
| **P2** | **扩展画面滤镜不可用** | `src/render/src/FilterChain.cpp` | Metal（`QRhi`）管线暂未接入扩展自定义着色器滤镜，当前提示 macOS 暂不支持。 |
| **P2** | **其余平台层占位实现** | `src/platform/src/mac/Speech.mm`<br>`src/platform/src/mac/Usb.mm`<br>`src/hardware/` / `src/capture/` | 语音指令识别（`Speech.mm` 仅实现了 TTS 朗读）、USB 插拔监听与 AOA 触控、HDMI 采集卡与扩展系统级硬件枚举仍为占位实现。 |
| **P2** | **macOS 发布打包** | `packaging/` | 已完成合规 `GameScrcpy.app` 结构与 ad-hoc 签名，后续可补充 Qt 框架打包（`macdeployqt`）与 `.dmg` 制作脚本。 |

---

## 2. 构建、运行与测试说明

- **CMake Presets**：
  - 完整应用（需 `~/Qt/6.10.3/macos`）：
    ```bash
    cmake --preset mac-app
    cmake --build --preset mac-app
    cmake --build --preset mac-app --target all_qmllint
    ```
  - 仅核心库与非 Qt 测试：
    ```bash
    cmake --preset posix-core
    cmake --build --preset posix-core
    ```
- **运行演示模式（无需真机）**：
  ```bash
  ./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --demo-device tests/data/video/emulator_h265.media
  ```
- **运行测试**：
  ```bash
  ctest --preset mac-app --output-on-failure
  ```
  > **受限终端沙盒说明**：在 AI 终端沙盒内运行测试时，`gs_app_smoke*`（访问外部 `~/Qt`）、`gs_scripthost_tests` / `gs_exthost_tests`（嵌套 `sandbox_init`）、`TextReaderMacTests`（Vision 内部创建 IOSurface `CVPixelBuffer` 被沙盒拦截，报 `Code=8 Failed to create CVPixelBuffer`）以及 `UdpSocketTests` 可能因沙盒策略失败；在宿主环境运行全部通过（169/169 非真机用例全绿，2 个真机用例在设置 `GS_ADB_TEST_SERIAL` 后运行）。
- **日志与数据目录**：
  `~/Library/Application Support/GameScrcpy/logs/GameScrcpy.log`

---

## 3. 代码规范红线（来自 `AGENTS.md`）

修改代码前务必遵守以下规则（可通过 `python3 tools/check_source_rules.py` 检查）：
1. **禁止**解析 `dumpsys` / `wm` 输出。
2. **禁止**固定延迟同步（`sleep`、延迟单次定时器等；确有必要需在同行加 `source-rule-allow R9: <reason>`）。
3. **QML 规范**：`all_qmllint` 必须 0 警告；`tools/check_qml_aot.py` 必须 0 未解析查找；每个 `Text` 必须显式声明 `textFormat`；`src/ui/controls` 中的颜色、字号、圆角、动效只能取自 `src/ui/theme`。
4. **多语言**：代码中的用户可见字符串统一用英文，改后运行 `update_translations` 并更新 `src/app/i18n/gamescrcpy_zh_CN.ts`（不能留 `unfinished`）。
5. **协议与时限约束**：`DirectHandshake::kGiveUpAfter`（1500 ms）与 `GS_READY ... ip=` 首地址选取规则由 `protocol/SPEC.md` 10.1/10.2 及单元/真机测试锁定，勿擅改协议常量。
6. **离线构建**：配置与构建阶段禁止新增联网下载，第三方依赖须放在 `third_party/`。

---

## 4. 协同工作记录区（各 AI 接力填写）

### [2026-11-24] 第二轮接力修复与全量验证（提交 `2ddac7d`）
- **分支纠正**：工作区曾切回旧 `main`（`85e1aa7`），已切回移植主线分支 `ccr-e32722c2-75w11u`（`063e488` -> `2ddac7d`）。
- **P0 修复 — 无线调试 WiFi 直连 (`DirectUnreachable`) 与 `.app` 本地网络权限**：
  - 真机验证确认：CLI 测试程序 `gs_session_device_tests` 通过 `adb-RFGL71YMDHD-g27GKU._adb-tls-connect._tcp` 直连 `SM-F971B`（`192.168.124.4`）立即推流成功；仅 `GameScrcpy.app` 失败。
  - 根因：`GameScrcpy.app` 原先使用默认空模板 `Info.plist`（缺少 `NSLocalNetworkUsageDescription` 与 `NSBonjourServices`），且把非代码资源（`components/`、`tasks/`、`typescript/`、`*.apk`）放进了 `Contents/MacOS/`，导致 `GameScrcpy.app` 仅具 linker-signed 签名（`Identifier=GameScrcpy`、`Info.plist=not bound`），无法通过 macOS 15+ 本地网络权限绑定。
  - 修复：新增 `src/app/Info.plist.in`（声明本地网络用途、`_gs-companion._tcp` / `_adb-tls-connect._tcp` / `_adb-tls-pairing._tcp` Bonjour 服务及 `gamescrcpy://` URL scheme）；将非代码资源移入 `Contents/Resources/`（`main.cpp` 与 `ScriptTools.cpp` 兼容查找），并在 `POST_BUILD` 对辅助程序与 `GameScrcpy.app` 执行完整 ad-hoc 签名（`codesign --verify --deep --strict` 通过）；在 `UdpSocket.cpp` 与 `DirectTransport.cpp` 增加 UDP 发送失败计数与日志；更新 `FailureAdvice.cpp` 及中文翻译引导用户检查“系统设置 > 隐私与安全性 > 本地网络”。
- **P1 核实 — `TextReaderMacTests`**：
  - 实测宿主环境运行 `gs_platform_tests --test-case="macOS*"` **3 个用例（22 个断言）全部通过**；仅在受限沙盒内因 Vision 框架无法分配 IOSurface `CVPixelBuffer` 失败，无需改动 `TextReader.mm`。
- **P1 修复 — 窗口标题栏拖拽与布局**：
  - 根因：Qt 6.10.3 的 `QCocoaWindow` 未实现 `startSystemMove()`，且 `QNSView` 的 `mouseDownCanMoveWindow` 返回 `NO`。
  - 修复：在 `WindowChromeMac.mm` / `WindowChrome.cpp` 实现基于 `[NSWindow performWindowDragWithEvent:]` 的原生拖拽（拖拽结束后向 Qt 补发鼠标释放事件避免 `MouseArea` 悬空抓取）以及遵循 `AppleActionOnDoubleClick` 的标题栏双击缩放/最小化；接入 `TitleBar.qml`、`MirrorOverlay.qml`（常规标题栏及迷你窗）与 `GridOverlay.qml`，并将投屏窗与宫格窗标题左边距调整为 `78` 以避让 macOS 红绿灯按钮；在 `Typography.qml` 为缺失的 Segoe 图标字体提供 `SF Pro` 回退，消除启动时 48 ms 字体扫描警告。
- **附加修复 — 脚本定时器精度与源码规范检查**：
  - `ScriptHostImpl.h`：macOS `pthread_cond_timedwait` 默认存在 50% 定时器合并裕量（导致 `sleep(4)` 稳定晚约 `2033 µs`，使 `gs_script_tests` 的 `SCR-003` 失败）；改用 `kevent64`（`EVFILT_TIMER` + `NOTE_NSECONDS | NOTE_CRITICAL | NOTE_LEEWAY` + `EVFILT_USER` 唤醒，严格沙盒下自动回退 `condition_variable`），`sleep(4)` 中位误差从 `2033 µs` 降至 **`63 µs`**，`gs_script_tests` 202 个用例（4056 断言）全通。
  - `SessionController.cpp` / `SessionController.h`：将帧率“跟随手机”（`0`）上限限制为 60 fps，避免 120 Hz 手机在全分辨率下硬件编码器积压导致画面延迟数秒。
  - `check_source_rules.py`：补齐 `McpRelay.cpp` 与 `exthost/src/posix` 兼容层的 `R9` 注释，恢复 **0 违规**。
- **文档更新**：重写 `README.md`、`README.en.md` 为 macOS 移植版说明，同步更新 `docs/macos-port.md`。
- **下一步建议**：优先攻克 P2 的 `src/platform/src/mac/InputCapture.mm`（接入 macOS 键盘/鼠标/手柄事件捕获以打通完整键位映射操作），或视需求接入 Metal 扩展滤镜与 DMG 打包。

### [2026-11-24] 初步排查（上一轮会话）
- 梳理仓库结构与 `063e488` 提交状态，定位 `DirectUnreachable`、`TextReaderMacTests`（沙盒内）及标题栏无法拖拽问题线索。
