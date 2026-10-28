#include "log.hpp"
#include "paths.hpp"

#include <windows.h>
#include <cstdarg>
#include <ctime>
#include <mutex>

namespace mh::log {

static FILE* g_file = nullptr;
static std::mutex g_mutex;

void init() {
    std::lock_guard lock(g_mutex);
    if (g_file) return;
    auto p = paths::dataDir() / "log.txt";
    g_file = _wfopen(p.c_str(), L"w");
}

void write(const char* level, const char* fmt, ...) {
    char msg[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);

    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_s(&tmv, &t);
    char line[2200];
    snprintf(line, sizeof(line), "[%02d:%02d:%02d] [%s] %s\n", tmv.tm_hour, tmv.tm_min, tmv.tm_sec, level, msg);

    std::lock_guard lock(g_mutex);
    OutputDebugStringA(line);
    if (g_file) {
        fputs(line, g_file);
        fflush(g_file);
    }
}

} // namespace mh::log
