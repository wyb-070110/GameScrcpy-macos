#pragma once

#include <gs/render/FrameSlot.h>

#include <QObject>

#include <array>
#include <cstdint>
#include <functional>


class QQuickWindow;

namespace gs::render {

class PresentClock final : public QObject
{
public:
    struct Sample {
        std::int64_t ptsUs = 0;
        std::int64_t receivedUs = 0;
        std::int64_t takenUs = 0;
        std::int64_t screenUs = 0;
    };

    using Sink = std::function<void(const Sample&, PresentationMode)>;
    using ModeSink = std::function<void(PresentationMode)>;
    using DiscardSink = std::function<void(std::uint32_t frames)>;

    static constexpr std::size_t kMostPending = 16;
    static constexpr std::uint32_t kMostRefreshesBetweenSyncs = 16;

    explicit PresentClock(QObject* parent = nullptr);
    ~PresentClock() override;
    PresentClock(const PresentClock&) = delete;
    PresentClock& operator=(const PresentClock&) = delete;

    void setSink(Sink sink);
    void setModeSink(ModeSink sink);
    void setDiscardSink(DiscardSink sink);
    bool attach(QQuickWindow* window);
    void detach();
    void drew(std::int64_t ptsUs, std::int64_t receivedUs, std::int64_t takenUs);
    void afterFrame();

private:
    struct Pending {
        std::uint32_t presentCount = 0;
        Sample sample;
    };

    Sink m_sink;
    ModeSink m_modeSink;
    DiscardSink m_discardSink;
    QMetaObject::Connection m_connection;
    QQuickWindow* m_window = nullptr;
    bool m_drew = false;
    Sample m_drawn;
    PresentationMode m_mode = PresentationMode::Unknown;
};

}
