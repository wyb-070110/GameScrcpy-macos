Short answers to the questions players ask most. Select a question to see its answer; the groups above narrow the list.

## Connecting {icon=smartphone} {tone=azure}

### The phone does not show up on the Devices page {#no-phone}

- Use a cable that carries data; some cables only charge.
- Make sure USB debugging is on. The steps for each brand are in "Connecting a phone" and in the getting-started guide.
- If the phone asks what the USB connection is for, choose "File transfer".
- Unlock the phone and look for the question "Allow USB debugging?".

> [!TIP]
> The getting-started guide walks you through it for your phone's brand.
> [Open the guide](action:guide)

### The phone says "Waiting for authorization" {#waiting-authorization}

Unlock the phone. When it asks "Allow USB debugging?", tick "Always allow from this computer" and tap **Allow**.

### The phone is "Offline" {#offline}

Unplug the cable and plug it in again. If it stays offline, turn USB debugging off and on again in Developer options.

### The Devices page says adb is not available {#adb-missing}

anmacgamer talks to phones through adb. Install Android SDK Platform-Tools (for example with Homebrew: `brew install --cask android-platform-tools`), or make sure no other program blocks adb. Then select **Retry** on the **Devices** page.

### USB debugging is greyed out on a Samsung phone {#samsung-grey}

Turn off **Settings > Security and privacy > Auto Blocker** first.

## Picture and sound {icon=monitor-play} {tone=orange}

### There is no sound {#no-sound}

- Sound needs Android 11 or later.
- On Android 11, unlock the phone before you connect.
- Check that **Settings > Audio > Sound** is on, and that the phone is not muted on the mirror window's toolbar.

### WiFi direct does not connect {#wifi-direct}

This Mac and the phone must be on the same network, and anmacgamer must be allowed under **System Settings > Privacy & Security > Local Network**. Some routers keep their devices apart (AP isolation), and a firewall can block UDP. If it still fails, choose **adb tunnel** on the phone's card on the **Devices** page.

### The picture stutters or lags {#lag}

- A USB cable is the steadiest connection.
- In **Settings > Mirroring & quality**, lower **Maximum resolution** or **Bit rate limit**.
- **Low-latency mode** in the same place gives the least delay, with occasional tearing.
- The status capsule and the diagnostics overlay show the frame rate and the delay.
- If the **Devices** page says Metal hardware acceleration is not available, run anmacgamer directly on a Mac with Metal support. In some virtual machines the processor draws the picture, which may stutter.

When the network gets slow, anmacgamer lowers the resolution or the frame rate by itself, and brings them back when the network recovers.

### Chinese and other characters do not reach the phone {#typing}

Characters that are not on the keyboard reach the phone through its clipboard. Check whether the phone restricts clipboard access.

## Keys and control {icon=keyboard} {tone=lime}

### The picture works, but the mouse and keyboard do nothing {#no-input}

Some phones only accept simulated input after one more developer option is on. Turn it on, then connect again:

| Phone | The switch to turn on |
|---|---|
| Xiaomi · Redmi · POCO | "USB debugging (Security settings)". It needs a signed-in Mi account; restart the phone after you turn it on. |
| OPPO · OnePlus · realme | "Disable permission monitoring". From ColorOS 16 it only shows while the phone's language is English, as "Disable system optimization"; switch the language back afterwards. |
| vivo · iQOO | "USB simulate click". |

### The keys of my profile do nothing {#keys-nothing}

- Turn key mapping on with the keyboard button on the mirror window's toolbar.
- Pick a profile in **Key mapping options**.
- Click into the mirror window, so it is in front.

### The mouse pointer is gone {#pointer-gone}

The camera is locked. Press the camera lock key again: the key above Tab, left of the 1 key, unless the profile sets another one.

### A keyboard shortcut does nothing {#shortcut-nothing}

Look in **Settings > Keyboard shortcuts**; another command may use the same keys. In a mirror window, a key that the current profile uses goes to the game first.

## Other {icon=settings-2} {tone=gray}

### anmacgamer closed unexpectedly {#crashed}

At the next start, anmacgamer offers to export diagnostics; you can also do it at any time in **Settings > About > Export diagnostics…**. The zip file holds the logs, the settings, the device list and the newest crash report; serial numbers, addresses, user names, Wi‑Fi names and clipboard text are taken out. Attach the file to your problem report.

### An extension causes trouble {#extension-trouble}

On the **Extensions** page, **Find a problem extension** loads your extensions a few at a time and asks whether the problem still happens, until it finds the one behind it. To start once without any extension, run `open -a anmacgamer --args --safe-mode`.

When an extension stops three times within a minute, anmacgamer turns it off by itself and its card says why; the card also counts the stops of the last 7 days, and the extension's **Details** list each one with its crash report. More in [Extensions · When something goes wrong](help:extensions#trouble).

### Where are my settings and profiles? {#data-folder}

In `~/Library/Application Support/anmacgamer`: your profiles are in `profiles`, the logs in `logs`.

To use a separate data folder, set the `GS_DATA_DIR` environment variable before starting anmacgamer.

### Which phones work? {#which-phones}

Phones with Android 9 or later. HarmonyOS 5 and later is not Android and cannot be mirrored.

### Where can I find all commands? {#all-commands}

Press <kbd>⌘ Command</kbd>+<kbd>K</kbd> for the command palette. Type a few letters to find a command, then press <kbd>Return</kbd> to run it; the palette also shows each command's keys. <kbd>F1</kbd> opens this help.
