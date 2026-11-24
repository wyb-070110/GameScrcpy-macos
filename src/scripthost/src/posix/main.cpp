// gs-scripthost off Windows: sandbox the process before anything is read (macOS: the pure-computation
// profile), then serve the pipes the parent passed.
#include "Server.h"

#include <cstdlib>
#include <string>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <cstdint>

// From <sandbox.h>, which this module's own Sandbox.h hides on a case-insensitive file system.
extern "C" {
extern const char kSBXProfilePureComputation[];
int sandbox_init(const char* profile, std::uint64_t flags, char** error);
void sandbox_free_error(char* error);
}
#endif

namespace {

constexpr int kSandboxFailed = 4;

bool sandboxed() noexcept
{
#if defined(__APPLE__)
    constexpr std::uint64_t kNamedProfile = 0x0001;
    char* error = nullptr;
    const bool applied = sandbox_init(kSBXProfilePureComputation, kNamedProfile, &error) == 0;
    if (error != nullptr) {
        sandbox_free_error(error);
    }
    return applied;
#else
    const char* allowed = std::getenv("GS_SCRIPTHOST_UNSANDBOXED");
    return allowed != nullptr && std::string_view(allowed) == "1";
#endif
}

}

int main(int argc, char** argv)
{
    std::vector<std::wstring> words;
    for (int index = 0; index < argc; ++index) {
        const std::string_view word(argv[index]);
        words.emplace_back(word.begin(), word.end());
        if (word == "--sandbox" && !sandboxed()) {
            return kSandboxFailed;
        }
    }
    std::vector<wchar_t*> pointers;
    for (std::wstring& word : words) {
        pointers.push_back(word.data());
    }
    pointers.push_back(nullptr);
    return gs::scripthost::serve(argc, pointers.data());
}
