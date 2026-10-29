// Finestra LEVEL
#include "common.hpp"
#include "core/paths.hpp"

#ifdef _MSC_VER
#include <intrin.h>
#endif

#include <random>
#include <unordered_map>
#include <windows.h>

using namespace mh;

bool mh::g_forceDeath = false;

void mh::killPlayer(void* pl, void* player) {
    if (!pl || !player) return;
    g_forceDeath = true;
    gd::call<void>(gd::addr::PlayLayer::destroyPlayer, pl, player, static_cast<void*>(nullptr));
    g_forceDeath = false;
}

namespace {

// ------------------------------------------------------------------ Noclip
bool s_noclipP1 = true, s_noclipP2 = true, s_noclipTint = true, s_noclipSafe = true;
float s_tintColor[4] = {1.f, 0.f, 0.f, 0.35f};
Hack& noclip = hack("LEVEL", "LEVEL/NOCLIP/HACK_NOCLIP").isCheat()
    .optBool("LEVEL/NOCLIP/DROPDOWN/PLAYER_1", &s_noclipP1)
    .optBool("LEVEL/NOCLIP/DROPDOWN/PLAYER_2", &s_noclipP2)
    .optBool("LEVEL/NOCLIP/DROPDOWN/TINT/CHECKBOX", &s_noclipTint)
    .optColor("NX/COLOUR", s_tintColor)
    .optBool("LEVEL/NOCLIP/DROPDOWN/SAFE_MODE", &s_noclipSafe);

bool s_accOn = false, s_deathOn = false;
float s_accLimit = 90.f;
int s_deathLimit = 10;
Hack& noclipLimits = hack("LEVEL", "LEVEL/NOCLIP_LIMITS/HACK_NOCLIP_LIMITS")
    .optBool("LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT_ON", &s_accOn)
    .optFloat("LEVEL/NOCLIP_LIMITS/DROPDOWN/ACCURACY_LIMIT", &s_accLimit, 0, 100, "%.1f%%")
    .optBool("LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATH_LIMIT_ON", &s_deathOn)
    .optInt("LEVEL/NOCLIP_LIMITS/DROPDOWN/DEATHS_LIMIT", &s_deathLimit, 1, 1000);

float s_tintAlpha = 0.f;
bool s_diedThisFrame = false; // contatto con un ostacolo in questo frame
bool s_diedLastFrame = false; // ... e nel frame precedente

} // namespace

// statistiche noclip lette dallo Status
namespace mh::stats {
int noclipDeaths = 0;
int totalFrames = 0;
int deadFrames = 0;
float noclipAccuracy() { return totalFrames ? 100.f * static_cast<float>(totalFrames - deadFrames) / static_cast<float>(totalFrames) : 100.f; }
} // namespace mh::stats

MH_ON(ev::plDestroyPlayer, [](void* pl, void* player, void*, bool& allow) {
    if (g_forceDeath || !noclip.enabled) return;
    bool p1 = player == gd::player1(pl);
    if ((p1 && s_noclipP1) || (!p1 && s_noclipP2)) allow = false;
});

MH_ON(ev::plNoclipDeath, [](void*, void*) {
    // GD chiama destroyPlayer ad ogni tick passato dentro l'ostacolo: si conta una morte
    // solo quando il giocatore ci entra (nessun contatto nel frame precedente)
    if (!s_diedThisFrame && !s_diedLastFrame) stats::noclipDeaths++;
    s_diedThisFrame = true;
    if (s_noclipTint) s_tintAlpha = 1.f;
});

MH_ON(ev::plReset, [](void*) {
    s_diedThisFrame = s_diedLastFrame = false;
    stats::noclipDeaths = 0;
    stats::totalFrames = 0;
    stats::deadFrames = 0;
});

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    stats::totalFrames++;
    if (s_diedThisFrame) stats::deadFrames++;
    s_diedLastFrame = s_diedThisFrame;
    s_diedThisFrame = false;
    if (noclip.enabled && noclipLimits.enabled) {
        bool kill = (s_accOn && stats::noclipAccuracy() < s_accLimit) || (s_deathOn && stats::noclipDeaths >= s_deathLimit);
        if (kill) killPlayer(pl, gd::player1(pl));
    }
});

static gui::Overlay s_tintOverlay([] {
    if (s_tintAlpha <= 0.f || !gd::playLayer()) {
        s_tintAlpha = 0.f;
        return;
    }
    auto* dl = ImGui::GetBackgroundDrawList();
    auto sz = ImGui::GetIO().DisplaySize;
    ImVec4 c(s_tintColor[0], s_tintColor[1], s_tintColor[2], s_tintColor[3] * s_tintAlpha);
    dl->AddRectFilled(ImVec2(0, 0), sz, ImGui::ColorConvertFloat4ToU32(c));
    s_tintAlpha -= ImGui::GetIO().DeltaTime * 4.f;
});

// ------------------------------------------------------------------ Auto Kill
namespace {
float s_killPercent = 50.f, s_killTime = 60.f;
Hack& autoKill = hack("LEVEL", "LEVEL/AUTO_KILL/HACK_AUTO_KILL")
    .optFloat("LEVEL/AUTO_KILL/DROPDOWN/CLASSIC", &s_killPercent, 0, 100, "%.1f%%")
    .optFloat("LEVEL/AUTO_KILL/DROPDOWN/PLATFORMER", &s_killTime, 0, 3600, "%.1fs");
} // namespace

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!autoKill.enabled) return;
    auto p1 = gd::player1(pl);
    if (!p1 || GDF(p1, PlayerObject, m_isDead, bool)) return;
    bool plat = GDF(pl, GJBaseGameLayer, m_isPlatformer, bool);
    if ((!plat && gd::percent() >= s_killPercent) || (plat && gd::levelTime() >= s_killTime)) killPlayer(pl, p1);
});

// ------------------------------------------------------------------ Auto Clicker
namespace {
int s_acHold = 1, s_acRelease = 1, s_acOffset = 0, s_acMode = 0;
Hack& autoClicker = hack("LEVEL", "LEVEL/AUTO_CLICKER/HACK_AUTO_CLICKER").isCheat()
    .optInt("LEVEL/AUTO_CLICKER/DROPDOWN/HOLD", &s_acHold, 1, 1000, "%d frames")
    .optInt("LEVEL/AUTO_CLICKER/DROPDOWN/RELEASE", &s_acRelease, 1, 1000, "%d frames")
    .optInt("LEVEL/AUTO_CLICKER/DROPDOWN/OFFSET", &s_acOffset, 0, 1000, "%d frames")
    .optCombo("LEVEL/AUTO_CLICKER/DROPDOWN/MODE", &s_acMode,
              {"LEVEL/AUTO_CLICKER/DROPDOWN/MODE/0", "LEVEL/AUTO_CLICKER/DROPDOWN/MODE/1", "LEVEL/AUTO_CLICKER/DROPDOWN/MODE/2"});
int s_acFrame = 0;
bool s_acDown = false;
} // namespace

MH_ON(ev::plReset, [](void*) {
    s_acFrame = 0;
    s_acDown = false;
});

MH_ON(ev::bglPostUpdate, [](void* layer, float) {
    if (!autoClicker.enabled || layer != gd::playLayer()) return;
    int f = s_acFrame++ - s_acOffset;
    if (f < 0) return;
    int period = s_acHold + s_acRelease;
    bool down = (f % period) < s_acHold;
    if (down == s_acDown) return;
    s_acDown = down;
    if (s_acMode == 0 || s_acMode == 1) gd::call<void>(gd::addr::GJBaseGameLayer::handleButton, layer, down, 1, true);
    if (s_acMode == 0 || s_acMode == 2) gd::call<void>(gd::addr::GJBaseGameLayer::handleButton, layer, down, 1, false);
});

