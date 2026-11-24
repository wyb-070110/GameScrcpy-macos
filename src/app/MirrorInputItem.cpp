#include "MirrorInputItem.h"

#include <QGuiApplication>
#include <QHoverEvent>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QWheelEvent>

#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
#include <utility>

namespace gs::app {

namespace {

constexpr double kWheelStep = 120.0;
constexpr double kScrollUnitsPerStep = 256.0;
constexpr std::array kKeyButtons{Qt::RightButton, Qt::MiddleButton, Qt::BackButton, Qt::ForwardButton};

std::uint8_t modifierBits(Qt::KeyboardModifiers modifiers) noexcept
{
    std::uint8_t bits = 0;
    if (modifiers.testFlag(Qt::ControlModifier)) {
        bits |= input::modifier::kCtrl;
    }
    if (modifiers.testFlag(Qt::ShiftModifier)) {
        bits |= input::modifier::kShift;
    }
    if (modifiers.testFlag(Qt::AltModifier)) {
        bits |= input::modifier::kAlt;
    }
    return bits;
}

}

MirrorInputItem::MirrorInputItem(QQuickItem* parent)
    : QQuickItem(parent)
{
    setAcceptedMouseButtons(Qt::AllButtons);
    m_coastIdle.setSingleShot(true);
    connect(&m_coastIdle, &QTimer::timeout, this, &MirrorInputItem::liftSwipe);
}

std::optional<input::Vec2> MirrorInputItem::normalized(const QRectF& content, QPointF position, bool clamped)
{
    if (!(content.width() > 0.0) || !(content.height() > 0.0)) {
        return std::nullopt;
    }
    const double x = (position.x() - content.left()) / content.width();
    const double y = (position.y() - content.top()) / content.height();
    if (!clamped && (x < 0.0 || x > 1.0 || y < 0.0 || y > 1.0)) {
        return std::nullopt;
    }
    return input::Vec2{std::clamp(x, 0.0, 1.0), std::clamp(y, 0.0, 1.0)};
}

std::int16_t MirrorInputItem::scrollUnits(int angleDelta) noexcept
{
    const double units = std::round(angleDelta * kScrollUnitsPerStep / kWheelStep);
    return static_cast<std::int16_t>(std::clamp(units, -32768.0, 32767.0));
}

int MirrorInputItem::mouseButtonBit(Qt::MouseButton button) noexcept
{
    switch (button) {
    case Qt::LeftButton:
        return 0x01;
    case Qt::RightButton:
        return 0x02;
    case Qt::MiddleButton:
        return 0x04;
    case Qt::BackButton:
        return 0x08;
    case Qt::ForwardButton:
        return 0x10;
    default:
        return 0;
    }
}

void MirrorInputItem::setRelativeMouse(bool on, QSizeF target, int turn)
{
    const bool turned = (turn & 3) != m_turn;
    const bool resized = target != m_target;
    m_target = target;
    m_turn = turn & 3;
    if (on == m_relative) {
        if (resized || turned) {
            m_pointerOnPhone.reset();
            m_rest = {};
            m_pendingRecenter = true;
        }
        return;
    }
    cancel();
    m_relative = on;
    m_pointerOnPhone.reset();
    m_rest = {};
    m_pendingRecenter = on;
    setAcceptHoverEvents(on);
}

void MirrorInputItem::follow(QPointF position)
{
    const bool front = window() != nullptr && window()->isActive();
    if (!m_relative || m_capturing || !front || !(m_content.width() > 0.0) || !(m_content.height() > 0.0) ||
        !std::isfinite(position.x()) || !std::isfinite(position.y())) {
        return;
    }
    const double scaleX = m_target.width() > 0.0 ? m_target.width() / m_content.width() : 1.0;
    const double scaleY = m_target.height() > 0.0 ? m_target.height() / m_content.height() : 1.0;
    const QPointF onPhone(std::clamp(position.x() - m_content.left(), 0.0, m_content.width()) * scaleX,
                          std::clamp(position.y() - m_content.top(), 0.0, m_content.height()) * scaleY);
    if (std::exchange(m_pendingRecenter, false)) {
        m_rest = {};
        m_pointerOnPhone = onPhone;
        emit mouseRecenter(static_cast<int>(std::lround(onPhone.x())), static_cast<int>(std::lround(onPhone.y())));
        return;
    }
    if (!m_pointerOnPhone) {
        m_pointerOnPhone = onPhone;
        return;
    }
    m_rest += onPhone - *m_pointerOnPhone;
    m_pointerOnPhone = onPhone;
    const int dx = static_cast<int>(std::trunc(m_rest.x()));
    const int dy = static_cast<int>(std::trunc(m_rest.y()));
    if (dx == 0 && dy == 0) {
        return;
    }
    m_rest -= QPointF(dx, dy);
    emit mouseMoved(dx, dy);
}

bool MirrorInputItem::pressRelative(QMouseEvent* event, bool down)
{
    const int bit = mouseButtonBit(event->button());
    if (bit == 0) {
        return false;
    }
    if (down) {
        constexpr std::array<std::pair<Qt::MouseButton, std::string_view>, 5> kSources{{{Qt::LeftButton, "Mouse.Left"},
                                                                                      {Qt::RightButton, "Mouse.Right"},
                                                                                      {Qt::MiddleButton, "Mouse.Middle"},
                                                                                      {Qt::BackButton, "Mouse.X1"},
                                                                                      {Qt::ForwardButton, "Mouse.X2"}}};
        for (const auto& [button, name] : kSources) {
            const std::optional<input::InputSource> source = input::parseSource(name);
            if (button == event->button() && source && claimed(*source, modifierBits(event->modifiers()))) {
                return false;
            }
        }
    }
    if (down == ((m_buttons & bit) != 0)) {
        return down;
    }
    m_buttons = down ? (m_buttons | bit) : (m_buttons & ~bit);
    if (down) {
        forceActiveFocus(Qt::MouseFocusReason);
        follow(event->position());
    }
    emit mouseButtonsChanged(m_buttons);
    return true;
}

void MirrorInputItem::setButtonKeys(std::map<Qt::MouseButton, std::uint32_t> keys)
{
    if (keys == m_buttonKeys) {
        return;
    }
    for (const Qt::MouseButton button : kKeyButtons) {
        if (m_keysHeld.testFlag(button)) {
            if (const std::optional<std::uint32_t> keycode = keycodeOf(button)) {
                emit androidKey(static_cast<int>(*keycode), 0, false, 0);
            }
        }
    }
    m_keysHeld = Qt::NoButton;
    m_buttonKeys = std::move(keys);
}

std::optional<std::uint32_t> MirrorInputItem::keycodeOf(Qt::MouseButton button) const
{
    const auto found = m_buttonKeys.find(button);
    return found != m_buttonKeys.end() ? std::optional<std::uint32_t>(found->second) : std::nullopt;
}

bool MirrorInputItem::claimed(input::InputSource source, std::uint8_t modifiers) const
{
    return std::ranges::any_of(m_claimed, [&](const input::Chord& chord) {
        return chord.source == source && (modifiers & chord.modifiers) == chord.modifiers;
    });
}

std::vector<input::Chord> MirrorInputItem::liveClaims(const input::Profile& profile,
                                                      const std::vector<input::CursorClaim>& claims,
                                                      const QList<bool>& live)
{
    std::vector<input::Chord> chords;
    for (const input::CursorClaim& claim : claims) {
        bool on = false;
        if (const auto at = static_cast<qsizetype>(claim.index); at < live.size()) {
            on = live[at];
        } else {
            on = claim.index < profile.controls.size() && profile.controls[claim.index].enabled;
        }
        if (on) {
            chords.push_back(claim.chord);
        }
    }
    return chords;
}

void MirrorInputItem::setCapturing(bool on)
{
    if (on == m_capturing) {
        return;
    }
    m_capturing = on;
    if (on) {
        cancel();
        ungrabMouse();
        if (m_relative) {
            emit mouseParked();
        }
    } else if (m_relative) {
        m_pointerOnPhone.reset();
        m_rest = {};
        m_pendingRecenter = true;
    }
    setAcceptedMouseButtons(on ? Qt::NoButton : Qt::AllButtons);
}

void MirrorInputItem::setTyping(bool on)
{
    if (on == m_typing) {
        return;
    }
    m_typing = on;
    if (!on) {
        releaseKeys();
    }
    setFlag(ItemAcceptsInputMethod, on);
    if (hasActiveFocus()) {
        QGuiApplication::inputMethod()->update(Qt::ImEnabled);
    }
}

QVariant MirrorInputItem::inputMethodQuery(Qt::InputMethodQuery query) const
{
    switch (query) {
    case Qt::ImEnabled:
        return m_typing;
    case Qt::ImCursorRectangle:
        return QRectF(m_lastPress.isNull() ? m_content.center() : m_lastPress, QSizeF(1, 1));
    case Qt::ImHints:
        return static_cast<int>(Qt::ImhNoPredictiveText);
    default:
        return QQuickItem::inputMethodQuery(query);
    }
}

bool MirrorInputItem::event(QEvent* event)
{
    if (event->type() == QEvent::ShortcutOverride && m_typing && textBoxWants(*static_cast<QKeyEvent*>(event))) {
        event->accept();
        return true;
    }
    return QQuickItem::event(event);
}

void MirrorInputItem::mousePressEvent(QMouseEvent* event)
{
    const Qt::MouseButton button = event->button();
    if (m_capturing) {
        event->ignore();
        return;
    }
    if (m_relative) {
        event->setAccepted(pressRelative(event, true));
        return;
    }
    if (button == Qt::LeftButton) {
        const std::optional<input::Vec2> at = normalized(m_content, event->position(), false);
        if (!at || m_touching) {
            event->setAccepted(m_touching);
            return;
        }
        m_touching = true;
        m_lastPress = event->position();
        forceActiveFocus(Qt::MouseFocusReason);
        emit touched(input::CursorPhase::Down, *at);
        event->accept();
        return;
    }
    const std::optional<std::uint32_t> keycode = keycodeOf(button);
    if (!keycode || m_keysHeld.testFlag(button)) {
        event->setAccepted(keycode.has_value());
        return;
    }
    m_keysHeld.setFlag(button);
    forceActiveFocus(Qt::MouseFocusReason);
    emit androidKey(static_cast<int>(*keycode), 0, true, 0);
    event->accept();
}

void MirrorInputItem::mouseMoveEvent(QMouseEvent* event)
{
    if (m_relative) {
        follow(event->position());
        event->accept();
        return;
    }
    if (m_touching && event->buttons().testFlag(Qt::LeftButton)) {
        if (const std::optional<input::Vec2> at = normalized(m_content, event->position(), true)) {
            emit touched(input::CursorPhase::Move, *at);
        }
    }
    event->accept();
}

void MirrorInputItem::mouseReleaseEvent(QMouseEvent* event)
{
    const Qt::MouseButton button = event->button();
    if (m_relative) {
        static_cast<void>(pressRelative(event, false));
        event->accept();
        return;
    }
    if (button == Qt::LeftButton && std::exchange(m_touching, false)) {
        const std::optional<input::Vec2> at = normalized(m_content, event->position(), true);
        emit touched(at ? input::CursorPhase::Up : input::CursorPhase::Cancel, at.value_or(input::Vec2{}));
    } else if (m_keysHeld.testFlag(button)) {
        m_keysHeld.setFlag(button, false);
        if (const std::optional<std::uint32_t> keycode = keycodeOf(button)) {
            emit androidKey(static_cast<int>(*keycode), 0, false, 0);
        }
    }
    event->accept();
}

void MirrorInputItem::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->setAccepted(m_touching || m_keysHeld != Qt::NoButton || m_buttons != 0);
}

