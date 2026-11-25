# Antigravity — next batch (2026-11-25 midday, set by Claude before handing over)

Claude is out of quota; you carry on alone until it is back. Rules as before (TEAM.md): work only in
`~/GameScrcpy-macos-antigravity` on `mac/antigravity`; no push, no downloads, no brew installs, no Overages; never quit
the user's running GameScrcpy (test with a demo copy and its own `GS_DATA_DIR`); one commit per task; keep
`ctest --preset mac-app` green; log in `docs/handoff/antigravity.md` and mark each task 可合并 when done.

**First:** merge the mainline into your branch: `git merge ccr-e32722c2-75w11u` from `~/GameScrcpy-macos`
(fetch it with `git fetch ~/GameScrcpy-macos ccr-e32722c2-75w11u`). It already contains your A2/A5 (trial merge
reviewed by Claude), the touch timing fix, the icon pipeline and Gemini's icon proposals.

## A6 — The app icon: Gemini's direction B (the user's choice)

Spec: `docs/handoff/gemini/icon.md`, section 方向 B「双屏跃迁与极速帧流（Dual-Frame Stream）」. Gemini only drew
layers for direction A (`docs/handoff/gemini/icon/layers/`): do not use those.

1. Draw B's layers as 1024×1024 full-bleed canvases (transparent PNG or SVG; keep everything inside 164…860 px so the
   system mask never cuts it): back = navy portrait phone `#1D3557`→`#0F2038`, rounded; front = larger landscape
   glass screen `#0A84FF`→`#38BDF8`, overlapping it to the right; on the front screen a bold white right chevron (›).
   Extend `packaging/macos/icon/make_icon.swift` (it already has a `layers` mode and the CoreGraphics helpers) or
   write SVGs by hand.
2. Rewrite `packaging/macos/icon/GameScrcpy.icon/icon.json` and `Assets/`: background
   `"fill": {"linear-gradient": ...}` or `"automatic-gradient"` close to `#F0F6FF`→`#CFE4FF`; one group per layer,
   `"glass": true` on the front screen. Check the JSON keys against Xcode's template
   (`/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/Library/Xcode/Templates/File Templates/macOS/Resource/Icon Composer Icon.xctemplate`)
   and open it in Icon Composer (`/Applications/Xcode.app/Contents/Applications/Icon Composer.app`) to see it.
3. Compile: `xcrun actool packaging/macos/icon/GameScrcpy.icon --compile <dir> --platform macosx
   --minimum-deployment-target 14.0 --app-icon GameScrcpy --output-partial-info-plist <dir>/partial.plist`. This gives
   `Assets.car` and `GameScrcpy.icns`.
4. Ship both: replace `src/app/resources/GameScrcpy.icns`, add `src/app/resources/Assets.car` the same way the
   `.icns` is added in `src/app/CMakeLists.txt` (`MACOSX_PACKAGE_LOCATION Resources`), and add
   `<key>CFBundleIconName</key><string>GameScrcpy</string>` to `src/app/Info.plist.in`. Better still, make CMake run
   actool at build time from the `.icon` (a custom command) so nobody commits binaries by hand — your call.
5. Check: build, then render the bundle's icon as Finder sees it
   (`NSWorkspace.shared.icon(forFile:)` in a small Swift script, see how Claude did it in
   `docs/handoff/daytime-claude.md`) — on macOS 27 it must NOT sit on a grey plate. Also look at 16/32 px. Put
   the renders in `docs/handoff/glass/icon-b-*.png`. Update `packaging/macos/icon/GameScrcpy-1024.png`.

## A7 — If time is left: touch trace tool

`GS_TOUCH_TRACE=<file>` (see `src/app/ControlChannel.cpp`) logs the touch frames sent. Write
`tools/touch_trace.py` (like `tools/frame_trace.py`): per gesture, the moves' spacing in ms, the speed over the last
100 ms before the lift, and the gap between the last move and the lift (Android treats > 40 ms as a stopped finger;
see `docs/handoff/swipe-design.md`). Test it on a trace from a demo copy; do not touch the user's phone.
