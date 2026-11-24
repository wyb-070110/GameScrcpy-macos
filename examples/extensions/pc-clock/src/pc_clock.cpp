#include <gamescrcpy_ext.h>

#include <cstdio>
#include <ctime>

namespace {

const gs_ext_host* g_host = nullptr;

int32_t now(void*, const gs_ext_value*, uint32_t, gs_ext_result* result)
{
    const std::time_t seconds = std::time(nullptr);
    std::tm time{};
    localtime_r(&seconds, &time);
    char text[16] = {};
    const int length = std::snprintf(text, sizeof(text), "%02d:%02d:%02d", time.tm_hour, time.tm_min, time.tm_sec);
    if (length <= 0) {
        constexpr char kFailed[] = "the time could not be written";
        g_host->set_error(result, kFailed, sizeof(kFailed) - 1);
        return GS_EXT_FAILED;
    }
    return g_host->set_text(result, GS_EXT_TYPE_STRING, text, static_cast<size_t>(length));
}

const gs_ext_node kNow = {sizeof(gs_ext_node), "now", nullptr, 0, GS_EXT_TYPE_STRING, nullptr, &now};

const gs_ext_contribution kContributions[] = {
    {sizeof(gs_ext_contribution), GS_EXT_KIND_NODE, "now", &kNow},
};

const gs_ext_plugin kPlugin = {
    sizeof(gs_ext_plugin), GS_EXT_ABI_VERSION, nullptr, 1, kContributions, nullptr,
};

}

GS_EXT_EXPORT int32_t gs_ext_main(const gs_ext_host* host, const gs_ext_plugin** plugin)
{
    if (host == nullptr || plugin == nullptr || host->abi_version != GS_EXT_ABI_VERSION) {
        return GS_EXT_UNSUPPORTED;
    }
    g_host = host;
    *plugin = &kPlugin;
    return GS_EXT_OK;
}
