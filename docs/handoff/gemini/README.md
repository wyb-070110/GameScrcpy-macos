# Gemini 调研与文档日志

> 角色：GameScrcpy-macos 调研与用户文档成员（只出文档，不改代码）  
> 协同规则见根目录 `TEAM.md`

---

## 2026-11-24

### 做了什么
按 `TEAM.md` 第 4 节完成 G1、G2、G3 三篇文档（未修改任何代码）：
1. **G1**：[`macos-guide.md`](macos-guide.md) — macOS 使用指南（中文为主，附英文版），覆盖源码构建安装、首次打开系统授权（本地网络、输入监控、麦克风/语音识别、Gatekeeper）、USB 与无线调试连接、以及已知问题排查。
2. **G2**：[`research-filters.md`](research-filters.md) — 扩展滤镜（Direct3D HLSL）在 Metal 上运行调研，对比 DXC + SPIRV-Cross、glslang HLSL 前端、Qt ShaderTools (`QShaderBaker`) 与 Slang；给出推荐结论（DXC + SPIRV-Cross，规避 `Qt6ShaderTools` 运行时 GPL-3.0 许可风险并保证 HLSL 兼容度）。
3. **G3**：[`research-universal.md`](research-universal.md) — Universal（`arm64` + `x86_64`）依赖构建（FFmpeg + dav1d、SDL3、OpenSSL，附 libusb/TypeScript 提示）与 Developer ID 签名、Hardened Runtime Entitlements（含第三方 `.dylib` 加载所需的库验证豁免）、`notarytool` 公证及 `stapler` 完整步骤调研。

### 验证了什么
- 代码现状核对：已核对 `src/ext/include/gs/ext/FilterSource.h`、`src/ext/src/FilterSource.cpp`、`examples/extensions/crt-scanlines/shaders/scanlines.hlsl`、`src/render/src/FilterChain.cpp`、`src/render/CMakeLists.txt`、`third_party/Posix.cmake`、`third_party/README.md`、`src/platform/src/mac/InputCapture.mm`、`src/platform/src/mac/Accessibility.mm`、`src/app/CMakeLists.txt`。
- 外部许可、构建方式与 Apple 公证流程均已通过联网检索官方/权威文档核实并附来源链接。

### 没做完 / 需要用户或 Claude 做的事
- 无代码修改（遵守 `TEAM.md` 角色边界）。
- 若后续 Claude / Antigravity 决定采用 **G2（DXC + SPIRV-Cross）** 或 **G3（Universal 依赖预编译包）**，新增二进制依赖需按 `TEAM.md` 红线第 6 条先由 Claude 汇总来源与体积后向用户确认下载。
