// The POSIX platform backend: atomic files, the running marker and word spacing for read text.
#include "gs/platform/AtomicFile.h"
#include "gs/platform/RunningMarker.h"
#include "gs/platform/TextReader.h"

#include <doctest/doctest.h>

#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>

using namespace gs::platform;

namespace {

class Folder
{
public:
    Folder()
    {
        std::string pattern = (std::filesystem::temp_directory_path() / "gs-posix-XXXXXX").string();
        REQUIRE(::mkdtemp(pattern.data()) != nullptr);
        m_path = pattern;
    }
    ~Folder()
    {
        std::error_code error;
        std::filesystem::remove_all(m_path, error);
    }
    Folder(const Folder&) = delete;
    Folder& operator=(const Folder&) = delete;

    [[nodiscard]] const std::filesystem::path& path() const { return m_path; }

private:
    std::filesystem::path m_path;
};

std::string contentsOf(const std::filesystem::path& file)
{
    std::ifstream in(file, std::ios::binary);
    return {std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>()};
}

}

TEST_CASE("an atomic write creates or replaces the file and leaves no temporary")
{
    const Folder folder;
    const auto file = folder.path() / "settings.json";
    REQUIRE_FALSE(writeFileAtomically(file, "first"));
    CHECK(contentsOf(file) == "first");
    REQUIRE_FALSE(writeFileAtomically(file, "second, longer"));
    CHECK(contentsOf(file) == "second, longer");
    CHECK_FALSE(std::filesystem::exists(folder.path() / "settings.json.tmp"));
}

TEST_CASE("a temporary left by an interrupted write is overwritten")
{
    const Folder folder;
    const auto file = folder.path() / "a.json";
    std::ofstream(folder.path() / "a.json.tmp") << "half written";
    REQUIRE_FALSE(writeFileAtomically(file, "whole"));
    CHECK(contentsOf(file) == "whole");
    CHECK_FALSE(std::filesystem::exists(folder.path() / "a.json.tmp"));
}

TEST_CASE("an atomic write into a missing directory fails and creates nothing")
{
    const Folder folder;
    const auto file = folder.path() / "missing" / "a.json";
    const std::error_code error = writeFileAtomically(file, "x");
    CHECK(error);
    CHECK(fileMissing(error));
    CHECK_FALSE(std::filesystem::exists(folder.path() / "missing"));
}

TEST_CASE("reading tells a missing and an oversized file apart and reads a whole one")
{
    const Folder folder;
    std::string contents;
    const std::error_code missing = readFile(folder.path() / "none", 100, contents);
    CHECK(fileMissing(missing));
    CHECK_FALSE(fileBusy(missing));
    REQUIRE_FALSE(writeFileAtomically(folder.path() / "big", std::string(200, 'x')));
    CHECK(readFile(folder.path() / "big", 100, contents) == std::make_error_code(std::errc::file_too_large));
    REQUIRE_FALSE(readFile(folder.path() / "big", 200, contents));
    CHECK(contents == std::string(200, 'x'));
}

TEST_CASE("replacing moves a finished file over the target, and flushing works on files and folders")
{
    const Folder folder;
    const auto from = folder.path() / "from";
    const auto to = folder.path() / "to";
    std::ofstream(from) << "new";
    std::ofstream(to) << "old";
    REQUIRE_FALSE(replaceFile(from, to));
    CHECK(contentsOf(to) == "new");
    CHECK_FALSE(std::filesystem::exists(from));
    CHECK_FALSE(flushFile(to));
    CHECK_FALSE(flushTree(folder.path()));
    CHECK(fileMissing(flushFile(folder.path() / "none")));
}

TEST_CASE("removing a file works, and a missing file counts as removed")
{
    const Folder folder;
    const auto file = folder.path() / "gone";
    std::ofstream(file) << "x";
    CHECK_FALSE(removeFile(file));
    CHECK_FALSE(std::filesystem::exists(file));
    CHECK_FALSE(removeFile(file));
}

TEST_CASE("a writer killed at any moment leaves the file whole")
{
    const Folder folder;
    const auto file = folder.path() / "state.json";
    const std::string before(64 * 1024, 'a');
    const std::string after(64 * 1024, 'b');
    REQUIRE_FALSE(writeFileAtomically(file, before));
    for (int attempt = 0; attempt < 20; ++attempt) {
        const pid_t child = ::fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            for (;;) {
                static_cast<void>(writeFileAtomically(file, after));
                static_cast<void>(writeFileAtomically(file, before));
            }
        }
        ::usleep(static_cast<useconds_t>(1000 + attempt * 500)); // source-rule-allow R9: let the child get into a write before it is killed
        ::kill(child, SIGKILL);
        int status = 0;
        ::waitpid(child, &status, 0);
        const std::string seen = contentsOf(file);
        CHECK((seen == before || seen == after));
    }
}

TEST_CASE("the running marker is seen while held and gone after")
{
    const std::wstring name = L"GameScrcpyTest-" + std::to_wstring(::getpid());
    CHECK_FALSE(runningMarkerPresent(name));
    {
        const RunningMarker marker(name);
        REQUIRE(marker.held());
        const RunningMarker second(name);
        CHECK(second.held());
        const pid_t child = ::fork();
        REQUIRE(child >= 0);
        if (child == 0) {
            ::_exit(runningMarkerPresent(name) ? 0 : 1);
        }
        int status = 0;
        ::waitpid(child, &status, 0);
        CHECK((WIFEXITED(status) && WEXITSTATUS(status) == 0));
    }
    CHECK_FALSE(runningMarkerPresent(name));
}

TEST_CASE("words are spaced only between two letters that are not Chinese, Japanese or full width")
{
    CHECK(joinsWithSpace(U'a', U'1'));
    CHECK(joinsWithSpace(U'%', U'B'));
    CHECK(joinsWithSpace(U'한', U'국'));
    CHECK_FALSE(joinsWithSpace(U'电', U'池'));
    CHECK_FALSE(joinsWithSpace(U'a', U'池'));
    CHECK_FALSE(joinsWithSpace(U'ア', U'イ'));
    CHECK_FALSE(joinsWithSpace(U'Ａ', U'b'));
    CHECK_FALSE(joinsWithSpace(0, U'b'));
}
