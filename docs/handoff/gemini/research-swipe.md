# 调研：macOS 触控板滑动映射成手机触摸的顺滑方案（G6）

> **日期**：2026-11-24
> **结论**：当前 [MirrorInputItem.cpp](file:///Users/york/GameScrcpy-macos/src/app/MirrorInputItem.cpp) 的 `swipeWithTrackpad()` 把双指滑动映射为一次单指 `Down → Move → Up` 并丢弃 `ScrollMomentum`，方向正确（单次轻扫对应桌面翻一页），但桌面左右滑仍有卡顿的核心原因有四点：
> 1. **`ScrollEnd` 抬手瞬间的“零位移停顿”杀死了 Android Fling**：macOS 触控板抬手事件 `ScrollEnd` 通常 `pixelDelta == (0, 0)` 且距上一帧可能间隔 16–40 ms，触发了 Android `VelocityTracker` 的 40 ms 停止判定阈值，导致桌面翻页只能硬拖过半屏而失去惯性；
> 2. **服务端按“收包时刻”打 `eventTime` 放大了传输抖动**：[TouchPlanner.java](file:///Users/york/GameScrcpy-macos/android/server/src/main/java/com/gamescrcpy/server/input/TouchPlanner.java) 用服务端 `nowMillis` 作为 `MotionEvent.eventTime`，USB/adb 几毫秒抖动会让 `VelocityTracker` 算出的瞬时速度大幅震荡；
> 3. **未按窗口与手机屏宽做灵敏度缩放 + 撞边过早截断**：直接累加 `pixelDelta()` 在小窗口或高分屏下位移比例不对，滑到边直接 `Up` 容易在未达翻页阈值或速度异常时中断；
> 4. **Qt 事件队列与 `ControlChannel` 触摸节流叠加**：不均匀的 `Move` 到达节奏会让 Android 帧同步采样（InputConsumer resampling）产生视觉顿挫。

---

## 一、macOS 触控板事件的频率与相位机制

根据 Apple [NSEvent.phase](https://developer.apple.com/documentation/appkit/nsevent/phase-swift.property) / [NSEvent.momentumPhase](https://developer.apple.com/documentation/appkit/nsevent/momentumphase) 文档与实际硬件行为：

1. **事件频率**：
   - MacBook 内置触控板与 Magic Trackpad 2 早期固定约 **90–95 Hz**，在较新 macOS（macOS 16+ / 高刷 ProMotion 机型）上可达 **120–125 Hz**（参考 [MagicTrackpad2 轮询率实测](https://github.com/vitoplantamura/MagicTrackpad2ForWindows/issues/18)）。
   - Qt 通过 `QWheelEvent` 将 AppKit 的 `NSEvent` 包装为 `pixelDelta()`（点坐标，受系统滚动速度和自然滚动方向影响）与 `phase()`：
     - `Qt::ScrollBegin`（手指放上开始滑，对应 `NSEventPhaseBegan`）
     - `Qt::ScrollUpdate`（持续拖动，对应 `NSEventPhaseChanged`）
     - `Qt::ScrollEnd`（手指离开触控板，对应 `NSEventPhaseEnded`——**注意此帧 `pixelDelta` 往往为 `(0, 0)`**）
     - `Qt::ScrollMomentum`（系统合成的惯性衰减阶段，对应 `NSEvent.momentumPhase`）
2. **为什么必须丢弃 `ScrollMomentum`**：
   - 正如 `MirrorInputItem.cpp` 注释所写，Android 桌面（Launcher）和列表本身自带物理惯性（`OverScroller` / 翻页吸附）。如果把 macOS 的 `ScrollMomentum` 继续注入，等效于手指一直按在屏幕上缓慢拖行，既会打断 Android 自己的翻页动画，又可能导致连续翻多页。
   - **但** `ScrollMomentum` 第一帧的 `pixelDelta` 实际上隐含了 macOS 系统平滑估算出的“离手初速度”，在需要判断用户是“快速甩动（fling）”还是“缓慢停住”时非常有参考价值。

---

## 二、Android 判定 Fling 的速度与时间戳条件

参考 AOSP [ViewConfiguration.java](https://android.googlesource.com/platform/frameworks/base/+/master/core/java/android/view/ViewConfiguration.java)、[VelocityTracker.cpp](https://android.googlesource.com/platform/frameworks/native/+/refs/heads/main/libs/input/VelocityTracker.cpp) 及 [运动感知调研](https://github.com/anomalyco/rune/blob/dev/research/15-research-motion-perception.md)：

1. **速度阈值（`ViewConfiguration`）**：
   - **最小 Fling 速度**：默认 `MINIMUM_FLING_VELOCITY = 50 dp/s`（按屏幕密度缩放为 `50 * density` px/s，在 3x 密度手机上约 **150 px/s**；部分 Launcher 如 Samsung One UI Home 翻页还会要求更高的翻页速度阈值，通常在 300–500 dp/s 级别，或拖动距离超过屏宽 50%）。
   - **最大 Fling 速度**：默认 `MAXIMUM_FLING_VELOCITY = 4000–8000 dp/s`（取决于 Android 版本）。
2. **`VelocityTracker` 的计算窗口与“停止规则”**：
   - **时间窗口（Horizon）**：默认取抬手前最近 **100 ms** 内的 `ACTION_MOVE` 采样点（通过最小二乘 `LSQ2` 或新版默认的冲量模型 `IMPULSE` 计算离手速度）。
   - **停止判定（`ASSUME_POINTER_STOPPED_TIME`）**：如果最后一次有效 `ACTION_MOVE` 与 `ACTION_UP`（或前一个点）之间间隔超过 **40 ms**，或者最后连续两帧坐标相同，`VelocityTracker` 会判定手指已经停住并**清空历史速度**（返回 `0`）。
   - **强依赖 `MotionEvent.eventTime`**：速度 = $\Delta x / \Delta t$。在 `TouchPlanner.java` 中，`eventTime = nowMillis - leadMillis` 使用的是**手机收到网络包的时刻**。如果两个原本相隔 8 ms 的 `Move` 因为 USB/adb 抖动在手机端相隔 1 ms 或 25 ms 到达，算出的速度就会偏差 3–8 倍，导致桌面翻页时而飞过、时而弹回卡住。

---

## 三、scrcpy、Vysor 等主流工具的做法对比

- **scrcpy**：默认把鼠标滚轮和触控板滚动映射为 Android 的 `ACTION_SCROLL`（`SOURCE_MOUSE` 的 `AXIS_HSCROLL` / `AXIS_VSCROLL`）。优点是垂直列表滚动极其省带宽且顺滑；**缺点**是绝大多数 Android 桌面（Launcher）和游戏不响应 `ACTION_SCROLL` 进行左右翻页，用户仍需鼠标左键按住拖拽。
- **Vysor / 远程桌面类工具**：类似 scrcpy，滚轮走 `ACTION_SCROLL`，桌面翻页依赖左键拖拽。
- **GameScrcpy 的差异化设计**：把“带 phase 的触控板双指滑动”直接映射为单指触摸，体验上更接近在 Mac 上直接摸手机屏，但这也意味着**必须严格满足 Android 触摸子系统对采样节奏、速度和时间戳的预期**。

---

## 四、造成当前“桌面左右滑卡顿”的 4 个具体问题与改进方案

对照 [MirrorInputItem.cpp](file:///Users/york/GameScrcpy-macos/src/app/MirrorInputItem.cpp#L288-L335)：

### 问题 1：`ScrollEnd` 零位移抬手破坏了 Fling 速度
- **现象**：`ScrollEnd` 到达时 `pixelDelta` 常为 `(0, 0)`，如果距上次 `ScrollUpdate` 接近或超过 40 ms，或者在抬手前已经减速，桌面会弹回原页，显得“滑不动/卡顿”。
- **改进**：
  - 在客户端维护一个 **100 ms 环形缓冲**（记录 `QWheelEvent` 的时间戳与累加位移），用类似 Android `VelocityTracker` 的方式估算离手瞬时速度 $v$。
  - 若 `ScrollEnd` 时 $v$ 明显大于阈值（用户是“甩手”翻页），在发送 `Up` 之前，若最后一帧距今时间偏长或位移为 0，**沿当前速度方向补 1 个小步 `Move`（保持速度连续）再紧跟 `Up`**；且不要在撞边时直接 `Up` 导致速度归零（当前代码在 `!inside && at` 时做了 `Move(clamped)` + `Up(clamped)`，如果连续几帧都被 clamp 到同一个边缘坐标 `0.0` 或 `1.0`，位移差变为 0，抬手速度直接归零！改为**刚触边那一帧立刻抬手，且不发重复的同坐标 `Move`**）。

### 问题 2：触摸起手位置与屏幕映射比例
- **现象**：`m_swipePoint` 从鼠标当前位置开始累加 `pixelDelta()`。如果鼠标正好停在窗口靠左/靠右 15% 处，往边缘方向滑 50 px 就撞边截断了；且 `pixelDelta()` 是 macOS 逻辑点，直接按投屏窗口像素归一化时，窗口缩放越大/越小，滑动力度会跟着变。
- **改进**：
  - **起手居中或按方向留余量**（可选）：桌面/通用滑动时，或至少在灵敏度上使用**独立于窗口大小的归一化增益**（例如按固定逻辑距离如 300 pt 对应手机屏幕宽度 1.0，或提供一个灵敏度系数），避免小窗太快、大窗太慢。
  - **轴锁定（Axis Lock）**：前 10–15 ms 内若水平位移远大于垂直位移（如 $|\Delta x| > 2|\Delta y|$），轻微抑制垂直抖动，防止桌面左右滑被误判成下拉通知栏或斜向卡顿。

### 问题 3：发送节拍与客户端时间戳
- **现象**：目前 `ControlChannel` 合并 `Move` 后按节拍发包，但手机端 `TouchPlanner.java` 用收包时刻打 `eventTime`，受 USB/adb 传输抖动影响。
- **改进（两级方案）**：
  - **轻量方案（不改协议）**：在客户端以**稳定节拍**（对齐手机刷新率如 60/120 Hz，或每 8 ms 均匀发点）对触控板原始位移做轻度重采样/插值（EMA 平滑 + 匀速补点），避免一帧发 3 个包、下一帧空窗 25 ms，让手机端收包间隔尽量均匀。
  - **彻底方案（若未来升级协议）**：在 `TouchFrame` 中携带相对时间偏移（如 `uint16_t deltaMs`），服务端基于首帧 `downTime` 累加客户端原始时间间隔来填充 `MotionEvent.eventTime`，从根源消除传输抖动对 `VelocityTracker` 测速的干扰。

### 问题 4：兼容传统滚轮与可切换选项
- 保留无 `phase` 的鼠标滚轮走 `ACTION_SCROLL`（当前逻辑已正确保留）。
- 建议对触控板双指滑动增加平滑调优开关，并用环境变量 `GS_TOUCH_TRACE`（[ControlChannel.cpp](file:///Users/york/GameScrcpy-macos/src/app/ControlChannel.cpp#L48) 已有现成埋点！）录制一次桌面左右滑的 `ts / action / x / y` 轨迹，直接验证每帧间隔与抬手末段速度是否达标。
