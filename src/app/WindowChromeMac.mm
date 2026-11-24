// The window frame on macOS: the system's title bar, made transparent with the content drawn under it, so the
// window keeps the traffic lights and macOS moves it by its top strip and zooms it on a double click there (the
// System Settings choice). QWindowKit's frameless window is not used on macOS: it makes the window unmovable and
// the transparent title bar then keeps the mouse from Qt.
#include "WindowChromeMac.h"

#import <AppKit/AppKit.h>

#include <QWindow>

namespace gs::app {

namespace {

NSWindow* nativeWindow(QWindow* window)
{
    if (window == nullptr) {
        return nil;
    }
    auto* view = (__bridge NSView*)reinterpret_cast<void*>(window->winId());
    return view != nil ? [view window] : nil;
}

}

bool styleMacWindow(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    if (host == nil) {
        return false;
    }
    host.styleMask |= NSWindowStyleMaskFullSizeContentView;
    host.titlebarAppearsTransparent = YES;
    host.titleVisibility = NSWindowTitleHidden;
    host.movable = YES;
    return true;
}

void setMacWindowDark(QWindow* window, bool dark)
{
    NSWindow* host = nativeWindow(window);
    if (host != nil) {
        host.appearance = [NSAppearance appearanceNamed:dark ? NSAppearanceNameDarkAqua : NSAppearanceNameAqua];
    }
}


}
