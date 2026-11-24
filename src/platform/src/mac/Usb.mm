// Phones over USB without adb, on macOS: not done yet. adb reaches phones over USB itself on macOS (no driver to
// install, unlike Windows), so the list here stays empty and opening a phone says it was not found.
#include "gs/platform/Usb.h"

namespace gs::platform {

struct UsbDevice::Impl {};
struct UsbWatcher::Impl {};

std::vector<UsbPhone> listUsbPhones()
{
    return {};
}

std::string usbLibraryVersion()
{
    return "none (adb reaches phones over USB on macOS)";
}

UsbDevice::UsbDevice(std::unique_ptr<Impl> impl) : m_impl(std::move(impl)) {}

UsbDevice::~UsbDevice() = default;

std::unique_ptr<UsbDevice> UsbDevice::open(const UsbPhone&, UsbFailure& failure)
{
    failure = UsbFailure::NotFound;
    return nullptr;
}

bool UsbDevice::controlOut(std::uint8_t, std::uint16_t, std::uint16_t, std::span<const std::uint8_t>)
{
    return false;
}

std::optional<std::vector<std::uint8_t>> UsbDevice::controlIn(std::uint8_t, std::uint16_t, std::uint16_t, std::size_t)
{
    return std::nullopt;
}

std::size_t UsbDevice::maxPacketSize() const noexcept
{
    return 0;
}

UsbFailure UsbDevice::failure() const noexcept
{
    return UsbFailure::NotFound;
}

UsbWatcher::UsbWatcher(Changed) : m_impl(std::make_unique<Impl>()) {}

UsbWatcher::~UsbWatcher() = default;

bool UsbWatcher::started() const noexcept
{
    return false;
}

}
