#pragma once

class QWindow;

namespace gs::app {

// macOS: the system title bar, transparent over the content; false if the window has no native window yet.
bool styleMacWindow(QWindow* window);
void setMacWindowDark(QWindow* window, bool dark);
// Liquid Glass: the desktop shows through the window, blurred by the system, wherever its content is transparent.
// Off puts back an opaque window.
void setMacWindowGlass(QWindow* window, bool on);
bool startMacWindowDrag(QWindow* window);
void performMacTitleBarDoubleClick(QWindow* window);

}
