# Gemini 调研与文档日志

> 角色：GameScrcpy-macos 调研与用户文档成员（只出文档，不改代码）  
> 协同规则见根目录 `TEAM.md`

---

## 2026-11-25（G8 补充定案：方向 B 双端方案）

### 做了什么
根据用户选定（**方向 B「双屏跃迁与极速帧流」**用于双端，手机端采用 **配色方案 1「极光翠绿」**），更新 [`icon.md`](icon.md) 第 5 节并补充双端矢量分层素材（只写文档与 SVG，未改代码）：
1. **macOS 端（系统蓝）**：在 [`icon/layers-b-macos/`](icon/layers-b-macos/) 产出 4 组 `1024×1024` Icon Composer 分层 SVG（`01-phone-back.svg`、`02-glass-screen.svg`、`03-glass-sheen.svg`、`04-chevron.svg`）与合层预览 [`icon/preview-b-macos.svg`](icon/preview-b-macos.svg)。
2. **Android 手机端（极光翠绿）**：按 Android Adaptive Icon（`108×108 dp` 画布、`66×66 dp` 安全圆）在 [`icon/layers-b-android/`](icon/layers-b-android/) 产出背景层 `ic_launcher_background.svg`、前景层 `ic_launcher_foreground.svg`、Material You 单色层 `ic_launcher_monochrome.svg` 与 `512×512` 预览 [`icon/preview-b-android.svg`](icon/preview-b-android.svg)。
3. 在 [`icon.md`](icon.md) 第 5 节补充双端色值对照表、Icon Composer 参数、Android `adaptive-icon` XML 参考与坐标安全区说明。

