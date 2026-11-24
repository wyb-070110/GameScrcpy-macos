// The per-user key as a generic password in the user's login keychain.
#include "UserKey.h"

#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>

#include <cstring>
#include <mutex>

namespace gs::platform::detail {

namespace {

constexpr char kService[] = "GameScrcpy";
constexpr char kAccount[] = "user-protection-key";

class Ref
{
public:
    explicit Ref(CFTypeRef ref = nullptr) noexcept : m_ref(ref) {}
    ~Ref()
    {
        if (m_ref != nullptr) {
            CFRelease(m_ref);
        }
    }
    Ref(const Ref&) = delete;
    Ref& operator=(const Ref&) = delete;

    [[nodiscard]] CFTypeRef get() const noexcept { return m_ref; }
    CFTypeRef* out() noexcept { return &m_ref; }

private:
    CFTypeRef m_ref;
};

Ref string(const char* text) noexcept
{
    return Ref(CFStringCreateWithCString(kCFAllocatorDefault, text, kCFStringEncodingUTF8));
}

Ref query(const Ref& service, const Ref& account) noexcept
{
    CFMutableDictionaryRef dictionary =
        CFDictionaryCreateMutable(kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    if (dictionary != nullptr) {
        CFDictionarySetValue(dictionary, kSecClass, kSecClassGenericPassword);
        CFDictionarySetValue(dictionary, kSecAttrService, service.get());
        CFDictionarySetValue(dictionary, kSecAttrAccount, account.get());
    }
    return Ref(dictionary);
}

std::optional<UserKey> find(const Ref& service, const Ref& account) noexcept
{
    const Ref search = query(service, account);
    if (search.get() == nullptr) {
        return std::nullopt;
    }
    auto* dictionary = static_cast<CFMutableDictionaryRef>(const_cast<void*>(search.get()));
    CFDictionarySetValue(dictionary, kSecReturnData, kCFBooleanTrue);
    CFDictionarySetValue(dictionary, kSecMatchLimit, kSecMatchLimitOne);
    Ref data;
    if (SecItemCopyMatching(dictionary, data.out()) != errSecSuccess || data.get() == nullptr ||
        CFGetTypeID(data.get()) != CFDataGetTypeID()) {
        return std::nullopt;
    }
    const auto bytes = static_cast<CFDataRef>(data.get());
    UserKey key{};
    if (static_cast<std::size_t>(CFDataGetLength(bytes)) != key.size()) {
        return std::nullopt;
    }
    std::memcpy(key.data(), CFDataGetBytePtr(bytes), key.size());
    return key;
}

std::optional<UserKey> add(const Ref& service, const Ref& account) noexcept
{
    UserKey key{};
    randomBytes(key);
    const Ref item = query(service, account);
    const Ref data(CFDataCreate(kCFAllocatorDefault, key.data(), static_cast<CFIndex>(key.size())));
    if (item.get() == nullptr || data.get() == nullptr) {
        return std::nullopt;
    }
    auto* dictionary = static_cast<CFMutableDictionaryRef>(const_cast<void*>(item.get()));
    CFDictionarySetValue(dictionary, kSecValueData, data.get());
    CFDictionarySetValue(dictionary, kSecAttrAccessible, kSecAttrAccessibleWhenUnlockedThisDeviceOnly);
    const OSStatus status = SecItemAdd(dictionary, nullptr);
    if (status == errSecDuplicateItem) {
        return find(service, account);
    }
    return status == errSecSuccess ? std::optional<UserKey>(key) : std::nullopt;
}

}

std::optional<UserKey> userKey() noexcept
{
    static std::mutex mutex;
    const std::lock_guard lock(mutex);
    const Ref service = string(kService);
    const Ref account = string(kAccount);
    if (service.get() == nullptr || account.get() == nullptr) {
        return std::nullopt;
    }
    if (std::optional<UserKey> existing = find(service, account)) {
        return existing;
    }
    return add(service, account);
}

}
