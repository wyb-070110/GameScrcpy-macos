# Antigravity 工作日志（`mac/antigravity`）

> 工作目录：`~/GameScrcpy-macos-antigravity` | 分支：`mac/antigravity`
> 状态：**可合并**

---

## [2026-11-25] 完成 A1（语音合成与语音命令识别）、合并主线、完成 A4（清理 Windows 遗留文案与中英翻译）及 G5 延迟方案评估

### 1. 完成 A1：macOS 语音命令识别与 16 kHz PCM 导出（提交 `2a59040`）

- **改动文件**：
  - `src/platform/src/mac/Speech.mm`：`SystemSpeech::voices()` / `languages()`（规范化 BCP-47 标签）、`say()` / `stop()`（`AVSpeechSynthesizer` 队列朗读与静默缓冲渲染）、`record()`（`AVSpeechSynthesizer writeUtterance:toBufferCallback:` + `AVAudioConverter` 重采样为 16 kHz 单声道 Int16 PCM；修复后台线程调用超时时 `SpeechLine` 按值捕获避免悬垂引用）、`hearFrom()` + `listen()` / `unlisten()`（`SFSpeechRecognizer`，支持时优先 `requiresOnDeviceRecognition = YES` 本机离线识别；支持中英文指令归一化、前后缀与前缀歧义消解，以及 `AVAudioEngine` 实时麦克风流式识别与分段重装）。
  - `src/app/Info.plist.in`（仅新增 2 个键，共 4 行）：新增 `NSMicrophoneUsageDescription` 与 `NSSpeechRecognitionUsageDescription`。
  - `src/platform/Posix.cmake`（改动 1 行）：`gs_platform` 链接 `-framework Speech`。
  - `tests/PortProbe.cmake`（改动 5 行）：`gs_platform_tests` 加入 `unit/platform/mac/SpeechMacTests.mm` 与 `-framework Speech`。
  - `tests/unit/platform/mac/SpeechMacTests.mm`（新增）：覆盖语音与识别语言列表、16 kHz 单声道 PCM 合成、从合成 PCM 识别匹配中文（“打开地图”）与英文（“Open Map”）指令并忽略无关语句（“今天天气很好”）、队列朗读与中途取消、非法声音与非法语言报错。
- **验证命令与结果**：
  - `cmake --preset mac-app && cmake --build --preset mac-app -j8`：编译通过（0 警告、0 错误）。
  - `python3 tools/check_source_rules.py`：`source rules: 0 violation(s)`。
  - `ctest --preset mac-app -R gs_platform_tests --output-on-failure -V`：33/33 用例（965 断言）全部通过，识别本机 180 个语音、62 种识别语言，中英文合成 PCM 离线识别与指令匹配均一次通过。
  - `ctest --preset mac-app -j4 --output-on-failure`：169/169 非真机用例通过（2 个真机用例跳过）。
- **需要用户在 Mac 上授权的步骤（首次使用实时麦克风语音命令时）**：
  1. 必须从签名后的 `GameScrcpy.app` 启动（命令行裸二进制无 `Info.plist` 用途说明，代码会安全返回提示而不会触发 TCC 崩溃）。
  2. 首次在方案或脚本中调用实时 `speech.listen`（非 `hearFrom` 测试缓冲）时，macOS 会依次弹出 **麦克风（Microphone）** 与 **语音识别（Speech Recognition）** 两个系统授权弹窗，请点击“好 / 允许”。
  3. 若曾拒绝授权，可前往 **系统设置 → 隐私与安全性 → 麦克风** 及 **语音识别** 中勾选开启 `GameScrcpy`（注：每次本地重编重签 `.app` 后，macOS TCC 可能需要重新确认）。

---

### 2. 同步主线 `ccr-e32722c2-75w11u`（合并提交 `26b5337`）

- **做了什么**：
  - 执行 `git merge ccr-e32722c2-75w11u`（合入 `ed4672b`..`dfeb2d2` 共 11 个主线提交）。
  - 解决唯一冲突 `tests/PortProbe.cmake`：同时保留本分支的 `SpeechMacTests.mm`（`-framework Speech`）与主线新增的 `InputCaptureMacTests.mm`（`-framework AppKit`）。