void MirrorInputItem::mouseUngrabEvent()
{
    cancel();
}

void MirrorInputItem::hoverEnterEvent(QHoverEvent* event)
{
    follow(event->position());
    event->accept();
}

void MirrorInputItem::hoverMoveEvent(QHoverEvent* event)
{
    follow(event->position());
    event->accept();
}

void MirrorInputItem::hoverLeaveEvent(QHoverEvent* event)
{
    event->accept();
}

// A two-finger swipe on a Mac trackpad comes as scroll events with phases, then momentum. Sent as scroll events it
// turns the phone's pages again and again; here the whole swipe is one finger dragged on the phone instead (down at
// the pointer, moved as the content moves, scaled by the trackpad speed). With inertia the finger then keeps
// going with the momentum, scaled by its strength, and lifts when the momentum ends; without it, it lifts as the
// fingers leave. A swipe is often longer than the phone's picture is wide: when the finger reaches the edge it is
// lifted there, moving, which the phone takes as a fling, and the rest of the swipe is dropped. Wheels without
// phases still scroll.
void MirrorInputItem::setTrackpad(double speed, bool inertia, double inertiaStrength)
{
    m_trackpadSpeed = std::clamp(speed, 0.1, 10.0);
    m_inertia = inertia;
    m_inertiaStrength = std::clamp(inertiaStrength, 0.0, 10.0);
}

