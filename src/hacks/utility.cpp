// Finestre UTILITY (Shortcuts) e SCREENSHOT
#include "common.hpp"
#include "core/paths.hpp"

#include <windows.h>
#include <GL/gl.h>
#include <shellapi.h>

#include <ctime>
#include <fstream>

using namespace mh;

namespace {

void click(bool p1, int button) {
    auto l = gd::gameLayer();
    if (!l) return;
    gd::call<void>(gd::addr::GJBaseGameLayer::handleButton, l, true, button, p1);
    gd::call<void>(gd::addr::GJBaseGameLayer::handleButton, l, false, button, p1);
}

void openFolder(const std::filesystem::path& p) {
    ShellExecuteW(nullptr, L"open", p.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
}

int s_crashStep = 0;
void crashStep(void*) {
    s_crashStep++;
    if (s_crashStep < 3) {
        confirm(lang::tr("MESSAGE/UTILITY/CRASH/" + std::to_string(s_crashStep)), crashStep, nullptr);
        return;
    }
    s_crashStep = 0;
    config::save();
    TerminateProcess(GetCurrentProcess(), 0xDEAD);
}

Hack& p1Click = action("UTILITY", "UTILITY/CLICKS/P1", [] { click(true, 1); });
Hack& p2Click = action("UTILITY", "UTILITY/CLICKS/P2", [] { click(false, 1); });
Hack& p1Left = action("UTILITY", "UTILITY/DIRECTIONAL/P1_LEFT", [] { click(true, 2); }).name("P1 Left");
Hack& p1Right = action("UTILITY", "UTILITY/DIRECTIONAL/P1_RIGHT", [] { click(true, 3); }).name("P1 Right");
Hack& p2Left = action("UTILITY", "UTILITY/DIRECTIONAL/P2_LEFT", [] { click(false, 2); }).name("P2 Left");
Hack& p2Right = action("UTILITY", "UTILITY/DIRECTIONAL/P2_RIGHT", [] { click(false, 3); }).name("P2 Right");
Hack& crash = action("UTILITY", "UTILITY/CRASH", [] {
    s_crashStep = 0;
    confirm(lang::tr("MESSAGE/UTILITY/CRASH/0"), crashStep, nullptr);
});
Hack& devTools = action("UTILITY", "UTILITY/DEV_TOOLS", [] {}).partial("Richiede Geode DevTools: non disponibile senza Geode");
Hack& uncomplete = action("UTILITY", "UTILITY/UNCOMPLETE_LEVEL", [] {
    auto lvl = gd::level();
    auto gs = gd::gameStats();
    if (!lvl || !gs) return;
    confirm(lang::tr("UTILITY/UNCOMPLETE_LEVEL/TIP") + "?", [](void* l) {
        gd::call<void>(gd::addr::GameStatsManager::uncompleteLevel, gd::gameStats(), l);
        gd::setSeedValue(&GDF(l, GJGameLevel, m_newNormalPercent2, char), 0);
        GDF(l, GJGameLevel, m_practicePercent, int) = 0;
        // m_normalPercent e' SeedValueVSR: [valore, seed, random]
        auto v = reinterpret_cast<int*>(&GDF(l, GJGameLevel, m_normalPercent, char));
        v[0] = 0;
        v[2] = v[1];
    }, lvl);
});
Hack& restart = action("UTILITY", "UTILITY/RESTART_LEVEL", [] {
    if (auto pl = gd::playLayer()) gd::call<void>(gd::addr::PlayLayer::resetLevel, pl);
});
Hack& practice = action("UTILITY", "UTILITY/PRACTICE_MODE", [] {
    if (auto pl = gd::playLayer())
        gd::call<void>(gd::addr::PlayLayer::togglePracticeMode, pl, !GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool));
});
Hack& settings = action("UTILITY", "UTILITY/SETTINGS", [] {
    auto gm = gd::gameManager();
    auto menu = gm ? GDF(gm, GameManager, m_menuLayer, void*) : nullptr;
    if (menu && !gd::gameLayer()) {
        gd::call<void>(gd::addr::MenuLayer::onOptions, menu, static_cast<void*>(nullptr));
        return;
    }
    // altrove: apre le opzioni video (unico layer di opzioni con create() non inline)
    auto layer = gd::call<void*>(gd::addr::VideoOptionsLayer::create);
    if (layer) gd::call<void>(gd::addr::FLAlertLayer::show, layer);
});
Hack& resources = action("UTILITY", "UTILITY/FOLDERS/RESOURCES", [] { openFolder(paths::gameDir() / "Resources"); });
Hack& appdata = action("UTILITY", "UTILITY/FOLDERS/APPDATA", [] { openFolder(paths::appData()); });

// ================================================================== SCREENSHOT
int s_shotMode = 0;
bool s_shotRequested = false;
Hack& screenshot = action("SCREENSHOT", "SCREENSHOT/SCREENSHOT", [] { s_shotRequested = true; })
    .optCombo("SCREENSHOT/COMBO", &s_shotMode, {"SCREENSHOT/COMBO/0", "SCREENSHOT/COMBO/1", "SCREENSHOT/COMBO/2"});

uint32_t crc32(const uint8_t* data, size_t len, uint32_t crc = 0) {
    static uint32_t table[256];
    static bool init = false;
    if (!init) {
        for (uint32_t i = 0; i < 256; i++) {
            uint32_t c = i;
            for (int k = 0; k < 8; k++) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            table[i] = c;
        }
        init = true;
    }
    crc = ~crc;
    for (size_t i = 0; i < len; i++) crc = table[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
    return ~crc;
}

void put32(std::vector<uint8_t>& v, uint32_t x) {
    v.push_back(static_cast<uint8_t>(x >> 24));
    v.push_back(static_cast<uint8_t>(x >> 16));
    v.push_back(static_cast<uint8_t>(x >> 8));
    v.push_back(static_cast<uint8_t>(x));
}

void chunk(std::vector<uint8_t>& out, const char* type, const std::vector<uint8_t>& data) {
    put32(out, static_cast<uint32_t>(data.size()));
    std::vector<uint8_t> td(type, type + 4);
    td.insert(td.end(), data.begin(), data.end());
    out.insert(out.end(), td.begin(), td.end());
    put32(out, crc32(td.data(), td.size()));
}

// PNG RGB senza compressione (blocchi deflate "stored"): semplice e senza dipendenze
std::vector<uint8_t> encodePng(const std::vector<uint8_t>& rgb, int w, int h) {
    std::vector<uint8_t> raw;
    raw.reserve(static_cast<size_t>(h) * (w * 3 + 1));
    for (int y = h - 1; y >= 0; y--) { // OpenGL legge dal basso
        raw.push_back(0);
        raw.insert(raw.end(), rgb.begin() + static_cast<size_t>(y) * w * 3, rgb.begin() + static_cast<size_t>(y + 1) * w * 3);
    }
    std::vector<uint8_t> z = {0x78, 0x01};
    size_t pos = 0;
    while (pos < raw.size()) {
        size_t n = std::min<size_t>(65535, raw.size() - pos);
        z.push_back(pos + n == raw.size() ? 1 : 0);
        z.push_back(static_cast<uint8_t>(n));
        z.push_back(static_cast<uint8_t>(n >> 8));
        z.push_back(static_cast<uint8_t>(~n));
        z.push_back(static_cast<uint8_t>((~n) >> 8));
        z.insert(z.end(), raw.begin() + pos, raw.begin() + pos + n);
        pos += n;
    }
    uint32_t a = 1, b = 0;
    for (uint8_t c : raw) {
        a = (a + c) % 65521;
        b = (b + a) % 65521;
    }
    put32(z, (b << 16) | a);
    std::vector<uint8_t> out = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    std::vector<uint8_t> ihdr;
    put32(ihdr, static_cast<uint32_t>(w));
    put32(ihdr, static_cast<uint32_t>(h));
    ihdr.insert(ihdr.end(), {8, 2, 0, 0, 0});
    chunk(out, "IHDR", ihdr);
    chunk(out, "IDAT", z);
    chunk(out, "IEND", {});
    return out;
}

void copyToClipboard(const std::vector<uint8_t>& rgb, int w, int h) {
    int stride = (w * 3 + 3) & ~3;
    size_t size = sizeof(BITMAPINFOHEADER) + static_cast<size_t>(stride) * h;
    HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, size);
    if (!mem) return;
    auto* p = static_cast<uint8_t*>(GlobalLock(mem));
    auto* bih = reinterpret_cast<BITMAPINFOHEADER*>(p);
    *bih = {};
    bih->biSize = sizeof(BITMAPINFOHEADER);
    bih->biWidth = w;
    bih->biHeight = h; // bottom-up come OpenGL
    bih->biPlanes = 1;
    bih->biBitCount = 24;
    bih->biCompression = BI_RGB;
    uint8_t* px = p + sizeof(BITMAPINFOHEADER);
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            const uint8_t* s = &rgb[(static_cast<size_t>(y) * w + x) * 3];
            uint8_t* d = &px[static_cast<size_t>(y) * stride + x * 3];
            d[0] = s[2];
            d[1] = s[1];
            d[2] = s[0];
        }
    }
    GlobalUnlock(mem);
    if (OpenClipboard(gui::window())) {
        EmptyClipboard();
        if (!SetClipboardData(CF_DIB, mem)) GlobalFree(mem);
        CloseClipboard();
    } else {
        GlobalFree(mem);
    }
}

} // namespace

// lo screenshot viene preso prima di disegnare il menu
MH_EVERY_FRAME(screenshot) {
    if (!s_shotRequested) return;
    s_shotRequested = false;
    HWND hwnd = gui::window();
    if (!hwnd) return;
    RECT r;
    GetClientRect(hwnd, &r);
    int w = r.right - r.left, h = r.bottom - r.top;
    if (w <= 0 || h <= 0) return;
    std::vector<uint8_t> rgb(static_cast<size_t>(w) * h * 3);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, rgb.data());
    if (s_shotMode == 0 || s_shotMode == 1) copyToClipboard(rgb, w, h);
    if (s_shotMode == 0 || s_shotMode == 2) {
        char name[64];
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
        localtime_s(&tmv, &t);
        strftime(name, sizeof(name), "screenshot_%Y%m%d_%H%M%S.png", &tmv);
        auto png = encodePng(rgb, w, h);
        std::ofstream f(paths::dataDir() / "screenshots" / name, std::ios::binary);
        f.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
        MH_LOG("screenshot salvato: %s", name);
    }
}