- **验证命令与结果**：
  - `cmake --preset mac-app && cmake --build --preset mac-app -j8`：通过。
  - `ctest --preset mac-app -j4 --output-on-failure`：170/170 非真机用例全部通过（2 个真机用例跳过）。
- **给 Claude 的备注（主线既有项，未触碰）**：
  - 合并进来的主线代码当前有 3 处 `check_source_rules.py` 提示（`src/app/CompanionLink.cpp:733`、`src/app/MirrorInputItem.cpp:47`、`src/app/PhoneScreenWatch.cpp:15`）以及 `src/ui/controls/ToolbarButton.qml:18-19` 的 3 条 `qmllint` 提示；按协作规则未改动 Claude 负责的文件与 `src/ui/controls/`。

---

### 3. 完成 A4：替换残留 Windows 文案并更新简体中文翻译

分 4 个小提交完成，未改动任何行为逻辑，也未触碰 `src/ui/theme/` 与 `src/ui/controls/`：

1. **C++ 设置、服务与错误建议文案**（`src/app/*.cpp`, `src/app/Settings.h`）
2. **QML 界面文案**（`src/app/qml/*.qml`）
3. **内置中英文帮助文档**（`src/app/help/index.json`, `src/app/help/en/*.md`, `src/app/help/zh_CN/*.md`）
4. **简体中文翻译更新**（`src/app/i18n/gamescrcpy_zh_CN.ts`，运行 `GameScrcpy_lupdate` 后人工逐条翻译全部 70 条新增词条，`unfinished` 计数为 `0`）

#### 修改对照表（文件 / 旧文案 / 新文案摘要）