void MirrorInputItem::moveSwipe(QPointF delta, bool ending)
{
    m_swipePoint += delta;
    const std::optional<input::Vec2> inside = normalized(m_content, m_swipePoint, false);
    const std::optional<input::Vec2> at = normalized(m_content, m_swipePoint, true);
    if (!inside && at) {
        emit touched(input::CursorPhase::Move, *at);
        emit touched(input::CursorPhase::Up, *at);
        m_swiping = false;
        m_coasting = false;
        m_coastIdle.stop();
        m_swipeSpent = !ending;
        return;
    }
    if (at) {
        emit touched(input::CursorPhase::Move, *at);
    }
}

void MirrorInputItem::liftSwipe()
{
    m_coastIdle.stop();
    m_coasting = false;
    if (!m_swiping) {
        return;
    }
    m_swiping = false;
    const std::optional<input::Vec2> at = normalized(m_content, m_swipePoint, true);
    emit touched(at ? input::CursorPhase::Up : input::CursorPhase::Cancel, at.value_or(input::Vec2{}));
}

bool MirrorInputItem::swipeWithTrackpad(QWheelEvent* event)
{
    // Below this speed (points a millisecond) when the fingers leave, the swipe ends there without inertia.
    constexpr double kCoastFrom = 0.3;
    // Momentum longer than this, farther than kFarthestCoast of the picture's width at full strength, or a pause
    // in it longer than kCoastPauseMs, ends the coasting: the phone adds its own fling when the finger lifts, and
    // the two together must not throw the page much farther than the swipe meant.
    constexpr qint64 kLongestCoastMs = 300;
    constexpr double kFarthestCoast = 0.25;
    constexpr int kCoastPauseMs = 80;
    if (event->phase() == Qt::NoScrollPhase || m_capturing || m_relative) {
        return false;
    }
    const QPointF delta = event->pixelDelta().toPointF() * m_trackpadSpeed;
    if (event->phase() == Qt::ScrollMomentum) {
        // Some systems send the momentum before the end of the swipe: the fingers have left all the same.
        if (m_swiping && !m_coasting) {
            if (!m_inertia || m_swipeSpeed < kCoastFrom) {
                liftSwipe();
                return true;
            }
            m_coasting = true;
            m_coasted = 0.0;
            m_coastClock.start();
        }
        if (!m_coasting) {
            return true;
        }
        if (m_coastClock.elapsed() > kLongestCoastMs) {
            liftSwipe();
            return true;
        }
        const QPointF step = delta * m_inertiaStrength;
        m_coasted += std::hypot(step.x(), step.y());
        if (m_coasted > kFarthestCoast * m_content.width() * std::min(m_inertiaStrength, 1.0)) {
            liftSwipe();
            return true;
        }
        m_coastIdle.start(kCoastPauseMs);
        moveSwipe(step, true);
        return true;
    }
    if (m_coasting) {
        liftSwipe();
        if (event->phase() != Qt::ScrollBegin) {
            return true;
        }
    }
    if (m_swipeSpent) {
        if (event->phase() == Qt::ScrollEnd || event->phase() == Qt::ScrollBegin) {
            m_swipeSpent = false;
        }
        if (event->phase() != Qt::ScrollBegin) {
            return true;
        }
    }
    if (!m_swiping) {
        if (event->phase() == Qt::ScrollEnd || m_touching) {
            return true;
        }
        const std::optional<input::Vec2> at = normalized(m_content, event->position(), false);
        if (!at) {
            return false;
        }
        m_swiping = true;
        m_swipePoint = event->position();
        m_swipeSpeed = 0.0;
        m_swipeClock.start();
        forceActiveFocus(Qt::MouseFocusReason);
        emit touched(input::CursorPhase::Down, *at);
    }
    if (!delta.isNull()) {
        const qint64 sinceMs = std::max<qint64>(m_swipeClock.restart(), 1);
        m_swipeSpeed = std::hypot(delta.x(), delta.y()) / static_cast<double>(sinceMs);
    }
    moveSwipe(delta, event->phase() == Qt::ScrollEnd);
    if (event->phase() == Qt::ScrollEnd && m_swiping) {
        if (m_inertia && m_swipeSpeed >= kCoastFrom && m_inertiaStrength > 0.0) {
            m_coasting = true;
            m_coasted = 0.0;
            m_coastClock.start();
            m_coastIdle.start(kCoastPauseMs);
        } else {
            liftSwipe();
        }
    }
    return true;
}