// ------------------------------------------------------------------ Auto Deafen
namespace {
float s_deafAt = 50.f, s_undeafAt = 100.f;
bool s_deafPractice = false, s_deafStartPos = false, s_undeafPause = true;
int s_deafKey = 0;
bool s_deafened = false;
Hack& autoDeafen = hack("LEVEL", "LEVEL/AUTO_DEAFEN/HACK_AUTO_DEAFEN")
    .optFloat("LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_AT", &s_deafAt, 0, 100, "%.1f%%")
    .optFloat("LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_AT", &s_undeafAt, 0, 100, "%.1f%%")
    .optBool("LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_IN_PRACTICE", &s_deafPractice)
    .optBool("LEVEL/AUTO_DEAFEN/DROPDOWN/DEAFEN_WITH_STARTPOS", &s_deafStartPos)
    .optBool("LEVEL/AUTO_DEAFEN/DROPDOWN/UNDEAFEN_ON_PAUSE", &s_undeafPause)
    .optKey("LEVEL/AUTO_DEAFEN/DROPDOWN/SHORTCUT", &s_deafKey)
    .partial("Premi la scorciatoia di Discord (Ctrl+Shift + tasto scelto) simulando la tastiera");

void sendDeafenShortcut() {
    if (!s_deafKey) return;
    INPUT in[6]{};
    WORD keys[3] = {VK_CONTROL, VK_SHIFT, static_cast<WORD>(s_deafKey)};
    for (int i = 0; i < 3; i++) {
        in[i].type = INPUT_KEYBOARD;
        in[i].ki.wVk = keys[i];
        in[5 - i].type = INPUT_KEYBOARD;
        in[5 - i].ki.wVk = keys[i];
        in[5 - i].ki.dwFlags = KEYEVENTF_KEYUP;
    }
    SendInput(6, in, sizeof(INPUT));
    s_deafened = !s_deafened;
}
} // namespace

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!autoDeafen.enabled) return;
    if (gd::isPractice() && !s_deafPractice) return;
    if (GDF(pl, GJBaseGameLayer, m_startPosObject, void*) && !s_deafStartPos) return;
    float p = gd::percent();
    if (!s_deafened && p >= s_deafAt && p < s_undeafAt) sendDeafenShortcut();
    else if (s_deafened && p >= s_undeafAt) sendDeafenShortcut();
});
MH_ON(ev::plReset, [](void*) {
    if (s_deafened) sendDeafenShortcut();
});
MH_ON(ev::plQuit, [](void*) {
    if (s_deafened) sendDeafenShortcut();
});
MH_ON(ev::plPause, [](void*, bool&) {
    if (s_deafened && s_undeafPause) sendDeafenShortcut();
});

// ------------------------------------------------------------------ Auto Music Sync
namespace {
float s_syncThreshold = 80.f;
bool s_noLegacySync = false;
Hack& autoMusicSync = hack("LEVEL", "LEVEL/AUTO_MUSIC_SYNC/HACK_AUTO_MUSIC_SYNC")
    .optFloat("LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/THRESHOLD", &s_syncThreshold, 10, 1000, "%.0f ms")
    .optBool("LEVEL/AUTO_MUSIC_SYNC/DROPDOWN/NO_LEGACY_SYNC", &s_noLegacySync)
    .partial("Solo livelli lineari con una canzone; usa il tempo del livello come riferimento");
double s_musicStart = -1;
} // namespace

MH_ON(ev::plReset, [](void* pl) {
    auto f = gd::fmod();
    s_musicStart = -1;
    if (!f || !autoMusicSync.enabled) return;
    double t = gd::call<unsigned>(gd::addr::FMODAudioEngine::getMusicTimeMS, f, 0) / 1000.0;
    s_musicStart = t - gd::levelTime();
    (void)pl;
});

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!autoMusicSync.enabled || s_musicStart < 0 || gd::isPractice()) return;
    auto f = gd::fmod();
    auto p1 = gd::player1(pl);
    if (!f || !p1 || GDF(p1, PlayerObject, m_isDead, bool)) return;
    double expected = s_musicStart + gd::levelTime();
    double actual = gd::call<unsigned>(gd::addr::FMODAudioEngine::getMusicTimeMS, f, 0) / 1000.0;
    if (std::abs(expected - actual) * 1000.0 > s_syncThreshold)
        gd::call<void>(gd::addr::FMODAudioEngine::setMusicTimeMS, f, static_cast<unsigned>(expected * 1000.0), true, 0);
});

// ------------------------------------------------------------------ Auto Pickup Coins
namespace {
bool s_coin[3] = {true, true, true};
Hack& autoCoins = hack("LEVEL", "LEVEL/AUTO_PICKUP_COINS/HACK_AUTO_PICKUP_COINS").isCheat()
    .optBool("LEVEL/AUTO_PICKUP_COINS/DROPDOWN/0", &s_coin[0])
    .optBool("LEVEL/AUTO_PICKUP_COINS/DROPDOWN/1", &s_coin[1])
    .optBool("LEVEL/AUTO_PICKUP_COINS/DROPDOWN/2", &s_coin[2]);
bool s_coinsDone = false;
} // namespace

MH_ON(ev::plReset, [](void*) { s_coinsDone = false; });
MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!autoCoins.enabled || s_coinsDone) return;
    s_coinsDone = true;
    auto coins = GDF(pl, PlayLayer, m_coinArray, void*);
    unsigned n = cc::arrayCount(coins);
    for (unsigned i = 0; i < n && i < 3; i++) {
        if (!s_coin[i]) continue;
        auto c = cc::arrayAt(coins, i);
        if (c) gd::call<void>(gd::addr::GJBaseGameLayer::pickupItem, pl, c);
    }
});

// ------------------------------------------------------------------ Frame Stepper
namespace {
int s_fsStep = 'C', s_fsPause = 'V', s_fsBack = 0;
bool s_fsPaused = true;
int s_fsPendingSteps = 0;
Hack& frameStepper = hack("LEVEL", "LEVEL/FRAME_STEPPER/HACK_FRAME_STEPPER").isCheat()
    .optKey("LEVEL/FRAME_STEPPER/DROPDOWN/STEP", &s_fsStep)
    .optKey("LEVEL/FRAME_STEPPER/DROPDOWN/PAUSE", &s_fsPause)
    .optKey("LEVEL/FRAME_STEPPER/DROPDOWN/BACK", &s_fsBack)
    .partial("Il passo indietro non e' disponibile senza salvare lo stato completo del livello");
} // namespace

static gui::KeyHandler s_fsKeys([](int vk, bool down) {
    if (!frameStepper.enabled || !down || !gd::playLayer()) return false;
    if (vk == s_fsStep) {
        s_fsPendingSteps++;
        return true;
    }
    if (vk == s_fsPause) {
        s_fsPaused = !s_fsPaused;
        return true;
    }
    return false;
});

MH_ON(ev::bglPreUpdate, [](void* layer, float& dt, bool& run) {
    if (!frameStepper.enabled || layer != gd::playLayer() || !s_fsPaused) return;
    if (s_fsPendingSteps > 0) {
        s_fsPendingSteps--;
        dt = 1.f / 240.f;
        return;
    }
    run = false;
});

// ------------------------------------------------------------------ Force Ice
namespace {
bool s_disableIce = false;
Hack& forceIce = hack("LEVEL", "LEVEL/FORCE_ICE/HACK_FORCE_ICE").isCheat()
    .optBool("LEVEL/FORCE_ICE/DROPDOWN/DISABLE", &s_disableIce);
} // namespace

MH_ON(ev::plInit, [](void* pl) {
    if (!forceIce.enabled) return;
    forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [](void* o) {
        auto t = objType(o);
        if (isSolid(t) || isSlope(t)) GDF(o, GameObject, m_isIceBlock, bool) = !s_disableIce;
    });
});

// ------------------------------------------------------------------ 0% Practice Complete
namespace {
Hack& zeroPractice = hack("LEVEL", "LEVEL/HACK_0_PRACTICE_COMPLETE").isCheat();
int s_practiceDeaths = 0;
} // namespace
MH_ON(ev::plTogglePractice, [](void*, bool) { s_practiceDeaths = 0; });
MH_ON(ev::plDeath, [](void*, void*) { s_practiceDeaths++; });
MH_ON(ev::plPreComplete, [](void* pl, bool&) {
    if (!zeroPractice.enabled || !GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool) || s_practiceDeaths > 0) return;
    GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool) = false;
});

// ------------------------------------------------------------------ Allow Pause Buffering
namespace {
Hack& pauseBuffer = hack("LEVEL", "LEVEL/HACK_ALLOW_PAUSE_BUFFERING");
}
MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!pauseBuffer.enabled) return;
    gameState<unsigned>(pl, gd::off::GJGameState::m_pauseCounter) = 0;
    gameState<unsigned>(pl, gd::off::GJGameState::m_pauseBufferTimer) = 0;
});

