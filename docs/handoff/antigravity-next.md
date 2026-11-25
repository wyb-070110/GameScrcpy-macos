# Antigravity — next batch (2026-11-25, set by Claude)

Same rules as `TEAM.md` and `antigravity-tonight.md`: only `~/GameScrcpy-macos-antigravity`, branch
`mac/antigravity`, `git add <paths>`, no push, no downloads, log in `docs/handoff/antigravity.md`. Start with
`git merge ccr-e32722c2-75w11u` (Claude merged your overnight work, thank you — it was clean).

## A2 — USB phones on macOS (`src/platform/src/mac/Usb.mm`)

Replace the empty stub with libusb (already a dependency: Homebrew `libusb`, found by `third_party/Posix.cmake` as
`libusb::libusb`; macOS needs no driver):

- `listUsbPhones()`: Android phones on USB with serial, name, VID/PID; `debugging = true` when an ADB interface
  (class 0xFF, subclass 0x42, protocol 0x01) is present; the driver state `Ready`.
- `UsbWatcher`: plug/unplug callbacks (libusb hotplug, on a thread of its own).
- `UsbDevice::open/controlOut/controlIn/bulk…` as `src/platform/include/gs/platform/Usb.h` asks (AOA uses them),
  `usbLibraryVersion()` returning libusb's version.
- Then the AOA texts that say "not available on macOS yet" (`DeviceManager.cpp`, `AoaSession.cpp`,
  `PictureTouch.cpp`, `SessionController.cpp`, `DevicePresentation.cpp`) can go back to their normal wording once
  AOA works; keep them if it does not.
- Unit tests in `tests/unit/platform/mac/` for what can run without a phone. With the phone plugged in
  (serial `RFGL71YMDHD`) log what `listUsbPhones()` returns. Do not switch the phone into accessory mode yourself:
  write in the log what to try and Claude will run it with the user.

## A3 — a Mac app that installs like any other (`packaging/macos/`)

A script `packaging/macos/package.sh` that, from `build/mac-app/bin/GameScrcpy.app`:

1. runs `macdeployqt` (from `~/Qt/6.10.3/macos/bin`) with `-qmldir=src` so Qt, QML and plugins are inside;
2. copies the non-Qt dylibs it links (FFmpeg, SDL3, libusb, OpenSSL from Homebrew) into `Contents/Frameworks`
   and rewrites their install names (`install_name_tool`), so `otool -L` shows nothing under `/opt/homebrew`;
3. signs ad hoc inside out (helpers `gs-exthost`, `gs-scripthost`, `gs` too) with the entitlements in
   `packaging/macos/GameScrcpy.entitlements` (write it: library validation off for native extensions, see
   `docs/handoff/gemini/research-universal.md`);
4. makes `GameScrcpy-macos-arm64.dmg` with `hdiutil` (app + a link to /Applications).

Check: copy the `.app` to `/tmp`, `otool -L` every binary inside, open it with `open`, and confirm it starts and
the demo device plays (`--demo-device tests/data/video/emulator_h264.media`). Developer ID signing and
notarization need the user's Apple account: only write the steps in `packaging/macos/README.md`.

## If time is left

A5 — serial devices for touch boxes: `src/hardware/src/posix/NoDevicesBackend.cpp` lists nothing; list
`/dev/cu.*` serial ports with IOKit and open them with termios as the Windows backend does with COM ports. Tests
for the parts that need no device.

Write "可合并" in the log after each item that is done.
