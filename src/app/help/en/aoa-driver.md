With AOA the phone takes anmacgamer for a touch screen on the USB cable, together with a mouse and a keyboard. It works even when USB debugging is off, and macOS needs no USB driver.

## Choose AOA

1. **Plug in the USB cable**: connect the phone to this Mac with a data-capable USB cable and unlock the phone.
2. **Select AOA**: in **Settings > Input & key mapping > Touch**, set the touch method to **AOA**, for every phone or for one phone. While you choose, the features that do not work without a picture are listed; it takes effect once you confirm, the next time the phone connects.

## With a picture, or without one

- **USB debugging or wireless debugging on**: the picture comes over debugging while touch goes over AOA on the USB cable. The phone's card shows "USB · picture + AOA touch" or "Wireless · picture + USB · AOA touch".
- **Debugging off**: a control window opens instead, a dark canvas in the shape of the phone's screen with the key hints and the words "No picture · Watch the phone". The keyboard and mouse turn into touches only while the control window is in front.
- **The pointer on the phone**: without a picture the phone shows a mouse pointer of its own; with a free cursor, look at the phone and click. While the camera is locked the pointer stops moving and the phone hides it.
- **How the phone is held**: without adb nothing tells whether the phone is upright or turned. Choose Portrait, Landscape (turned left) or Landscape (turned right) under "How the phone is held" in the control window's toolbar. It starts from the profile's orientation and each phone remembers the last choice.

## Connect anmacgamer Companion by scanning

Without USB debugging, anmacgamer Companion on the phone connects to this Mac over Wi-Fi. It then draws the key hints on the phone's own screen and lets you type into the phone's input boxes with this computer's keyboard, Chinese included.

1. **The same Wi-Fi**: the Mac and the phone must be on the same Wi-Fi or local network.
2. **Install the companion**: if the phone does not have anmacgamer Companion yet, scan the code in the control window with the phone's own camera or scanner. It opens a page on this computer to download the app; install it, and allow installing from this source if the phone asks.
3. **Scan to connect**: open anmacgamer Companion on the phone, tap Scan to connect, allow the camera and scan the code in the control window. The code goes away once the phone is connected.
4. **Type with the computer's keyboard**: on the companion's **Permissions** page, tap **Open** on the Input method row and switch on anmacgamer Companion, then come back and tap **Choose** to pick it. When an input box opens on the phone, a thin bar says "Typing with the computer's keyboard"; tap it to get the phone's keyboard back. The companion's **Features** page can move the bar to the top.

> [!TIP]
> **Next time**: both sides remember each other. On the same Wi-Fi the companion connects again by itself as soon as the control window is open; there is nothing to scan (the companion's Auto connect is on by default).

The companion's other features, switches and permissions: [anmacgamer Companion](help:companion)

> [!WARNING]
> **Local Network & Firewall**: the first time, macOS asks whether anmacgamer may find and communicate with devices on your local network. Allow it under **System Settings > Privacy & Security > Local Network** (and allow anmacgamer in the macOS Firewall if enabled), otherwise the phone cannot connect. anmacgamer only listens while a control window shows the code or waits for a paired phone.

## What needs a picture

Vision and the vision workbench, `vision.*` in logic and scripts (which answers null, [] or false), recording, clipboard sharing, starting apps, the latency test, and sound, turning the screen off and rotation. Keys, the camera lock, editing the key mapping and the touches of logic all work as usual.