// ------------------------------------------------------------------ Click Between Steps / Click on Steps
namespace {
Hack& cbs = hack("LEVEL", "LEVEL/HACK_CBF_GMVAR_CBS");
Hack& clickOnSteps = hack("LEVEL", "LEVEL/HACK_CBF_GMVAR_COS");
Hack& cbf = hack("LEVEL", "LEVEL/CBF/HACK_CBF")
    .partial("Click Between Frames e' una mod Geode: non disponibile in questa versione standalone. Usa Click Between Steps.");
} // namespace
MH_ON(ev::plInit, [](void* pl) {
    if (cbs.enabled) GDF(pl, GJBaseGameLayer, m_clickBetweenSteps, bool) = true;
    if (clickOnSteps.enabled) GDF(pl, GJBaseGameLayer, m_clickOnSteps, bool) = true;
});

// ------------------------------------------------------------------ Checkpoint Limit Bypass
namespace {
Hack& cpLimit = hack("LEVEL", "LEVEL/HACK_CHECKPOINT_LIMIT_BYPASS");
void (*o_removeCheckpoint)(void*, bool);
void h_removeCheckpoint(void* self, bool first) {
    if (cpLimit.enabled && first) return;
    o_removeCheckpoint(self, first);
}
} // namespace

// ------------------------------------------------------------------ Collect Coins In Practice
namespace {
Hack& practiceCoins = hack("LEVEL", "LEVEL/HACK_COLLECT_COINS_IN_PRACTICE");
void (*o_pickupItem)(void*, void*);
void h_pickupItem(void* self, void* obj) {
    if (g_simulating) return;
    bool& practice = GDF(self, GJBaseGameLayer, m_isPracticeMode, bool);
    if (practiceCoins.enabled && practice) {
        practice = false;
        o_pickupItem(self, obj);
        practice = true;
        return;
    }
    o_pickupItem(self, obj);
}
} // namespace

// ------------------------------------------------------------------ Confirm ...
namespace {
Hack& confirmExit = hack("LEVEL", "LEVEL/HACK_CONFIRM_EXIT");
Hack& confirmFull = hack("LEVEL", "LEVEL/HACK_CONFIRM_FULL_RESET");
Hack& confirmNormal = hack("LEVEL", "LEVEL/HACK_CONFIRM_NORMAL");
Hack& confirmPractice = hack("LEVEL", "LEVEL/HACK_CONFIRM_PRACTICE");
Hack& confirmReset = hack("LEVEL", "LEVEL/HACK_CONFIRM_RESET");

struct PauseCall {
    void (*orig)(void*, void*);
    void* self;
    void* sender;
};
PauseCall s_pending{};

void runPending(void*) {
    if (s_pending.orig) s_pending.orig(s_pending.self, s_pending.sender);
    s_pending = {};
}

#define MH_CONFIRM_HOOK(name, flag, msgKey)                                   \
    void (*o_##name)(void*, void*);                                           \
    void h_##name(void* self, void* sender) {                                 \
        if (!flag.enabled) {                                                  \
            o_##name(self, sender);                                           \
            return;                                                           \
        }                                                                     \
        s_pending = {o_##name, self, sender};                                 \
        confirm(lang::tr(msgKey), runPending, nullptr);                       \
    }

MH_CONFIRM_HOOK(onQuit, confirmExit, "LEVEL/HACK_CONFIRM_EXIT")
MH_CONFIRM_HOOK(onRestartFull, confirmFull, "LEVEL/HACK_CONFIRM_FULL_RESET")
MH_CONFIRM_HOOK(onNormalMode, confirmNormal, "LEVEL/HACK_CONFIRM_NORMAL")
MH_CONFIRM_HOOK(onPracticeMode, confirmPractice, "LEVEL/HACK_CONFIRM_PRACTICE")
MH_CONFIRM_HOOK(onRestart, confirmReset, "LEVEL/HACK_CONFIRM_RESET")
} // namespace

// ------------------------------------------------------------------ Force Platformer / All Modes Platformer
namespace {
Hack& forcePlat = hack("LEVEL", "LEVEL/HACK_FORCE_PLATFORMER").isCheat();
Hack& allModesPlat = hack("LEVEL", "LEVEL/HACK_ALL_MODES_PLATFORMER")
    .partial("In GD 2.2 tutte le modalita' sono gia' disponibili in platformer: nessun effetto");
} // namespace
MH_ON(ev::plInit, [](void* pl) {
    if (!forcePlat.enabled) return;
    GDF(pl, GJBaseGameLayer, m_isPlatformer, bool) = true;
    for (auto p : {gd::player1(pl), gd::player2(pl)})
        if (p) GDF(p, PlayerObject, m_isPlatformer, bool) = true;
});

// ------------------------------------------------------------------ Instant Complete
namespace {
Hack& instantComplete = hack("LEVEL", "LEVEL/HACK_INSTANT_COMPLETE").isCheat();
bool s_icDone = false;
} // namespace
MH_ON(ev::plReset, [](void*) { s_icDone = false; });
MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!instantComplete.enabled || s_icDone || GDF(pl, PlayLayer, m_hasCompletedLevel, bool)) return;
    s_icDone = true;
    gd::call<void>(gd::addr::PlayLayer::levelComplete, pl);
});

// ------------------------------------------------------------------ Jump Hack
namespace {
Hack& jumpHack = hack("LEVEL", "LEVEL/HACK_JUMP_HACK").isCheat();
}
MH_ON(ev::poPushButton, [](void* player, int button, bool&) {
    if (!jumpHack.enabled || button != static_cast<int>(gd::PlayerButton::Jump)) return;
    GDF(player, PlayerObject, m_isOnGround, bool) = true;
    GDF(player, PlayerObject, m_isOnGround2, bool) = true;
    GDF(player, PlayerObject, m_isOnGround3, bool) = true;
    GDF(player, PlayerObject, m_isOnGround4, bool) = true;
});
MH_ON(ev::poPreUpdate, [](void* player, float) {
    if (!jumpHack.enabled) return;
    if (gui::keyDown(VK_SPACE) || gui::keyDown(VK_UP) || (GetAsyncKeyState(VK_LBUTTON) & 0x8000)) {
        GDF(player, PlayerObject, m_isOnGround, bool) = true;
    }
});

// ------------------------------------------------------------------ Pause During Complete
namespace {
Hack& pauseComplete = hack("LEVEL", "LEVEL/HACK_PAUSE_DURING_COMPLETE");
bool s_savedEndAnim = false;
} // namespace
MH_ON(ev::plPause, [](void* pl, bool&) {
    if (!pauseComplete.enabled) return;
    bool& f = GDF(pl, GJBaseGameLayer, m_levelEndAnimationStarted, bool);
    s_savedEndAnim = f;
    f = false;
    gui::runOnMainThread([](void*) {
        if (auto pl2 = gd::playLayer()) GDF(pl2, GJBaseGameLayer, m_levelEndAnimationStarted, bool) = s_savedEndAnim;
    }, nullptr);
});

// ------------------------------------------------------------------ Practice Bug Fix
namespace {
Hack& practiceFix = hack("LEVEL", "LEVEL/HACK_PRACTICE_BUG_FIX");
struct PlayerSnap {
    double yVel;
    double xVelRel;
    float rotation;
    bool upsideDown;
};
std::unordered_map<void*, std::pair<PlayerSnap, PlayerSnap>> s_snaps;

PlayerSnap snap(void* p) {
    PlayerSnap s{};
    if (!p) return s;
    s.yVel = GDF(p, PlayerObject, m_yVelocity, double);
    s.xVelRel = GDF(p, PlayerObject, m_platformerXVelocity, double);
    s.upsideDown = GDF(p, PlayerObject, m_isUpsideDown, bool);
    return s;
}
void restore(void* p, const PlayerSnap& s) {
    if (!p) return;
    GDF(p, PlayerObject, m_yVelocity, double) = s.yVel;
    GDF(p, PlayerObject, m_platformerXVelocity, double) = s.xVelRel;
}

void (*o_loadFromCheckpoint)(void*, void*);
void h_loadFromCheckpoint(void* self, void* cp) {
    o_loadFromCheckpoint(self, cp);
    if (!practiceFix.enabled) return;
    auto it = s_snaps.find(cp);
    if (it == s_snaps.end()) return;
    restore(gd::player1(self), it->second.first);
    restore(gd::player2(self), it->second.second);
}
} // namespace
MH_ON(ev::plCheckpoint, [](void* pl, void* cp) {
    if (practiceFix.enabled) s_snaps[cp] = {snap(gd::player1(pl)), snap(gd::player2(pl))};
});
MH_ON(ev::plQuit, [](void*) { s_snaps.clear(); });