| 文件 | 旧文案（摘要） | 新文案（摘要） |
|---|---|---|
| `src/app/AiAssistant.cpp` | `Windows Credential Manager did not keep/remove the key` | `macOS Keychain did not keep/remove the key` |
| `src/app/AoaSession.cpp` / `SessionController.cpp` | `no WinUSB driver` / `AOA needs the WinUSB driver...` | `AOA over USB is not available on macOS` |
| `src/app/DeviceManager.cpp` | `Needs a driver` / `composed by Windows` / `dropped by Windows` / `on the PC` / `running in the tray` / `WinUSB driver... Zadig` | `Not available on macOS` / `composed by macOS` / `dropped by macOS` / `on this Mac` / `running in the menu bar` / `AOA touch over USB is not available on macOS yet` |
| `src/app/DevicePresentation.cpp` | `Needs a driver` | `Not available on macOS` |
| `src/app/ExtensionHost.cpp` | `Windows Security found a threat` / `could not be scanned with Windows Security` | `macOS security checks found a threat` / `macOS has no on-demand malware scan API; Gatekeeper and XProtect check native libraries when they load.` |
| `src/app/ExtensionPanels.cpp` | `group in the tray menu` | `group in the menu bar item's menu` |
| `src/app/FailureAdvice.cpp` | `Install Android SDK Platform-Tools...` / `No usable graphics card... up-to-date graphics driver helps` | 补充 `Homebrew android-platform-tools`；改为 `Metal hardware acceleration is not available... running directly on a Mac with Metal support helps` |
| `src/app/FileAccess.cpp` | `Windows does not allow it` / `without .., a drive or characters Windows does not allow` | `macOS does not allow it` / `without .. or invalid path characters` |
| `src/app/McpTools.cpp` | `gs.exe did not finish / is missing / did not start` | `gs did not finish / is missing / did not start` |
| `src/app/PictureTouch.cpp` | `Needs this phone on a USB cable with the WinUSB driver` | `AOA touch over USB is not available on macOS yet; use Normal or UHID touch screen` |
| `src/app/Recordings.cpp` | `moved to the Recycle Bin` | `moved to the Trash` |
| `src/app/ScriptWorkbench.cpp` | `gs.exe did not start / is missing` | `gs did not start / is missing` |
| `src/app/ScriptWorkbenchFiles.cpp` | `missing next to gs.exe (typescript/tsc.exe)` | `missing next to gs (typescript/tsc)`（与 macOS `ScriptTools.cpp` 实际输出对齐） |
| `src/app/Settings.h` | `Following Windows` / `Close to the tray... notification area` / `Windows settings` / `speech: Windows` / `winusb zadig` / `dll` / `Videos folder` / `Windows proxy` | `Following macOS` / `Keep in the menu bar` / `System Settings > Accessibility > Display` / `speech: macOS` / 移除 `winusb zadig` / `dylib` / `Movies folder` / `macOS network proxy` |
| `src/app/SpeechEngines.cpp` | `.title = "Windows"`, `Windows has no voice`, `Windows only listens...` | `.title = "macOS"`, `macOS has no voice`, `macOS only listens...` |
| `src/app/VisionEngines.cpp` | `.title = "Windows"`, `Windows Settings, Time & language...`, `Windows could not read...` | `.title = "macOS"`, `macOS Vision text recognition...`, `macOS could not read...` |
| `src/app/qml/AboutPage.qml` & `Main.qml` | `...for Windows.` / `The driver is installed: %1...` | `...for macOS.` / `%1 can be controlled over AOA now` |
| `src/app/qml/AiSettings.qml` | `Kept in Windows Credential Manager under %1` | `Kept in the macOS Keychain under %1` |
| `src/app/qml/CaptureCardSheet.qml` & `DeviceCard.qml` | `needs the WinUSB driver` / `How to install the driver` | `AOA unavailable on macOS` / `About AOA on macOS` |
| `src/app/qml/ExtensionsPage.qml` | `Tray menu` | `Menu bar menu` |
| `src/app/qml/HardwarePicker.qml` | `Bluetooth is on in Windows` | `Bluetooth is on in System Settings > Bluetooth` |
| `src/app/qml/InstallDialog.qml` | `own Windows code` / `Windows Security found no threat` | `own native macOS code` / `Security check found no threat` |
| `src/app/qml/LanguageSetting.qml` | `follows Windows` | `follows macOS` |
| `src/app/qml/LogicWindow.qml` | `Save all files (Ctrl+S)` | `Save all files (⌘S)` |
| `src/app/qml/ProfilesPage.qml` / `RecordingsDialog.qml` / `TaskSettings.qml` | `Recycle Bin` | `Trash`（中文对应“废纸篓”） |
| `src/app/qml/SettingsPage.qml` | `Follow Windows` / `On Windows USB debugging... WinUSB driver` / `How to install the WinUSB driver` / `named pipe... Windows account` | `Follow macOS` / `AOA over USB is not available on macOS yet...` / `About AOA on macOS` / `local Unix socket... macOS user account` |
| `src/app/qml/ShortcutRow.qml` | `Keys without Ctrl, Alt or Win...` | `Keys without Command, Option or Control...` |
| `src/app/qml/SpeechSettings.qml` | `Windows has no voice / speech recognizer; add under Time & language, Speech` | `macOS has no voice / speech recognizer; System Settings > Accessibility > Spoken Content / Dictation / Speech Recognition` |
| `src/app/qml/TasksPage.qml` | `leaves GameScrcpy in the notification area` | `leaves GameScrcpy in the menu bar` |
| `src/app/help/index.json` & `en,zh_CN/aoa-driver.md` | WinUSB + Zadig 驱动安装教程、设备管理器卸载、Windows 防火墙 | 明确说明 macOS 无需驱动且暂未支持 USB AOA（推荐普通/UHID 或触控盒），保留无画面控制窗与扫码连伴侣说明，防火墙改为“本地网络与防火墙” |
| `src/app/help/en,zh_CN/capture-card.md` | Windows 相机应用、WinUSB 驱动链接 | 注明 macOS 暂未支持采集卡输入（标准 UVC 说明）及 AOA 状态 |
| `src/app/help/en,zh_CN/companion.md` | WinUSB 链接、Windows 防火墙 | `AOA touch over USB` 链接、`系统设置 > 隐私与安全性 > 本地网络` 与 macOS 防火墙 |
| `src/app/help/en,zh_CN/extension-features.md` | Windows 语音/设置、跟随 Windows、托盘菜单 | macOS 语音与权限说明、跟随 macOS、菜单栏图标菜单 |
| `src/app/help/en,zh_CN/extensions.md` | `GameScrcpy.exe`、Windows DLL、Windows 安全中心、画面滤镜未注 | `GameScrcpy.app` / `open -a GameScrcpy --args --safe-mode`、`.dylib` 动态库、Gatekeeper/XProtect 说明、注明扩展画面滤镜暂不支持 |
| `src/app/help/en,zh_CN/faq.md` | 显卡驱动、`GameScrcpy.exe`、`%APPDATA%\GameScrcpy` + `portable.flag`、`<kbd>Ctrl</kbd>+<kbd>K</kbd>` | Homebrew 安装 `adb`、本地网络权限、Metal 硬件加速、`~/Library/Application Support/GameScrcpy` + `GS_DATA_DIR`、`<kbd>⌘ Command</kbd>+<kbd>K</kbd>` |
| `src/app/help/en,zh_CN/keymap-editor.md` | `<kbd>Ctrl</kbd>` / `<kbd>Alt</kbd>` / `Ctrl+C` / `Ctrl+V` | `<kbd>⌘ Command</kbd>` / `<kbd>⌥ Option</kbd>` / `⌘C` / `⌘V` |
| `src/app/help/en,zh_CN/profiles.md` | `Recycle Bin` / `File Explorer`（回收站 / 资源管理器） | `Trash` / `Finder`（废纸篓 / 访达） |
| `src/app/help/en,zh_CN/scripts.md` | `Ctrl` / `Alt`、`gs.exe` / `.\gs.exe` | `⌘ Command` / `⌥ Option`、`GameScrcpy.app/Contents/MacOS/` 下的 `./gs` |
| `src/app/i18n/gamescrcpy_zh_CN.ts` | 70 条变更词条 | 全部补齐自然中文翻译（`unfinished` = 0） |

