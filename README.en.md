<div align="center">

<img src="docs/images/logo.png" width="96" height="96" alt="GameScrcpy">

# GameScrcpy (macOS)

**Mirror and automate Android devices on macOS with blueprints and TypeScript (port in progress)**

[![CI](https://img.shields.io/github/actions/workflow/status/wyb-070110/GameScrcpy-macos/macos.yml?style=flat-square&label=macOS%20CI)](https://github.com/wyb-070110/GameScrcpy-macos/actions/workflows/macos.yml)
[![License](https://img.shields.io/badge/license-Apache--2.0-blue?style=flat-square)](LICENSE)
[![Platform](https://img.shields.io/badge/macOS%2014%2B-Apple%20Silicon-000000?style=flat-square)](#requirements)

[简体中文](README.md) · English

[Port status](docs/macos-port.md) · [User guide](docs/README.md) · [Changelog](CHANGELOG.md) · [Upstream project](https://github.com/NuoRan/GameScrcpy)

<img src="docs/images/keymap-editor.png" width="860" alt="Placing keys right on the game picture">

</div>

This repository is the **Apple Silicon macOS port (work in progress)** of [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy). GameScrcpy mirrors an Android phone to your Mac with low latency, lets you place controls right on the game picture, and runs automation logic written as blueprints (nodes and wires) or TypeScript with image search, color matching, OCR, and scheduled tasks. The idea comes from [scrcpy](https://github.com/Genymobile/scrcpy).

The user interface is available in Simplified Chinese and English, and language pack extensions can add more languages.

## Features and macOS port status

### Working on macOS

- **Low-latency video and audio**: VideoToolbox hardware decoding (H.264, H.265, plus AV1 through dav1d) drawn through Qt RHI on Metal (NV12, P010 and planar YUV 4:2:0); phone audio playback through SDL3 (Android 11 and later).
- **Connections**: USB cable; wireless debugging paired with a six-digit code or QR code; WiFi direct (UDP with Reed-Solomon forward error correction and automatic session resumption); companion app over BSD sockets and Bonjour (`_gs-companion._tcp`).
- **Device management and mirroring**: per-device mirror windows or a multi-device grid, trackpad swipe gestures, recording and MP4 export, file and app management, launching apps on an independent virtual display, and end-to-end latency measurement.
- **Blueprints, scripts and OCR**: two-way blueprint / TypeScript editing, script execution isolated in a `sandbox_init`-confined `gs-scripthost` process, text recognition via the macOS Vision framework, and the `gs` command-line tool.
- **AI and local MCP**: AI assistant API keys stored in the macOS Keychain; local MCP server over a Unix domain socket for Claude, Cursor and other AI tools.
- **Extensions**: `.gsext` / `.gsbundle` packages and `.dylib` native extensions (Mach-O import inspection, out-of-process `gs-exthost` with macOS sandbox confinement), plus `gamescrcpy://` URL handling.

### Not yet ported

- **Global keyboard, mouse and gamepad capture**: `InputCapture` is currently a placeholder on macOS (pointer clicks and trackpad swipes inside the mirror window work; global key/mouse capture for keymap gameplay and gamepad input are still pending).
- **USB / AOA and capture cards**: USB hotplug watching, AOA touch transport, HDMI capture cards and system hardware enumeration for extensions are not yet implemented.
- **Extension video filters and voice commands**: custom shader filters are not wired into the Metal pipeline yet; speech synthesis uses macOS voices, while voice command recognition is still a placeholder.

See [docs/macos-port.md](docs/macos-port.md) for detailed architecture notes and device test results.

## Requirements

| | |
|:--|:--|
| Mac | Apple Silicon Mac running macOS 14 Sonoma or later |
| Phone | Android 9 or later; audio and wireless pairing need Android 11 or later |
| Connection | A USB cable, or Wi-Fi / Ethernet on the same local network as the Mac |

## Building from source

### 1. Install dependencies

```bash
xcode-select --install
brew install cmake ninja openssl@3 ffmpeg sdl3 libusb android-platform-tools
```

Install **Qt 6.10.3 for macOS** at `~/Qt/6.10.3/macos` (or pass `-DCMAKE_PREFIX_PATH=<path>` when configuring).

### 2. Configure, build and run

```bash
# Build the full macOS app bundle (GameScrcpy.app)
cmake --preset mac-app
cmake --build --preset mac-app

# Try the mirror window without a phone
./build/mac-app/bin/GameScrcpy.app/Contents/MacOS/GameScrcpy --demo-device tests/data/video/emulator_h265.media
```

To build and test only the Qt-free core libraries:

```bash
cmake --workflow --preset posix-core
```

### 3. First-run permissions on macOS

- **Local Network (macOS 15 and later)**: the first time you use wireless debugging pairing, WiFi direct or the companion app, allow `GameScrcpy` when macOS asks to find devices on local networks (or enable it under **System Settings > Privacy & Security > Local Network**).
- **Data and logs**: settings, profiles, extensions and logs live in `~/Library/Application Support/GameScrcpy`.

## Getting started

1. On the phone, turn on "Developer options" and "USB debugging" (or "Wireless debugging").
2. Connect the phone over USB and tap Allow, or pair it on the Devices page under Wireless debugging.
3. On the Devices page, choose "Mirror".
4. Press `F1` for the illustrated built-in help covering the mirror window, key mapping editor, profiles, scripts and extensions.

## Documentation

| Topic | Where |
|:--|:--|
| macOS port progress and device verification | [docs/macos-port.md](docs/macos-port.md) |
| User guide (the same pages as F1 in the app) | [docs/README.md](docs/README.md) |
| Script API | [sdk/docs](sdk/docs/README.md) |
| Writing extensions | [docs/extensions](docs/extensions/README.en.md) |
| Architecture | [docs/architecture.md](docs/architecture.md) |
| Protocol | [protocol/SPEC.md](protocol/SPEC.md) |
| Contributing guidelines | [CONTRIBUTING.en.md](CONTRIBUTING.en.md) |

## Feedback

- macOS port issues: [GameScrcpy-macos Issues](https://github.com/wyb-070110/GameScrcpy-macos/issues). Attach the archive exported from "Settings > About > Export diagnostics" when reporting a bug.
- Upstream project: [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy).
- Report security issues privately as described in [SECURITY.md](SECURITY.md).

## License and acknowledgements

GameScrcpy is released under the [Apache License 2.0](LICENSE).

- [NuoRan/GameScrcpy](https://github.com/NuoRan/GameScrcpy): the upstream project this port builds on.
- [scrcpy](https://github.com/Genymobile/scrcpy): the pioneer of Android mirroring, where the idea of GameScrcpy comes from.
- [QtScrcpy](https://github.com/barry-ran/QtScrcpy): the base of GameScrcpy 1.x.
- The third-party components (Qt, FFmpeg, dav1d, SDL, libusb, QuickJS-ng, TypeScript, QWindowKit, nlohmann/json, miniz, QR Code generator, Lucide, doctest, tl::expected), their versions and licenses are listed in [third_party/README.md](third_party/README.md) and in the app under "Settings > About > Open-source licenses".
