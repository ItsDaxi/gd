// Finestra UNIVERSAL
#include "common.hpp"

#include <windows.h>

using namespace mh;

namespace {

Hack& H(const char* id) { return hack("UNIVERSAL", id); }

float s_bgColor[4] = {0.25f, 0.1f, 0.45f, 1.f};
Hack& customBg = H("UNIVERSAL/CUSTOM_BG/HACK_CUSTOM_BG").optColor("NX/COLOUR", s_bgColor)
    .partial("Colora lo sfondo dei menu (sprite di sfondo della scena)");
Hack& fastChests = H("UNIVERSAL/HACK_FAST_CHESTS");
Hack& lockCursor = H("UNIVERSAL/HACK_LOCK_CURSOR");
Hack& mainMenuPlay = H("UNIVERSAL/HACK_MAIN_MENU_PLAY");
Hack& noMusicFade = H("UNIVERSAL/HACK_NO_AUDIO_FADE_OUT");
Hack& noTransition = H("UNIVERSAL/HACK_NO_TRANSITION");

float s_semitones = 0.f;
Hack& pitchShifter = H("UNIVERSAL/PITCH_SHIFTER/HACK_PITCH_SHIFTER")
    .optFloat("UNIVERSAL/PITCH_SHIFTER/DROPDOWN/PITCH", &s_semitones, -12.f, 12.f, "%.1f st");

int s_priority = 2;
void applyPriority();
Hack& threadPriority = H("UNIVERSAL/THREAD_PRIORITY/HACK_THREAD_PRIORITY")
    .optCombo("UNIVERSAL/THREAD_PRIORITY/COMBO", &s_priority,
              {"UNIVERSAL/THREAD_PRIORITY/COMBO/0", "UNIVERSAL/THREAD_PRIORITY/COMBO/1", "UNIVERSAL/THREAD_PRIORITY/COMBO/2",
               "UNIVERSAL/THREAD_PRIORITY/COMBO/3", "UNIVERSAL/THREAD_PRIORITY/COMBO/4"})
    .toggled([](bool) { applyPriority(); });

int s_transType = 0;
float s_transDuration = 0.5f;
std::vector<std::string> transitionItems() {
    std::vector<std::string> v;
    for (int i = 0; i <= 30; i++) v.push_back("UNIVERSAL/TRANSITION_CUSTOMISER/DROPDOWN/COMBO/" + std::to_string(i));
    return v;
}
Hack& transitionCustomiser = H("UNIVERSAL/TRANSITION_CUSTOMISER/HACK_TRANSITION_CUSTOMISER")
    .optCombo("UNIVERSAL/TRANSITION_CUSTOMISER/DROPDOWN/COMBO", &s_transType, transitionItems())
    .optFloat("UNIVERSAL/TRANSITION_CUSTOMISER/DROPDOWN/DURATION", &s_transDuration, 0.f, 5.f, "%.2fs");

DWORD s_mainThread = GetCurrentThreadId(); // inizializzazione statica: thread del loader (rimpiazzato al primo frame)

void applyPriority() {
    static const int map[] = {THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL,
                              THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST};
    HANDLE t = OpenThread(THREAD_SET_INFORMATION, FALSE, s_mainThread);
    if (!t) return;
    SetThreadPriority(t, threadPriority.enabled ? map[std::clamp(s_priority, 0, 4)] : THREAD_PRIORITY_NORMAL);
    CloseHandle(t);
}

// --- Pitch shifter (DSP FMOD sul gruppo della musica)
void* s_pitchDsp = nullptr;
void* s_pitchGroup = nullptr;
float s_appliedPitch = 0.f;

template <class R, class... A>
R fmodCall(const char* sym, A... a) {
    static HMODULE m = GetModuleHandleW(L"fmod.dll");
    if (!m) m = GetModuleHandleW(L"fmod.dll");
    auto fn = m ? reinterpret_cast<R (*)(A...)>(GetProcAddress(m, sym)) : nullptr;
    return fn ? fn(a...) : R();
}

void updatePitchShifter() {
    auto f = gd::fmod();
    if (!f) return;
    void* sys = GDF(f, FMODAudioEngine, m_system, void*);
    void* group = GDF(f, FMODAudioEngine, m_backgroundMusicChannel, void*);
    bool want = pitchShifter.enabled && std::abs(s_semitones) > 0.01f;
    if (!want) {
        if (s_pitchDsp && s_pitchGroup) {
            fmodCall<int>("?removeDSP@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@PEAVDSP@2@@Z", s_pitchGroup, s_pitchDsp);
            fmodCall<int>("?release@DSP@FMOD@@QEAA?AW4FMOD_RESULT@@XZ", s_pitchDsp);
        }
        s_pitchDsp = nullptr;
        s_pitchGroup = nullptr;
        return;
    }
    if (!sys || !group) return;
    if (!s_pitchDsp) {
        constexpr int FMOD_DSP_TYPE_PITCHSHIFT = 13;
        fmodCall<int>("?createDSPByType@System@FMOD@@QEAA?AW4FMOD_RESULT@@W4FMOD_DSP_TYPE@@PEAPEAVDSP@2@@Z", sys,
                      FMOD_DSP_TYPE_PITCHSHIFT, &s_pitchDsp);
        if (!s_pitchDsp) return;
        fmodCall<int>("?addDSP@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@HPEAVDSP@2@@Z", group, 0, s_pitchDsp);
        s_pitchGroup = group;
        s_appliedPitch = -999.f;
    }
    if (s_appliedPitch != s_semitones) {
        s_appliedPitch = s_semitones;
        float ratio = std::pow(2.f, s_semitones / 12.f);
        fmodCall<int>("?setParameterFloat@DSP@FMOD@@QEAA?AW4FMOD_RESULT@@HM@Z", s_pitchDsp, 0, ratio);
    }
}

// --- No Transition / Transition Customiser
} // namespace