#### 存疑与留给 Claude 决定的边界点（未擅改）

1. **脚本 API 生成表 `src/script/src/Api.cpp` 与 `sdk/`**：`gamescrcpy_zh_CN.ts` 中 `[ScriptApi]` 上下文来自 `sdk/i18n/api-texts.js`（共 19 条提到 `Windows` 或 `tray menu`）。因改动 `Api.cpp` 并运行 `gs api write sdk` 会跨出 `src/app` 批量重生成 `sdk/` 下的类型与接口文档，本次保持未动，若确认需要一并改为 macOS，可改 `src/script/src/Api.cpp` 后执行 `gs api write sdk` 并跑 `GameScrcpy_lupdate`。
2. **键帽显示格式化函数 `Commands::keyCaps`（`src/app/Commands.cpp:150-161`）与 `KeyLabels.cpp:27-28`**：
   - `Commands::keyCaps(QKeyCombination)` 目前把 `Qt::ControlModifier` / `AltModifier` / `MetaModifier` 格式化为字符串 `"Ctrl"` / `"Alt"` / `"Win"` 送给 `Keycap.qml` 显示（macOS 上 Qt 的 `ControlModifier` 实际是 `⌘ Command`，`AltModifier` 是 `⌥ Option`，`MetaModifier` 是 `⌃ Control`）；`MenuItem.qml` 的 `shortcut` 属性也是直接渲染字符串（如 `"Ctrl+D"`）而非经 `QKeySequence(NativeText)` 转换。因涉及快捷键展示逻辑与 `src/ui/controls/MenuItem.qml`，按今晚规则未碰，交由 Claude 在 Liquid Glass UI 改造时统一决定是否改为 `⌘` / `⌥` / `⌃`。
3. **未纳入 macOS 构建的旧单元测试常量**：`tests/unit/app/`（当前仅在 `WIN32` 分支编译，不在 `PortProbe.cmake` 中）里有若干断言旧字符串（如 `SpeechEnginesTests.cpp` 断言 `title == u"Windows"`、`PictureTouchTests.cpp` 断言 WinUSB 文案）；将来若把 `gs_app_tests` 接入 `PortProbe.cmake`，同步更新那几个断言即可。

---

### 4. G5 有线投屏延迟调研（第 2–4 项）优先级评估（仅写结论，未改代码）

对照 `docs/handoff/gemini/research-latency.md` 第 2–4 项与 `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java` 现有实现，**建议最先尝试第 2 项（同时顺手带上第 4 项的编码器名与实际 `outputFormat` 日志），暂缓第 3 项（CBR）**，理由如下：

