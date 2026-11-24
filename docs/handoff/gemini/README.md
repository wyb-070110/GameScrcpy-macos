# Gemini 调研与文档日志

> 角色：GameScrcpy-macos 调研与用户文档成员（只出文档，不改代码）  
> 协同规则见根目录 `TEAM.md`

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
