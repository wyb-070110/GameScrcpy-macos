#pragma once

class QWindow;

namespace gs::app {

// macOS: the system title bar, transparent over the content; false if the window has no native window yet.
bool styleMacWindow(QWindow* window);
void setMacWindowDark(QWindow* window, bool dark);
bool startMacWindowDrag(QWindow* window);
void performMacTitleBarDoubleClick(QWindow* window);

}
