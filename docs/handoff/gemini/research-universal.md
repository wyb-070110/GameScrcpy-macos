# 调研：macOS Universal 版构建与 Apple 公证（G3）

> **日期**：2026-11-24
> **结论**：当前 macOS 构建通过 Homebrew 仅链接单架构（`arm64`）动态库；要发布面向所有 Mac 的 **Universal（`arm64` + `x86_64`）** 安装包，需将 **FFmpeg（含 dav1d）** 分别编译双架构后用 `lipo` 合并（注意修正 `@rpath`），**SDL3** 直接用 CMake `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64"` 编译，**OpenSSL**（若网络/Qt TLS 携带或静态接入）同样双架构编译后 `lipo` 合并；此外还需注意仓库内现有的 **libusb** 与 **TypeScript 编译器** 也是单架构。分发前必须使用 **Developer ID Application** 证书开启 **Hardened Runtime** 由内而外逐层签名，并在 Entitlements 中按需开启 JIT（QuickJS-ng）与 **Disable Library Validation**（否则系统会阻止加载第三方 `.dylib` 扩展），最后通过 `xcrun notarytool` 提交公证并 `staple` 装订票据。

---

## 一、当前依赖架构现状

根据 [third_party/Posix.cmake](file:///Users/york/GameScrcpy-macos/third_party/Posix.cmake) 与 [third_party/README.md](file:///Users/york/GameScrcpy-macos/third_party/README.md)：
- **FFmpeg（9.0.2 + dav1d 1.5.4）**、**SDL3（3.4.18）**、**libusb（1.0.30）**：目前在 macOS 上通过 Homebrew（`/opt/homebrew`）的 `pkg-config` / `find_package` 引入，Homebrew 默认只安装当前机器的单架构（Apple Silicon 上为 `arm64`），直接打包无法在 Intel Mac（`x86_64`）上运行，且绝对路径 `/opt/homebrew/opt/...` 无法直接分发给其他用户。
- **TypeScript（7.0.2）**：当前使用的是 `@typescript/typescript-darwin-arm64` 原生二进制，若做 Universal 分发，需同时处理 `x86_64` 二进制（或合并/按架构分发）。
- **Qt 6.10.3**：macOS 官方在线安装器提供的 Qt 框架通常本身已是 Universal（可用 `lipo -info` 确认 `QtCore.framework/QtCore` 是否同时含 `x86_64` 与 `arm64`）。

---

## 二、核心依赖的 Universal 获取与构建方案

### 1. FFmpeg（`libavcodec`、`libavformat`、`libavutil`、`libswresample` + `dav1d`）

FFmpeg 的 `configure` 脚本在混编双架构汇编时容易冲突，业界标准做法是**分别编译 `arm64` 与 `x86_64` 两套产物，再用 `lipo` 合并**（参考 [Qt Multimedia 官方文档](https://doc.qt.io/qt-6/qtmultimedia-building-ffmpeg-macos.html) 与 [ffmpeg-macos-universal-binary-builder](https://github.com/WayneKoorts/ffmpeg-macos-universal-binary-builder)）：

1. **先编译 `dav1d`（Meson）**：分别以 `arm64` 与 `x86_64` 交叉编译为静态库，并用 `lipo -create` 合并 `libdav1d.a`。
2. **分别编译两套 FFmpeg**：
   - `arm64`：`./configure --enable-cross-compile --arch=arm64 --cc="clang -arch arm64" ...`
   - `x86_64`：`./configure --enable-cross-compile --arch=x86_64 --cc="clang -arch x86_64" ...`
   - 与仓库 Windows 侧保持一致：只启用 LGPL 最小集（`--disable-programs --disable-doc --enable-libdav1d`、`avformat` 仅开 `mp4` 封装与 `file` 协议，开启 macOS VideoToolbox 硬解）。
3. **合并与路径修正**：
   - 使用 `lipo -create arm64/lib/libavcodec.dylib x86_64/lib/libavcodec.dylib -output universal/lib/libavcodec.dylib` 合并所有 `.dylib`（或静态库）。
   - 若为动态库，用 `install_name_tool -id @rpath/libavcodec.XX.dylib` 与 `-change` 把内部相互依赖的绝对路径改写为 `@rpath/...`（参考 [FFmpeg-macOS](https://github.com/ColorsWind/FFmpeg-macOS)）。
   - 预估产物体积：精简版 LGPL 动态库 Universal 约 **15–25 MB**。

### 2. SDL3

SDL3 原生支持 CMake 单次构建 macOS Universal 二进制（参考 [SDL3 CMake 官方文档](https://wiki.libsdl.org/SDL3/README-cmake)）：

```bash
cmake -S SDL3 -B build-sdl3 \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_SYSTEM_NAME=Darwin \
  -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=13.0
cmake --build build-sdl3
cmake --install build-sdl3 --prefix third_party/sdl3-macos
```

- 按上游配置仅保留手柄与 HIDAPI 所需子系统即可，Universal `libSDL3.dylib` / `SDL3.framework` 体积约 **4–6 MB**。

### 3. OpenSSL（如需随包分发）

OpenSSL 的 `Configure` 按目标架构生成不同的 `<openssl/opensslconf.h>` 宏定义（尤其是 `BN_ULONG` / 字节序配置），**不能直接传 `-arch arm64 -arch x86_64` 一次编出**（参考 [OpenSSL macOS 编译实践](https://blog.andrewmadsen.com/2020/06/22/building-openssl-for.html)）：

1. 分别运行 `./Configure darwin64-arm64-cc` 与 `./Configure darwin64-x86_64-cc` 编译安装到两个前缀目录。
2. 用 `lipo -create` 合并 `libssl` 和 `libcrypto`（`.a` 或 `.dylib`）。
3. 若头文件需要共用，需对 `opensslconf.h` 做 `#ifdef __arm64__ ... #else ... #endif` 包装；如非必需（例如 Qt Network 直接使用 macOS 原生 `SecureTransport` / `Network.framework`），可优先确认是否真的需要捆绑 OpenSSL，以减少体积与维护成本。

> **补充提醒**：仓库还依赖 `libusb-1.0`（同样可用 `-arch arm64 -arch x86_64` 或双编 `lipo`，约 300 KB）以及 `TypeScript` 原生 `tsc`（`@typescript/typescript-darwin-arm64` + `@typescript/typescript-darwin-x64`）。

---

## 三、Apple Developer ID 签名与公证（Notarization）要求与步骤

参考 Apple 官方文档：[Notarizing macOS software before distribution](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution) 与 [Hardened Runtime](https://developer.apple.com/documentation/security/hardened-runtime)。

### 1. 前置条件

- **Apple Developer Program** 付费账号（注意：仅 **Account Holder** 角色可创建 `Developer ID Application` 证书，见 [Upload a macOS app to be notarized](https://help.apple.com/xcode/mac/current/en.lproj/dev88332a81e.html)）。
- 准备 App Store Connect API Key（`.p8`）或 Apple ID + App 专用密码，用于 `notarytool` 认证。

### 2. Hardened Runtime 与 Entitlements 配置

Apple 公证**强制要求**所有可执行文件开启 **Hardened Runtime**（`--options runtime`），否则公证直接失败（见 [Resolving common notarization issues](https://developer.apple.com/documentation/security/resolving-common-notarization-issues)）。结合 GameScrcpy 特性，建议准备独立的 `entitlements.plist`：

- `com.apple.security.cs.disable-library-validation`（`true`）：**关键项**。Hardened Runtime 默认只允许加载同一 Team 签名或苹果系统签名的动态库；如果不加此项，用户安装第三方开发者编译的 `.dylib` 原生扩展时会被系统拦截拒绝加载（可仅授予需要加载外部扩展的宿主进程，或按安全策略评估）。
- `com.apple.security.cs.allow-jit` / `com.apple.security.cs.allow-unsigned-executable-memory`：若 QuickJS-ng 或脚本引擎需要可执行内存页，按需开启；纯解释执行则不需要。
- **隐私用途说明（`Info.plist`）**：确保已声明 `NSLocalNetworkUsageDescription`（无线调试/伴侣）、以及后续需要的 `NSMicrophoneUsageDescription`、`NSSpeechRecognitionUsageDescription` 等。

### 3. 完整签名、打包与公证流水线

1. **收集依赖并修正 rpath**：使用 `macdeployqt` 将 Qt 框架与 QML 插件打入 `GameScrcpy.app`，并将内置工具（`gs-exthost`、`gs-scripthost`、`gs`、`adb`）与 Universal 动态库放入 `Contents/MacOS/` 或 `Contents/Frameworks/`，确保 `otool -L` 无任何指向 `/opt/homebrew` 或 `~/Qt` 的绝对路径。
2. **由内而外逐层签名（Inside-Out）**：必须先签最内层的 `.dylib`、`.framework` 和子可执行程序，最后再签外层 `.app`（切忌仅依赖 `--deep`，公证要求每个嵌套可执行体都带 `--options runtime --timestamp`）：
   ```bash
   # 1) 签名所有动态库与框架
   codesign --force --sign "Developer ID Application: <Name> (<TEAMID>)" \
     --timestamp --options runtime \
     GameScrcpy.app/Contents/Frameworks/*.dylib
   # 2) 签名内置辅助可执行程序（gs-exthost、gs-scripthost、gs 等）
   codesign --force --sign "Developer ID Application: <Name> (<TEAMID>)" \
     --timestamp --options runtime --entitlements entitlements.plist \
     GameScrcpy.app/Contents/MacOS/gs-exthost
   # 3) 签名主应用包
   codesign --force --sign "Developer ID Application: <Name> (<TEAMID>)" \
     --timestamp --options runtime --entitlements entitlements.plist \
     GameScrcpy.app
   ```
3. **打包为 `.dmg` 并签名**（对 `.dmg` 本身也执行 `codesign --timestamp --sign ...`）。
4. **提交公证并装订票据**：
   ```bash
   xcrun notarytool submit GameScrcpy-macos.dmg \
     --key AuthKey_XXXX.p8 --key-id XXXX --issuer XXXX-XXXX \
     --wait
   xcrun stapler staple GameScrcpy-macos.dmg
   ```
5. **离线验证**：
   ```bash
   spctl -a -vvv -t install GameScrcpy-macos.dmg
   codesign --verify --deep --strict --verbose=2 GameScrcpy.app
   ```

---

## 四、参考资料

- [Apple Developer — Notarizing macOS software before distribution](https://developer.apple.com/documentation/security/notarizing-macos-software-before-distribution)
- [Apple Developer — Resolving common notarization issues](https://developer.apple.com/documentation/security/resolving-common-notarization-issues)
- [Apple Developer — Hardened Runtime](https://developer.apple.com/documentation/security/hardened-runtime)
- [Apple Help — Upload a macOS app to be notarized](https://help.apple.com/xcode/mac/current/en.lproj/dev88332a81e.html)
- [Qt 6 Documentation — Building FFmpeg from source on macOS](https://doc.qt.io/qt-6/qtmultimedia-building-ffmpeg-macos.html)
- [SDL3 Wiki — CMake Universal Binaries](https://wiki.libsdl.org/SDL3/README-cmake)
- [Building OpenSSL for ARM/Apple silicon Macs](https://blog.andrewmadsen.com/2020/06/22/building-openssl-for.html)
