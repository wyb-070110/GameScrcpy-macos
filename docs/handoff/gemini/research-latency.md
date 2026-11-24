# 调研：有线投屏延迟忽高忽低与 120 Hz 编码积压（G5）

> **日期**：2026-11-24
> **结论**：对照 [MediaCodecSessionFactory.java](file:///Users/york/GameScrcpy-macos/android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java) 与 [BlockModelSession.java](file:///Users/york/GameScrcpy-macos/android/server/src/main/java/com/gamescrcpy/server/video/BlockModelSession.java) 源码，三星 SM-F971B（120 Hz）出现“跟随帧率积压到 8 秒、限 60 fps 降至平均 5 ms / 最大 33 ms 仍感不稳”有两个高度耦合的根因：
> 1. **120 Hz 自动切入 BlockModel 与 `KEY_OPERATING_RATE = Short.MAX_VALUE`**：源码中 `INPUT_SURFACE_FPS_LIMIT = 100`，不限帧时 120 Hz 会进入 `BlockModelSession`（4 帧在途 + `ImageReader` 7 帧缓冲），且对高通编码器同时设置了 `KEY_OPERATING_RATE = 32767`，容易在三星折叠屏（散热受限）上触发编码器/VPU 队列积压或热节流降频；限到 60 fps 后回退到普通 `Surface` 输入，积压立刻消失。
> 2. **未显式设置码率控制模式（默认 VBR）+ `adb forward` 转发抖动 + 电脑端仅看均值**：画面静止转剧烈运动时 VBR 瞬时码率尖峰叠加 USB/adb 转发队列，造成 5 ms → 33 ms 的周期性尖峰，而 [SessionMetrics.cpp](file:///Users/york/GameScrcpy-macos/src/app/SessionMetrics.cpp) 目前只展示平均值，掩盖了长尾抖动（P95/P99 与帧间隔抖动）。

---

## 一、MediaCodec 低延迟相关键与当前代码核对

基于 [Android MediaFormat 官方文档](https://developer.android.com/reference/android/media/MediaFormat)、[MediaFormat.KEY_LATENCY](https://learn.microsoft.com/en-us/dotnet/api/android.media.mediaformat.keylatency?view=net-android-35.0) 及 [Codec2/Vendor 参数实践](https://juejin.cn/post/7627405638160793627)，逐项核对仓库当前实现（`MediaCodecSessionFactory.java`）：

| MediaFormat 键 | 作用 | 仓库当前现状 | 针对三星 / 高通 120 Hz 的分析 |
|---|---|---|---|
| `KEY_PRIORITY` | `0` = 实时优先（低延迟），`1` = 尽力而为（省电）。[scrcpy #6238](https://github.com/Genymobile/scrcpy/issues/6238) 实测 `priority=0` 可在画面剧变时减 1–3 帧延迟。 | **已设为 `0`**（全局） | 保持不变。 |
| `KEY_MAX_B_FRAMES` | B 帧需要参考未来帧，会直接引入重排延迟。 | **已设为 `0`**（Android 10+） | 保持不变；需检查 `getOutputFormat()` 返回的 `reorder depth` 是否确实为 0。 |
| `KEY_LATENCY` | 编码器允许的输入到输出缓冲帧数（[官方说明](https://learn.microsoft.com/en-us/dotnet/api/android.media.mediaformat.keylatency?view=net-android-35.0)）。 | **已设为 `1`** | 部分 Android 12+ 编码器支持 `0`（零帧延迟），但部分 ROM 会拒收或忽略 `0`；可探测或在 `0` 失败时回退 `1`。 |
| `KEY_OPERATING_RATE` | 告知硬件编解码器目标运行帧率以调整 VPU 时钟。 | **仅对高通（`c2.qti.*`/`omx.qcom.*`）设为 `Short.MAX_VALUE`（32767）**，非高通未设 | **高风险点**：1) 传 `32767` 在部分高通/三星机型上会导致 VPU 锁高频引发热降频或内部码率控制失准；2) 若设备为 Exynos（`c2.exynos.*`）则完全未设。建议改为显式设为 `fps`（或 `min(fps * 2, 240)`）并在所有硬编上尝试。 |
| `KEY_INTRA_REFRESH_PERIOD` | 用周期性帧内刷新替代大体积 I 帧，消除关键帧码率尖峰。 | 仅在 `settings.intraRefreshMs() > 0` 时设置 | 有线模式下当前 `KEY_FRAME_INTERVAL_SECONDS = 3600`，平时几乎不发 I 帧，但切场景/恢复时单帧体积仍可能暴涨；若需平滑码率尖峰可测试开启。 |
| `KEY_BITRATE_MODE` | `BITRATE_MODE_CBR`（恒定码率）vs `BITRATE_MODE_VBR`（可变码率）。 | **未设置（默认 VBR）** | **核心抖动源之一**：VBR 在静止画面后突然滑动桌面/切画面时，会输出比均值大数倍的瞬时帧，直接造成 USB/adb 发送尖峰（5 ms → 33 ms）。实时投屏建议测试 `CBR`（带 fallback）。 |
| 高通/三星私有键 | 高通：`vendor.qti-ext-enc-low-latency.enable=1`（[scrcpy #6238](https://github.com/Genymobile/scrcpy/issues/6238) 实测在骁龙 8 Gen 3 上再降 1–2 帧且无画质劣化）、`vendor.rtc-ext-enc-low-latency.enable=1`。三星/Exynos 侧无稳定公开通用低延迟私有键，依赖标准 `KEY_LATENCY`/`KEY_PRIORITY`/`KEY_OPERATING_RATE`。 | 已对高通设 `vendor.qti-ext-enc-low-latency.enable=1` | 若该键被三星 One UI 魔改的 `c2.qti.*` 拒收，会走回退；可补充尝试 `vendor.rtc-ext-enc-low-latency.enable=1`（同样带 try-catch 回退）并在日志打印实际生效的 `outputFormat` 与编码器名称。 |

---

## 二、scrcpy 上游与社区对三星机型的经验

结合 [scrcpy #5676](https://github.com/Genymobile/scrcpy/issues/5676)、[scrcpy #6742](https://github.com/Genymobile/scrcpy/issues/6742)、[scrcpy #6418](https://github.com/Genymobile/scrcpy/issues/6418) 及 [scrcpy #6238](https://github.com/Genymobile/scrcpy/issues/6238) 的社区案例：
1. **三星 120 Hz LTPO 动态刷新率 + 折叠屏温控**：Galaxy Z Flip 系列采用自适应刷新率（1–120 Hz）且机身紧凑易触发热节流。画面静止时屏幕降频、滑动时瞬间拉到 120 Hz，若不限制送入编码器的帧率（`max-fps-to-encoder`），编码器瞬间吃进 120 fps 高分辨率帧，容易超出实时编码吞吐导致队列雪崩（积压至数秒）。
2. **高码率与分辨率放大抖动**：社区案例（如 [#6418](https://github.com/Genymobile/scrcpy/issues/6418)）显示，高码率（≥ 16 Mbps）配合原生高分辨率在部分三星机型上会引发编码器周期性卡顿/延迟尖峰，适度降低码率或设 `max-fps=60` 是最立竿见影的缓解手段。
3. **本仓库特有的 `BlockModelSession` 分水岭**：本仓库在 `peakFps > 100` 时会自动启用 `BlockModelSession`（`ImageReader` + `HardwareBuffer` 异步喂帧），限到 60 fps 时不走此路径。建议优先用环境变量 `GS_BLOCK_INPUT=0` 实测 120 Hz 下禁用 BlockModel 是否还会积压到 8 秒，以彻底区分是“120 Hz 编码器算力/热节流”还是“BlockModel 队列积压”。

---

## 三、USB 2 / USB 3 与 `adb forward` 对抖动的影响

参考 [scrcpy #5940](https://github.com/Genymobile/scrcpy/issues/5940) 与 [scrcpy #3896](https://github.com/Genymobile/scrcpy/issues/3896)：
- **多跳转发开销**：有线模式走 `gs-server (localabstract)` → 手机 `adbd` → USB Bulk 传输 → Mac 端 `adb` 守护进程 → `127.0.0.1` TCP → `GameScrcpy`。`adb` 在单进程内同时复用控制流、视频流与日志流，大体积视频帧会短暂阻塞同一条 USB 通道上的其他包，产生几毫秒到十几毫秒的抖动。
- **USB 2.0 vs USB 3.x**：很多手机附带的充电线仅支持 **USB 2.0（480 Mbps 理论带宽，实际有效吞吐约 30–35 MB/s，微帧 125 µs）**；当外接扩展坞共享同一 USB 2.0 Hub（键盘、鼠标、网卡等）或线材接触不良触发 USB 重传时，会出现 10–30 ms 的突发性传输毛刺。换用支持 USB 3.2 Gen 1/2（5–10 Gbps）的直连数据线能显著降低突发大帧的传输排队时间。
- **TCP 缓冲与 Nagle**：需确认 `TcpStream`（客户端与服务端）均已开启 `TCP_NODELAY`；否则小控制包或尾包可能撞上 Delayed ACK（最高 40 ms 抖动）。

---

## 四、可在电脑端新增的诊断指标

目前 [SessionMetrics.cpp](file:///Users/york/GameScrcpy-macos/src/app/SessionMetrics.cpp) 每 500 ms 只统计均值（`encodeMs`、`decodeMs`、`networkMs`、`captureToReceiptMs`），单帧 33 ms 的尖峰会被 30 帧的均值稀释成“平均 5 ms”，看不出抖动来源。建议在电脑端增加以下指标（可输出到诊断浮层或导出 CSV/日志）：

1. **分阶段 P95 / P99 与 Max 延迟**：把 `encodeMs`、`networkMs`、`decodeMs`、`arrivalToRenderMs` 从单纯均值扩充为 `avg / p95 / max`，一眼看出 33 ms 尖峰到底发生在手机编码、USB/adb 传输，还是 Mac 端 VideoToolbox 解码/Metal 渲染。
2. **帧到达间隔抖动（Inter-Arrival Jitter）与 PTS 间隔对比**：对比 `(receivedUs[i] - receivedUs[i-1])` 与 `(ptsUs[i] - ptsUs[i-1])`。如果 PTS 间隔均匀（16.7 ms）而到达间隔忽大忽小（2 ms / 35 ms），根因就在 **USB/adb 传输或编码器批量吐帧**；如果 PTS 本身不均，根因在 **手机屏幕 LTPO 刷新率波动或采集端丢帧**。
3. **单帧体积（Frame Bytes）与传输延迟相关性**：记录大帧（> 100 KB）出现时 `networkMs` 是否同步飙升，可直接验证是否为 VBR 码率尖峰打满 USB/adb 通道。
4. **服务端发送队列指标回传**：回传 `VideoSendQueue.droppedFrames()` 与 `waitedUs()`，确认手机端发送队列是否出现瞬时排队。

---

## 五、可落地的优化建议清单

| # | 优化项 | 修改文件 | 预期效果 | 风险与回退 |
|---|---|---|---|---|
| 1 | **排查 120 Hz 积压：测试 `GS_BLOCK_INPUT=0` 并收紧 `BlockModel` 在途帧数**（如将 `BLOCK_MODEL_MAX_IN_FLIGHT` 从 4 降到 2，或在连续超限时自动降帧/回退 Surface） | `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java`<br>`android/server/src/main/java/com/gamescrcpy/server/video/BlockModelPump.java` | 快速确认并消除 120 Hz 下 8 秒级编码器积压；将最大在途延迟压在 1–2 帧内。 | 在途数过小可能在高负载时略微掉帧；可用 `GS_BLOCK_INPUT` 开关对比验证。 |
| 2 | **修正 `KEY_OPERATING_RATE` 并在非高通机型也设置**（从 `Short.MAX_VALUE` 改为实际目标帧率如 `fps` 或 `Math.min(fps * 2, 240)`，并对所有硬编带 try-catch 配置） | `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java` | 避免三星折叠屏 VPU 锁满频导致热节流周期性降频，同时让 Exynos 等非高通编码器也能进入实时频点。 | 极少数旧 ROM 可能拒收 `KEY_OPERATING_RATE`，已有的 `codec.reset()` 回退逻辑可兜底。 |
| 3 | **尝试 `KEY_BITRATE_MODE = BITRATE_MODE_CBR`（可做成配置或实验项）** | `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java` | 压平画面突变时的单帧码率尖峰，减少 33 ms 级别的传输/解码突发抖动。 | 静止画面占用带宽略增、复杂场景画质略低于同码率 VBR；部分编码器不支持 CBR，需检测 `EncoderCapabilities.isBitrateModeSupported()` 后再设。 |
| 4 | **在日志中记录 `encoderName`、`outputFormat`（实际 `latency` / `reorder`）及尝试 `vendor.rtc-ext-enc-low-latency.enable=1`** | `android/server/src/main/java/com/gamescrcpy/server/video/MediaCodecSessionFactory.java` | 明确三星 SM-F971B 实际使用的编码器名与是否真正启用了低延迟模式，避免盲调。 | 风险极低，私有键失败自动回退。 |
| 5 | **确认有线通道 `TCP_NODELAY` 并增加电脑端 P95/Max 与到达间隔抖动统计** | `src/platform/src/posix/TcpStream.cpp`（核对 `TCP_NODELAY`）<br>`src/app/SessionMetrics.h` / `src/app/SessionMetrics.cpp`<br>`src/app/DeviceManager.cpp`（诊断浮层展示） | 消除潜在 Nagle 延迟，并让 33 ms 尖峰精准定位到编码 / USB / 解码 / 渲染四段中的哪一段。 | 仅增加少量环形缓冲统计开销，无行为风险。 |