// ------------------------------------------------------------------ Practice Music
namespace {
Hack& practiceMusic = hack("LEVEL", "LEVEL/HACK_PRACTICE_MUSIC");
}
MH_ON(ev::itemUnlocked, [](int type, int id, bool& r) {
    if (practiceMusic.enabled && type == static_cast<int>(gd::UnlockType::GJItem) && id == 17) r = true;
});

// ------------------------------------------------------------------ Replay Last Checkpoint
namespace {
Hack& replayCheckpoint = hack("LEVEL", "LEVEL/HACK_REPLAY_LAST_CHECKPOINT");
}
MH_ON(ev::endLevelSetup, [](void* layer) {
    if (!replayCheckpoint.enabled || !gd::isPractice()) return;
    static void* s_layer = nullptr;
    s_layer = layer;
    gui::runOnMainThread([](void*) {
        if (gd::playLayer()) gd::call<void>(gd::addr::EndLevelLayer::onRestartCheckpoint, s_layer, static_cast<void*>(nullptr));
    }, nullptr);
});

// ------------------------------------------------------------------ Show Hitboxes
namespace {
bool s_hbSolids = true, s_hbHazards = true, s_hbSpecials = true, s_hbPassable = true, s_hbPlayer = true;
bool s_hbFill = false, s_hbHideUsed = false, s_hbAxis = false, s_hbSection = false, s_hbSelected = false;
bool s_hbPlayerRot = false, s_hbPlayerColl = false;
float s_hbOpacity = 100.f, s_hbStroke = 1.f;
Hack& showHitboxes = hack("LEVEL", "LEVEL/SHOW_HITBOXES/HACK_SHOW_HITBOXES").isCheat()
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/SOLIDS/CHECKBOX", &s_hbSolids)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/HAZARDS/CHECKBOX", &s_hbHazards)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/SPECIALS/CHECKBOX", &s_hbSpecials)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/PASSABLE/CHECKBOX", &s_hbPassable)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/PLAYER", &s_hbPlayer)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/PLAYER_ROTATION", &s_hbPlayerRot)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/PLAYER_COLLISION", &s_hbPlayerColl)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/FILL", &s_hbFill)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/HIDE_USED", &s_hbHideUsed)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/AXIS_ALIGNED_BOX", &s_hbAxis)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/SECTION", &s_hbSection)
    .optBool("LEVEL/SHOW_HITBOXES/DROPDOWN/SELECTED_OBJECTS", &s_hbSelected)
    .optFloat("LEVEL/SHOW_HITBOXES/DROPDOWN/OPACITY", &s_hbOpacity, 0, 100, "%.0f%%")
    .optFloat("LEVEL/SHOW_HITBOXES/DROPDOWN/STROKE_SIZE", &s_hbStroke, 0.1f, 5)
    .partial("Usa il disegno delle hitbox integrato in GD 2.2; le opzioni di stile dipendono dal gioco");

bool s_hdNoclip = false, s_hdDeathObj = true;
Hack& hitboxesOnDeath = hack("LEVEL", "LEVEL/SHOW_HITBOXES_ON_DEATH/HACK_SHOW_HITBOXES_ON_DEATH")
    .optBool("LEVEL/SHOW_HITBOXES_ON_DEATH/DROPDOWN/NOCLIP", &s_hdNoclip)
    .optBool("LEVEL/SHOW_HITBOXES_ON_DEATH/DROPDOWN/DEATH_OBJ", &s_hdDeathObj);

int s_trailMax = 240;
bool s_trailRequire = true, s_trailUncapped = false, s_trailHalf = false;
float s_trailOpacity = 100.f;
Hack& hitboxTrail = hack("LEVEL", "LEVEL/SHOW_HITBOXES_TRAIL/HACK_SHOW_HITBOXES_TRAIL").isCheat()
    .optInt("LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/MAX_DRAW", &s_trailMax, 1, 10000, "%d frames")
    .optBool("LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/REQUIRE_SHOW_HITBOXES", &s_trailRequire)
    .optBool("LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/UNCAPPED_EDITOR_LENGTH", &s_trailUncapped)
    .optFloat("LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/OPACITY", &s_trailOpacity, 0, 100, "%.0f%%")
    .optBool("LEVEL/SHOW_HITBOXES_TRAIL/DROPDOWN/HIGHLIGHT_HALFTICKS", &s_trailHalf);

bool s_deadNow = false;

bool wantHitboxes(void* pl) {
    if (showHitboxes.enabled) return true;
    if (hitboxesOnDeath.enabled) {
        auto p1 = gd::player1(pl);
        if (p1 && GDF(p1, PlayerObject, m_isDead, bool)) return true;
        if (s_hdNoclip && s_deadNow) return true;
    }
    return false;
}

bool (*o_shouldDrawTrail)(void*);
bool h_shouldDrawTrail(void* self) {
    if (hitboxTrail.enabled && (!s_trailRequire || wantHitboxes(self))) return true;
    return o_shouldDrawTrail(self);
}
bool (*o_shouldRecordTrail)(void*);
bool h_shouldRecordTrail(void* self) {
    if (hitboxTrail.enabled) return true;
    return o_shouldRecordTrail(self);
}
} // namespace

MH_ON(ev::plDrawHitboxes, [](void* pl, bool& draw) {
    if (wantHitboxes(pl)) draw = true;
});
MH_ON(ev::plNoclipDeath, [](void*, void*) { s_deadNow = true; });
MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (wantHitboxes(pl)) {
        auto node = GDF(pl, GJBaseGameLayer, m_debugDrawNode, void*);
        if (node && !cc::isVisible(node)) cc::setVisible(node, true);
        gd::call<void>(gd::addr::GJBaseGameLayer::updateDebugDraw, pl);
    }
    s_deadNow = false;
});

// ------------------------------------------------------------------ Show Layout / Show Triggers
namespace {
bool s_layBg = true, s_layGround = true, s_layMg = true, s_layObj = true;
Hack& showLayout = hack("LEVEL", "LEVEL/SHOW_LAYOUT/HACK_SHOW_LAYOUT")
    .optBool("LEVEL/SHOW_LAYOUT/DROPDOWN/BG/ENABLED", &s_layBg)
    .optBool("LEVEL/SHOW_LAYOUT/DROPDOWN/GROUND/ENABLED", &s_layGround)
    .optBool("LEVEL/SHOW_LAYOUT/DROPDOWN/MG/ENABLED", &s_layMg)
    .optBool("LEVEL/SHOW_LAYOUT/DROPDOWN/OBJ/ENABLED", &s_layObj);
// In gioco GD non mette i trigger nella scena (rimangono solo nelle sezioni), quindi renderli
// visibili non basta: si disegnano dei riquadri con il nome nella posizione del trigger.
Hack& showTriggers = hack("LEVEL", "LEVEL/HACK_SHOW_TRIGGERS")
    .partial("Disegna un riquadro con il nome al posto di ogni trigger (GD in gioco non ha gli sprite dei trigger nella scena)");

const char* triggerName(int id) {
    switch (id) {
    case 29: case 30: case 104: case 105: case 221: case 717: case 718: case 743: case 744: case 899: case 900: case 915: return "Color";
    case 22: case 23: case 24: case 25: case 26: case 27: case 28: case 55: case 56: case 57: case 58: case 59: case 1915: return "Enter";
    case 901: return "Move";
    case 1006: return "Pulse";
    case 1007: return "Alpha";
    case 1049: return "Toggle";
    case 1268: return "Spawn";
    case 1346: return "Rotate";
    case 1347: return "Follow";
    case 1520: return "Shake";
    case 1585: return "Animate";
    case 1595: return "Touch";
    case 1611: return "Count";
    case 1616: return "Stop";
    case 1811: return "Instant Count";
    case 1812: return "On Death";
    case 1814: return "Follow Y";
    case 1815: return "Collision";
    case 1817: return "Pickup";
    case 1818: return "BG Effect On";
    case 1819: return "BG Effect Off";
    case 1912: return "Random";
    case 1913: return "Zoom";
    case 1914: return "Static Cam";
    case 1916: return "Offset Cam";
    case 1917: return "Reverse";
    case 1932: return "Player Control";
    case 1934: return "Song";
    case 1935: return "Time Warp";
    case 2015: return "Rotate Cam";
    case 2062: return "Edge Cam";
    case 2066: return "Gravity";
    case 2067: return "Scale";
    case 2068: return "Adv Random";
    case 2899: return "Options";
    case 2900: return "Gradient";
    case 2901: return "Gameplay Offset";
    case 2903: return "Particles";
    case 3006: case 3007: case 3008: case 3009: case 3010: return "Area";
    case 3600: return "End";
    case 3602: return "SFX";
    case 3603: return "Edit SFX";
    case 3604: return "Event";
    case 3606: return "BG Speed";
    case 3608: return "Spawn Particle";
    case 3609: return "Instant Collision";
    case 3612: return "MG Speed";
    case 3613: return "UI";
    case 3615: return "Time";
    case 3617: return "Time Control";
    case 3618: return "Reset";
    case 3619: return "Item Edit";
    case 3620: return "Item Compare";
    case 3640: return "Collision State";
    case 3641: return "Persistent Item";
    case 3660: return "Edit Adv Follow";
    case 3661: return "Retarget Adv Follow";
    case 3662: return "Link Visible";
    default: return nullptr;
    }
}

std::vector<void*> s_triggers;
void* s_triggerLayer = nullptr;
unsigned s_triggerObjCount = 0;
} // namespace

