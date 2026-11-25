# Licensing / 许可证

## 中文

本仓库是 [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) 的 macOS 移植版，包含两部分代码：

| 部分 | 许可证 | 怎么认 |
|---|---|---|
| 沿用的上游代码（包括为 macOS 修改过的上游文件） | [Apache License 2.0](LICENSES/Apache-2.0.txt)（原文同 [LICENSE](LICENSE)） | 没有 SPDX 标记的文件 |
| macOS 移植新写的代码（2026-11-16 之后新增的文件：macOS 平台层、POSIX 移植、着色器、打包与构建脚本、测试等） | [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) | 文件开头写着 `SPDX-License-Identifier: GPL-3.0-or-later` |

GPL 3.0 与 Apache 2.0 兼容：两部分可以合在一起编译。**发布出去的 macOS 程序（.dmg）作为整体按 GPL 3.0 或更高版本提供**，
这意味着任何人分发它（原样或修改后）都必须以同样条款公开对应的源代码。上游文件本身仍保留 Apache 2.0 与其版权声明。

程序附带的第三方组件各有许可证（程序里“设置 > 关于 > 开源许可”，以及 [third_party/README.md](third_party/README.md)）。
macOS 版自行编译、随程序分发的库：

| 组件 | 版本 | 许可证 |
|---|---|---|
| FFmpeg（avcodec、avformat、avutil、swresample；只含 h264/hevc/av1/opus/aac 解码与 mp4 封装，**不含 GPL 部分**） | 9.0.2 | LGPL 2.1 或更高 |
| dav1d（静态链入 avcodec） | 1.5.4 | BSD 2-Clause |
| SDL3 | 3.4.18 | zlib |
| OpenSSL | 3.5.9 | Apache 2.0 |
| libusb | 1.0.30 | LGPL 2.1 或更高 |
| Qt | 6.10.3 | LGPL 3.0 |
| adb（Android SDK Platform-Tools） | — | Apache 2.0 |

LGPL 库都以动态库形式放在 `GameScrcpy.app/Contents/Frameworks` 里，可以自行替换；它们的源代码与构建方式见
`tools/build_macos_deps.sh`。

## English

This repository is the macOS port of [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) and holds two kinds
of code:

| Part | Licence | How to tell |
|---|---|---|
| Code kept from upstream (including upstream files changed for macOS) | [Apache License 2.0](LICENSES/Apache-2.0.txt) (same text as [LICENSE](LICENSE)) | files without an SPDX tag |
| Code written for the macOS port (files added after 2026-11-16: the macOS platform layer, POSIX ports, shaders, packaging and build scripts, tests) | [GNU GPL 3.0 or later](LICENSES/GPL-3.0-or-later.txt) | files starting with `SPDX-License-Identifier: GPL-3.0-or-later` |

GPL 3.0 is compatible with Apache 2.0, so both build together. **The macOS program as distributed (the .dmg) is
offered as a whole under GPL 3.0 or later**: anyone who distributes it, as is or changed, must publish the
corresponding source under the same terms. Upstream files keep Apache 2.0 and their notices.

Third-party components keep their own licences (in the app under Settings > About > Open-source licences, and in
[third_party/README.md](third_party/README.md)). Libraries the macOS build compiles itself and ships: FFmpeg 9.0.2
(LGPL 2.1+, decoders and the mp4 muxer only, **no GPL parts**), dav1d 1.5.4 (BSD 2-Clause), SDL3 3.4.18 (zlib),
OpenSSL 3.5.9 (Apache 2.0), libusb 1.0.30 (LGPL 2.1+), Qt 6.10.3 (LGPL 3.0), adb (Apache 2.0). The LGPL libraries
are separate dynamic libraries in `GameScrcpy.app/Contents/Frameworks` and can be replaced; their sources and build
are in `tools/build_macos_deps.sh`.
