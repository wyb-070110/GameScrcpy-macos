# Antigravity — tonight's work (2026-11-25, set by Claude)

The user is asleep. Work through this list on your own; do not wait for answers. Rules from `TEAM.md` still hold:
work only in `~/GameScrcpy-macos-antigravity` on branch `mac/antigravity`, `git add <paths>` only, never push, no
downloads. Log every step in `docs/handoff/antigravity.md` (date, what, how verified, what is left).

Build and test with:

```bash
cmake --preset mac-app && cmake --build --preset mac-app && ctest --preset mac-app -j6
```

If you run out of quota, write where you stopped in the log first.

## 1. Finish A1 (speech) — it is half done in your worktree

`src/platform/src/mac/Speech.mm`, `Info.plist.in`, `Posix.cmake`, `tests/PortProbe.cmake` and
`tests/unit/platform/mac/SpeechMacTests.mm` are changed but not committed. Make it build, make `ctest` pass, then
commit exactly those files. If something cannot work without the user (a permission prompt), leave it reporting
a clear problem and note it in the log.

## 2. Catch up with the main line

```bash
git merge ccr-e32722c2-75w11u
```

Resolve conflicts keeping both sides' intent; build and test again.

## 3. A4 — replace leftover Windows wording (main task)

The app now runs only on macOS, but many texts the user sees still talk about Windows. Find and fix them.

Where to look: `qsTr(...)` in `src/app/qml/**`, `src/ui/**`; `GS_TRANSLATE_NOOP(...)` and `tr(...)` in `src/app/**`;
the help articles (`grep -rl Windows src/app/help` and similar); tray and notification texts; error and failure
advice (`src/app/FailureAdvice.cpp`, `StatusProblem*`); setup guide and tours.

Search terms: `Windows`, `Explorer`, `File Explorer`, `Task Manager`, `Device Manager`, `Control Panel`, `Ctrl`
(in prose), `Alt+` (in prose), `WinUSB`, `Zadig`, `driver`, `%APPDATA%`, `C:\`, `.exe`, `Microsoft Store`,
`SmartScreen`, `Defender`, `administrator`, `UAC`, `Direct3D`, `DirectX`, `HLSL` (user-facing only), `right-click
the tray`, `system tray`, `taskbar`, `Start menu`.

How to change them:

- Say what is true on a Mac: Finder instead of Explorer, the menu bar item instead of the tray icon, Activity
  Monitor instead of Task Manager, System Settings → Privacy & Security instead of Windows settings,
  `~/Library/Application Support/GameScrcpy` instead of `%APPDATA%`, ⌘ Command instead of Ctrl in prose, Option
  instead of Alt.
- Keyboard shortcuts given to Qt (`QKeySequence`, `shortcut:` strings such as `"Ctrl+F"`) stay as they are: Qt
  shows them as ⌘ on macOS. Only change prose that names keys.
- A feature that does not exist on macOS (WinUSB/Zadig driver setup, Direct3D video filters for now): say plainly
  that it is not available on macOS, or remove the text if the feature's UI is hidden on macOS anyway.
- Keep every user-visible string in English in the source, then update `src/app/i18n/gamescrcpy_zh_CN.ts`:
  run `cmake --build --preset mac-app --target GameScrcpy_lupdate`, then fill every `type="unfinished"` entry with
  a natural Chinese translation (no machine-literal wording). `grep -c 'type="unfinished"'` must end at 0.
- Do not change behaviour, only words. Do not touch files under `src/ui/theme/` or `src/ui/controls/` beyond text:
  Claude is redesigning the look there tonight (Liquid Glass) and edits would collide.
- Commit in small pieces (one area per commit), each with a message saying which texts changed.

At the end, write in the log a table: file, old text, new text (short), and anything you were unsure about.
Then write "可合并" (ready to merge).

## 4. If time is left

Read `docs/handoff/gemini/research-latency.md` items 2–4 and write (do not code) in the log which of them you
would try first in `android/server/.../MediaCodecSessionFactory.java` and why. Claude will decide.