### 验证了什么
- 联网核实了 Google 官方 [Adaptive icons — Android Developers](https://developer.android.com/develop/ui/compose/system/icon_design_adaptive) 规范（`108×108 dp`、`66×66 dp` 安全区、`monochrome` 主题图标层）。
- 验算了 Android 前景层几何坐标：竖屏外角距圆心约 `31.7 dp`、横屏外角距圆心 `29.15 dp`，均严格位于半径 `33 dp` 安全圆内。

### 没做完 / 需要用户或 Claude 做的事
- **需要 Claude**：
  - Mac 端：将 `docs/handoff/gemini/icon/layers-b-macos/01-*.svg` 至 `04-*.svg` 拖入 Xcode Icon Composer 生成 `.icon`（背景设为 `#F0F6FF → #CFE4FF`）。
  - Android 端：将 `docs/handoff/gemini/icon/layers-b-android/` 下的 3 个 SVG 转为 Vector Drawable XML 接入 Companion 工程（`mipmap-anydpi-v26/ic_launcher.xml`）。

---

## 2026-11-24（第三批：G8）

### 做了什么
按 `TEAM.md` 第 4 节第三批任务完成 **G8（GameScrcpy for macOS App 图标方案）**（只写文档与矢量素材，未改任何代码）：
1. 新增 [`icon.md`](icon.md)：结合软件定位（低延迟原画质高刷手游投屏、键鼠/触控板/手柄操控、键位映射、自动化脚本、录屏）、macOS 26 Liquid Glass 风格与 `#0A84FF` 强调色，提出 4 个独立设计方向（**A 棱镜投屏准星**、**B 双屏跃迁**、**C 玻璃操控视窗**、**D 零延迟电光菱镜**），每项含寓意、构图、具体色值与适配理由及横向对比表。
2. 最终推荐 **方向 A（棱镜投屏准星 / Precision Cast）**，并按 Apple Icon Composer 规范完成 1 背景填充 + 4 组前景图层的精确坐标、色值与材质参数拆解。
3. 在 [`icon/`](icon/) 下产出推荐方案的合层结构矢量预览 [`icon/preview-a-precision-cast.svg`](icon/preview-a-precision-cast.svg) 以及 4 组可直接拖入 Icon Composer 的 `1024×1024` SVG 分层素材（[`icon/layers/01-source-screen.svg`](icon/layers/01-source-screen.svg) 至 [`04-core-dot.svg`](icon/layers/04-core-dot.svg)）。4 个方向的高精度 Liquid Glass 渲染概念图已在 Gemini 对话中直接输出。

### 验证了什么
- 核对了 `TEAM.md` G8 要求与现有临时标志 `docs/images/logo.png`。
- 联网核实了 Apple 官方 [Human Interface Guidelines — App icons](https://developer.apple.com/design/human-interface-guidelines/app-icons) 与 [Creating your app icon using Icon Composer](https://developer.apple.com/documentation/xcode/creating-your-app-icon-using-icon-composer) 规范（1024×1024 全出血画布、4 组前景图层、材质参数与六种系统外观）。
- 校验了 4 层 SVG 坐标、安全边距（≥ 164 px）与 16 px 缩放比例。

### 没做完 / 需要用户或 Claude 做的事
- **需要用户**：在 4 个方向中确认是否采纳推荐的 **方向 A**（或偏好其他方向）；若需将对话中生成的 4 张高精度 Liquid Glass 概念图归档入库，可直接从对话另存至 `docs/handoff/gemini/icon/`。
- **需要 Claude**：用户拍板后，按 `icon.md` 第 4 节参数将 `docs/handoff/gemini/icon/layers/01-*.svg` 至 `04-*.svg` 拖入 Xcode Icon Composer 制作 `.icon` 文件，并在 Default / Dark / Clear / Tinted 与 16 px 尺寸下微调最终玻璃折射效果。

---

## 2026-11-24（第二批：G4–G7）

### 做了什么
按 `TEAM.md` 第 4 节第二批任务完成 G4 → G5 → G6 → G7（未修改任何代码）：
1. **G4**：更新 [`macos-guide.md`](macos-guide.md) — 移除“输入监控”权限需求（对应 `ed4672b`）；补充 SDL3 手柄（Xbox/PS4/PS5 + 震动）、GameScrcpy Companion 输入法自动安装与隐藏手机软键盘、工具栏四边位置（上/下/左/右）；移除赞助说明；在常见问题新增“打字时手机键盘还弹出来怎么办”（中英双语同步更新）。
2. **G5**：新增 [`research-latency.md`](research-latency.md) — 针对三星 SM-F971B（120 Hz）有线投屏延迟抖动与积压问题，核对 `MediaCodecSessionFactory.java`、`BlockModelSession.java`、`VideoSendQueue.java` 与 `SessionMetrics.cpp`，梳理 MediaCodec 低延迟键（重点指出 `BlockModel` 100 fps 阈值、`KEY_OPERATING_RATE = Short.MAX_VALUE` 热节流风险、未设 `KEY_BITRATE_MODE` 默认 VBR 尖峰）、scrcpy 三星社区经验、USB/adb 抖动与电脑端 P95/到达间隔抖动诊断指标，每条建议附目标文件、预期效果与风险。
3. **G6**：新增 [`research-swipe.md`](research-swipe.md) — 针对 `MirrorInputItem.cpp` 的 `swipeWithTrackpad()` 桌面左右滑卡顿，核对 `ControlChannel.cpp` 与 `TouchPlanner.java`，分析 macOS 触控板相位/频率（`ScrollEnd` 零位移停顿、90–125 Hz）、Android `ViewConfiguration` / `VelocityTracker`（100 ms 窗口、40 ms 停止判定、`eventTime` 受传输抖动影响）以及撞边重复坐标归零问题，给出具体优化方案与 `GS_TOUCH_TRACE` 验证手段。
4. **G7**：新增 [`release-notes-macos.md`](release-notes-macos.md) — 基于 `git log` 本分支全部提交整理出面向最终用户的中英双语 macOS 预览版更新说明草稿（不含内部实现细节）。

### 验证了什么
- 代码核对：`git log --oneline -n 50`、`src/app/MirrorInputItem.cpp`、`src/app/ControlChannel.cpp`、`src/app/SessionMetrics.cpp`、`android/server/.../MediaCodecSessionFactory.java`、`BlockModelSession.java`、`VideoSendQueue.java`、`TouchPlanner.java`。
- 外部权威资料均已联网核实并附来源链接（Android `MediaFormat` / `ViewConfiguration` / `VelocityTracker`、Apple `NSEvent.phase` / `momentumPhase`、scrcpy 相关 Issues）。

### 没做完 / 需要用户或 Claude 做的事
- 无代码修改（遵守 `TEAM.md` 角色边界）。
- **给 Claude 的实测建议**：
  - G5：建议先用 `GS_BLOCK_INPUT=0` 测一次 SM-F971B 120 Hz 是否还会积压到 8 秒，并打印 `MediaCodecSessionFactory.formatSummary()` 看实际编码器名与延迟报告。
  - G6：建议用 `GS_TOUCH_TRACE=/tmp/touch.log` 录一次桌面左右滑，检查 `ScrollEnd` 前后时间间隔与坐标变化，验证是否为 40 ms 停止规则或撞边归零导致卡顿。

---

## 2026-11-24（第一批：G1–G3）

### 做了什么
按 `TEAM.md` 第 4 节完成 G1、G2、G3 三篇文档（未修改任何代码）：
1. **G1**：[`macos-guide.md`](macos-guide.md) — macOS 使用指南（中文为主，附英文版）。
2. **G2**：[`research-filters.md`](research-filters.md) — 扩展滤镜（Direct3D HLSL）在 Metal 上运行调研，推荐 DXC + SPIRV-Cross。
3. **G3**：[`research-universal.md`](research-universal.md) — Universal（`arm64` + `x86_64`）依赖构建与 Developer ID 签名公证调研。

### 验证了什么
- 核对了 `FilterSource.*`、`FilterChain.cpp`、`third_party/Posix.cmake`、`third_party/README.md`、`src/app/CMakeLists.txt` 等源码，并联网核实许可与 Apple 公证流程。
