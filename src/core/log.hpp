#pragma once
#include <cstdio>
#include <string>

namespace mh::log {

void init();
void write(const char* level, const char* fmt, ...);

} // namespace mh::log

#define MH_LOG(...) ::mh::log::write("INFO", __VA_ARGS__)
#define MH_WARN(...) ::mh::log::write("WARN", __VA_ARGS__)
#define MH_ERR(...) ::mh::log::write("ERR ", __VA_ARGS__)