MH_ON(ev::plInit, [](void*) {
    s_triggers.clear();
    s_triggerLayer = nullptr;
    s_triggerObjCount = 0;
});

static gui::Overlay s_triggerOverlay([] {
    if (!showTriggers.enabled) return;
    void* pl = gd::playLayer();
    if (!pl) return;
    void* objects = GDF(pl, GJBaseGameLayer, m_objects, void*);
    unsigned n = cc::arrayCount(objects);
    if (pl != s_triggerLayer || n != s_triggerObjCount) {
        s_triggerLayer = pl;
        s_triggerObjCount = n;
        s_triggers.clear();
        forEach(objects, [](void* o) {
            if (o && GDF(o, GameObject, m_isTrigger, bool)) s_triggers.push_back(o);
        });
    }
    void* objLayer = GDF(pl, GJBaseGameLayer, m_objectLayer, void*);
    auto toWorld = reinterpret_cast<cc::Point* (*)(void*, cc::Point*, const cc::Point*)>(
        cc::resolve("?convertToWorldSpace@CCNode@cocos2d@@QEAA?AVCCPoint@2@AEBV32@@Z"));
    if (!objLayer || !toWorld) return;
    cc::Size win = cc::winSize();
    ImVec2 disp = ImGui::GetIO().DisplaySize;
    if (win.width <= 0 || win.height <= 0) return;
    float sx = disp.x / win.width, sy = disp.y / win.height;
    float layerScale = cc::scale(objLayer);
    float half = 12.f * layerScale * sx;
    auto* dl = ImGui::GetBackgroundDrawList();
    int drawn = 0;
    for (void* t : s_triggers) {
        cc::Point local(static_cast<float>(GDF(t, GameObject, m_positionX, double)),
                        static_cast<float>(GDF(t, GameObject, m_positionY, double)));
        cc::Point w;
        toWorld(objLayer, &w, &local);
        ImVec2 c(w.x * sx, disp.y - w.y * sy);
        if (c.x < -50 || c.x > disp.x + 50 || c.y < -50 || c.y > disp.y + 50) continue;
        dl->AddRectFilled(ImVec2(c.x - half, c.y - half), ImVec2(c.x + half, c.y + half), IM_COL32(40, 40, 40, 110));
        dl->AddRect(ImVec2(c.x - half, c.y - half), ImVec2(c.x + half, c.y + half), IM_COL32(120, 220, 255, 220), 0.f, 0, 1.5f);
        int id = GDF(t, GameObject, m_objectID, int);
        char buf[24];
        const char* name = triggerName(id);
        if (!name) {
            snprintf(buf, sizeof(buf), "%d", id);
            name = buf;
        }
        dl->AddText(ImVec2(c.x - half, c.y + half + 1), IM_COL32(255, 255, 255, 230), name);
        if (++drawn > 600) break;
    }
});

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!showLayout.enabled) return;
    if (showLayout.enabled) {
        if (s_layBg) cc::setVisible(GDF(pl, GJBaseGameLayer, m_background, void*), false);
        if (s_layMg) cc::setVisible(GDF(pl, GJBaseGameLayer, m_middleground, void*), false);
        if (s_layGround) {
            cc::setVisible(GDF(pl, GJBaseGameLayer, m_groundLayer, void*), false);
            cc::setVisible(GDF(pl, GJBaseGameLayer, m_groundLayer2, void*), false);
        }
    }
    auto& vis = GDF(pl, GJBaseGameLayer, m_visibleObjects, void**);
    int count = GDF(pl, GJBaseGameLayer, m_visibleObjectsCount, int);
    if (!vis || count <= 0) return;
    for (int i = 0; i < count; i++) {
        void* o = vis[i];
        if (!o) continue;
        bool trigger = GDF(o, GameObject, m_isTrigger, bool);
        if (s_layObj && !trigger &&
            (objType(o) == gd::GameObjectType::Decoration || GDF(o, GameObject, m_isDecoration, bool)))
            cc::setVisible(o, false);
    }
});

// ------------------------------------------------------------------ Hitbox Multiplier
namespace {
bool s_hmHaz = true, s_hmSaw = true, s_hmSolid = false;
float s_hmHazMul = 1.f, s_hmSawMul = 1.f, s_hmSolidMul = 1.f;
Hack& hitboxMul = hack("LEVEL", "LEVEL/HITBOX_MULTIPLIER/HACK_HITBOX_MULTIPLIER").isCheat()
    .optBool("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/HAZARDS/ENABLED", &s_hmHaz)
    .optFloat("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/HAZARDS/MULTIPLIER", &s_hmHazMul, 0.f, 4.f, "%.2fx")
    .optBool("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/SAWS/ENABLED", &s_hmSaw)
    .optFloat("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/SAWS/MULTIPLIER", &s_hmSawMul, 0.f, 4.f, "%.2fx")
    .optBool("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/SOLIDS/ENABLED", &s_hmSolid)
    .optFloat("LEVEL/HITBOX_MULTIPLIER/DROPDOWN/SOLIDS/MULTIPLIER", &s_hmSolidMul, 0.f, 4.f, "%.2fx")
    .partial("Applicato all'avvio del livello (riavvia il livello dopo aver cambiato i valori)");
} // namespace
MH_ON(ev::plInit, [](void* pl) {
    if (!hitboxMul.enabled) return;
    forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [](void* o) {
        auto t = objType(o);
        float& radius = GDF(o, GameObject, m_objectRadius, float);
        float mul = 1.f;
        if (radius > 0.f && isHazard(t)) {
            if (s_hmSaw) radius *= s_hmSawMul;
        } else if (isHazard(t) && s_hmHaz) {
            mul = s_hmHazMul;
        } else if ((isSolid(t) || isSlope(t)) && s_hmSolid) {
            mul = s_hmSolidMul;
        }
        if (mul != 1.f) {
            GDF(o, GameObject, m_width, float) *= mul;
            GDF(o, GameObject, m_height, float) *= mul;
        }
        GDF(o, GameObject, m_isObjectRectDirty, bool) = true;
        GDF(o, GameObject, m_isOrientedBoxDirty, bool) = true;
    });
});

