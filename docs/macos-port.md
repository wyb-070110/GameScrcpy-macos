# macOS Port Notes

anmacgamer ports [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy) natively to macOS 14 Sonoma and later (Apple silicon and Intel). Platform-specific code lives in dedicated directories so cross-platform core modules stay clean.

## Status

| Stage | Scope | Status |
|:--|:--|:--|
| 1 | Core modules (`base`, `protocol`, `input`, `vision`, `tsstrip`, `blueprint`) building and passing tests on macOS | Done |
| 2 | POSIX platform layer, `adb` and `session`, launching the phone server and streaming on a real phone | Done (verified over USB and wireless debugging; see below) |
| 3 | Video and audio (VideoToolbox, Qt RHI / Metal, SDL3) | Done: VideoToolbox H.264/H.265 + dav1d AV1, SDL3 audio, and display-paced Qt RHI (Metal) rendering |
| 4 | Qt Quick UI, input capture, USB/AOA, and serial ports | Done: Liquid Glass UI, native menu bar and notifications, key/mouse mapping without Input Monitoring permission, Xbox/PS gamepads with rumble, four-mode trackpad swipes, libusb USB enumeration and AOA touch, IOKit `/dev/cu.*` serial ports |
| 5 | Sandboxed scripts, native extensions, OCR, speech, recording, and universal packaging | Done: `gs-scripthost` / `gs-exthost` sandboxing, `.dylib` extensions, Vision OCR, AVFoundation / Speech TTS and voice commands, MP4 recording, Icon Composer icon, universal `.dmg` packaging |

## Not yet

- HDMI capture cards
- Bluetooth LE peripherals
- Custom extension video shader filters (HLSL on Metal)
- TypeScript type checking inside the editor on Intel Macs (scripts still execute)
- Apple Developer ID notarization for `.dmg` releases

## Continuous integration

Each push runs `.github/workflows/macos.yml` on GitHub's Apple silicon runner (`macos-15`) with Apple Clang, building both Debug and Release configurations and running unit tests (the Release run also prints vision and script timing). Device tests are skipped in CI.

## Prerequisites

```bash
xcode-select --install
brew install cmake ninja openssl@3 android-platform-tools
```

- `openssl@3`: SHA-256, HMAC, AES-GCM, and random bytes (replaces Windows CNG).
- `android-platform-tools`: provides `adb`. The phone-side server APK does not need to be built on macOS.
- For the full app (stages 3–5): install Qt 6.10.3 for macOS (`~/Qt/6.10.3/macos`) and run `brew install ffmpeg sdl3 libusb`.

## Building and running unit tests

```bash
cmake --workflow --preset posix-core            # Debug
cmake --workflow --preset posix-core-release    # Release (checks timing tests)
```

CMake finds `openssl@3` in Homebrew's default prefix automatically; pass `-DOPENSSL_ROOT_DIR=$(brew --prefix openssl@3)` if installed elsewhere.

The first run of `gs_platform_tests` creates `GameScrcpy / user-protection-key` in the login Keychain. When macOS prompts after a rebuild, choose **Always Allow**.

## Real-device verification (Stage 2)

1. Enable Developer options and USB debugging on the phone, connect it, and verify `adb devices` lists it.
2. Place `server-release-unsigned.apk` under `android/server/build/outputs/apk/release/` or point `GS_SERVER_APK` at it.
3. Run:

```bash
export GS_ADB_TEST_SERIAL=$(adb get-serialno)
export GS_ADB_EXECUTABLE=$(which adb)
build/posix-core/bin/gs_adb_device_tests
build/posix-core/bin/gs_session_device_tests
```

`gs_session_device_tests` pushes and starts the phone server via adb, opens video, audio, and control channels, checks received frames, cycles sessions repeatedly, and verifies no thread, file descriptor, or memory leaks.

### Wireless debugging

WiFi direct tests only run when `GS_ADB_TEST_SERIAL` is an `IP:port` wireless serial, so run both USB and wireless passes (requires Android 11+ on the same subnet as the Mac).

1. Enable **Developer options > Wireless debugging** on the phone, open **Pair device with pairing code**, and run:

```bash
adb pair <phone-wlan-ip>:<pairing-port> <6-digit-code>
adb connect <phone-wlan-ip>:<debug-port>
export GS_ADB_TEST_SERIAL=<phone-wlan-ip>:<debug-port>
```

2. Run the two device test binaries above.

Notes:

