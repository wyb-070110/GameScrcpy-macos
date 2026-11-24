// Decoding with VideoToolbox (macOS): every picture comes back from the GPU, and snapshots, thumbnails and
// luma readings of those pictures are the same, byte for byte, as of the pictures decoded in software.
#include "gs/media/FrameSnapshot.h"
#include "gs/media/LumaSampler.h"
#include "gs/media/Thumbnailer.h"
#include "gs/media/VideoDecoder.h"

#include <gs/protocol/Codec.h>
#include <gs/protocol/Constants.h>

#include <doctest/doctest.h>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <span>
#include <string>
#include <string_view>
#include <vector>

using namespace gs;
using namespace gs::media;

namespace {

struct Seen {
    bool hardware = false;
    std::vector<std::uint8_t> luma;
    std::vector<std::uint8_t> chroma;
    Thumbnail thumbnail;
    std::vector<LumaReading> readings;
};

class Collect final : public FrameSink
{
public:
    void onFrame(VideoFrame frame) override
    {
        Seen seen;
        seen.hardware = frame.hardware();
        m_snapshots.offer(frame);
        const auto shot = m_snapshots.capture(true);
        REQUIRE(shot != nullptr);
        seen.luma = shot->luma;
        seen.chroma = shot->chroma;
        const auto thumbnail = m_thumbnailer.make(frame, 64);
        REQUIRE(thumbnail.has_value());
        seen.thumbnail = *thumbnail;
        const LumaSpot spot{frame.width() / 2, frame.height() / 2, 4, 6, 10};
        const auto readings = m_sampler.read(frame, std::span(&spot, 1));
        REQUIRE(readings.has_value());
        seen.readings = *readings;
        pictures.push_back(std::move(seen));
    }

    std::vector<Seen> pictures;

private:
    FrameSnapshots m_snapshots;
    Thumbnailer m_thumbnailer;
    LumaSampler m_sampler;
};

std::string difference(const std::vector<std::uint8_t>& gpu, const std::vector<std::uint8_t>& cpu)
{
    if (gpu.size() != cpu.size()) {
        return "sizes " + std::to_string(gpu.size()) + " and " + std::to_string(cpu.size());
    }
    std::size_t differing = 0;
    int largest = 0;
    std::size_t first = gpu.size();
    for (std::size_t at = 0; at < gpu.size(); ++at) {
        const int gap = std::abs(static_cast<int>(gpu[at]) - static_cast<int>(cpu[at]));
        if (gap != 0) {
            first = std::min(first, at);
            ++differing;
            largest = std::max(largest, gap);
        }
    }
    if (differing == 0) {
        return {};
    }
    return std::to_string(differing) + " of " + std::to_string(gpu.size()) + " bytes differ, by up to " +
           std::to_string(largest) + ", first at " + std::to_string(first);
}

std::vector<Seen> decoded(std::string_view name, Decoding decoding)
{
    std::ifstream file(std::filesystem::path(GS_VIDEO_TEST_DATA_DIR) / name, std::ios::binary);
    REQUIRE(file);
    const std::vector<std::uint8_t> recording{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    VideoDecoder decoder(decoding);
    Collect collect;
    for (std::size_t offset = 0; offset < recording.size();) {
        const auto packet =
            protocol::decodeMediaPacket(protocol::MediaChannel::Video, std::span(recording).subspan(offset));
        REQUIRE(packet);
        offset += protocol::kMediaHeaderSize + packet->header.size;
        const auto pushed = decoder.push(*packet, collect);
        REQUIRE_MESSAGE(pushed, (pushed ? std::string() : pushed.error().message));
    }
    return std::move(collect.pictures);
}

}

TEST_CASE("hardware decoding: the GPU decodes H.264 and H.265, and its pictures read back as in software")
{
    for (const std::string_view name : {"emulator_h264.media", "emulator_h265.media", "emulator_av1.media"}) {
        CAPTURE(name);
        const std::vector<Seen> gpu = decoded(name, Decoding::Hardware);
        const std::vector<Seen> cpu = decoded(name, Decoding::Software);
        REQUIRE(gpu.size() == cpu.size());
        REQUIRE(gpu.size() > 40);
        std::size_t onGpu = 0;
        for (std::size_t i = 0; i < gpu.size(); ++i) {
            CAPTURE(i);
            CHECK_FALSE(cpu[i].hardware);
            onGpu += gpu[i].hardware ? 1 : 0;
            CHECK(difference(gpu[i].luma, cpu[i].luma) == "");
            CHECK(difference(gpu[i].chroma, cpu[i].chroma) == "");
            CHECK(gpu[i].thumbnail.width == cpu[i].thumbnail.width);
            CHECK((gpu[i].thumbnail.pixels == cpu[i].thumbnail.pixels));
            CHECK(gpu[i].readings == cpu[i].readings);
        }
        MESSAGE(std::string(name) << ": " << onGpu << " of " << gpu.size() << " pictures from the GPU");
        CHECK((onGpu == 0 || onGpu == gpu.size()));
#if defined(__APPLE__)
        if (name != "emulator_av1.media") {
            CHECK(onGpu == gpu.size());
        }
#endif
    }
}
