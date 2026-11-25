# 参与 anmacgamer 开发

简体中文 · [English](CONTRIBUTING.en.md)

感谢你愿意参与！这份指南说明怎么报告问题、怎么准备 macOS 开发环境、怎么构建与测试，以及提交代码前要注意的约定。

## 报告问题与提建议

- 先在 [Issues](https://github.com/wyb-070110/anmacgammer/issues) 里搜一下有没有人提过。
- 报告问题请用“问题反馈”模板，写清版本、macOS 版本与芯片型号（Apple 芯片或 Intel）、手机型号与 Android 版本、连接方式和复现步骤，并附上程序里“设置 > 关于 > 导出诊断包”生成的文件。
- 安全问题不要公开发 Issue，按 [SECURITY.md](SECURITY.md) 私下报告。

## 提交改动的流程

1. Fork 仓库，从 `main` 拉一个分支。
2. 改动尽量小而完整：一个合并请求只做一件事，附带相应的测试。
3. 本地构建并跑相关测试（见下文），QML 静态检查保持 0 个警告。
4. 提交说明用英文，第一行概括做了什么（不超过 72 个字符），需要时空一行再写原因。
5. 发起合并请求，按模板填写改了什么、怎么验证的。通过检查后会进行审阅。

较大的改动（新功能、改协议、改方案格式）请先开 Issue 讨论，避免白做。

## 仓库结构

| 路径 | 内容 |
|:--|:--|
| `src/` | macOS 端：C++20 + Qt 6 / Qt Quick（Metal）的程序本体，以及命令行工具 `gs`（`src/cli`）；模块划分见 [docs/architecture.md](docs/architecture.md) |
| `android/` | 手机端：`protocol`（协议）、`server`（服务端，纯 Java）、`companion`（伴侣应用，Kotlin） |
| `protocol/` | 协议规范 `SPEC.md`，以及两端共用的金标准测试向量 |
| `sdk/` | 由 `gs api write` 生成的脚本接口类型声明与文档，以及原生扩展的 C 头文件 |
| `tests/` | 单元测试、集成测试、界面测试与测试数据 |
| `examples/` | 示例组件、任务与扩展 |
| `tools/` | 开发与发布脚本：依赖构建（`build_macos_deps.sh`）、源码规则检查、帧率与触控轨迹分析、生成器等 |
| `packaging/macos/` | `.dmg` 打包脚本、应用图标生成脚本与发布说明 |
| `third_party/` | 随仓库提供的第三方依赖，清单见 [third_party/README.md](third_party/README.md) |

## 开发环境

| 工具 | 版本 |
|:--|:--|
| 操作系统 | macOS 14 Sonoma 或更高版本（Apple 芯片或 Intel） |
| Xcode / Command Line Tools | Xcode 16+（重编译 `.icon` 液态玻璃图标资源需 Xcode 26+） |
| Qt | 6.10.3（macOS 套件，含 Qt Quick、Qt Quick Controls 2、Qt Shader Tools、Qt Test） |
| CMake 与 Ninja | CMake 3.28 或更新，生成器为 Ninja |
| 本地开发库 | `brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools` |
| Python | 3.10 或更新 |
| JDK 与 Android SDK（可选） | JDK 17，Android SDK platforms 35 / 36（仅修改 `android/` 时需要） |

配置阶段不联网，请不要引入 `FetchContent` 等构建期网络下载。

## 构建 macOS 端

默认从 `~/Qt/6.10.3/macos` 查找 Qt（若安装在其他路径，配置时传入 `-DCMAKE_PREFIX_PATH=<路径>`）：

```bash
cmake --preset mac-app
cmake --build --preset mac-app
ctest --preset mac-app
```

可用的 macOS / POSIX 预设：

| 预设 | 用途 |
|:--|:--|
| `posix-core` | 只构建和测试不依赖 Qt 的核心库（Debug） |
| `posix-core-release` | 核心库优化构建与基准测试 |
| `mac-app` | 完整的本地开发构建（产出 `GameScrcpy.app`、`gs` 及测试程序） |
| `mac-release` | 面向 macOS 14+ 的通用二进制（`arm64` + `x86_64`）发布构建，使用 `tools/build_macos_deps.sh` 自编译的 LGPL 依赖 |

产物在 `build/<预设>/bin`，包括应用包 `GameScrcpy.app`、命令行工具 `gs`、沙箱宿主 `gs-scripthost` / `gs-exthost` 和各个测试程序。

## 构建 Android 端

先把 `ANDROID_HOME` 设为 Android SDK 的位置，然后：

```bash
cd android
./gradlew build
```

`./gradlew build` 编译 `protocol`、`server` 和 `companion` 三个模块，运行 JUnit 测试和 lint（lint 警告视为错误）。服务端必须保持纯 Java，不引入 Kotlin。依赖版本统一写在 `android/gradle/libs.versions.toml`。

## 测试

- 每个模块有一个 doctest 测试程序（`gs_base_tests`、`gs_protocol_tests`、`gs_input_tests`、`gs_session_tests`、`gs_media_tests`、`gs_script_tests`、`gs_platform_tests`、`gs_hardware_tests`、`gs_render_tests` 等），经 CTest 运行：`ctest --preset mac-app`，只跑某几项用 `-R <正则>`。
- QML 静态检查必须是 0 个警告：`cmake --build --preset mac-app --target all_qmllint`。
- 协议测试读取 `protocol/vectors/` 里的全部金标准向量，C++ 与 Java 两边都必须逐字节通过。
- 需要真机的测试（`gs_adb_device_tests`、`gs_session_device_tests`）只在设置了环境变量 `GS_ADB_TEST_SERIAL`（设备序列号）时运行，否则自动跳过。
- 投屏窗口不接手机也能调试：`./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --demo-device tests/data/video/emulator_h265.media` 会多出一台循环播放录制画面的演示设备；设置 `GS_DATA_DIR` 可隔离测试配置目录。

## 修改协议

1. 修改 `protocol/SPEC.md`。
2. 修改 `protocol/tools/gen_vectors.py`，运行 `python3 protocol/tools/gen_vectors.py` 重新生成向量。
3. 让 C++（`src/protocol`）与 Java（`android/protocol`）两边的测试都通过。

不要手工修改生成的向量文件。

## 修改脚本接口

修改 `src/script/src/Api.cpp` 里的接口表之后，在仓库根目录用刚构建的 `gs` 运行 `./build/mac-app/bin/gs api write sdk`，重新生成 `sdk/`。

## 界面文字与翻译

- 界面文字在代码里写英文（`tr()` / `qsTr()`），中文只放在 `src/app/i18n/gamescrcpy_zh_CN.ts` 里。
- 加了或改了界面文字后运行翻译更新目标（如 `GameScrcpy_lupdate`）更新译文文件，再补齐中文译文。每一条都必须翻译完。
- 程序内帮助在 `src/app/help/`：`index.json` 写明文章的顺序、名称、图标和中英文标题，`en/` 与 `zh_CN/` 下各放一份 Markdown，新文章要加进 `src/app/CMakeLists.txt` 的资源列表。

## 源码规则

`python3 tools/check_source_rules.py` 会检查：

- 产品代码不运行、不解析 `dumpsys`、`wm` 这类面向人的命令输出（R5）。
- 不用固定延时来同步：不 sleep、不用带延时的单发定时器、不用 `postDelayed`（R9）。延时本身就是功能时，在同一行写上 `source-rule-allow R9: <理由>`。
- 面向用户的文字都有译文，源文字是英文。
- `src/ui/controls` 里的控件只从主题取颜色、尺寸和时长。
- 每个 QML `Text` 都写明 `textFormat`。

## 代码约定

- C++20 / Objective-C++20，格式遵循 `.clang-format` 与 `.editorconfig`；标识符与提交说明用英文。
- 让代码自己说清楚：代码里基本不写行内注释，文件开头介绍这个文件的说明除外；需要解释的写进合并请求或文档。上面的 `source-rule-allow` 标记是唯一要求写在行内的说明。
- 界面的颜色、字号、间距、圆角与动效都取自主题模块 `src/ui/theme`，不要写裸数值。
- 新功能要有测试；修缺陷时先写一个会失败的测试。

## 打包发布

1. 使用 `tools/build_macos_deps.sh` 构建面向 macOS 14+ 的通用依赖（FFmpeg LGPL、dav1d、SDL3、OpenSSL、libusb）。
2. 构建 `mac-release` 预设产出通用版应用包。
3. 运行 `packaging/macos/package.sh` 打入 Qt 框架、动态库、`adb` 与签名，生成自包含的 `.dmg` 安装镜像。
4. 若修改了 `packaging/macos/icon/` 下的图标图层或配置，在装有 Xcode 26+ 的环境下运行 `packaging/macos/icon/build_icon.sh` 更新 `Assets.car` 与 `.icns`。

## 许可证

提交新文件或 macOS 移植代码即表示你同意以 [GNU GPL 3.0 或更高版本](LICENSES/GPL-3.0-or-later.txt) 发布你的贡献（新文件开头请附 `SPDX-License-Identifier: GPL-3.0-or-later`）；对沿用文件的修改遵循其原有许可证，详见 [LICENSING.md](LICENSING.md)。参与社区请遵守[行为准则](CODE_OF_CONDUCT.md)。