- If the phone runs a VPN (for example FlClash), the Wireless debugging screen may show the VPN interface IP (such as `198.18.0.1`). Use the WLAN address from `adb shell ip -4 addr show wlan0`; both ports listen on all interfaces.
- Mac proxy tools in TUN mode do not need to be disabled when both devices are on the same subnet: LAN traffic uses the physical interface. Across subnets, traffic routes through TUN and WiFi direct cannot reach the Mac.
- Rebooting the phone or switching Wi-Fi disables wireless debugging and changes the port; re-enable it and run `adb connect` again without re-pairing.

### Device test record (2026-11-24)

Apple silicon Mac (macOS 27), Samsung Galaxy Z Fold8 (`SM-F971B`, Android 17), commit `5292501`, Debug build.

| Suite | Transport | Result |
|:--|:--|:--|
| `gs_adb_device_tests` | Wireless | 11 test cases passed, 88 assertions |
| `gs_session_device_tests` | Wireless (2 runs) | 15 test cases passed, 733 assertions |
| `gs_session_device_tests` | USB | 15 test cases passed, 123 assertions |

Key wireless metrics:

- Cold push-to-first-frame: 981 ms; median over 5 consecutive starts: 362 ms (USB: 215 ms).
- WiFi direct only: 417 ms to first frame.
- Lossless screenshot (1248x1972): 139 ms; median control round-trip during capture: 7.7 ms.
- 200 control round-trips under 10% bidirectional packet loss: 82 ms, all ordered correctly.
- 5 consecutive start/stop cycles: FDs 45 -> 45, private memory 11.3 -> 11.5 MB.
- UDP port closes within 58 ms after session end; held for 3 s after disconnect.

Not run in this pass: two phones streaming simultaneously (`GS_ADB_TEST_OTHER_SERIAL`) and recovery after adb server restart (`GS_ADB_TEST_RESTART=1`).

## Platform layer on macOS

| Header | Windows | macOS (`src/platform/src/posix`) |
|:--|:--|:--|
| `TcpStream.h` | Winsock | BSD sockets with `SO_NOSIGPIPE` |
| `UdpSocket.h` | Winsock + qWAVE video marking | BSD sockets, DSCP CS5 + `NET_SERVICE_TYPE_VI`; `interrupt()` sends a self-addressed empty datagram to wake blocking receives |
| `Thread.h` | Thread description and priority | `pthread_setname_np` and QoS classes |
| `Crypto.h` | CNG, DPAPI | OpenSSL; `protectForUser` encrypts with AES-GCM using a Keychain key |

Winsock calls in tests are mapped to BSD sockets through `tests/compat/posix/winsock2.h`, keeping test code largely unchanged.

## Native extensions on macOS

`contributes.native` in `manifest.json` supports a `dylib` entry alongside `dll`; macOS loads only `dylib` (see [Native extensions guide](extensions/native.md#macos)). `gs validate` inspects Mach-O binaries for architecture, exported `gs_ext_main`, and imports (chained fixups, bind opcodes, and symbol table), classifying them into network, file, process launch, and injection categories using macOS symbol tables.

Shared files in `src/exthost` (`Channel`, `MediaRing`, `ImageSection`, `TapSection`, `InProcess`, `Server`, `Hosted`) stay unchanged; their Win32 calls map to POSIX via `src/exthost/src/posix/compat/windows.h` (`posix/Compat.cpp`):

| Windows | macOS |
|:--|:--|
| Event | Pipe (`SetEvent` writes a byte; wait drains it, auto-reset) |
| File mapping | `shm_open` followed immediately by `shm_unlink` |
| Process / wait multiple | `kqueue` (`EVFILT_PROC`, `EVFILT_TIMER`) and `poll` |
| `DuplicateHandle` | Per-`gs-exthost` Unix domain socket passing FDs via `SCM_RIGHTS`; closing the app end makes `gs-exthost` exit, replacing job objects |
| AppContainer (restricted mode) | `sandbox_init` profile (`posix/Container.cpp`); files handed to the host later use sandbox extension tokens |
| Crash minidump | `crash.txt` and logs (`src/platform/src/posix/CrashHandler.cpp`) |

Only the `gs-exthost` launch block in `Hosted.cpp` is guarded by `#if defined(_WIN32)`; macOS uses `posix/Launch.inc` and `posix/Spawn.cpp`. Covered by `gs_ext_tests` (Mach-O parsing, both bind formats and stripped binaries), `gs_mockhost_tests` (in-process and `gs-exthost`), and `gs_exthost_tests` (shared memory round-trips, crash reporting, restricted sandbox, and exit when the parent app dies).