// ------------------------------------------------------------------ Jumpscare
// Alla morte (oltre la percentuale scelta) mostra un'immagine a tutto schermo con un suono forte.
// Immagine e suono personalizzabili: megahack/jumpscare.png e megahack/jumpscare.mp3 (o .ogg/.wav);
// senza file si usa un lampo rosso con una faccia disegnata e il suono di morte di GD.
namespace {
bool s_jsFrom0 = false;
float s_jsPercent = 50.f, s_jsProb = 100.f, s_jsTime = 1.f;
Hack& jumpscare = hack("LEVEL", "LEVEL/JUMPSCARE/HACK_JUMPSCARE")
    .optBool("LEVEL/JUMPSCARE/DROPDOWN/ONLY_FROM_0", &s_jsFrom0)
    .optFloat("LEVEL/JUMPSCARE/DROPDOWN/PERCENTAGE", &s_jsPercent, 0, 100, "%.0f%%")
    .optFloat("LEVEL/JUMPSCARE/DROPDOWN/PROBABILITY", &s_jsProb, 0, 100, "%.0f%%")
    .optFloat("LEVEL/JUMPSCARE/DROPDOWN/TIME_THRESHOLD", &s_jsTime, 0, 10, "%.1fs")
    .partial("Parte quando muori oltre la percentuale scelta. Immagine e suono: megahack/jumpscare.png e "
             "megahack/jumpscare.mp3 (o .ogg / .wav)");
float s_jsShow = 0.f;
constexpr float kJsDuration = 1.4f;
bool s_jsArmed = true;
std::mt19937 s_rng{std::random_device{}()};
unsigned s_jsTexture = 0;
float s_jsTexW = 0.f, s_jsTexH = 0.f;
bool s_jsTexTried = false;

// immagine personalizzata caricata con la cache delle texture di cocos (stesso contesto OpenGL di ImGui)
void loadJumpscareImage() {
    s_jsTexTried = true;
    auto file = paths::dataDir() / "jumpscare.png";
    std::error_code ec;
    if (!std::filesystem::exists(file, ec)) return;
    void* cache = cc::call<void*>("?sharedTextureCache@CCTextureCache@cocos2d@@SAPEAV12@XZ");
    if (!cache) return;
    std::string path = file.string();
    void* tex = cc::call<void*>("?addImage@CCTextureCache@cocos2d@@QEAAPEAVCCTexture2D@2@PEBD_N@Z", cache, path.c_str(), false);
    if (!tex) return;
    cc::retain(tex);
    s_jsTexture = cc::call<unsigned>("?getName@CCTexture2D@cocos2d@@UEAAIXZ", tex);
    s_jsTexW = static_cast<float>(cc::call<unsigned>("?getPixelsWide@CCTexture2D@cocos2d@@UEAAIXZ", tex));
    s_jsTexH = static_cast<float>(cc::call<unsigned>("?getPixelsHigh@CCTexture2D@cocos2d@@UEAAIXZ", tex));
    MH_LOG("Jumpscare: immagine %s (%.0fx%.0f)", path.c_str(), s_jsTexW, s_jsTexH);
}

// suono: file personalizzato oppure quello di morte di GD, a volume pieno con FMOD
void playJumpscareSound() {
    void* f = gd::fmod();
    void* sys = f ? GDF(f, FMODAudioEngine, m_system, void*) : nullptr;
    static HMODULE fm = GetModuleHandleW(L"fmod.dll");
    if (!sys || !fm) {
        MessageBeep(MB_ICONHAND);
        return;
    }
    using CreateFn = int (*)(void*, const char*, unsigned, void*, void**);
    using PlayFn = int (*)(void*, void*, void*, bool, void**);
    using VolFn = int (*)(void*, float);
    static auto create = reinterpret_cast<CreateFn>(GetProcAddress(fm, "?createSound@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z"));
    static auto play = reinterpret_cast<PlayFn>(GetProcAddress(fm, "?playSound@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEAVSound@2@PEAVChannelGroup@2@_NPEAPEAVChannel@2@@Z"));
    static auto setVol = reinterpret_cast<VolFn>(GetProcAddress(fm, "?setVolume@ChannelControl@FMOD@@QEAA?AW4FMOD_RESULT@@M@Z"));
    static void* sound = nullptr;
    if (!sound && create) {
        std::error_code ec;
        std::filesystem::path chosen;
        for (const char* n : {"jumpscare.mp3", "jumpscare.ogg", "jumpscare.wav"}) {
            auto p = paths::dataDir() / n;
            if (std::filesystem::exists(p, ec)) {
                chosen = p;
                break;
            }
        }
        if (chosen.empty()) chosen = paths::gameDir() / "Resources" / "explode_11.ogg";
        std::string path = chosen.string();
        constexpr unsigned FMOD_DEFAULT_MODE = 0x0, FMOD_CREATESAMPLE = 0x100;
        if (create(sys, path.c_str(), FMOD_DEFAULT_MODE | FMOD_CREATESAMPLE, nullptr, &sound) != 0) sound = nullptr;
    }
    void* channel = nullptr;
    if (!sound || !play || play(sys, sound, nullptr, false, &channel) != 0) {
        MessageBeep(MB_ICONHAND);
        return;
    }
    if (channel && setVol) setVol(channel, 1.f);
}
} // namespace
MH_ON(ev::plReset, [](void*) { s_jsArmed = true; });
MH_ON(ev::plDeath, [](void* pl, void*) {
    if (!jumpscare.enabled || !s_jsArmed) return;
    s_jsArmed = false;
    if (s_jsFrom0 && GDF(pl, GJBaseGameLayer, m_startPosObject, void*)) return;
    if (gd::percent() < s_jsPercent || gd::levelTime() < s_jsTime) return;
    if (std::uniform_real_distribution<float>(0, 100)(s_rng) > s_jsProb) return;
    s_jsShow = kJsDuration;
    playJumpscareSound();
});
static gui::Overlay s_jsOverlay([] {
    if (s_jsShow <= 0.f) return;
    if (!s_jsTexTried) loadJumpscareImage();
    float t = kJsDuration - s_jsShow; // tempo trascorso
    s_jsShow -= ImGui::GetIO().DeltaTime;
    auto* dl = ImGui::GetForegroundDrawList();
    auto sz = ImGui::GetIO().DisplaySize;
    // tremolio e zoom iniziale
    float shake = 18.f * std::max(0.f, 1.f - t * 1.5f);
    ImVec2 off(std::sin(t * 90.f) * shake, std::cos(t * 77.f) * shake);
    float zoom = 1.f + 0.25f * std::max(0.f, 1.f - t * 4.f);
    float fade = std::min(1.f, s_jsShow / 0.3f);
    auto alpha = static_cast<int>(255 * fade);
    if (s_jsTexture) {
        float k = std::max(sz.x / s_jsTexW, sz.y / s_jsTexH) * zoom;
        ImVec2 isz(s_jsTexW * k, s_jsTexH * k);
        ImVec2 p0((sz.x - isz.x) * 0.5f + off.x, (sz.y - isz.y) * 0.5f + off.y);
        dl->AddRectFilled(ImVec2(0, 0), sz, IM_COL32(0, 0, 0, alpha));
        dl->AddImage(static_cast<ImTextureID>(static_cast<intptr_t>(s_jsTexture)), p0, ImVec2(p0.x + isz.x, p0.y + isz.y),
                     ImVec2(0, 0), ImVec2(1, 1), IM_COL32(255, 255, 255, alpha));
        return;
    }
    // senza immagine: lampo rosso/nero e una faccia
    bool flash = static_cast<int>(t * 14.f) % 2 == 0 && t < 0.5f;
    dl->AddRectFilled(ImVec2(0, 0), sz, flash ? IM_COL32(200, 0, 0, alpha) : IM_COL32(0, 0, 0, alpha));
    ImVec2 c(sz.x * 0.5f + off.x, sz.y * 0.5f + off.y);
    float r = sz.y * 0.38f * zoom;
    ImU32 white = IM_COL32(235, 235, 235, alpha), red = IM_COL32(255, 0, 0, alpha), black = IM_COL32(0, 0, 0, alpha);
    dl->AddCircleFilled(c, r, IM_COL32(25, 25, 25, alpha), 64);
    dl->AddCircle(c, r, white, 64, r * 0.03f);
    for (int side : {-1, 1}) {
        ImVec2 eye(c.x + side * r * 0.38f, c.y - r * 0.22f);
        dl->AddCircleFilled(eye, r * 0.17f, white, 32);
        dl->AddCircleFilled(eye, r * 0.08f, red, 24);
        dl->AddLine(ImVec2(eye.x - side * r * 0.25f, eye.y - r * 0.3f), ImVec2(eye.x + side * r * 0.15f, eye.y - r * 0.18f), white, r * 0.05f);
    }
    ImVec2 m0(c.x - r * 0.5f, c.y + r * 0.25f), m1(c.x + r * 0.5f, c.y + r * 0.65f);
    dl->AddRectFilled(m0, m1, black, r * 0.08f);
    for (int i = 0; i < 6; i++) {
        float x = m0.x + (m1.x - m0.x) * (i + 0.5f) / 6.f;
        dl->AddTriangleFilled(ImVec2(x - r * 0.06f, m0.y), ImVec2(x + r * 0.06f, m0.y), ImVec2(x, m0.y + r * 0.14f), white);
        dl->AddTriangleFilled(ImVec2(x - r * 0.06f, m1.y), ImVec2(x + r * 0.06f, m1.y), ImVec2(x, m1.y - r * 0.14f), white);
    }
});

