#pragma once
#include <filesystem>

namespace mh::paths {

// Cartella del gioco (dove si trova GeometryDash.exe)
std::filesystem::path gameDir();
// <gioco>/megahack : config, lingue, macro, screenshot, log
std::filesystem::path dataDir();
// %LOCALAPPDATA%/GeometryDash
std::filesystem::path appData();

} // namespace mh::paths