1. **为什么先做第 2 项（修正 `KEY_OPERATING_RATE` 并覆盖非高通编码器）**：
   - 看 `MediaCodecSessionFactory.java:211-224` 与 `286-296`：目前只有 `qualcomm(attempt.encoder())` 为真时才进入低延迟分支，且把 `KEY_OPERATING_RATE` 硬编码为 `Short.MAX_VALUE`（`32767`），并与私有键 `vendor.qti-ext-enc-low-latency.enable = 1` 绑在**同一个** `try` 块里。
   - 这在三星机型上有两个直接隐患：
     - **若该机采用高通编码器（`c2.qti.*`）**：传 `32767` 会让折叠屏受限散热下的 VPU 锁满频运行，极易触发温控/DVFS 周期性降频抖动；或者若三星 One UI 的 `c2.qti.*` 拒收 `32767`，`catch` 会 `codec.reset()` 回退到普通 `format()`，**连同 `KEY_QTI_LOW_LATENCY = 1` 一起丢掉**。
     - **若该机采用三星 Exynos 编码器（`c2.exynos.*` / `c2.sec.*`）**：`qualcomm()` 返回 `false`，**全程根本没有设置 `KEY_OPERATING_RATE`**！VPU 默认停留在省电调频档，桌面静止后一旦滑动，编码器升频唤醒就会产生 5 ms → 33 ms 的典型延迟毛刺。
   - 将 `KEY_OPERATING_RATE` 改为实际目标帧率 `keys.frameRate`（如 60 或 120），放进通用硬编配置并与高通私有键解耦（各自独立 fallback），收益最直接且几乎零画质副作用。
2. **为什么顺手配合第 4 项的日志**：
   - `formatSummary(codec, ...)`（第 254 行）已经能读出 `KEY_LATENCY` 和 `KEY_OUTPUT_REORDER_DEPTH`，在 `open()` 启动编码器后打印一行 `attempt.encoder() + ": " + formatSummary(...)` 零风险，一次连接就能看清三星实机到底选了哪个编码器、低延迟键有没有被拒收。
3. **为什么把第 3 项（`BITRATE_MODE_CBR`）放后面**：
   - CBR 在静止转快速滑动时虽能削平单帧体积尖峰，但会明显降低高复杂度画面（文字边缘、游戏细节）的画质，且部分 Android 硬件编码器对 HEVC/AV1 不支持 CBR；应先用第 2+4 项排除 VPU 调频与低延迟回退问题，若仍有大帧引发的 USB/adb 排队尖峰，再在 `EncoderCapabilities.isBitrateModeSupported(BITRATE_MODE_CBR)` 保护下做实验对比。

---

**当前分支状态：可合并（Ready to merge）**


---

## [2026-11-25] 下一批：合并主线 `dcbff67` 并完成 A2（macOS USB `libusb` 支持与 AOA 文案恢复）

### 1. 合并主线 `ccr-e32722c2-75w11u`（快进至 `dcbff67`）

- 执行 `git merge ccr-e32722c2-75w11u` 快进到 `dcbff67`，全量构建并运行 `ctest --preset mac-app -j4`：172/172 通过（2 个真机用例跳过）。
- 按最新指示跳过 A3（Claude 已完成打包）。

### 2. 完成 A2：macOS USB 手机枚举、热插拔与 AOA 控制传输（`src/platform/src/mac/Usb.mm`）

