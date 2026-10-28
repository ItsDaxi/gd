#include "paths.hpp"

#include <windows.h>
#include <shlobj.h>

namespace mh::paths {

std::filesystem::path gameDir() {
    wchar_t buf[MAX_PATH]{};
    GetModuleFileNameW(nullptr, buf, MAX_PATH);
    return std::filesystem::path(buf).parent_path();
}

std::filesystem::path dataDir() {
    static std::filesystem::path dir = [] {
        auto d = gameDir() / "megahack";
        std::error_code ec;
        std::filesystem::create_directories(d, ec);
        std::filesystem::create_directories(d / "langs", ec);
        std::filesystem::create_directories(d / "replays", ec);
        std::filesystem::create_directories(d / "screenshots", ec);
        return d;
    }();
    return dir;
}

std::filesystem::path appData() {
    PWSTR path = nullptr;
    std::filesystem::path out;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        out = std::filesystem::path(path) / "GeometryDash";
        CoTaskMemFree(path);
    }
    return out;
}

} // namespace mh::paths
