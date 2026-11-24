# 调研：扩展滤镜（HLSL）在 Metal 上运行方案（G2）

> **日期**：2026-11-24
> **结论**：建议采用 **DXC（DirectXShaderCompiler）+ SPIRV-Cross** 离线随程序发布（或将编译产物缓存到扩展目录），**不建议在运行时引入 Qt ShaderTools 的 `QShaderBaker`**。原因是：Qt ShaderTools 开源版采用 **GPL-3.0** 许可（会与当前 `LGPL-3.0 + Apache-2.0` 工程许可冲突），且其内置 glslang 的 HLSL 前端兼容性较弱；而 DXC（Apache-2.0 with LLVM Exception / NCSA）+ SPIRV-Cross（Apache-2.0）许可干净、对现有 `cbuffer`/`Texture2D`/`SamplerState`/`Sample()` 兼容度最高，且无需修改现有扩展的 HLSL 源码规范。

---

## 一、现有扩展滤镜机制分析（代码现状）

根据 [FilterSource.h](file:///Users/york/GameScrcpy-macos/src/ext/include/gs/ext/FilterSource.h)、[FilterSource.cpp](file:///Users/york/GameScrcpy-macos/src/ext/src/FilterSource.cpp) 及示例 [scanlines.hlsl](file:///Users/york/GameScrcpy-macos/examples/extensions/crt-scanlines/shaders/scanlines.hlsl)，当前扩展滤镜具有非常规整、收敛的契约：

- **指令集**：`//! PARAM`（float/int/bool，最多 64 个）、`//! PASS`（最多 16 个 pass，支持 `scale` 和 `format=rgba8|rgba16f`）、`//! TEXTURE`（最多 8 张 `.png` 查找表纹理）。
- **固定前置注入代码**（由 `filterShaderText()` 生成）：
  - `cbuffer GsFrame : register(b0)`（`SourceSize`、`OriginalSize`、`OutputSize`、`Frame`、`GsPad`）
  - `cbuffer GsParams : register(b1)`（用户参数）
  - `SamplerState Linear : register(s0); SamplerState Point : register(s1);`
  - `Texture2D Original : register(t0); Texture2D Source : register(t1);` 以及 `t2..t9`
  - 为每个 pass 生成固定入口函数：`float4 GsPass_<name>(float4 position : SV_Position, float2 uv : TEXCOORD0) : SV_Target { return <name>(uv); }`
- **当前 macOS 状态**：[FilterChain.cpp](file:///Users/york/GameScrcpy-macos/src/render/src/FilterChain.cpp) 目前为占位实现，直接返回 `video filters are HLSL shaders for Direct3D and do not run on macOS yet`。

---

## 二、候选方案横向对比

| 维度 | 方案 A：DXC + SPIRV-Cross（推荐） | 方案 B：glslang HLSL 前端 + SPIRV-Cross | 方案 C：Qt ShaderTools（`QShaderBaker` 运行时） | 方案 D：Slang 编译器 |
|---|---|---|---|---|
| **编译链路** | HLSL → DXC（`-spirv`）→ SPIR-V → SPIRV-Cross → MSL（再由 Metal 编译或 `QRhi` 加载） | HLSL → glslang（HLSL parser）→ SPIR-V → SPIRV-Cross → MSL | HLSL → `QShaderBaker`（内部调 glslang + SPIRV-Cross）→ `QShader`（含 MSL） | HLSL → Slang → MSL（或 SPIR-V） |
| **能否离线随程序发布** | **可以**。可把 `libdxcompiler.dylib` + `SPIRV-Cross`（静态库）打进 `GameScrcpy.app`，首次加载滤镜时离线转译并缓存 `.metallib`/MSL。 | **可以**。均为 C++ 库，可静态链接进 `gs_render` 或独立工具进程。 | **技术上可随 Qt 发布，但许可风险极高**（见下一行）。 | **可以**。可作为动态库/静态库打包。 |
| **开源许可证** | DXC：[Apache-2.0 with LLVM Exception / NCSA](https://github.com/thefixinhixon/RealSteelRecomp/blob/main/CREDITS.md)<br>SPIRV-Cross：[Apache-2.0](https://doc.qt.io/qt-6/qtshadertools-attribution-spirvcross.html) | glslang：[BSD-3-Clause 等宽松许可](https://github.com/KhronosGroup/glslang/blob/main/LICENSE.txt)<br>SPIRV-Cross：[Apache-2.0](https://doc.qt.io/qt-6/qtshadertools-attribution-spirvcross.html) | Qt ShaderTools 模块开源版多为 **GPL-3.0**（非 LGPL），运行时链接会触发 GPLv3 传染；项目当前仅在构建期使用 `qt_add_shaders`（`qsb` 工具）。 | Apache-2.0 with LLVM Exception（[shader-slang/slang](https://github.com/shader-slang/slang)）。 |
| **对现有 HLSL 滤镜兼容度** | **极高**。DXC 是微软官方 HLSL 编译器，对 `cbuffer`、`register(b*/t*/s*)`、`Texture2D.Sample()`、隐式类型转换、FXC/SM5 风格写法兼容性最好。 | **中等偏低**。官方明确标注为 [*partial front end for HLSL*](https://github.com/KhronosGroup/glslang/blob/main/LICENSE.txt)，且社区中其 HLSL 前端已逐步被边缘化（见 [bgfx 历史回顾](https://daily.dev/sources/bkaradzic)）；Direct3D 常见写法容易报语法错。 | **中等偏低**。底层同样使用 glslang 的 HLSL 前端，[QShaderBaker 源码注释](https://codebrowser.dev/qt6/qtshadertools/src/shadertools/qshaderbaker.cpp.html)也明确注明对 HLSL 的支持存在限制。 | **较高**。语法高度兼容 HLSL（[Slang 文档](https://github.com/shader-slang/slang)），但部分老式 Direct3D 语义与 `register` 细节仍可能需微调。 |
| **产物包体积增量** | 较大（`libdxcompiler.dylib` arm64 约 25–40 MB；SPIRV-Cross 静态库约 1–2 MB）。 | 较小（glslang + SPIRV-Cross 静态库合计约 5–10 MB）。 | 依赖额外分发 `Qt6ShaderTools.framework`。 | 较大（Slang 库约 20–30 MB）。 |
| **工程工作量** | **中**（约 1–2 周） | **中**（约 1–2 周，但后续排错兼容成本高） | **低–中**（API 现成，但需处理许可与 HLSL 兼容问题） | **中**（约 1–2 周） |

---

## 三、推荐方案（DXC + SPIRV-Cross）实施要点

1. **着色器转译与资源绑定映射**：
   - 由于 `FilterSource.cpp` 已经统一生成了标准的 `register(b0/b1)`、`register(t0..t9)`、`register(s0/s1)`，使用 DXC 加 `-spirv -fspv-target-env=vulkan1.1`（并将 `b`/`t`/`s` 映射到统一 descriptor set 或直接由 SPIRV-Cross `CompilerMSL` 重排绑定槽位）可以做到**零改动现有 `.hlsl` 滤镜**。
   - 顶点着色器全屏三角形/四边形无需转译，直接复用一套内置 `QRhi` 顶点着色器（参考现有 [video.vert](file:///Users/york/GameScrcpy-macos/src/render/shaders/video.vert)），注意 Metal NDC 与 UV 原点方向（`QRhi::isYUpInNDC()` / `isYUpInFramebuffer()`）与 Direct3D 的一致性，在注入的 `GsPass_` 入口或内置顶点着色器里统一翻转即可。
2. **与 `QRhi`（Metal）渲染管线集成**：
   - 在 `FilterChain.cpp` 中参考原 Windows D3D11 多 Pass 逻辑，用 `QRhiTexture`（`RGBA8` / `RGBA16F`）和 `QRhiTextureRenderTarget` 实现 Ping-Pong 中间帧缓冲与 `scale` 缩放。
   - 扩展滤镜本身已经限制在“仅像素着色器 + 固定输入”的受限模型内，转译后的 MSL 可通过 `QShaderBaker` 的替代方式——直接封装为 `QShader`（`QShaderCode` 写入 `QShader::MslShader` 源码与手动/SPIRV-Cross 反射出的 `QShaderDescription`）交给 `QRhiGraphicsPipeline`，完全绕开运行时链接 `Qt6ShaderTools` 的 GPL-3.0 问题。
3. **沙盒与离线缓存**：
   - 转译过程可放在独立缓存目录（`~/Library/Application Support/GameScrcpy/cache/shaders/`），按滤镜内容 SHA-256 缓存编译后的 `QShader`/MSL，避免每次启动重新调用 DXC。
   - 若担心 `libdxcompiler.dylib` 体积，也可评估在扩展打包/验证阶段（或独立下载的可选组件）预编译，但在“完全离线、现有扩展包开箱即用”的要求下，内置 `libdxcompiler.dylib` 是兼容性最稳妥的做法。

---

## 四、参考资料

- [KhronosGroup/glslang LICENSE.txt](https://github.com/KhronosGroup/glslang/blob/main/LICENSE.txt)
- [Qt Shader Tools — SPIRV-Cross Attribution](https://doc.qt.io/qt-6/qtshadertools-attribution-spirvcross.html)
- [Qt 6 `qshaderbaker.cpp` Source](https://codebrowser.dev/qt6/qtshadertools/src/shadertools/qshaderbaker.cpp.html)
- [shader-slang/slang](https://github.com/shader-slang/slang)
- [RealSteelRecomp CREDITS.md (DXC / SPIRV-Tools License Reference)](https://github.com/thefixinhixon/RealSteelRecomp/blob/main/CREDITS.md)
- [The bgfx shader compiler, 14 years of not being a compiler](https://daily.dev/sources/bkaradzic)
