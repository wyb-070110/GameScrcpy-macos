// The per-user key behind protectForUser: created on first use, then read back.
#pragma once

#include "gs/platform/Crypto.h"

#include <array>
#include <cstdint>
#include <optional>

namespace gs::platform {

using UserKey = std::array<std::uint8_t, kAesGcmKeySize>;

namespace detail {

[[nodiscard]] std::optional<UserKey> userKey() noexcept;

}

}
