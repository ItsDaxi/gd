// Finestre DISPLAY e SPEEDHACK
#include "common.hpp"

#include <windows.h>

#include <cstring>
#include <mutex>

#include "core/scan.hpp"

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
// Physics TPS: GD 2.2 usa la costante 240 (e 1/240) dentro GJBaseGameLayer::update/getModifiedDelta.
// Senza l'eseguibile si cercano a runtime le istruzioni che leggono quelle costanti e si fanno
// puntare a una copia modificabile (allocata vicino a GeometryDash.exe).
float s_tps = 240.f;
void applyTps(bool on);
Hack& physicsBypass = hack("DISPLAY", "DISPLAY/PHYSICS_BYPASS/HACK_PHYSICS_BYPASS").isCheat()
    .optFloat("DISPLAY/PHYSICS_BYPASS/SPINNER", &s_tps, 60.f, 2000.f, "%.0f Hz")
    .toggled([](bool on) { applyTps(on); })
    .partial("Sperimentale: reindirizza le costanti 240 / 1/240 trovate in GJBaseGameLayer::update e getModifiedDelta. "
             "Non replica la fisica della 2.1");

struct TpsRef {
    scan::RipRef ref;
    int kind; // 0 = double 240, 1 = double 1/240, 2 = float 240, 3 = float 1/240
};
std::vector<TpsRef> s_tpsRefs;
double* s_tpsConst = nullptr; // [0] tps, [1] 1/tps, poi due float
bool s_tpsScanned = false, s_tpsActive = false;

void updateTpsConstants() {
    if (!s_tpsConst) return;
    double tps = std::clamp<double>(s_tps, 1.0, 10000.0);
    s_tpsConst[0] = tps;
    s_tpsConst[1] = 1.0 / tps;
    auto f = reinterpret_cast<float*>(s_tpsConst + 2);
    f[0] = static_cast<float>(tps);
    f[1] = static_cast<float>(1.0 / tps);
}

void scanTps() {
    s_tpsScanned = true;
    const uintptr_t funcs[] = {gd::addr::GJBaseGameLayer::update, gd::addr::GJBaseGameLayer::getModifiedDelta};
    for (uintptr_t rva : funcs) {
        if (!rva) continue;
        for (auto& r : scan::sseRipRefs(mem::base() + rva)) {
            if (!mem::readable(reinterpret_cast<void*>(r.target), 8)) continue;
            double d;
            float fl;
            std::memcpy(&d, reinterpret_cast<void*>(r.target), 8);
            std::memcpy(&fl, reinterpret_cast<void*>(r.target), 4);
            int kind = -1;
            if (d == 240.0) kind = 0;
            else if (std::abs(d - 1.0 / 240.0) < 1e-15) kind = 1;
            else if (fl == 240.f) kind = 2;
            else if (std::abs(fl - 1.f / 240.f) < 1e-9f) kind = 3;
            if (kind >= 0) s_tpsRefs.push_back({r, kind});
        }
    }
    MH_LOG("Physics TPS: %zu riferimenti a 240/(1/240) trovati", s_tpsRefs.size());
    if (!s_tpsRefs.empty()) s_tpsConst = static_cast<double*>(scan::allocNear(mem::base(), 64));
}

void applyTps(bool on) {
    if (on && !s_tpsScanned) scanTps();
    if (s_tpsRefs.empty() || !s_tpsConst) return;
    updateTpsConstants();
    if (on == s_tpsActive) return;
    for (auto& t : s_tpsRefs) {
        if (!on) {
            scan::restore(t.ref);
            continue;
        }
        uintptr_t target = t.kind == 0   ? reinterpret_cast<uintptr_t>(&s_tpsConst[0])
                           : t.kind == 1 ? reinterpret_cast<uintptr_t>(&s_tpsConst[1])
                           : t.kind == 2 ? reinterpret_cast<uintptr_t>(s_tpsConst + 2)
                                         : reinterpret_cast<uintptr_t>(reinterpret_cast<float*>(s_tpsConst + 2) + 1);
        scan::redirect(t.ref, target);
    }
    s_tpsActive = on;
}
Hack& lockDelta = hack("DISPLAY", "DISPLAY/DELTA/HACK_LOCK_DELTA").isCheat();
Hack& realTime = hack("DISPLAY", "DISPLAY/DELTA/HACK_REAL_TIME").partial("Approssimato: il delta bloccato viene moltiplicato quando il gioco resta indietro");
Hack& frameExtrapolation = hack("DISPLAY", "DISPLAY/HACK_FRAME_EXTRAPOLATION").partial("Non disponibile: richiede di interpolare la posizione di tutti gli oggetti tra due tick");

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
    if (s_tpsActive) updateTpsConstants();
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
