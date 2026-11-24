#include "WindowChrome.h"
#include "WindowChromeMac.h"

#include <QCoreApplication>
#include <QCursor>
#include <QEvent>
#include <QGuiApplication>
#include <QLoggingCategory>
#include <QMouseEvent>
#include <QWKQuick/quickwindowagent.h>

namespace gs::app {

namespace {

Q_LOGGING_CATEGORY(lcChrome, "gs.app.chrome")

}

WindowChrome::WindowChrome(QObject* parent)
    : QObject(parent)
{
}

WindowChrome::~WindowChrome() = default;

bool WindowChrome::attach()
{
    if (m_agent != nullptr) {
        return true;
    }
    if (m_window == nullptr) {
        qCWarning(lcChrome) << "attach() called without a window";
        return false;
    }

    // macOS: the system's own title bar, transparent over the content (WindowChromeMac.mm), not QWindowKit.
    m_window->installEventFilter(this);
    m_attached = styleMacWindow(m_window);
    if (!m_attached) {
        qCWarning(lcChrome) << "the window has no native window to style";
        return false;
    }
    applyDarkMode();
    return true;
}

void WindowChrome::setHitTestVisible(QQuickItem* item, bool visible)
{
    if (m_agent != nullptr && item != nullptr) {
        m_agent->setHitTestVisible(item, visible);
    }
}

bool WindowChrome::startDrag()
{
    if (m_window == nullptr) {
        return false;
    }
    if (!startMacWindowDrag(m_window)) {
        return m_window->startSystemMove();
    }
    // AppKit's performWindowDragWithEvent: runs a modal tracking loop and consumes the mouse-up event; send a
    // matching release to Qt so the pressed MouseArea ungrabs cleanly once the drag ends.
    const QPointF globalPos(QCursor::pos());
    const QPointF localPos = m_window->mapFromGlobal(globalPos);
    QMouseEvent release(QEvent::MouseButtonRelease, localPos, localPos, globalPos, Qt::LeftButton,
                        QGuiApplication::mouseButtons() & ~Qt::LeftButton, QGuiApplication::keyboardModifiers());
    QCoreApplication::sendEvent(m_window, &release);
    return true;
}

void WindowChrome::doubleClickTitleBar()
{
    if (m_window != nullptr) {
        performMacTitleBarDoubleClick(m_window);
    }
}

bool WindowChrome::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == m_window && (event->type() == QEvent::Show || event->type() == QEvent::WindowStateChange)) {
        if (styleMacWindow(m_window)) {
            m_attached = true;
            applyDarkMode();
        }
    }
    return QObject::eventFilter(watched, event);
}

void WindowChrome::setWindow(QQuickWindow* window)
{
    if (m_window == window) {
        return;
    }
    if (m_agent != nullptr) {
        qCWarning(lcChrome) << "The window cannot change after attach()";
        return;
    }
    m_window = window;
    emit windowChanged();
}

void WindowChrome::setTitleBar(QQuickItem* item)
{
    if (m_titleBar == item) {
        return;
    }
    m_titleBar = item;
    applyTitleBar();
    emit titleBarChanged();
}

void WindowChrome::setMinimizeButton(QQuickItem* item)
{
    if (m_minimizeButton == item) {
        return;
    }
    m_minimizeButton = item;
    applySystemButtons();
    emit minimizeButtonChanged();
}

void WindowChrome::setMaximizeButton(QQuickItem* item)
{
    if (m_maximizeButton == item) {
        return;
    }
    m_maximizeButton = item;
    applySystemButtons();
    emit maximizeButtonChanged();
}

void WindowChrome::setCloseButton(QQuickItem* item)
{
    if (m_closeButton == item) {
        return;
    }
    m_closeButton = item;
    applySystemButtons();
    emit closeButtonChanged();
}

void WindowChrome::setDarkMode(bool dark)
{
    if (m_darkMode == dark) {
        return;
    }
    m_darkMode = dark;
    applyDarkMode();
    emit darkModeChanged();
}

void WindowChrome::applyTitleBar()
{
    if (m_agent != nullptr && m_titleBar != nullptr) {
        m_agent->setTitleBar(m_titleBar);
    }
}

void WindowChrome::applySystemButtons()
{
    if (m_agent == nullptr) {
        return;
    }
    if (m_minimizeButton != nullptr) {
        m_agent->setSystemButton(QWK::WindowAgentBase::Minimize, m_minimizeButton);
    }
    if (m_maximizeButton != nullptr) {
        m_agent->setSystemButton(QWK::WindowAgentBase::Maximize, m_maximizeButton);
    }
    if (m_closeButton != nullptr) {
        m_agent->setSystemButton(QWK::WindowAgentBase::Close, m_closeButton);
    }
}

void WindowChrome::applyDarkMode()
{
    if (m_attached) {
        setMacWindowDark(m_window, m_darkMode);
    }
}

}
