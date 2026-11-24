#include "PlaneReader.h"

extern "C" {
#include <libavutil/frame.h>
#include <libavutil/hwcontext.h>
#include <libavutil/pixfmt.h>
}

#include <cstdint>

namespace gs::media {

bool readMapped(const AVFrame& frame, const std::function<void(const AVFrame&)>& use)
{
    if (frame.hw_frames_ctx == nullptr) {
        return false;
    }
    AVFrame* mapped = av_frame_alloc();
    if (mapped == nullptr) {
        return false;
    }
    mapped->format = reinterpret_cast<const AVHWFramesContext*>(frame.hw_frames_ctx->data)->sw_format;
    const bool ok = av_hwframe_map(mapped, &frame, AV_HWFRAME_MAP_READ) >= 0;
    if (ok) {
        mapped->width = frame.width;
        mapped->height = frame.height;
        mapped->colorspace = frame.colorspace;
        mapped->color_range = frame.color_range;
        mapped->pts = frame.pts;
        use(*mapped);
    }
    av_frame_free(&mapped);
    return ok;
}

#if defined(_WIN32)
PlaneReader::PlaneReader(ID3D11Device* device)
{
    if (device != nullptr) {
        m_device = device;
        device->GetImmediateContext(&m_context);
    }
}
#else
PlaneReader::PlaneReader(ID3D11Device*) {}
#endif

bool PlaneReader::read(const AVFrame& frame, const std::function<void(const Planes&)>& use)
{
    if (frame.width <= 0 || frame.height <= 0) {
        return false;
    }
    switch (frame.format) {
    case AV_PIX_FMT_YUV420P:
    case AV_PIX_FMT_YUVJ420P:
        use({frame.data[0], frame.linesize[0], frame.data[1], frame.data[2], frame.linesize[1], 1});
        return true;
    case AV_PIX_FMT_NV12:
        use({frame.data[0], frame.linesize[0], frame.data[1], frame.data[1] + 1, frame.linesize[1], 2});
        return true;
#if defined(_WIN32)
    case AV_PIX_FMT_D3D11:
        return readSurface(frame, use);
#else
    case AV_PIX_FMT_VIDEOTOOLBOX:
        return readSurface(frame, use);
#endif
    default:
        return false;
    }
}

#if defined(_WIN32)
bool PlaneReader::readSurface(const AVFrame& frame, const std::function<void(const Planes&)>& use)
{
    if (!m_device) {
        return false;
    }
    auto* texture = reinterpret_cast<ID3D11Texture2D*>(frame.data[0]);
    const auto slice = static_cast<UINT>(reinterpret_cast<std::intptr_t>(frame.data[1]));
    D3D11_TEXTURE2D_DESC description{};
    texture->GetDesc(&description);
    if (description.Format != DXGI_FORMAT_NV12) {
        return false;
    }
    if (!m_staging || m_stagingWidth != description.Width || m_stagingHeight != description.Height) {
        D3D11_TEXTURE2D_DESC wanted{};
        wanted.Width = description.Width;
        wanted.Height = description.Height;
        wanted.MipLevels = 1;
        wanted.ArraySize = 1;
        wanted.Format = DXGI_FORMAT_NV12;
        wanted.SampleDesc.Count = 1;
        wanted.Usage = D3D11_USAGE_STAGING;
        wanted.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
        m_staging.Reset();
        if (FAILED(m_device->CreateTexture2D(&wanted, nullptr, &m_staging))) {
            return false;
        }
        m_stagingWidth = wanted.Width;
        m_stagingHeight = wanted.Height;
    }
    m_context->CopySubresourceRegion(m_staging.Get(), 0, 0, 0, 0, texture,
                                     D3D11CalcSubresource(0, slice, description.MipLevels), nullptr);
    D3D11_MAPPED_SUBRESOURCE mapped{};
    if (FAILED(m_context->Map(m_staging.Get(), 0, D3D11_MAP_READ, 0, &mapped))) {
        return false;
    }
    const auto* luma = static_cast<const std::uint8_t*>(mapped.pData);
    const auto pitch = static_cast<std::ptrdiff_t>(mapped.RowPitch);
    const std::uint8_t* chroma = luma + pitch * description.Height;
    use({luma, pitch, chroma, chroma + 1, pitch, 2});
    m_context->Unmap(m_staging.Get(), 0);
    return true;
}
#else
bool PlaneReader::readSurface(const AVFrame& frame, const std::function<void(const Planes&)>& use)
{
    bool read = false;
    const bool mapped = readMapped(frame, [&](const AVFrame& memory) {
        if (memory.format == AV_PIX_FMT_NV12) {
            use({memory.data[0], memory.linesize[0], memory.data[1], memory.data[1] + 1, memory.linesize[1], 2});
            read = true;
        }
    });
    return mapped && read;
}
#endif

}
