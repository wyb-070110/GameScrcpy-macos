// Scanning a native extension's code before it is installed. Windows asks the antivirus through AMSI; macOS has no
// such interface for programs to call, so the scan is unavailable and the install check says so (Gatekeeper and
// XProtect still look at code when macOS runs it).
#include "AmsiScan.h"

namespace gs::app {

ext::ScanVerdict scanWithAmsi(std::string_view, std::string_view)
{
    return ext::ScanVerdict::Unavailable;
}

}
