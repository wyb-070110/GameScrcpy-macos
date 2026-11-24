#include "FilterChain.h"

#include <utility>

namespace gs::render {

void FilterChain::setFilters(std::vector<VideoFilter> filters)
{
    if (filters == m_wanted) {
        return;
    }
    m_wanted = std::move(filters);
    m_statusChanged = true;
}

bool FilterChain::takeStatus(std::vector<FilterStatus>& status)
{
    if (!m_statusChanged) {
        return false;
    }
    m_statusChanged = false;
    status.clear();
    for (const VideoFilter& filter : m_wanted) {
        status.push_back({.id = filter.id,
                          .ready = false,
                          .error = "video filters are HLSL shaders for Direct3D and do not run on macOS yet",
                          .passes = {},
                          .passMs = {}});
    }
    return true;
}

}