MH_ON(ev::transition, [](float& duration, int& type) {
    if (transitionCustomiser.enabled) {
        type = s_transType;
        duration = s_transDuration;
    }
    if (noTransition.enabled) {
        duration = 0.f;
        type = 0;
    }
});

MH_ON(ev::musicFade, [](bool& allow) {
    if (noMusicFade.enabled) allow = false;
});

MH_ON(ev::fmodUpdate, [](void*) { updatePitchShifter(); });

MH_EVERY_FRAME(universal) {
    static bool first = true;
    if (first) {
        first = false;
        s_mainThread = GetCurrentThreadId();
        applyPriority();
    }
    // Lock Cursor: confina il mouse nella finestra mentre si gioca/si edita
    static bool clipped = false;
    HWND hwnd = gui::window();
    bool want = lockCursor.enabled && hwnd && GetForegroundWindow() == hwnd && (gd::playLayer() || gd::editorLayer()) &&
                !gui::menuOpen();
    if (want) {
        RECT r;
        GetClientRect(hwnd, &r);
        MapWindowPoints(hwnd, nullptr, reinterpret_cast<POINT*>(&r), 2);
        ClipCursor(&r);
        clipped = true;
    } else if (clipped) {
        ClipCursor(nullptr);
        clipped = false;
    }
}

// --- Fast Chests: accelera il gioco mentre e' aperta l'animazione di una cassa
namespace {
bool s_chestOpen = false;
bool (*o_rewardInit)(void*, int, void*);
bool h_rewardInit(void* self, int type, void* page) {
    bool r = o_rewardInit(self, type, page);
    if (r && fastChests.enabled) s_chestOpen = true;
    return r;
}
void (*o_rewardStep3)(void*);
void h_rewardStep3(void* self) {
    o_rewardStep3(self);
    s_chestOpen = false;
}
} // namespace
MH_ON(ev::schedulerUpdate, [](float& dt) {
    if (s_chestOpen && fastChests.enabled) dt *= 4.f;
});

// --- Main Menu Play: controlla il cubo del menu principale
MH_ON(ev::menuGameUpdate, [](void* mgl, float) {
    if (!mainMenuPlay.enabled) return;
    auto p = GDF(mgl, MenuGameLayer, m_playerObject, void*);
    if (!p) return;
    static bool down = false;
    bool now = (GetAsyncKeyState(VK_SPACE) & 0x8000) || (GetAsyncKeyState(VK_UP) & 0x8000);
    if (gui::menuOpen()) now = false;
    if (now != down) {
        down = now;
        gd::call<bool>(now ? gd::addr::PlayerObject::pushButton : gd::addr::PlayerObject::releaseButton, p, 1);
    }
});

// --- Custom Background: colora lo sprite di sfondo dei menu.
// Prima si prendeva il primo figlio del layer senza controllarne il tipo: nel menu principale e' un
// MenuGameLayer (non uno sprite) e setColor scriveva nel posto sbagliato (crash). Ora si controlla
// il tipo con l'RTTI e nel menu principale si usa MenuGameLayer::m_backgroundSprite.
void* findBackground(void* layer) {
    unsigned n = cc::arrayCount(cc::children(layer));
    for (unsigned i = 0; i < n && i < 4; i++) {
        void* c = cc::arrayAt(cc::children(layer), i);
        if (cc::isKindOf(c, ".?AVMenuGameLayer@@")) return GDF(c, MenuGameLayer, m_backgroundSprite, void*);
        if (cc::isKindOf(c, ".?AVCCSprite@cocos2d@@")) return c;
    }
    return nullptr;
}

MH_EVERY_FRAME(customBg) {
    if (!customBg.enabled || gd::gameLayer()) return;
    void* scene = cc::runningScene();
    if (!scene || cc::isKindOf(scene, ".?AVCCTransitionScene@cocos2d@@")) return;
    void* layer = cc::arrayAt(cc::children(scene), 0);
    if (!layer) return;
    void* bg = findBackground(layer);
    if (!bg || !cc::isKindOf(bg, ".?AVCCSprite@cocos2d@@")) return;
    cc::Color3B c{static_cast<uint8_t>(s_bgColor[0] * 255), static_cast<uint8_t>(s_bgColor[1] * 255),
                  static_cast<uint8_t>(s_bgColor[2] * 255)};
    cc::setColor(bg, c);
}

MH_INSTALL(universal) {
    using namespace gd;
    hooks::gd("RewardUnlockLayer::init", addr::RewardUnlockLayer::init, h_rewardInit, &o_rewardInit);
    hooks::gd("RewardUnlockLayer::step3", addr::RewardUnlockLayer::step3, h_rewardStep3, &o_rewardStep3);
}
