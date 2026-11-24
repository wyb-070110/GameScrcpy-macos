# Liquid Glass for GameScrcpy (macOS) — Claude's plan

Goal (user, 2026-11-25): the whole app looks like Apple's Liquid Glass (macOS 26): translucent glass surfaces that
show what is behind them, light edges, soft depth, rounder shapes, Apple's system blue accent, SF fonts. The phone
picture stays opaque and untouched.

The theme is token based (`src/ui/theme/`: `Theme.qml`, `ColorTokens.qml`, `Radius.qml`, `Shadow.qml`, `Size.qml`,
`Motion.qml`), and controls in `src/ui/controls/` read the tokens. Work from the tokens outwards, check each step in
the demo window (`GS_DATA_DIR=$TMPDIR/gsdemo …/GameScrcpy --demo-device tests/data/video/emulator_h264.media`,
capture with `screencapture -l <window id>`), and keep `ctest --preset mac-app` green.

1. **Window glass.** Hub and mirror windows: an `NSVisualEffectView` (material `.underWindowBackground` /
   `.hudWindow`, blending behind-window) under the Qt content in `WindowChromeMac.mm`, Qt window and root items
   transparent, so `bgBase` surfaces can become partly see-through. Respect "Reduce transparency"
   (`NSWorkspace.accessibilityDisplayShouldReduceTransparency` → `Theme.reducedTransparency`).
2. **Tokens.** `ColorTokens`: glass fills (white/black at 8–18% instead of solid greys), `strokeStrong` → a light
   top-edge highlight, text colours from Apple's label colours; default accent → system blue `#0A84FF` (dark) /
   `#007AFF` (light), keep the other accent choices. `Radius`: larger (cards 16–20, controls 10–12, pills full).
   `Shadow`: softer and wider. `Motion`: spring-like easing.
3. **A Glass control.** One reusable `GlassSurface` (in `src/ui/controls/`): fill + 1 px inner highlight
   (gradient stroke, brighter at the top) + soft shadow + optional `MultiEffect` blur of what is underneath for
   surfaces that float over the picture (toolbar, menus, dialogs, toasts, the start panel, the side panels).
   Swap `FloatingToolbar`'s face, `Menu`, `Dialog`, `Toast`, `Tooltip`, `SettingCard`, the navigation rail to it.
4. **Controls.** Buttons as capsules with glass fill; switches and sliders in Apple proportions; segmented
   controls; focus rings in the accent colour.
5. **Settings.** Appearance: keep the accent choices, add "System blue" first and make it the default; the
   "Reduce transparency" setting follows the system unless the user changes it.
6. Translations for any new text; screenshots of hub, settings, mirror window with toolbar left/top for the user.

Not part of this: the Windows wording (Antigravity, `antigravity-tonight.md` item 3) — merge its branch before
touching the same QML files' texts.
