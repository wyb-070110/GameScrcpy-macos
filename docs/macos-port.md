# macOS 移植（进行中）

本分支把 GameScrcpy 移植到 Apple Silicon Mac。原则：Windows 代码不动，各系统自己的实现放在单独的目录里，共享文件里只留极少量 `#if defined(_WIN32)`，便于继续合并原项目的更新。

## 进度

| 阶段 | 内容 | 状态 |
|---|---|---|
| 1 | 核心层（base、protocol、input、vision、tsstrip、blueprint）在 macOS / Linux 上编译、测试 | 完成（GitHub Actions macOS arm64 通过） |
| 2 | platform 层 POSIX 实现，adb 与 session 模块，真机启动手机端服务并收流 | 完成（2026-11-24 真机验收，USB 与无线调试均通过，见下文） |
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

### 无线调试

WiFi 直连那一项只在无线序列号（`IP:端口`）下运行，所以 USB 和无线都要跑一遍。手机需要 Android 11 或更高版本，并且和 Mac 在同一网段。

1. 手机「开发者选项 → 无线调试」打开，点「使用配对码配对设备」，然后在 Mac 上：

```bash
adb pair <手机 WLAN 地址>:<配对端口> <六位配对码>
adb connect <手机 WLAN 地址>:<无线调试端口>
export GS_ADB_TEST_SERIAL=<手机 WLAN 地址>:<无线调试端口>
```

2. 再运行上面两个测试程序。

注意：

- 手机开着 VPN（比如 FlClash）时，无线调试页面显示的可能是 VPN 网卡的地址（例如 `198.18.0.1`）。要用 `adb shell ip -4 addr show wlan0` 查到的 WLAN 地址；两个端口都监听所有网卡。
- Mac 开着代理软件的 TUN 模式也不用关，只要两边在同一网段：发往局域网的流量走物理网卡，不经过 TUN。跨网段时流量会经过默认路由（也就是 TUN），手机也连不回 Mac，WiFi 直连会失败。
- 手机重启或换 Wi-Fi 后无线调试会关掉、端口会变，重新打开后再 `adb connect` 即可，不必重新配对。

### 验收记录（2026-11-24）

Apple Silicon Mac（macOS 27），三星 Galaxy Z Fold8（SM-F971B，Android 17），提交 `5292501`，Debug 构建。

| 测试 | 连接 | 结果 |
|---|---|---|
| `gs_adb_device_tests` | 无线 | 11 个用例通过，88 个断言 |
| `gs_session_device_tests` | 无线（两轮） | 15 个用例通过，733 个断言 |
| `gs_session_device_tests` | USB | 15 个用例通过，123 个断言 |

无线下的主要数据：

- 推送服务到开始收流 981 ms；之后连续 5 次启动，中位数 362 ms（USB 215 ms）。
- 只用 WiFi 直连，417 ms 开始收流。
- 无损截图 1248×1972，139 ms；截图期间控制通道往返中位数 7.7 ms。
- 双向各丢 10% 的链路上 200 次控制往返 82 ms，顺序正确。
- 连续 5 次开关会话：句柄 45 → 45，私有内存 11.3 → 11.5 MB。
- 会话结束后 UDP 端口 58 ms 内关闭；断线后保留 3 s 再关闭。

未运行：两台设备同时收流（需要 `GS_ADB_TEST_OTHER_SERIAL`）、adb 服务重启后恢复（需要 `GS_ADB_TEST_RESTART=1`）。

## 平台层的 macOS 实现

| 头文件 | Windows | macOS（`src/platform/src/posix`） |
|---|---|---|
| `TcpStream.h` | Winsock | BSD 套接字，`SO_NOSIGPIPE` |
| `UdpSocket.h` | Winsock + qWAVE 标记视频流量 | BSD 套接字，DSCP CS5 + `NET_SERVICE_TYPE_VI`；`interrupt()` 额外给自己发一个空数据报来唤醒阻塞的接收 |
| `Thread.h` | 线程描述、优先级 | `pthread_setname_np`、QoS 等级 |
| `Crypto.h` | CNG、DPAPI | OpenSSL；`protectForUser` 用钥匙串里的用户密钥做 AES-GCM |

其余平台头文件（托盘通知、语音、输入捕获、光标、前台窗口等）属于界面层，到第 4 步再实现。

测试里的 Winsock 调用经 `tests/compat/posix/winsock2.h` 映射到 BSD 套接字，测试代码本身基本不用改。
