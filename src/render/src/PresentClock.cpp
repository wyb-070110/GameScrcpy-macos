// When drawn frames reach the screen, on macOS: Qt Quick's frameSwapped after the frame that drew a new picture.
// That is when the frame was handed to the compositor, a little before the display shows it (Windows reads the
// swap chain's statistics for the refresh that showed it). macOS always composes windows, so the mode is Composed.
#include "PresentClock.h"

#include <QQuickWindow>

#include <chrono>
#include <utility>

namespace gs::render {

namespace {

std::int64_t nowUs()
{
    return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

}

PresentClock::PresentClock(QObject* parent) : QObject(parent) {}

PresentClock::~PresentClock()
{
    detach();
}

void PresentClock::setSink(Sink sink)
{
    m_sink = std::move(sink);
}

void PresentClock::setModeSink(ModeSink sink)
{
    m_modeSink = std::move(sink);
}

void PresentClock::setDiscardSink(DiscardSink sink)
{
    m_discardSink = std::move(sink);
}

bool PresentClock::attach(QQuickWindow* window)
{
    detach();
    m_window = window;
    if (window == nullptr) {
        return false;
    }
    m_connection = QObject::connect(window, &QQuickWindow::frameSwapped, this, [this] { afterFrame(); },
                                    Qt::DirectConnection);
    return true;
}

void PresentClock::detach()
{
    if (m_connection) {
        QObject::disconnect(m_connection);
    }
    m_window = nullptr;
    m_drew = false;
}

void PresentClock::drew(std::int64_t ptsUs, std::int64_t receivedUs, std::int64_t takenUs)
{
    m_drawn = {.ptsUs = ptsUs, .receivedUs = receivedUs, .takenUs = takenUs, .screenUs = 0};
    m_drew = true;
}

void PresentClock::afterFrame()
{
    if (!m_drew) {
        return;
    }
    m_drew = false;
    if (m_mode != PresentationMode::Composed) {
        m_mode = PresentationMode::Composed;
        if (m_modeSink) {
            m_modeSink(m_mode);
        }
    }
    Sample shown = m_drawn;
    shown.screenUs = nowUs();
    if (m_sink) {
        m_sink(shown, m_mode);
    }
}

}
