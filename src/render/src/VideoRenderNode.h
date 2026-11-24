#pragma once

// The video picture in Qt Quick on macOS (PLAN 4.5): a render node drawing with QRhi, which Qt Quick runs on Metal.
// Each new frame's planes are uploaded into textures (VideoToolbox surfaces are read in place, software pictures
// from their planes); video.vert with video_nv12.frag or video_planar.frag turns them into RGB, sharpened when asked.

#include "FilterChain.h"
#include "PresentClock.h"

#include <gs/media/VideoDecoder.h>
#include <gs/render/FrameSlot.h>

#include <QRectF>
#include <QSGRenderNode>
#include <QSize>

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

class QQuickWindow;
class QRhi;
class QRhiBuffer;
class QRhiGraphicsPipeline;
class QRhiRenderPassDescriptor;
class QRhiResourceUpdateBatch;
class QRhiSampler;
class QRhiShaderResourceBindings;
class QRhiTexture;

namespace gs::render {

struct WakeFlag {
    std::atomic<bool> posted{false};
};

class VideoRenderNode final : public QSGRenderNode
{
public:
    explicit VideoRenderNode(QQuickWindow* window);
    ~VideoRenderNode() override;

    void setSource(std::shared_ptr<FrameSlot> slot, bool frozen);
    void setBounds(const QRectF& bounds);
    void setCrop(const QRectF& crop);
    void setSharpness(float sharpness);
    void setFilters(std::vector<VideoFilter> filters);
    bool takeFilterStatus(std::vector<FilterStatus>& status);
    [[nodiscard]] QSize frameSize() const;

    void prepare() override;
    void render(const RenderState* state) override;
    void releaseResources() override;
    [[nodiscard]] StateFlags changedStates() const override;
    [[nodiscard]] RenderingFlags flags() const override;
    [[nodiscard]] QRectF rect() const override;

private:
    // One plane of the picture as a texture, made again when its size or format changes.
    struct Plane {
        std::unique_ptr<QRhiTexture> texture;
        QSize size;
        int format = 0;
    };

    void takeFrame();
    void installWake();
    void postWake();
    bool ensureShared(QRhi* rhi);
    bool ensurePipeline(QRhi* rhi);
    bool uploadFrame(QRhi* rhi, QRhiResourceUpdateBatch* updates);
    bool uploadPlane(QRhi* rhi, QRhiResourceUpdateBatch* updates, std::size_t index, int format, QSize size,
                     const void* data, std::size_t stride, std::size_t bytesPerTexel);

    QQuickWindow* m_window;
    std::shared_ptr<FrameSlot> m_slot;
    std::shared_ptr<WakeFlag> m_wake;
    PresentClock m_present;
    bool m_guiWake = false;
    bool m_frozen = false;
    media::VideoFrame m_frame;
    bool m_frameChanged = false;
    bool m_ready = false;
    QSize m_reportedSize;
    QRectF m_bounds;
    QRectF m_drawnRect;
    QRectF m_crop{0.0, 0.0, 1.0, 1.0};
    float m_sharpness = 0.0F;
    std::int64_t m_takenPtsUs = 0;
    std::int64_t m_takenReceivedUs = 0;
    std::int64_t m_takenUs = 0;
    bool m_drewFrame = false;
    bool m_unsupportedLogged = false;

    std::unique_ptr<QRhiBuffer> m_parameters;
    std::unique_ptr<QRhiSampler> m_sampler;
    std::unique_ptr<QRhiShaderResourceBindings> m_bindings;
    std::unique_ptr<QRhiGraphicsPipeline> m_pipeline;
    std::unique_ptr<QRhiRenderPassDescriptor> m_pipelinePass;
    int m_pipelineSamples = 0;
    bool m_pipelineNv12 = false;
    bool m_bindingsStale = true;
    std::array<Plane, 3> m_planes;
    std::size_t m_planeCount = 0;
    bool m_nv12 = false;
    float m_texelWidth = 0.0F;
    float m_texelHeight = 0.0F;
    FilterChain m_filters;
};

}
