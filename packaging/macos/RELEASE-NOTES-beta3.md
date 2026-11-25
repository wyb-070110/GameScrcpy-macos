# anmacgamer 2.2.1 beta 3

**测试版**：第三个 macOS 测试版。 / **Beta**: the third macOS test build.

文件 / File: `GameScrcpy-2.2.1-beta3-macos-universal.dmg`（Apple 芯片与 Intel 通用 / Apple silicon and Intel）

### 相比 beta 2 / Since beta 2
- **触控板滑动像真手指**：手机按电脑的节奏给触摸打时间，松手时带着手指真实的速度；主屏幕快甩只翻一页，列表和抖音上下滑由手机自己的惯性接着走，不再卡顿。默认改为“跟手”。
  / **Trackpad swipes feel like a real finger**: the phone times touches at the Mac's pace and gets the finger's real
  speed at lift-off; a fast fling on the home screen turns one page, and lists scroll on with the phone's own
  momentum. "Direct" is now the default.
- **工具栏可自定义**：更多 → “工具栏按钮…”，可以收起不用的按钮（它们会移到“更多”菜单），也可以把通知栏、快捷设置、文件、延迟测量、迷你窗口等命令放到工具栏上；侧边工具栏所有按钮都能正常显示。
  / **Customisable toolbar**: More → "Toolbar buttons…" hides buttons (they move to the More menu) and adds commands
  such as notifications, quick settings, files, latency and the mini window; a side toolbar draws all its buttons.
- **投屏时关闭手机屏幕**：设置里新开关，连上后手机自动熄屏，画面照常投到 Mac。 / **Turn the phone's screen off
  while mirroring**: a new switch in Settings.
- **拼音输入**：用 Mac 打中文时，正在拼的拼音显示在画面上。 / Pinyin being composed is shown on the picture.
- **新图标**（macOS 26 起为液态玻璃图标）与安装盘图标；项目更名为 anmacgamer（程序名仍是 GameScrcpy）。 / **New
  icon** (a Liquid Glass icon on macOS 26 and later) and disk image icon; the project is now called anmacgamer (the
  app is still GameScrcpy).
- USB 免调试的 AOA 触控与串口设备已经接入 macOS（还没在很多真机上测过）。 / AOA touch over USB and serial devices
  are now wired up on macOS (not yet tested on many devices).

---

## 中文

### 系统要求
- macOS 14 Sonoma 或更新；Apple 芯片与 Intel 的 Mac 都可以。
- 安卓 7 及以上的手机，开启 USB 调试（或安卓 11 以上的无线调试）。不需要另装 adb，程序里已自带。

### 安装
1. 打开 `.dmg`，把 GameScrcpy 拖进“应用程序”。
2. 第一次打开：本测试版没有 Apple 开发者签名，请在“应用程序”里**右键 GameScrcpy → 打开**，再点“打开”。
   如果提示“已损坏”，在“终端”运行一次：`xattr -dr com.apple.quarantine /Applications/GameScrcpy.app`
3. 第一次使用无线调试时允许“本地网络”；第一次投屏时手机上会自动装上 GameScrcpy Companion（用来打字）。

### 有什么
- **低延迟投屏**：显卡解码，画面按屏幕刷新均匀显示；H.264、H.265、AV1。
- **USB 与无线调试**（配对码 / 二维码），WiFi 直连或 adb 隧道。
- **键鼠映射与手柄**：不需要“输入监控”权限；Xbox、PS4、PS5 手柄与震动。
- **用 Mac 键盘打字**，中文也可以，手机键盘不会弹出。
- **触控板**：双指滑动操作手机；默认跟手（像真手指一样），也可选翻页、自然甩动、滚轮。
- **投屏窗口**：工具栏可放在上下左右、按钮可自定义；缩放时保持手机比例；手机熄屏时可自动停止投屏；点画面可唤醒息屏（AOD）的手机。
- **macOS 原生界面**：Liquid Glass 风格、跟随系统深浅色、菜单栏（文件、显示、设备、窗口、帮助）、菜单栏图标与通知。
- 录屏与导出 MP4、方案与键位编辑、蓝图与 TypeScript 自动化、语音命令、AI 助手（密钥存在钥匙串）、扩展。

### 已知问题
- 触控板默认“跟手”；翻页、自然甩动、滚轮方式仍可在设置里选。
- AOA 触控与串口是新加的，如有问题请反馈；采集卡、蓝牙外设、扩展画面滤镜暂不可用。
- 个别网络下“WiFi 直连”连不上：改用“adb 隧道”。
- 每次更新后 macOS 可能再次询问“本地网络”等权限（测试版没有固定签名）。
- Intel Mac 上 TypeScript 脚本照常运行，但编辑器里不做类型检查。

---

## English

### Requirements
- macOS 14 Sonoma or later, on a Mac with Apple silicon or Intel.
- An Android 7+ phone with USB debugging (or Android 11+ wireless debugging). adb is included.

### Install
1. Open the `.dmg` and drag GameScrcpy to Applications.
2. First launch: this beta is not signed with an Apple developer ID. In Applications, **right-click GameScrcpy →
   Open**, then Open. If macOS says it is damaged, run once in Terminal:
   `xattr -dr com.apple.quarantine /Applications/GameScrcpy.app`
3. Allow Local Network the first time you use wireless debugging; the GameScrcpy Companion app is installed on the
   phone the first time you mirror (it receives what you type).

### What's in it
- **Low-latency mirroring**: decoded on the graphics card, shown evenly on the screen's refresh; H.264, H.265, AV1.
- **USB and wireless debugging** (pairing code or QR), WiFi direct or adb tunnel.
- **Key mapping and gamepads**: no Input Monitoring permission needed; Xbox, PS4, PS5 controllers with rumble.
- **Type with the Mac keyboard**, Chinese included, without the phone's keyboard popping up.
- **Trackpad**: two-finger swipes drive the phone, Direct by default (like a real finger), or page turn, natural
  fling or scroll wheel.
- **Mirror window**: a customisable toolbar on any side; resizing keeps the phone's proportions; can stop mirroring when the phone's
  screen turns off; a click wakes a phone on its Always On Display.
- **Native macOS look**: Liquid Glass style, follows light/dark mode, a menu bar (File, View, Device, Window, Help),
  a menu bar item and notifications.
- Recording and MP4 export, profiles and key editing, Blueprints and TypeScript automation, voice commands, the AI
  assistant (keys in the Keychain), extensions.

### Known issues
- The trackpad defaults to Direct; page turn, natural fling and scroll wheel are still in Settings.
- AOA touch and serial devices are new: please report problems. Capture cards, Bluetooth devices and extension
  video filters are not available on macOS yet.
- WiFi direct may fail on some networks: use adb tunnel.
- macOS may ask again for Local Network and other permissions after each update (no stable signature yet).
- On Intel Macs TypeScript scripts run as usual, but the editor does not type-check them.
