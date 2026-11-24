# macOS 移植（进行中）

本分支把 GameScrcpy 移植到 Apple Silicon Mac。原则：Windows 代码不动，各系统自己的实现放在单独的目录里，共享文件里只留极少量 `#if defined(_WIN32)`，便于继续合并原项目的更新。

## 进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| 1 | 核心层（base、protocol、input、vision、tsstrip、blueprint）在 macOS / Linux 上编译、测试 | 完成（GitHub Actions macOS arm64 通过） |
| 2 | platform 层 POSIX 实现，adb 与 session 模块，真机启动手机端服务并收流 | 单元测试在 macOS arm64 通过，待接手机验证 |
| 3 | 画面与声音（VideoToolbox、Qt RHI / Metal、SDL3） | 未开始 |
| 4 | Qt Quick 界面与键鼠捕获 | 未开始 |
| 5 | 脚本进程隔离、OCR、录屏、打包 | 未开始 |

## 持续集成

每次推送都会触发 `.github/workflows/macos.yml`，在 GitHub 的 Apple Silicon 虚拟机（`macos-15`）上用 Apple Clang 编译 Debug 和优化两个版本并运行单元测试，优化版本还会打印识图和脚本处理的耗时。真机测试在那里会显示为“跳过”。

## 准备 Mac

```bash
xcode-select --install
brew install cmake ninja openssl@3 android-platform-tools
```

- `openssl@3`：SHA-256、HMAC、AES-GCM 和随机数（Windows 上是 CNG）。
- `android-platform-tools`：提供 `adb`。手机端服务的 APK 不需要在 Mac 上编译。
- 第 3、4 步才需要：Qt 6.10（官方安装器）、`brew install ffmpeg sdl3 libusb`。

## 编译和单元测试

```bash
cmake --workflow --preset posix-core            # Debug
cmake --workflow --preset posix-core-release    # 优化版本，计时类测试会检查耗时
```

构建脚本会自动在 Homebrew 的默认位置找 `openssl@3`；装在别处时加上 `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)`。

`gs_platform_tests` 第一次运行时会在登录钥匙串里创建 `GameScrcpy / user-protection-key`。重新编译后的测试程序再读取它时，macOS 可能弹窗询问，选“始终允许”。

## 真机验收（第 2 步）

1. 手机打开开发者选项和 USB 调试，连上 Mac，`adb devices` 能看到它。
2. 把手机端服务 `server-release-unsigned.apk` 放到 `android/server/build/outputs/apk/release/`，或用 `GS_SERVER_APK` 指向它。
3. 运行：

```bash
export GS_ADB_TEST_SERIAL=$(adb get-serialno)
export GS_ADB_EXECUTABLE=$(which adb)
build/posix-core/bin/gs_adb_device_tests
build/posix-core/bin/gs_session_device_tests
```

`gs_session_device_tests` 会通过 adb 推送并启动手机端服务，打开视频、音频和控制通道，检查收到的画面数据，反复开关会话，并确认线程、文件描述符和内存没有泄漏。全部通过就算第 2 步验收完成。

## 平台层的 macOS 实现

| 头文件 | Windows | macOS（`src/platform/src/posix`） |
|---|---|---|
| `TcpStream.h` | Winsock | BSD 套接字，`SO_NOSIGPIPE` |
| `UdpSocket.h` | Winsock + qWAVE 标记视频流量 | BSD 套接字，DSCP CS5 + `NET_SERVICE_TYPE_VI`；`interrupt()` 额外给自己发一个空数据报来唤醒阻塞的接收 |
| `Thread.h` | 线程描述、优先级 | `pthread_setname_np`、QoS 等级 |
| `Crypto.h` | CNG、DPAPI | OpenSSL；`protectForUser` 用钥匙串里的用户密钥做 AES-GCM |

其余平台头文件（托盘通知、语音、输入捕获、光标、前台窗口等）属于界面层，到第 4 步再实现。

测试里的 Winsock 调用经 `tests/compat/posix/winsock2.h` 映射到 BSD 套接字，测试代码本身基本不用改。
