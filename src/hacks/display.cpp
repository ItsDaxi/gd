// Finestre DISPLAY e SPEEDHACK
#include "common.hpp"

#include <windows.h>

#include <mutex>

using namespace mh;

namespace {

// ================================================================== DISPLAY
float s_fps = 240.f;
void applyFps();
Hack& unlockFps = hack("DISPLAY", "DISPLAY/FPS/HACK_FPS")
    .optFloat("DISPLAY/FPS/SPINNER", &s_fps, 10.f, 1000.f, "%.0f FPS")
    .toggled([](bool) { applyFps(); });
Hack& vsync = hack("DISPLAY", "DISPLAY/HACK_VSYNC").toggled([](bool on) {
    using Fn = BOOL(WINAPI*)(int);
    auto fn = reinterpret_cast<Fn>(wglGetProcAddress("wglSwapIntervalEXT"));
    if (fn) fn(on ? 1 : 0);
    if (auto gm = gd::gameManager()) GDF(gm, GameManager, m_vsyncEnabled, bool) = on;
});
float s_tps = 240.f;
Hack& physicsBypass = hack("DISPLAY", "DISPLAY/PHYSICS_BYPASS/HACK_PHYSICS_BYPASS").isCheat()
    .optFloat("DISPLAY/PHYSICS_BYPASS/SPINNER", &s_tps, 60.f, 2000.f, "%.0f Hz")
    .partial("Non disponibile: in GD 2.2 i 240 tick/s sono una costante dentro GJBaseGameLayer::update");
Hack& lockDelta = hack("DISPLAY", "DISPLAY/DELTA/HACK_LOCK_DELTA").isCheat();
Hack& realTime = hack("DISPLAY", "DISPLAY/DELTA/HACK_REAL_TIME").partial("Approssimato: il delta bloccato viene moltiplicato quando il gioco resta indietro");
Hack& frameExtrapolation = hack("DISPLAY", "DISPLAY/HACK_FRAME_EXTRAPOLATION").partial("Non disponibile in questa versione");

LONG_PTR s_oldStyle = 0;
RECT s_oldRect{};
bool s_borderless = false;
void setBorderless(bool on) {
    HWND hwnd = gui::window();
    if (!hwnd || on == s_borderless) return;
    if (on) {
        s_oldStyle = GetWindowLongPtrW(hwnd, GWL_STYLE);
        GetWindowRect(hwnd, &s_oldRect);
        MONITORINFO mi{sizeof(mi)};
        GetMonitorInfoW(MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST), &mi);
        SetWindowLongPtrW(hwnd, GWL_STYLE, (s_oldStyle & ~WS_OVERLAPPEDWINDOW) | WS_POPUP);
        SetWindowPos(hwnd, HWND_TOP, mi.rcMonitor.left, mi.rcMonitor.top, mi.rcMonitor.right - mi.rcMonitor.left,
                     mi.rcMonitor.bottom - mi.rcMonitor.top, SWP_FRAMECHANGED | SWP_SHOWWINDOW);
    } else {
        SetWindowLongPtrW(hwnd, GWL_STYLE, s_oldStyle);
        SetWindowPos(hwnd, nullptr, s_oldRect.left, s_oldRect.top, s_oldRect.right - s_oldRect.left,
                     s_oldRect.bottom - s_oldRect.top, SWP_FRAMECHANGED | SWP_NOZORDER | SWP_SHOWWINDOW);
    }
    s_borderless = on;
}
Hack& borderlessClassic = hack("DISPLAY", "DISPLAY/HACK_BORDERLESS_CLASSIC").toggled([](bool on) { setBorderless(on); });
int s_fsMode = 1;
Hack& fullscreen = hack("DISPLAY", "DISPLAY/FULLSCREEN/HACK_FULLSCREEN")
    .optCombo("DISPLAY/FULLSCREEN/HACK_FULLSCREEN", &s_fsMode, {"DISPLAY/FULLSCREEN/COMBO/0", "DISPLAY/FULLSCREEN/COMBO/1"})
    .toggled([](bool on) { setBorderless(on || borderlessClassic.enabled); })
    .partial("La modalita' esclusiva usa la finestra senza bordi");

