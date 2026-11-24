#include <cstdint>

#if defined(_WIN32)
#define GS_TEST_EXPORT __declspec(dllexport)
#else
#define GS_TEST_EXPORT __attribute__((visibility("default")))
#endif

extern "C" GS_TEST_EXPORT std::int32_t gs_ext_entry(const void*, void*)
{
    return 0;
}
