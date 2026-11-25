# Plan: macOS 14+ and Intel Macs, and an LGPL-only FFmpeg (Claude, 2026-11-25)

## Why

Beta 1 (`GameScrcpy-2.2.1-beta1-macos-arm64.dmg`) bundles the libraries Homebrew installed on the build Mac:

| Library | Needs macOS | Arch | Size | Note |
|---|---|---|---|---|
| libavcodec, avformat, avutil, swresample (FFmpeg 9.1) | 27.0 | arm64 | 13 MB | **Homebrew's build is `--enable-gpl`** and links the encoders below |
| libx264, libx265 | 26.0 / 27.0 | arm64 | 8.4 MB | **GPL** encoders — the app never encodes with them |
| libvpx, libSvtAv1Enc, libmp3lame, libopus, libmpg123, liblzma | 26.0–27.2 | arm64 | 5.7 MB | pulled in by Homebrew's FFmpeg, unused |
| libdav1d | 27.2 | arm64 | 0.7 MB | AV1 software decoder (BSD) |
| libSDL3 | 27.2 | arm64 | 2.1 MB | gamepads, sound |
| libcrypto.3 (OpenSSL) | 27.0 | arm64 | 4.3 MB | the app's crypto |
| libssl.4 / libcrypto.4 | 27.0 | arm64 | 5.4 MB | Qt's TLS plugin |
| Qt 6.10.3 frameworks, adb | 13.0 / 10.14 | **universal** | — | already fine |

Two problems:

1. **Licence.** The Windows build ships a minimal **LGPL** FFmpeg (`third_party/ffmpeg/BUILDINFO.md`: decoders
   h264, hevc, av1, opus, aac; mp4 muxer; file protocol; no GPL parts). Beta 1 instead ships Homebrew's **GPL** FFmpeg
   with x264/x265. Distributing it means the GPL's source-offer terms apply to the download. **Replace the dmg with a
   build using an LGPL FFmpeg as soon as possible** (or add the source offer and GPL notices until then).
2. **Reach.** Everything Homebrew built needs macOS 27.0–27.2 and Apple silicon. Our own code has no deployment
   target set either, so it is built for the build Mac's 27.2.

## What to build (downloads need the user's OK)

All with `MACOSX_DEPLOYMENT_TARGET=14.0`, arm64 and x86_64 (lipo-merged), into `deps/macos/` (not committed;
`third_party/Posix.cmake` then looks there before Homebrew).

| # | Source | From | Download | Build |
|---|---|---|---|---|
| 1 | FFmpeg **9.0.2** (same as Windows, SHA-256 `8c3850…e002e`) | ffmpeg.org release tarball | ~11 MB | configure per arch with the Windows flags, swapping D3D11VA for VideoToolbox: `--enable-videotoolbox --enable-hwaccel=h264_videotoolbox,hevc_videotoolbox,av1_videotoolbox`, `--disable-autodetect --disable-everything --enable-libdav1d --enable-decoder=h264,hevc,av1,libdav1d,opus,aac --enable-parser=h264,hevc,av1,opus,aac --enable-muxer=mp4 --enable-protocol=file`, `--arch=arm64|x86_64 --enable-cross-compile --cc="clang -arch …" --extra-cflags=-mmacosx-version-min=14.0`, then lipo |
| 2 | dav1d **1.5.4** (SHA-256 `686616…5c5a5`) | code.videolan.org | ~1 MB | meson per arch (`--default-library=static`), linked into avcodec as on Windows |
| 3 | SDL3 (same version as Homebrew's 3.4.x) | github.com/libsdl-org | ~5 MB | one CMake build: `-DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=14.0` |
| 4 | OpenSSL 3.x LTS | openssl.org | ~18 MB | `./Configure darwin64-arm64-cc` and `darwin64-x86_64-cc`, `-mmacosx-version-min=14.0`, lipo; one copy for both the app and Qt's TLS plugin |
| 5 | libusb 1.0.x (after A2 merges) | github.com/libusb | ~1 MB | autotools per arch, lipo |
| tools | nasm (x86_64 FFmpeg), meson, ninja | Homebrew / pip | ~10 MB | build tools only, not shipped |

Total downloads about 45 MB. Build time on this Mac about 15–20 minutes.

## Then

- `CMakePresets.json`: `mac-app` gets `CMAKE_OSX_DEPLOYMENT_TARGET=14.0`; a `mac-app-universal` preset adds
  `CMAKE_OSX_ARCHITECTURES=arm64;x86_64` and points at `deps/macos/`.
- Check APIs newer than 14: `grep -rn "@available(macOS"` (only `Speech.mm` uses 27.0, guarded) and build warnings
  with `-Wunguarded-availability-new`.
- `package.sh` then reports "needs macOS 14.0" and the dmg name says `universal`; size drops (no x264/x265/vpx/svt…,
  ~25 MB less).
- Ship as beta 2, replacing beta 1's dmg.