// ------------------------------------------------------------------ Random Seed
namespace {
int s_seed = 1;
bool s_lockVariance = false;
Hack& randomSeed = hack("LEVEL", "LEVEL/RANDOM_SEED/HACK_RANDOM_SEED").isCheat()
    .optInt("LEVEL/RANDOM_SEED/DROPDOWN/SEED", &s_seed, 0, 0x7fffffff)
    .optBool("LEVEL/RANDOM_SEED/DROPDOWN/HACK_LOCK_VARIANCE", &s_lockVariance)
    .partial("Imposta il seed del generatore C (rand) ad ogni tentativo; i trigger che usano il generatore interno di GD potrebbero non essere influenzati");
} // namespace
MH_ON(ev::plReset, [](void* pl) {
    if (!randomSeed.enabled) return;
    srand(static_cast<unsigned>(s_seed));
    if (s_lockVariance) {
        auto* v = &GDF(pl, GJBaseGameLayer, m_varianceValues, float);
        for (int i = 0; i < 2000; i++) v[i] = 0.f;
    }
});

// ------------------------------------------------------------------ Respawn Time
namespace mh::speed {
float current(); // display.cpp
}
namespace {
float s_respawnMs = 1000.f;
bool s_respawnIgnoreSpeed = false;
Hack& respawnTime = hack("LEVEL", "LEVEL/RESPAWN_TIME/HACK_RESPAWN_TIME")
    .optFloat("LEVEL/RESPAWN_TIME/DROPDOWN/SPINNER", &s_respawnMs, 0, 5000, "%.0f ms")
    .optBool("LEVEL/RESPAWN_TIME/DROPDOWN/IGNORE_SPEEDHACK", &s_respawnIgnoreSpeed);

// PlayLayer::destroyPlayer (2.2091) crea il ritardo del respawn con
//   CCSequence(CCDelayTime::create(max(1.0, m_respawnTime)), CCCallFunc(delayedResetLevel))
// (0.5 invece di 1.0 nell'altro ramo): scrivere m_respawnTime puo' solo allungarlo. Si cambia quindi la
// durata passata a CCDelayTime::create quando la chiamata arriva da quei due punti.
#if GD_VERSION == 22091
constexpr uintptr_t kRespawnDelayReturns[] = {0x513099, 0x513622}; // indirizzi dopo le due call
#endif

bool isRespawnDelay(uintptr_t ret, float d) {
#if GD_VERSION == 22091
    (void)d;
    for (uintptr_t r : kRespawnDelayReturns)
        if (ret == mem::base() + r) return true;
    return false;
#else
    // senza indirizzi verificati: il ritardo standard creato dentro destroyPlayer
    (void)ret;
    return d == 1.f || d == 0.5f;
#endif
}

void* (*o_delayCreate)(float);
void* h_delayCreate(float d) {
    if (g_inDestroyPlayer && respawnTime.enabled) {
#ifdef _MSC_VER
        auto ret = reinterpret_cast<uintptr_t>(_ReturnAddress());
#else
        auto ret = reinterpret_cast<uintptr_t>(__builtin_return_address(0));
#endif
        if (isRespawnDelay(ret, d)) {
            float t = s_respawnMs / 1000.f;
            // il ritardo scorre col tempo del gioco: con lo speedhack va scalato per restare in tempo reale
            if (s_respawnIgnoreSpeed) t *= speed::current();
            d = std::max(0.f, t);
        }
    }
    return o_delayCreate(d);
}
} // namespace

MH_INSTALL(respawnTime) {
    hooks::exported("libcocos2d.dll", "?create@CCDelayTime@cocos2d@@SAPEAV12@M@Z", h_delayCreate, &o_delayCreate);
}

// ------------------------------------------------------------------ StartPos Switcher
namespace {
int s_spLeft = VK_LEFT, s_spRight = VK_RIGHT;
bool s_spHide = false, s_spRated = false, s_spNoPractice = false, s_spAfterDeath = false, s_spQuick = false;
float s_spScale = 1.f;
int s_spIndex = 0;
int s_spQuickKeys[16]{};
Hack& spSwitcher = hack("LEVEL", "LEVEL/STARTPOS_SWITCHER/HACK_STARTPOS_SWITCHER")
    .optKey("LEVEL/STARTPOS_SWITCHER/DROPDOWN/L", &s_spLeft)
    .optKey("LEVEL/STARTPOS_SWITCHER/DROPDOWN/R", &s_spRight)
    .optBool("LEVEL/STARTPOS_SWITCHER/DROPDOWN/HIDE_INTERFACE", &s_spHide)
    .optBool("LEVEL/STARTPOS_SWITCHER/DROPDOWN/ALLOW_RATED", &s_spRated)
    .optBool("LEVEL/STARTPOS_SWITCHER/DROPDOWN/DISABLE_IN_PRACTICE", &s_spNoPractice)
    .optBool("LEVEL/STARTPOS_SWITCHER/DROPDOWN/SWITCH_AFTER_DEATH", &s_spAfterDeath)
    .optFloat("LEVEL/STARTPOS_SWITCHER/DROPDOWN/INTERFACE_SIZE", &s_spScale, 0.25f, 3.f, "%.2fx")
    .optBool("LEVEL/STARTPOS_SWITCHER/DROPDOWN/QUICK_SWITCH/TOGGLE", &s_spQuick)
    .ui([] {
        if (!s_spQuick) return;
        for (int i = 0; i < 16; i++) {
            std::string key = "LEVEL/STARTPOS_SWITCHER/DROPDOWN/QUICK_SWITCH/SHORTCUTS/" + std::to_string(i);
            gui::keyButton(T(key), &s_spQuickKeys[i]);
        }
    });

// Le StartPos (oggetto ID 31) vengono raccolte una volta a inizio livello da m_objects:
// funziona allo stesso modo su 2.209 e 2.2091
std::vector<void*> s_startPositions;
void* s_startPosLayer = nullptr;
} // namespace
namespace mh {
std::vector<void*> g_liveStartPos; // StartPos create a runtime (Auto StartPos, extras.cpp)
}
namespace {

const std::vector<void*>& startPositions(void* pl) {
    if (pl != s_startPosLayer) {
        s_startPosLayer = pl;
        s_startPositions.clear();
        forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [](void* o) {
            if (o && GDF(o, GameObject, m_objectID, int) == 31) s_startPositions.push_back(o);
        });
        for (void* o : g_liveStartPos) s_startPositions.push_back(o);
        std::sort(s_startPositions.begin(), s_startPositions.end(), [](void* a, void* b) {
            return GDF(a, GameObject, m_positionX, double) < GDF(b, GameObject, m_positionX, double);
        });
    }
    return s_startPositions;
}

bool allowedHere(void* pl) {
    if (s_spNoPractice && GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool)) return false;
    auto lvl = GDF(pl, GJBaseGameLayer, m_level, void*);
    if (!s_spRated && lvl && gd::seedValue(&GDF(lvl, GJGameLevel, m_stars, char)) > 0) return false;
    return true;
}

void switchTo(int index) {
    auto pl = gd::playLayer();
    if (!pl || !allowedHere(pl)) return;
    const auto& sps = startPositions(pl);
    int n = static_cast<int>(sps.size());
    if (n == 0) return;
    if (index < 0) index = n;
    if (index > n) index = 0;
    s_spIndex = index;
    GDF(pl, GJBaseGameLayer, m_startPosObject, void*) = index == 0 ? nullptr : sps[index - 1];
    if (GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool)) gd::call<void>(gd::addr::PlayLayer::removeAllCheckpoints, pl);
    gd::call<void>(gd::addr::PlayLayer::resetLevel, pl);
    gd::call<void>(gd::addr::PlayLayer::startMusic, pl);
}
int s_spQueued = 0;
} // namespace

MH_ON(ev::plQuit, [](void*) { s_startPosLayer = nullptr; });
MH_ON(ev::plInit, [](void* pl) {
    s_spIndex = 0;
    s_startPosLayer = nullptr;
    auto cur = GDF(pl, GJBaseGameLayer, m_startPosObject, void*);
    if (!cur) return;
    const auto& sps = startPositions(pl);
    for (size_t i = 0; i < sps.size(); i++)
        if (sps[i] == cur) s_spIndex = static_cast<int>(i) + 1;
});

