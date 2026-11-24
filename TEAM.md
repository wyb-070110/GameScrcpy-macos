# GameScrcpy-macos 分工与协作规则（TEAM）

> 由 Claude（总协调）维护。用户只和 Claude 对接：有问题、要下载、要用户在 Mac 上点授权，都写进自己的日志里，由 Claude 汇总后去问用户。
> 现状、构建命令、代码红线见根目录 `HANDOFF.md` 与 `AGENTS.md`，开工前先读这两份。

## 1. 角色

| 成员 | 工作位置 | 分支 | 职责 |
|---|---|---|---|
| **Claude**（Claude Code 桌面版） | `~/GameScrcpy-macos` | `ccr-e32722c2-75w11u`（集成主线） | 总协调；关键路径开发；审查并合并其他成员的分支；维护 `HANDOFF.md`、`TEAM.md`；推送 |
| **Antigravity** | `~/GameScrcpy-macos-antigravity`（独立 git worktree） | `mac/antigravity` | 边界清晰的 macOS 原生模块开发，自己编译、测试、提交 |
| **Gemini 桌面版** | 不碰代码 | — | 调研、用户文档、方案对比；产出写到 `docs/handoff/gemini/` |

Antigravity 的模型：选它能用的推理强度最高的一档 Gemini（例如 Gemini 4 Carbon High；没有 High 就用 Medium）。

## 2. 协作红线（违反会互相覆盖代码）

1. **各用各的文件夹。** Antigravity 只在 `~/GameScrcpy-macos-antigravity` 里改代码、编译、提交；不要在 `~/GameScrcpy-macos` 里改任何文件，也不要在那里 `git checkout` / `git switch`（那是集成主线，Claude 在用）。
2. **只提交自己改的文件。** 用 `git add <具体路径>`，不要 `git add -A` / `git commit -a`。
3. **不推送、不改别人的分支、不 rebase 主线。** 做完一项就提交到自己的分支，并在日志里写一句"可合并"；Claude 审查后合并进主线并推送。开新任务前先 `git merge ccr-e32722c2-75w11u` 跟上主线。
4. **任务表里写了"负责文件"。** 只改自己负责的文件；必须动别人负责的文件（例如共用的 `src/app/Info.plist.in`、`CMakeLists.txt`）时，改动要小，并在日志里写明改了哪几行。
5. **手机和程序是共用的。** 同一时间只有一个 GameScrcpy 在连手机。Antigravity 测试自己编出来的程序时，用独立的数据目录：`GS_DATA_DIR=~/GameScrcpy-data-antigravity`，以免和用户正在用的程序抢同一份设置和单实例锁。需要真机时在日志里说一声，由 Claude 安排。
6. **不联网下载新依赖。** 需要下载（例如 Universal 版 FFmpeg）时，在日志里写清楚包名、来源、大小和用途，由 Claude 问用户。
7. **用户可见文字一律英文**，再更新 `src/app/i18n/gamescrcpy_zh_CN.ts`（规则见 `AGENTS.md`）。

## 3. 日志

每个成员只写自己的日志文件，写在自己的工作位置里：

- Antigravity：`docs/handoff/antigravity.md`（在它的分支里，随合并进主线）
- Gemini：`docs/handoff/gemini/README.md` 与各篇调研
- Claude：根目录 `HANDOFF.md` 的"协同工作记录区"（合并后统一整理）

每条记录写：日期、做了什么、验证了什么（命令和结果）、没做完/没验证的、需要用户或 Claude 做的事。

## 4. 任务表

### Claude（关键路径）

| # | 任务 | 负责文件 |
|---|---|---|
| C1 | **键鼠与手柄捕获**：IOHIDManager 读键盘扫描码与鼠标原始位移，GameController 读手柄与振动；打通按键映射，处理"输入监控"授权 | `src/platform/src/mac/InputCapture.mm`、`KeyCodes.h` |
| C2 | 触控板滑动顺滑度、投屏窗口交互 | `src/app/MirrorInputItem.*` |
| C3 | 审查合并、推送、CI、延迟与性能问题 | 全部 |

### Antigravity（按顺序做）

| # | 任务 | 负责文件 | 完成标准 |
|---|---|---|---|
| A1 | **语音命令与录音**：`SystemSpeech::listen` 用 SFSpeechRecognizer（尽量本机识别）匹配给定短语，`hearFrom` 识别传入的 16 kHz PCM，`record` 用 AVSpeechSynthesizer 写出 16 kHz 单声道 PCM；在 `Info.plist.in` 加麦克风与语音识别用途说明 | `src/platform/src/mac/Speech.mm`、`src/app/Info.plist.in`（只加两个键） | 新增 macOS 单元测试覆盖 `record` 与短语匹配；`ctest --preset mac-app` 全过；日志写明需要用户授权的步骤 |
| A2 | **USB**：`listUsbPhones` 用 IOKit 列出安卓手机（序列号、名称、VID/PID，有 adb 接口时 `debugging=true`），`UsbWatcher` 用 IOKit 通知报告插拔，`UsbDevice` 的控制传输（AOA 用） | `src/platform/src/mac/Usb.mm` | 接上手机时能列出 `RFGL71YMDHD` 且 `debugging=true`；插拔触发回调；测试或日志给出实测结果 |
| A3 | **打包**：`packaging/macos/` 下的脚本把 `GameScrcpy.app` 做成可单独分发的包（`macdeployqt` 带上 QML 与 Qt 框架、签名、`.dmg`）；另写一份 Universal（arm64 + x86_64）构建方案：需要哪些依赖的 Universal 版、从哪里来、多大 | `packaging/macos/`、新文档 | 在另一个用户目录里打开 `.dmg` 里的程序能启动；Universal 方案只写方案，下载前经 Claude 问用户 |

### Gemini 桌面版（只出文档）

| # | 任务 | 产出 |
|---|---|---|
| G1 | **macOS 使用指南**（中文为主，附英文）：安装、首次打开时的系统授权（本地网络、输入监控、麦克风）、USB 与无线调试连接、常见问题 | `docs/handoff/gemini/macos-guide.md` |
| G2 | **调研：扩展滤镜在 Metal 上运行**。扩展滤镜是 Direct3D 的 HLSL 像素着色器（见 `src/ext/` 的 FilterSource 与 `docs/extensions/`）。比较 DXC/SPIRV-Cross、glslang 的 HLSL 前端、Qt ShaderTools 等方案：能否离线随程序发布、许可证、对现有滤镜的兼容程度、工作量 | `docs/handoff/gemini/research-filters.md` |
| G3 | **调研：Universal 版与公证**。FFmpeg、SDL3、OpenSSL 的 Universal 构建或获取方式，Developer ID 签名与公证（notarization）的要求与步骤 | `docs/handoff/gemini/research-universal.md` |

Gemini 桌面版如果不能直接写文件，就把成文的 Markdown 交给用户，存到上面的路径即可，Claude 会读。
