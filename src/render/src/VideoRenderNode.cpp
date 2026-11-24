#include "VideoRenderNode.h"

#include "VideoGeometry.h"

#include <gs/media/ColorMatrix.h>

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/pixfmt.h>
}

#include <CoreVideo/CoreVideo.h>

#include <QDebug>
#include <QFile>
#include <QMatrix4x4>
#include <QQuickWindow>
#include <QRunnable>
#include <QVector4D>
#include <rhi/qrhi.h>

#include <cstring>
#include <utility>

namespace gs::render {

namespace {

struct Parameters {
    float corners[4][4];
    float toRgb[3][4];
    float extra[4];
    float sharpen[4];
    float crop[4];
};
static_assert(sizeof(Parameters) % 16 == 0);

QShader shaderAt(const QString& name)
{
    QFile file(QStringLiteral(":/gs/render/shaders/") + name + QStringLiteral(".qsb"));
    return file.open(QIODevice::ReadOnly) ? QShader::fromSerialized(file.readAll()) : QShader();
}

class RepaintJob final : public QRunnable
{
public:
    RepaintJob(QQuickWindow* window, std::shared_ptr<WakeFlag> wake) : m_window(window), m_wake(std::move(wake)) {}

    void run() override
    {
        m_wake->posted.store(false, std::memory_order_release);
        if (m_window != nullptr) {
            m_window->update();
        }
    }

private:
    QQuickWindow* m_window;
    std::shared_ptr<WakeFlag> m_wake;
};

}

VideoRenderNode::VideoRenderNode(QQuickWindow* window)
    : m_window(window), m_wake(std::make_shared<WakeFlag>())
{
    m_guiWake = qEnvironmentVariable("GS_RENDER_WAKE") == u"gui";
    m_present.setSink([this](const PresentClock::Sample& sample, PresentationMode) {
        if (m_slot) {
            m_slot->recordScreen(sample.ptsUs, sample.receivedUs, sample.takenUs, sample.screenUs);
        }
    });
    m_present.setModeSink([this](PresentationMode mode) {
        if (m_slot) {
            m_slot->setPresentationMode(mode);
        }
    });
    m_present.setDiscardSink([this](std::uint32_t frames) {
        if (m_slot) {
            m_slot->recordDiscarded(frames);
        }
    });
    if (m_window != nullptr) {
        m_present.attach(m_window);
    }
}

VideoRenderNode::~VideoRenderNode()
{
    if (m_slot) {
        m_slot->clearRenderWaker(this);
    }
    m_present.detach();
    releaseResources();
}

void VideoRenderNode::setSource(std::shared_ptr<FrameSlot> slot, bool frozen)
{
    if (slot != m_slot) {
        if (m_slot) {
            m_slot->clearRenderWaker(this);
        }
        m_slot = std::move(slot);
        installWake();
    }
    m_frozen = frozen;
}

void VideoRenderNode::installWake()
{
    if (!m_slot || m_window == nullptr || m_guiWake) {
        return;
    }
    m_slot->setRenderWaker(this, [this] { postWake(); });
}

void VideoRenderNode::postWake()
{
    QQuickWindow* window = m_window;
    if (window == nullptr || !window->isExposed()) {
        return;
    }
    if (m_wake->posted.exchange(true, std::memory_order_acq_rel)) {
        return;
    }
    window->scheduleRenderJob(new RepaintJob(window, m_wake), QQuickWindow::NoStage);
}

void VideoRenderNode::takeFrame()
{
    if (!m_slot) {
        return;
    }
    std::int64_t takenUs = 0;
    media::VideoFrame frame = m_slot->take(&takenUs);
    if (frame && !m_frozen) {
        const QSize size(frame.width(), frame.height());
        m_takenPtsUs = frame.ptsUs();
        m_takenReceivedUs = frame.receivedUs();
        m_takenUs = takenUs;
        m_frame = std::move(frame);
        m_frameChanged = true;
        m_drewFrame = true;
        if (size != m_reportedSize) {
            m_reportedSize = size;
            m_slot->wakeGui();
        }
    }
    if (m_slot->pending()) {
        if (m_guiWake || m_window == nullptr) {
            m_slot->wakeGui();
        } else {
            m_window->update();
        }
    }
}

void VideoRenderNode::setBounds(const QRectF& bounds)
{
    m_bounds = bounds;
}

void VideoRenderNode::setCrop(const QRectF& crop)
{
    m_crop = crop;
}

void VideoRenderNode::setSharpness(float sharpness)
{
    m_sharpness = sharpness;
}

void VideoRenderNode::setFilters(std::vector<VideoFilter> filters)
{
    m_filters.setFilters(std::move(filters));
}

bool VideoRenderNode::takeFilterStatus(std::vector<FilterStatus>& status)
{
    return m_filters.takeStatus(status);
}

QSize VideoRenderNode::frameSize() const
{
    return m_frame ? QSize(m_frame.width(), m_frame.height()) : QSize();
}

void VideoRenderNode::prepare()
{
    m_wake->posted.store(false, std::memory_order_release);
    takeFrame();
    m_drawnRect = fitted(croppedSize(frameSize(), m_crop), m_bounds);
    QRhi* rhi = m_window != nullptr ? m_window->rhi() : nullptr;
    QRhiCommandBuffer* commands = commandBuffer();
    if (rhi == nullptr || commands == nullptr || renderTarget() == nullptr || !m_frame || !ensureShared(rhi)) {
        m_ready = false;
        return;
    }
    QRhiResourceUpdateBatch* updates = rhi->nextResourceUpdateBatch();
    if (m_frameChanged) {
        m_frameChanged = false;
        m_ready = uploadFrame(rhi, updates);
    }
    if (!m_ready || !ensurePipeline(rhi)) {
        m_ready = false;
        updates->release();
        return;
    }

    Parameters parameters{};
    const QMatrix4x4 transform = *projectionMatrix() * *matrix();
    const QPointF corners[4] = {m_drawnRect.topLeft(), m_drawnRect.topRight(), m_drawnRect.bottomLeft(),
                                m_drawnRect.bottomRight()};
    for (std::size_t i = 0; i < 4; ++i) {
        const QVector4D clip = transform.map(QVector4D(static_cast<float>(corners[i].x()),
                                                       static_cast<float>(corners[i].y()), 0.0F, 1.0F));
        parameters.corners[i][0] = clip.x();
        parameters.corners[i][1] = clip.y();
        parameters.corners[i][2] = clip.z();
        parameters.corners[i][3] = clip.w();
    }
    const media::ColorMatrix color = media::yuvToRgb(*m_frame.avFrame());
    for (std::size_t row = 0; row < 3; ++row) {
        std::memcpy(parameters.toRgb[row], color.rows[row].data(), sizeof(parameters.toRgb[row]));
    }
    parameters.extra[0] = static_cast<float>(inheritedOpacity());
    parameters.extra[1] = 1.0F;
    parameters.extra[2] = 1.0F;
    parameters.sharpen[0] = m_sharpness;
    parameters.sharpen[1] = m_texelWidth;
    parameters.sharpen[2] = m_texelHeight;
    parameters.crop[0] = static_cast<float>(m_crop.x());
    parameters.crop[1] = static_cast<float>(m_crop.y());
    parameters.crop[2] = static_cast<float>(m_crop.width());
    parameters.crop[3] = static_cast<float>(m_crop.height());
    updates->updateDynamicBuffer(m_parameters.get(), 0, sizeof(parameters), &parameters);
    commands->resourceUpdate(updates);
}

void VideoRenderNode::render(const RenderState* state)
{
    if (!m_ready || m_pipeline == nullptr || m_bindings == nullptr) {
        return;
    }
    if (m_drewFrame) {
        m_drewFrame = false;
        m_present.drew(m_takenPtsUs, m_takenReceivedUs, m_takenUs);
    }
    QRhiCommandBuffer* commands = commandBuffer();
    const QSize target = renderTarget()->pixelSize();
    commands->setGraphicsPipeline(m_pipeline.get());
    commands->setViewport(QRhiViewport(0.0F, 0.0F, static_cast<float>(target.width()),
                                       static_cast<float>(target.height())));
    if (state->scissorEnabled()) {
        const QRect clip = state->scissorRect();
        commands->setScissor(QRhiScissor(clip.x(), clip.y(), clip.width(), clip.height()));
    } else {
        commands->setScissor(QRhiScissor(0, 0, target.width(), target.height()));
    }
    commands->setShaderResources(m_bindings.get());
    commands->draw(4);
}

void VideoRenderNode::releaseResources()
{
    m_ready = false;
    m_frameChanged = static_cast<bool>(m_frame);
    m_pipeline.reset();
    m_pipelinePass.reset();
    m_bindings.reset();
    m_bindingsStale = true;
    m_planes = {};
    m_planeCount = 0;
    m_parameters.reset();
    m_sampler.reset();
    m_filters.release();
}

QSGRenderNode::StateFlags VideoRenderNode::changedStates() const
{
    return ViewportState | ScissorState;
}

QSGRenderNode::RenderingFlags VideoRenderNode::flags() const
{
    return BoundedRectRendering;
}

QRectF VideoRenderNode::rect() const
{
    return m_bounds;
}

bool VideoRenderNode::ensureShared(QRhi* rhi)
{
    if (m_parameters == nullptr) {
        m_parameters.reset(rhi->newBuffer(QRhiBuffer::Dynamic, QRhiBuffer::UniformBuffer, sizeof(Parameters)));
        if (!m_parameters->create()) {
            m_parameters.reset();
            return false;
        }
    }
    if (m_sampler == nullptr) {
        m_sampler.reset(rhi->newSampler(QRhiSampler::Linear, QRhiSampler::Linear, QRhiSampler::None,
                                        QRhiSampler::ClampToEdge, QRhiSampler::ClampToEdge));
        if (!m_sampler->create()) {
            m_sampler.reset();
            return false;
        }
    }
    return true;
}

bool VideoRenderNode::ensurePipeline(QRhi* rhi)
{
    if (m_bindingsStale || m_bindings == nullptr) {
        QVarLengthArray<QRhiShaderResourceBinding, 4> entries;
        entries.append(QRhiShaderResourceBinding::uniformBuffer(
            0, QRhiShaderResourceBinding::VertexStage | QRhiShaderResourceBinding::FragmentStage, m_parameters.get()));
        for (std::size_t index = 0; index < m_planeCount; ++index) {
            entries.append(QRhiShaderResourceBinding::sampledTexture(static_cast<int>(index) + 1,
                                                                     QRhiShaderResourceBinding::FragmentStage,
                                                                     m_planes[index].texture.get(), m_sampler.get()));
        }
        m_bindings.reset(rhi->newShaderResourceBindings());
        m_bindings->setBindings(entries.cbegin(), entries.cend());
        if (!m_bindings->create()) {
            m_bindings.reset();
            return false;
        }
        m_bindingsStale = false;
        m_pipeline.reset();
    }
    QRhiRenderPassDescriptor* pass = renderTarget()->renderPassDescriptor();
    const int samples = renderTarget()->sampleCount();
    if (m_pipeline != nullptr && m_pipelineNv12 == m_nv12 && m_pipelineSamples == samples && m_pipelinePass &&
        m_pipelinePass->isCompatible(pass)) {
        return true;
    }
    const QShader vertex = shaderAt(QStringLiteral("video.vert"));
    const QShader fragment = shaderAt(m_nv12 ? QStringLiteral("video_nv12.frag") : QStringLiteral("video_planar.frag"));
    if (!vertex.isValid() || !fragment.isValid()) {
        qWarning() << "video shaders are missing from the resources";
        return false;
    }
    m_pipeline.reset(rhi->newGraphicsPipeline());
    m_pipeline->setTopology(QRhiGraphicsPipeline::TriangleStrip);
    m_pipeline->setShaderStages({{QRhiShaderStage::Vertex, vertex}, {QRhiShaderStage::Fragment, fragment}});
    m_pipeline->setVertexInputLayout({});
    m_pipeline->setShaderResourceBindings(m_bindings.get());
    m_pipeline->setRenderPassDescriptor(pass);
    m_pipeline->setSampleCount(samples);
    m_pipeline->setFlags(QRhiGraphicsPipeline::UsesScissor);
    QRhiGraphicsPipeline::TargetBlend blend;
    blend.enable = true;
    blend.srcColor = QRhiGraphicsPipeline::One;
    blend.dstColor = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    blend.srcAlpha = QRhiGraphicsPipeline::One;
    blend.dstAlpha = QRhiGraphicsPipeline::OneMinusSrcAlpha;
    m_pipeline->setTargetBlends({blend});
    if (!m_pipeline->create()) {
        m_pipeline.reset();
        return false;
    }
    m_pipelinePass.reset(pass->newCompatibleRenderPassDescriptor());
    m_pipelineNv12 = m_nv12;
    m_pipelineSamples = samples;
    return true;
}

bool VideoRenderNode::uploadPlane(QRhi* rhi, QRhiResourceUpdateBatch* updates, std::size_t index, int format,
                                  QSize size, const void* data, std::size_t stride, std::size_t bytesPerTexel)
{
    Plane& plane = m_planes[index];
    if (plane.texture == nullptr || plane.size != size || plane.format != format) {
        plane.texture.reset(rhi->newTexture(static_cast<QRhiTexture::Format>(format), size));
        if (!plane.texture->create()) {
            plane.texture.reset();
            return false;
        }
        plane.size = size;
        plane.format = format;
        m_bindingsStale = true;
    }
    const std::size_t rowBytes = static_cast<std::size_t>(size.width()) * bytesPerTexel;
    QRhiTextureSubresourceUploadDescription part(data, static_cast<quint32>(stride * (size.height() - 1) + rowBytes));
    part.setDataStride(static_cast<quint32>(stride));
    updates->uploadTexture(plane.texture.get(), QRhiTextureUploadDescription({0, 0, part}));
    return true;
}

bool VideoRenderNode::uploadFrame(QRhi* rhi, QRhiResourceUpdateBatch* updates)
{
    const AVFrame* frame = m_frame.avFrame();
    const QSize full(frame->width, frame->height);
    const QSize half((frame->width + 1) / 2, (frame->height + 1) / 2);
    m_texelWidth = frame->width > 0 ? 1.0F / static_cast<float>(frame->width) : 0.0F;
    m_texelHeight = frame->height > 0 ? 1.0F / static_cast<float>(frame->height) : 0.0F;
    const bool wasNv12 = m_nv12;
    const std::size_t hadPlanes = m_planeCount;
    bool uploaded = false;
    switch (static_cast<AVPixelFormat>(frame->format)) {
    case AV_PIX_FMT_VIDEOTOOLBOX: {
        // A decoder surface: NV12 in an IOSurface-backed pixel buffer, 8 or 10 bits (P010 layout).
        auto* buffer = reinterpret_cast<CVPixelBufferRef>(frame->data[3]);
        if (buffer == nullptr || CVPixelBufferGetPlaneCount(buffer) != 2 ||
            CVPixelBufferLockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly) != kCVReturnSuccess) {
            break;
        }
        const OSType type = CVPixelBufferGetPixelFormatType(buffer);
        const bool deep = type == kCVPixelFormatType_420YpCbCr10BiPlanarVideoRange ||
                          type == kCVPixelFormatType_420YpCbCr10BiPlanarFullRange;
        const std::size_t texel = deep ? 2 : 1;
        const QSize luma(static_cast<int>(CVPixelBufferGetWidthOfPlane(buffer, 0)),
                         static_cast<int>(CVPixelBufferGetHeightOfPlane(buffer, 0)));
        const QSize chroma(static_cast<int>(CVPixelBufferGetWidthOfPlane(buffer, 1)),
                           static_cast<int>(CVPixelBufferGetHeightOfPlane(buffer, 1)));
        uploaded = uploadPlane(rhi, updates, 0, deep ? QRhiTexture::R16 : QRhiTexture::R8, luma,
                               CVPixelBufferGetBaseAddressOfPlane(buffer, 0), CVPixelBufferGetBytesPerRowOfPlane(buffer, 0),
                               texel) &&
                   uploadPlane(rhi, updates, 1, deep ? QRhiTexture::RG16 : QRhiTexture::RG8, chroma,
                               CVPixelBufferGetBaseAddressOfPlane(buffer, 1), CVPixelBufferGetBytesPerRowOfPlane(buffer, 1),
                               texel * 2);
        CVPixelBufferUnlockBaseAddress(buffer, kCVPixelBufferLock_ReadOnly);
        m_nv12 = true;
        m_planeCount = 2;
        break;
    }
    case AV_PIX_FMT_NV12:
    case AV_PIX_FMT_P010LE: {
        const bool deep = frame->format == AV_PIX_FMT_P010LE;
        uploaded = uploadPlane(rhi, updates, 0, deep ? QRhiTexture::R16 : QRhiTexture::R8, full, frame->data[0],
                               static_cast<std::size_t>(frame->linesize[0]), deep ? 2 : 1) &&
                   uploadPlane(rhi, updates, 1, deep ? QRhiTexture::RG16 : QRhiTexture::RG8, half, frame->data[1],
                               static_cast<std::size_t>(frame->linesize[1]), deep ? 4 : 2);
        m_nv12 = true;
        m_planeCount = 2;
        break;
    }
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        uploaded = uploadPlane(rhi, updates, 0, QRhiTexture::R8, full, frame->data[0],
                               static_cast<std::size_t>(frame->linesize[0]), 1) &&
                   uploadPlane(rhi, updates, 1, QRhiTexture::R8, half, frame->data[1],
                               static_cast<std::size_t>(frame->linesize[1]), 1) &&
                   uploadPlane(rhi, updates, 2, QRhiTexture::R8, half, frame->data[2],
                               static_cast<std::size_t>(frame->linesize[2]), 1);
        m_nv12 = false;
        m_planeCount = 3;
        break;
    default:
        if (!m_unsupportedLogged) {
            m_unsupportedLogged = true;
            qWarning() << "video frames in pixel format" << frame->format << "are not drawn on macOS";
        }
        break;
    }
    if (m_nv12 != wasNv12 || m_planeCount != hadPlanes) {
        m_bindingsStale = true;
    }
    return uploaded;
}

}