void applyFps() {
    double fps = s_fps;
    if (!unlockFps.enabled) {
        auto gm = gd::gameManager();
        float custom = gm ? GDF(gm, GameManager, m_customFPSTarget, float) : 0.f;
        fps = custom > 0.f ? custom : 60.0;
    }
    cc::setAnimationInterval(1.0 / std::max(1.0, fps));
}

// ================================================================== SPEEDHACK
float s_speed = 1.f;
Hack& speedhack = hack("SPEEDHACK", "SPEEDHACK/LAYOUT/HACK_SPEEDHACK").isCheat()
    .optFloat("SPEEDHACK/LAYOUT/SPINNER", &s_speed, 0.01f, 10.f, "%.2fx");
Hack& speedhackAudio = hack("SPEEDHACK", "SPEEDHACK/HACK_SPEEDHACK_AUDIO");
Hack& speedhackClassic = hack("SPEEDHACK", "SPEEDHACK/HACK_SPEEDHACK_CLASSIC").isCheat();

float currentSpeed() { return speedhack.enabled ? s_speed : 1.f; }

// --- modalita' classica: rallenta l'orologio di sistema (QueryPerformanceCounter)
std::mutex s_clockMutex;
LARGE_INTEGER s_realBase{}, s_fakeBase{};
double s_clockSpeed = 1.0;
BOOL(WINAPI* o_QPC)(LARGE_INTEGER*) = nullptr;
BOOL WINAPI h_QPC(LARGE_INTEGER* out) {
    LARGE_INTEGER real;
    BOOL r = o_QPC(&real);
    if (!r || !out) return r;
    std::lock_guard lock(s_clockMutex);
    double want = (speedhackClassic.enabled && speedhack.enabled) ? s_speed : 1.0;
    if (s_realBase.QuadPart == 0) {
        s_realBase = real;
        s_fakeBase = real;
    }
    if (want != s_clockSpeed) {
        // ribasa per non avere salti di tempo
        s_fakeBase.QuadPart += static_cast<LONGLONG>((real.QuadPart - s_realBase.QuadPart) * s_clockSpeed);
        s_realBase = real;
        s_clockSpeed = want;
    }
    out->QuadPart = s_fakeBase.QuadPart + static_cast<LONGLONG>((real.QuadPart - s_realBase.QuadPart) * s_clockSpeed);
    return TRUE;
}

} // namespace

namespace mh::speed {
float current() { return currentSpeed(); }
} // namespace mh::speed

MH_ON(ev::schedulerUpdate, [](float& dt) {
    if (lockDelta.enabled && gd::gameLayer()) {
        float locked = 1.f / std::max(1.f, s_fps);
        if (realTime.enabled && dt > locked * 1.5f) locked *= std::round(dt / locked);
        dt = locked;
    }
    if (speedhack.enabled && !speedhackClassic.enabled) dt *= s_speed;
});

MH_ON(ev::fmodUpdate, [](void* f) {
    static float applied = 1.f;
    float want = (speedhack.enabled && speedhackAudio.enabled) ? s_speed : 1.f;
    if (want == applied) return;
    void* sys = GDF(f, FMODAudioEngine, m_system, void*);
    if (!sys) return;
    static HMODULE m = GetModuleHandleW(L"fmod.dll");
    if (!m) return;
    using GetMaster = int (*)(void*, void**);
    using SetPitch = int (*)(void*, float);
    auto getMaster = reinterpret_cast<GetMaster>(GetProcAddress(m, "?getMasterChannelGroup@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEAPEAVChannelGroup@2@@Z"));
    auto setPitch = reinterpret_cast<SetPitch>(GetProcAddress(m, "?setPitch@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z"));
    if (!getMaster || !setPitch) return;
    void* master = nullptr;
    if (getMaster(sys, &master) == 0 && master) setPitch(master, want);
    applied = want;
});

MH_EVERY_FRAME(display) {
    static double last = 0;
    double now = GetTickCount64() / 1000.0;
    if (unlockFps.enabled && now - last > 2.0) {
        last = now;
        applyFps();
    }
}

MH_INSTALL(display) {
    hooks::exported("kernel32.dll", "QueryPerformanceCounter", h_QPC, &o_QPC);
}
