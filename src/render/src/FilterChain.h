#pragma once

// Video filters on macOS: extension filters are HLSL pixel shaders that only Direct3D runs, so here every filter
// a window asks for is reported as not running, with the reason, and the picture is drawn without it.

#include <gs/render/VideoFilter.h>

#include <vector>

namespace gs::render {

class FilterChain
{
public:
    void setFilters(std::vector<VideoFilter> filters);
    [[nodiscard]] bool active() const noexcept { return false; }
    bool takeStatus(std::vector<FilterStatus>& status);
    void release() {}

private:
    std::vector<VideoFilter> m_wanted;
    bool m_statusChanged = false;
};

}