- **改动文件**：
  - `src/platform/Posix.cmake`：在 `APPLE` 分支为 `gs_platform` 链接 `libusb::libusb` 并定义 `GS_HAVE_LIBUSB`。
  - `src/platform/src/mac/Usb.mm`：
    - `usbLibraryVersion()`：返回 `libusb` 版本号（当前环境实测 `1.0.30`）。
    - `listUsbPhones()`：通过 `libusb` 枚举 USB 设备（排除 Hub 与 Apple `0x05AC` iPhone/iPad 设备），检查接口描述符识别 ADB 接口（`0xFF/0x42/0x01` → `debugging = true`）、MTP（`0x06/0x01/0x01`）、Google AOA 配件模式（`0x18D1:0x2D00..0x2D05`）及主流安卓厂商复合设备，优先通过 `libusb` 读取 UTF-16LE/ASCII 字符串描述符并在设备被占用时回退读取 IOKit `IOUSBHostDevice` 注册表属性，`driver` 恒为 `UsbDriver::Ready`，按 `serial` 排序去重。
    - `UsbDevice`：实现 `open`、`controlOut`（`0x40`）、`controlIn`（`0xC0`）、`maxPacketSize` 与 `failure` 错误映射，供 `AoaTouchSink` 发送 AOA v2 HID 控制传输。
    - `UsbWatcher`：独立 `libusb_context` + `libusb_hotplug_register_callback` + 独立事件线程（`"usb watch"`），析构时通过 `libusb_hotplug_deregister_callback` + `libusb_interrupt_event_handler` 零等待干净退出（无任何固定延时 sleep）。
  - `tests/unit/platform/mac/UsbMacTests.mm` & `tests/PortProbe.cmake`：覆盖 `usbLibraryVersion()`、`listUsbPhones()` 不变量与实机输出打印、`UsbDevice::open` 空/不存在设备返回 `UsbFailure::NotFound`（不触碰真实设备配件模式）、`UsbWatcher` 单次及连续多轮启停生命周期测试。
  - AOA 文案恢复（去除“macOS 暂未支持 AOA”占位说明，且不含任何 WinUSB/Zadig 字样）：
    - C++：`src/app/AoaSession.cpp`、`src/app/DeviceManager.cpp`、`src/app/DevicePresentation.cpp`、`src/app/PictureTouch.cpp`（未插线原因改为 `"Needs this phone on a USB cable"`）、`src/app/SessionController.cpp`。
    - QML：`src/app/qml/SettingsPage.qml`、`src/app/qml/CaptureCardSheet.qml`、`src/app/qml/DeviceCard.qml`。
    - 帮助与翻译：`src/app/help/index.json`、`src/app/help/{en,zh_CN}/aoa-driver.md`、`src/app/help/{en,zh_CN}/capture-card.md`、`src/app/i18n/gamescrcpy_zh_CN.ts`（`GameScrcpy_lupdate` 更新并补齐全部 12 条新译文，5732/5732 完成，`unfinished = 0`）。
- **验证命令与结果**：
  - `cmake --build --preset mac-app -j8 && cmake --build --preset mac-app --target all_qmllint`：0 编译警告、所改 QML 文件 0 `qmllint` 警告，所改文件 `check_source_rules.py` 0 违规。
  - `ctest --preset mac-app -R gs_platform_tests --output-on-failure -V`：39/39 用例（993 断言）通过；打印 `libusb version: 1.0.30`，当前运行测试时手机 `RFGL71YMDHD` 未插在 USB 口上（`adb devices -l` 与 `libusb` 均为 `attached USB phones: 0`）。
  - `ctest --preset mac-app -j4 --output-on-failure`：172/172 全部通过（170 通过，2 个真机测试跳过）。
- **给 Claude 与用户的真机 / AOA 验证步骤（未自行切手机配件模式）**：
  1. 插上手机 `RFGL71YMDHD` 后运行 `./build/mac-app/bin/gs_platform_tests -tc="*lists attached Android phones*" -s`，可查看 `listUsbPhones()` 输出的 `serial`、`name`、`vid/pid`、`driver=Ready` 与 `debugging=true/false`。
  2. **有画面 + USB AOA 触控测试**：保持手机 USB 数据线连接（开启 USB 调试或无线调试），启动 `GS_DATA_DIR=~/GameScrcpy-data-antigravity ./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --touch aoa`，投屏后在镜像窗口点击/滑动/按键，日志应打印 `touch on <serial> goes through an AOA touch screen on the USB cable`（GameScrcpy 使用 AOA v2 HID EP0 控制请求 `51/54/55/56/57`，macOS 下无需重装驱动，也不抢占 `adb` 接口）。
  3. **无 USB 调试纯 AOA 控制窗测试**：在手机上关闭 USB 调试、仅插 USB 线，在设置中将触控方式设为 `AOA`，设备页会出现 `aoa-<serial>` 卡片（连接方式 `USB · AOA`），点连接后打开无画面控制窗口并在手机上看到触控/鼠标指针响应。
- **状态**：**可合并**
