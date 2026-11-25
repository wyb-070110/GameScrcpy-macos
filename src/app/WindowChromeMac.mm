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

void setMacWindowGlass(QWindow* window, bool on)
{
    NSString* const kGlassId = @"io.github.nuoran.gamescrcpy.glass";
    NSWindow* host = nativeWindow(window);
    if (host == nil || host.contentView == nil) {
        return;
    }
    NSView* frame = host.contentView.superview;
    NSVisualEffectView* glass = nil;
    for (NSView* view in frame.subviews) {
        if ([view isKindOfClass:[NSVisualEffectView class]] && [view.identifier isEqualToString:kGlassId]) {
            glass = (NSVisualEffectView*)view;
        }
    }
    if (!on) {
        [glass removeFromSuperview];
        host.opaque = YES;
        host.backgroundColor = NSColor.windowBackgroundColor;
        return;
    }
    host.opaque = NO;
    host.backgroundColor = NSColor.clearColor;
    if (glass == nil) {
        glass = [[NSVisualEffectView alloc] initWithFrame:frame.bounds];
        glass.identifier = kGlassId;
        glass.autoresizingMask = NSViewWidthSizable | NSViewHeightSizable;
        glass.blendingMode = NSVisualEffectBlendingModeBehindWindow;
        glass.material = NSVisualEffectMaterialSidebar;
        glass.state = NSVisualEffectStateFollowsWindowActiveState;
        [frame addSubview:glass positioned:NSWindowBelow relativeTo:host.contentView];
    }
    host.contentView.wantsLayer = YES;
    host.contentView.layer.opaque = NO;
}

bool startMacWindowDrag(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    NSEvent* current = [NSApp currentEvent];
    if (host == nil || current == nil) {
        return false;
    }
    const NSEventType type = current.type;
    if (type != NSEventTypeLeftMouseDown && type != NSEventTypeLeftMouseDragged) {
        return false;
    }
    [host performWindowDragWithEvent:current];
    return true;
}

void performMacTitleBarDoubleClick(QWindow* window)
{
    NSWindow* host = nativeWindow(window);
    if (host == nil || (host.styleMask & NSWindowStyleMaskFullScreen) != 0) {
        return;
    }
    NSString* action = [[NSUserDefaults standardUserDefaults] stringForKey:@"AppleActionOnDoubleClick"];
    if ([action isEqualToString:@"None"]) {
        return;
    }
    if ([action isEqualToString:@"Minimize"]) {
        if ((host.styleMask & NSWindowStyleMaskMiniaturizable) != 0) {
            [host performMiniaturize:nil];
        }
        return;
    }
    if ((host.styleMask & NSWindowStyleMaskResizable) != 0) {
        [host zoom:nil];
    }
}

}
