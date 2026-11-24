#include "gs/adb/ServerLauncher.h"

#include "HostProtocol.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <fcntl.h>
#include <signal.h>
#include <spawn.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <thread>
#include <vector>

extern char** environ;
#endif

#include <format>
#include <string>

namespace gs::adb {

namespace {

AdbResult<int> checkVersion(int version)
{
    if (version < AdbClient::kMinimumServerVersion) {
        return detail::failure(AdbErrorKind::ServerTooOld,
                               std::format("adb server version {} is older than the required {}", version,
                                           AdbClient::kMinimumServerVersion));
    }
    return version;
}

#if defined(_WIN32)
AdbResult<void> startServer(const std::filesystem::path& adbExecutable, std::uint16_t port,
                            std::chrono::milliseconds timeout)
{
    std::wstring commandLine = std::format(L"\"{}\" -P {} start-server", adbExecutable.native(), port);
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(adbExecutable.c_str(), commandLine.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &startup, &process)) {
        return detail::failure(AdbErrorKind::LaunchFailed,
                               std::format("cannot run the bundled adb (error {})", GetLastError()));
    }
    CloseHandle(process.hThread);
    const DWORD waited = WaitForSingleObject(process.hProcess, static_cast<DWORD>(timeout.count()));
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hProcess);
    if (waited != WAIT_OBJECT_0) {
        return detail::failure(AdbErrorKind::LaunchFailed, "adb start-server did not finish in time");
    }
    if (exitCode != 0) {
        return detail::failure(AdbErrorKind::LaunchFailed, std::format("adb start-server exited with {}", exitCode));
    }
    return {};
}
#else
AdbResult<void> startServer(const std::filesystem::path& adbExecutable, std::uint16_t port,
                            std::chrono::milliseconds timeout)
{
    const std::string program = adbExecutable.string();
    std::string portText = std::to_string(port);
    std::vector<char*> arguments{const_cast<char*>(program.c_str()), const_cast<char*>("-P"), portText.data(),
                                 const_cast<char*>("start-server"), nullptr};
    posix_spawn_file_actions_t actions;
    posix_spawn_file_actions_init(&actions);
    posix_spawn_file_actions_addopen(&actions, STDIN_FILENO, "/dev/null", O_RDONLY, 0);
    pid_t child = 0;
    const int spawned = posix_spawn(&child, program.c_str(), &actions, nullptr, arguments.data(), environ);
    posix_spawn_file_actions_destroy(&actions);
    if (spawned != 0) {
        return detail::failure(AdbErrorKind::LaunchFailed, std::format("cannot run the bundled adb (error {})", spawned));
    }
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    int status = 0;
    for (;;) {
        const pid_t waited = waitpid(child, &status, WNOHANG);
        if (waited == child) {
            break;
        }
        if (waited < 0 && errno != EINTR) {
            return detail::failure(AdbErrorKind::LaunchFailed, "adb start-server could not be waited for");
        }
        if (std::chrono::steady_clock::now() >= deadline) {
            kill(child, SIGKILL);
            waitpid(child, &status, 0);
            return detail::failure(AdbErrorKind::LaunchFailed, "adb start-server did not finish in time");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(20)); // source-rule-allow R9: polling a child process exit; POSIX has no waitpid with a timeout.
    }
    const int exitCode = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    if (exitCode != 0) {
        return detail::failure(AdbErrorKind::LaunchFailed, std::format("adb start-server exited with {}", exitCode));
    }
    return {};
}
#endif

}

AdbResult<int> ensureServer(const AdbClient& client, const std::filesystem::path& adbExecutable,
                            std::chrono::milliseconds timeout)
{
    AdbResult<int> running = client.serverVersion();
    if (running) {
        return checkVersion(*running);
    }
    if (running.error().kind != AdbErrorKind::ServerUnavailable) {
        return running;
    }
    if (AdbResult<void> started = startServer(adbExecutable, client.port(), timeout); !started) {
        return makeUnexpected(std::move(started.error()));
    }
    AdbResult<int> version = client.serverVersion();
    if (!version) {
        return version;
    }
    return checkVersion(*version);
}

}