void MirrorInputItem::wheelEvent(QWheelEvent* event)
{
    if (swipeWithTrackpad(event)) {
        event->accept();
        return;
    }
    const std::optional<input::Vec2> at = m_capturing ? std::nullopt : normalized(m_content, event->position(), false);
    if (!at) {
        event->ignore();
        return;
    }
    std::int16_t horizontal = scrollUnits(-event->angleDelta().x());
    std::int16_t vertical = scrollUnits(event->angleDelta().y());
    const std::uint8_t modifiers = modifierBits(event->modifiers());
    if (vertical != 0 &&
        claimed(*input::parseSource(vertical > 0 ? "Mouse.WheelUp" : "Mouse.WheelDown"), modifiers)) {
        vertical = 0;
    }
    if (horizontal != 0 &&
        claimed(*input::parseSource(horizontal > 0 ? "Mouse.WheelRight" : "Mouse.WheelLeft"), modifiers)) {
        horizontal = 0;
    }
    if (horizontal != 0 || vertical != 0) {
        emit scrolled(*at, horizontal, vertical);
    }
    event->accept();
}

void MirrorInputItem::keyPressEvent(QKeyEvent* event)
{
    if (!m_typing) {
        event->ignore();
        return;
    }
    if (typesText(event->text(), event->modifiers())) {
        emit typed(event->text());
        event->accept();
        return;
    }
    const std::optional<ForwardedKey> key = androidKeyOf(event->key(), event->modifiers());
    if (!key) {
        event->ignore();
        return;
    }
    const auto held = m_heldKeys.find(event->key());
    if (held != m_heldKeys.end() && event->isAutoRepeat()) {
        ++held->second.repeat;
    } else {
        m_heldKeys[event->key()] = HeldKey{.key = *key, .repeat = 0};
    }
    const HeldKey& now = m_heldKeys[event->key()];
    emit androidKey(static_cast<int>(now.key.keycode), static_cast<int>(now.key.metaState), true, now.repeat);
    event->accept();
}