static gui::KeyHandler s_spKeys([](int vk, bool down) {
    if (!spSwitcher.enabled || !down || !gd::playLayer()) return false;
    int target = -1000;
    if (vk == s_spLeft) target = s_spIndex - 1;
    else if (vk == s_spRight) target = s_spIndex + 1;
    else if (s_spQuick)
        for (int i = 0; i < 16; i++)
            if (s_spQuickKeys[i] && vk == s_spQuickKeys[i]) target = i;
    if (target == -1000) return false;
    auto p1 = gd::player1(gd::playLayer());
    if (s_spAfterDeath && p1 && !GDF(p1, PlayerObject, m_isDead, bool)) {
        s_spQueued = target + 1; // applicato alla prossima morte
        return true;
    }
    switchTo(target);
    return true;
});

MH_ON(ev::plDeath, [](void*, void*) {
    if (s_spQueued) {
        int t = s_spQueued - 1;
        s_spQueued = 0;
        gui::runOnMainThread([](void* ctx) { switchTo(static_cast<int>(reinterpret_cast<intptr_t>(ctx))); },
                             reinterpret_cast<void*>(static_cast<intptr_t>(t)));
    }
});

static gui::Overlay s_spOverlay([] {
    auto pl = gd::playLayer();
    if (!spSwitcher.enabled || s_spHide || !pl) return;
    const auto& sps = startPositions(pl);
    if (sps.empty()) return;
    char buf[64];
    snprintf(buf, sizeof(buf), "<   %d / %d   >", s_spIndex, static_cast<int>(sps.size()));
    auto* f = gui::bigFont();
    float size = 26.f * s_spScale;
    auto sz = ImGui::GetIO().DisplaySize;
    ImVec2 ts = f->CalcTextSizeA(size, FLT_MAX, 0, buf);
    auto* dl = ImGui::GetBackgroundDrawList();
    ImVec2 pos((sz.x - ts.x) * 0.5f, sz.y - ts.y - 20.f * s_spScale);
    dl->AddText(f, size, ImVec2(pos.x + 2, pos.y + 2), IM_COL32(0, 0, 0, 160), buf);
    dl->AddText(f, size, pos, IM_COL32(255, 255, 255, 220), buf);
});

// ------------------------------------------------------------------ Auto Song Download
namespace {
Hack& autoSongDownload = hack("LEVEL", "LEVEL/HACK_AUTO_SONG_DOWNLOAD");
} // namespace

// Auto Song Download: preme "Download" nella pagina del livello
namespace mh::creator {
void beforeLevelInfo(void* level); // Copy Hack (creator.cpp)
}
namespace {
bool (*o_levelInfoInit)(void*, void*, bool);
bool h_levelInfoInit(void* self, void* level, bool challenge) {
    creator::beforeLevelInfo(level);
    bool r = o_levelInfoInit(self, level, challenge);
    if (r && autoSongDownload.enabled) {
        auto widget = GDF(self, LevelInfoLayer, m_songWidget, void*);
        if (widget) gd::call<void>(gd::addr::CustomSongWidget::onDownload, widget, static_cast<void*>(nullptr));
    }
    return r;
}
} // namespace

// ------------------------------------------------------------------ No Collision (+ extra dalla UI v8)
namespace {
bool s_ncHaz = true, s_ncSolid = false, s_ncSlope = false, s_ncSpecial = false;
Hack& noCollision = hack("LEVEL", "LEVEL/NO_COLLISION/HACK_NO_COLLISION").isCheat()
    .optBool("LEVEL/NO_COLLISION/DROPDOWN/HAZARDS", &s_ncHaz)
    .optBool("LEVEL/NO_COLLISION/DROPDOWN/SOLIDS", &s_ncSolid)
    .optBool("LEVEL/NO_COLLISION/DROPDOWN/SLOPES", &s_ncSlope)
    .optBool("LEVEL/NO_COLLISION/DROPDOWN/SPECIALS", &s_ncSpecial);
Hack& everythingHurts = hack("LEVEL", "LEVEL/HACK_EVERYTHING_HURTS").name("Everything Hurts").isCheat();
Hack& freezePlayer = hack("LEVEL", "LEVEL/HACK_FREEZE_PLAYER").name("Freeze Player").isCheat();
Hack& instaKill = action("LEVEL", "LEVEL/HACK_INSTA_KILL", [] {
    if (auto pl = gd::playLayer()) killPlayer(pl, gd::player1(pl));
}).name("Insta Kill");
Hack& ignoreEsc = hack("LEVEL", "LEVEL/HACK_IGNORE_ESC").name("Ignore ESC");
Hack& autoPractice = hack("LEVEL", "LEVEL/HACK_AUTO_PRACTICE_MODE").name("Auto Practice Mode");

std::vector<std::pair<void*, gd::GameObjectType>> s_hurtRestore;
} // namespace

MH_ON(ev::bglCollisionPre, [](void* layer, void*, void** objs, int& count) {
    if (layer != gd::playLayer()) return;
    if (noCollision.enabled) {
        int w = 0;
        for (int i = 0; i < count; i++) {
            auto t = objType(objs[i]);
            bool skip = (s_ncHaz && isHazard(t)) || (s_ncSolid && isSolid(t)) || (s_ncSlope && isSlope(t)) ||
                        (s_ncSpecial && isSpecial(t));
            if (!skip) objs[w++] = objs[i];
        }
        count = w;
    }
    if (everythingHurts.enabled) {
        s_hurtRestore.clear();
        for (int i = 0; i < count; i++) {
            auto& t = GDF(objs[i], GameObject, m_objectType, gd::GameObjectType);
            if (isSolid(t) || isSlope(t)) {
                s_hurtRestore.emplace_back(objs[i], t);
                t = gd::GameObjectType::Hazard;
            }
        }
    }
});
MH_ON(ev::bglCollisionPost, [](void*, void*, void**, int) {
    for (auto& [o, t] : s_hurtRestore) GDF(o, GameObject, m_objectType, gd::GameObjectType) = t;
    s_hurtRestore.clear();
});

MH_ON(ev::bglPreUpdate, [](void* layer, float&, bool& run) {
    if (freezePlayer.enabled && layer == gd::playLayer()) run = false;
});
MH_ON(ev::plPause, [](void*, bool& allow) {
    if (ignoreEsc.enabled && (GetAsyncKeyState(VK_ESCAPE) & 0x8000)) allow = false;
});
MH_ON(ev::plInit, [](void* pl) {
    if (autoPractice.enabled) {
        gui::runOnMainThread([](void*) {
            if (auto p = gd::playLayer()) gd::call<void>(gd::addr::PlayLayer::togglePracticeMode, p, true);
        }, pl);
    }
});

MH_INSTALL(level) {
    using namespace gd;
    hooks::gd("PlayLayer::removeCheckpoint", addr::PlayLayer::removeCheckpoint, h_removeCheckpoint, &o_removeCheckpoint);
    hooks::gd("GJBaseGameLayer::pickupItem", addr::GJBaseGameLayer::pickupItem, h_pickupItem, &o_pickupItem);
    hooks::gd("PlayLayer::loadFromCheckpoint", addr::PlayLayer::loadFromCheckpoint, h_loadFromCheckpoint, &o_loadFromCheckpoint);
    hooks::gd("PlayLayer::shouldDrawHitboxTrail", addr::PlayLayer::shouldDrawHitboxTrail, h_shouldDrawTrail, &o_shouldDrawTrail);
    hooks::gd("PlayLayer::shouldRecordHitboxTrail", addr::PlayLayer::shouldRecordHitboxTrail, h_shouldRecordTrail, &o_shouldRecordTrail);
    hooks::gd("PauseLayer::onQuit", addr::PauseLayer::onQuit, h_onQuit, &o_onQuit);
    hooks::gd("PauseLayer::onRestartFull", addr::PauseLayer::onRestartFull, h_onRestartFull, &o_onRestartFull);
    hooks::gd("PauseLayer::onNormalMode", addr::PauseLayer::onNormalMode, h_onNormalMode, &o_onNormalMode);
    hooks::gd("PauseLayer::onPracticeMode", addr::PauseLayer::onPracticeMode, h_onPracticeMode, &o_onPracticeMode);
    hooks::gd("PauseLayer::onRestart", addr::PauseLayer::onRestart, h_onRestart, &o_onRestart);
    hooks::gd("LevelInfoLayer::init", addr::LevelInfoLayer::init, h_levelInfoInit, &o_levelInfoInit);
}