void MirrorInputItem::keyReleaseEvent(QKeyEvent* event)
{
    const auto held = m_heldKeys.find(event->key());
    if (held == m_heldKeys.end()) {
        event->setAccepted(m_typing && typesText(event->text(), event->modifiers()));
        return;
    }
    if (event->isAutoRepeat()) {
        event->accept();
        return;
    }
    const ForwardedKey key = held->second.key;
    m_heldKeys.erase(held);
    emit androidKey(static_cast<int>(key.keycode), static_cast<int>(key.metaState), false, 0);
    event->accept();
}

void MirrorInputItem::inputMethodEvent(QInputMethodEvent* event)
{
    if (m_typing && !event->commitString().isEmpty()) {
        emit typed(event->commitString());
    }
    event->accept();
}

void MirrorInputItem::focusOutEvent(QFocusEvent* event)
{
    releaseKeys();
    if (std::exchange(m_buttons, 0) != 0) {
        emit mouseButtonsChanged(0);
    }
    QQuickItem::focusOutEvent(event);
}

void MirrorInputItem::cancel()
{
    if (std::exchange(m_touching, false)) {
        emit touched(input::CursorPhase::Cancel, input::Vec2{});
    }
    if (std::exchange(m_buttons, 0) != 0) {
        emit mouseButtonsChanged(0);
    }
    for (const Qt::MouseButton button : kKeyButtons) {
        if (m_keysHeld.testFlag(button)) {
            if (const std::optional<std::uint32_t> keycode = keycodeOf(button)) {
                emit androidKey(static_cast<int>(*keycode), 0, false, 0);
            }
        }
    }
    m_keysHeld = Qt::NoButton;
}

void MirrorInputItem::releaseKeys()
{
    const std::map<int, HeldKey> held = std::exchange(m_heldKeys, {});
    for (const auto& entry : held) {
        const ForwardedKey& key = entry.second.key;
        emit androidKey(static_cast<int>(key.keycode), static_cast<int>(key.metaState), false, 0);
    }
}

}
