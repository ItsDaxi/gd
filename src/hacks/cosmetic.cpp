// Finestra COSMETIC
#include "common.hpp"

#include <random>
#include <unordered_map>

using namespace mh;

namespace {

Hack& H(const char* id) { return hack("COSMETIC", id); }

// ------------------------------------------------------------------ Accurate Percentage
bool s_betterCalc = true;
int s_decimals = 2;
Hack& accuratePct = H("COSMETIC/ACCURATE_PERCENTAGE/HACK_ACCURATE_PERCENTAGE")
    .optInt("COSMETIC/ACCURATE_PERCENTAGE/DROPDOWN/DECIMALS", &s_decimals, 0, 10)
    .optBool("COSMETIC/ACCURATE_PERCENTAGE/DROPDOWN/BETTER_CALCULATION", &s_betterCalc);


Hack& noCircles = H("COSMETIC/HACK_NO_CIRCLES");
Hack& classicWaveTrail = H("COSMETIC/HACK_CLASSIC_WAVE_TRAIL").partial("Approssimato: scia dell'onda solida e senza pulsazione");
Hack& coinShower = H("COSMETIC/HACK_COIN_SHOWER");
Hack& noEndShake = H("COSMETIC/HACK_NO_END_SHAKE");
Hack& hidePauseBtn = H("COSMETIC/HACK_HIDE_PAUSE_BUTTON");
Hack& hidePauseMenu = H("COSMETIC/HACK_HIDE_PAUSE_MENU");
Hack& hidePlayer = H("COSMETIC/HACK_HIDE_PLAYER");
Hack& noCamera = H("COSMETIC/HACK_NO_CAMERA");
Hack& noCameraZoom = H("COSMETIC/HACK_NO_CAMERA_ZOOM");
Hack& noDashFire = H("COSMETIC/HACK_NO_DASH_FIRE");
Hack& noDeathEffect = H("COSMETIC/HACK_NO_DEATH_EFFECT");
Hack& noDoNotFlip = H("COSMETIC/HACK_NO_DO_NOT_FLIP");
Hack& noGhostTrail = H("COSMETIC/HACK_NO_GHOST_TRAIL");
Hack& noGlow = H("COSMETIC/HACK_NO_GLOW");
Hack& noNewBest = H("COSMETIC/HACK_NO_NEW_BEST_POPUP");
Hack& noMirror = H("COSMETIC/HACK_NO_MIRROR");
Hack& noOrbRing = H("COSMETIC/HACK_NO_ORB_RING");

bool s_noPart[22] = {};
Hack& noParticles = H("COSMETIC/NO_PARTICLES/HACK_NO_PARTICLES").ui([] {
    ImGui::TextDisabled("%s", T("COSMETIC/NO_PARTICLES/DROPDOWN"));
    for (int i = 0; i < 22; i++) {
        std::string k = "COSMETIC/NO_PARTICLES/DROPDOWN/" + std::to_string(i);
        if (ImGui::Checkbox(T(k), &s_noPart[i])) config::markDirty();
    }
});
Hack& noParticlesClassic = H("COSMETIC/HACK_NO_PARTICLES_CLASSIC");
Hack& noPortalCircle = H("COSMETIC/HACK_NO_PORTAL_CIRCLE");
Hack& noPortalLightning = H("COSMETIC/HACK_NO_PORTAL_LIGHTNING");
Hack& noPulse = H("COSMETIC/HACK_NO_PULSE");
Hack& noRespawnFlash = H("COSMETIC/HACK_NO_RESPAWN_FLASH");
Hack& noRobotFire = H("COSMETIC/HACK_NO_ROBOT_FIRE");
Hack& noShaders = H("COSMETIC/HACK_NO_SHADERS");
Hack& noShake = H("COSMETIC/HACK_NO_SHAKE");
Hack& noSpiderDash = H("COSMETIC/HACK_NO_SPIDER_DASH");
Hack& noSwingFire = H("COSMETIC/HACK_NO_SWING_FIRE");
Hack& noTrail = H("COSMETIC/HACK_NO_TRAIL");
Hack& noWavePulse = H("COSMETIC/HACK_NO_WAVE_PULSE");
Hack& noWaveTrail = H("COSMETIC/HACK_NO_WAVE_TRAIL");
Hack& noWaveTrailBehind = H("COSMETIC/HACK_NO_WAVE_TRAIL_BEHIND");
Hack& playerOnTop = H("COSMETIC/HACK_PLAYER_ON_TOP");
Hack& player1OnTop = H("COSMETIC/HACK_PLAYER_1_ON_TOP");
Hack& totalAttempts = H("COSMETIC/HACK_SHOW_TOTAL_ATTEMPTS");
Hack& solidWaveTrail = H("COSMETIC/HACK_SOLID_WAVE_TRAIL");
Hack& startPosResetCam = H("COSMETIC/HACK_STARTPOS_RESET_CAMERA");
Hack& stopTriggersOnDeath = H("COSMETIC/HACK_STOP_TRIGGERS_ON_DEATH");

bool s_randIcons = true, s_randColours = true;
Hack& iconRandomiser = H("COSMETIC/ICON_RANDOMISER/HACK_ICON_RANDOMISER")
    .optBool("COSMETIC/ICON_RANDOMISER/DROPDOWN/ICONS", &s_randIcons)
    .optBool("COSMETIC/ICON_RANDOMISER/DROPDOWN/COLOURS", &s_randColours)
    .partial("Cambia le icone a ogni livello (valori salvati in GameManager)");

float s_trailRate = 1.f;
Hack& trailCutting = H("COSMETIC/TRAIL_CUTTING/HACK_TRAIL_CUTTING")
    .optFloat("COSMETIC/TRAIL_CUTTING/DROPDOWN/RATE", &s_trailRate, 0.1f, 10.f, "%.2fx")
    .partial("Approssimato: la scia viene azzerata periodicamente");
Hack& trailAlwaysOn = H("COSMETIC/HACK_TRAIL_ALWAYS_ON");
Hack& waveTrailOnDeath = H("COSMETIC/HACK_WAVE_TRAIL_ON_DEATH");
float s_wavePulseMul = 1.f;
Hack& wavePulseSize = H("COSMETIC/WAVE_PULSE_SIZE/HACK_WAVE_PULSE_SIZE")
    .optFloat("COSMETIC/WAVE_PULSE_SIZE/DROPDOWN/SIZE", &s_wavePulseMul, 0.f, 5.f, "%.2fx");

bool s_inRingJump = false;
bool s_inPortal = false;

} // namespace

// ------------------------------------------------------------------ percentuale accurata
MH_ON(ev::plUpdateProgress, [](void* pl) {
    if (!accuratePct.enabled) return;
    auto label = GDF(pl, PlayLayer, m_percentageLabel, void*);
    if (!label || GDF(pl, GJBaseGameLayer, m_isPlatformer, bool)) return;
    float pct = gd::percent();
    if (s_betterCalc) {
        auto p1 = gd::player1(pl);
        float len = GDF(pl, GJBaseGameLayer, m_levelLength, float);
        if (p1 && len > 0.f) pct = std::clamp(cc::position(p1).x / len * 100.f, 0.f, 100.f);
    }
    char buf[32];
    snprintf(buf, sizeof(buf), "%.*f%%", s_decimals, pct);
    cc::labelSetString(label, buf);
});

// ------------------------------------------------------------------ cerchi / anelli
MH_ON(ev::poRingJump, [](void*, void*, bool&) {
    s_inRingJump = true;
    gui::runOnMainThread([](void*) { s_inRingJump = false; }, nullptr);
});
MH_ON(ev::circleWave, [](void* wave) {
    bool hide = noCircles.enabled || (noOrbRing.enabled && s_inRingJump) || (noPortalCircle.enabled && s_inPortal);
    if (hide) cc::setVisible(wave, false);
});

// ------------------------------------------------------------------ shake / camera / mirror
MH_ON(ev::bglShake, [](void* layer, bool& allow) {
    if (noShake.enabled) allow = false;
    if (noEndShake.enabled && GDF(layer, GJBaseGameLayer, m_levelEndAnimationStarted, bool)) allow = false;
});
MH_ON(ev::bglZoom, [](void*, float&, bool& allow) {
    if (noCameraZoom.enabled) allow = false;
});
MH_ON(ev::bglFlip, [](void*, bool&, bool& allow) {
    if (noMirror.enabled) allow = false;
});

// ------------------------------------------------------------------ effetti giocatore
MH_ON(ev::poDeathEffect, [](void*, bool& allow) {
    if (noDeathEffect.enabled) allow = false;
});
MH_ON(ev::poGhost, [](void*, int& type) {
    if (noGhostTrail.enabled) type = 0;
});
MH_ON(ev::poSpiderDash, [](void*, bool& allow) {
    if (noSpiderDash.enabled) allow = false;
});
MH_ON(ev::poStreak, [](void*, bool& allow) {
    if (noTrail.enabled) allow = false;
});
MH_ON(ev::poFadeStreak, [](void*, bool& allow) {
    if (trailAlwaysOn.enabled) allow = false;
});
MH_ON(ev::plNewBest, [](void*, bool& allow) {
    if (noNewBest.enabled) allow = false;
});
MH_ON(ev::particleDraw, [](void*, bool& allow) {
    if (noParticlesClassic.enabled) {
        allow = false;
        return;
    }
    if (!noParticles.enabled) return;
    // nessuna informazione sul tipo di particella a livello di draw: se e' selezionata
    // almeno una categoria vengono nascoste tutte le particelle del livello
    if (!gd::gameLayer()) return;
    for (bool b : s_noPart)
        if (b) {
            allow = false;
            return;
        }
});

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    for (auto p : {gd::player1(pl), gd::player2(pl)}) {
        if (!p) continue;
        if (hidePlayer.enabled) cc::setVisible(p, false);
        if (noDashFire.enabled) cc::setVisible(GDF(p, PlayerObject, m_dashFireSprite, void*), false);
        if (noTrail.enabled) cc::setVisible(GDF(p, PlayerObject, m_regularTrail, void*), false);
        if (trailAlwaysOn.enabled) GDF(p, PlayerObject, m_alwaysShowStreak, bool) = true;
        auto wave = GDF(p, PlayerObject, m_waveTrail, void*);
        bool isWave = GDF(p, PlayerObject, m_isDart, bool);
        if (wave) {
            if (noWaveTrail.enabled) cc::setVisible(wave, false);
            if (solidWaveTrail.enabled || classicWaveTrail.enabled) GDF(wave, HardStreak, m_isSolid, bool) = true;
            if (noWavePulse.enabled || classicWaveTrail.enabled) GDF(wave, HardStreak, m_pulseSize, float) = 0.f;
            else if (wavePulseSize.enabled) {
                // GD riscrive m_pulseSize ad ogni aggiornamento: moltiplica solo i valori nuovi
                static std::unordered_map<void*, std::pair<float, float>> seen; // base, scritto
                float& ps = GDF(wave, HardStreak, m_pulseSize, float);
                auto& e = seen[wave];
                if (ps != e.second) e.first = ps;
                e.second = e.first * s_wavePulseMul;
                ps = e.second;
            }
        }
        if (noWaveTrailBehind.enabled && isWave) cc::setVisible(GDF(p, PlayerObject, m_regularTrail, void*), false);
        if (noRobotFire.enabled) {
            cc::setVisible(GDF(p, PlayerObject, m_robotFire, void*), false);
            cc::stopSystem(GDF(p, PlayerObject, m_robotBurstParticles, void*));
        }
        if (noSwingFire.enabled) {
            cc::setVisible(GDF(p, PlayerObject, m_swingFireMiddle, void*), false);
            cc::setVisible(GDF(p, PlayerObject, m_swingFireBottom, void*), false);
            cc::setVisible(GDF(p, PlayerObject, m_swingFireTop, void*), false);
            cc::stopSystem(GDF(p, PlayerObject, m_swingBurstParticles1, void*));
            cc::stopSystem(GDF(p, PlayerObject, m_swingBurstParticles2, void*));
        }
        if (trailCutting.enabled) {
            static float acc = 0.f;
            acc += ImGui::GetIO().DeltaTime * s_trailRate;
            if (acc > 0.25f) {
                acc = 0.f;
                cc::streakReset(GDF(p, PlayerObject, m_regularTrail, void*));
            }
        }
    }
    auto p1 = gd::player1(pl), p2 = gd::player2(pl);
    if (playerOnTop.enabled) {
        if (p1) cc::setZOrder(p1, 100000);
        if (p2) cc::setZOrder(p2, 99999);
    }
    if (player1OnTop.enabled && p1 && p2) cc::setZOrder(p1, cc::zOrder(p2) + 1);
    if (hidePauseBtn.enabled) {
        auto ui = GDF(pl, GJBaseGameLayer, m_uiLayer, void*);
        if (ui) cc::setVisible(GDF(ui, UILayer, m_pauseBtn, void*), false);
    }
    if (noPulse.enabled) {
        if (auto f = gd::fmod()) {
            GDF(f, FMODAudioEngine, m_pulse1, float) = 0.f;
            GDF(f, FMODAudioEngine, m_pulse2, float) = 0.f;
            GDF(f, FMODAudioEngine, m_pulse3, float) = 0.f;
        }
    }
});

// ------------------------------------------------------------------ glow
MH_ON(ev::plInit, [](void* pl) {
    if (!noGlow.enabled) return;
    forEach(GDF(pl, GJBaseGameLayer, m_objects, void*), [](void* o) {
        GDF(o, GameObject, m_hasNoGlow, bool) = true;
        cc::setVisible(GDF(o, GameObject, m_glowSprite, void*), false);
    });
});

// ------------------------------------------------------------------ pausa / tentativi
MH_ON(ev::pauseSetup, [](void* layer) {
    if (hidePauseMenu.enabled) cc::setVisible(layer, false);
});
MH_ON(ev::plUpdateAttempts, [](void* pl) {
    if (!totalAttempts.enabled) return;
    auto lvl = GDF(pl, GJBaseGameLayer, m_level, void*);
    auto label = GDF(pl, PlayLayer, m_attemptLabel, void*);
    if (!lvl || !label) return;
    int total = gd::seedValue(&GDF(lvl, GJGameLevel, m_attempts, char));
    char buf[48];
    snprintf(buf, sizeof(buf), "Attempt %d", total);
    cc::labelSetString(label, buf);
});

// ------------------------------------------------------------------ coin shower
MH_ON(ev::plComplete, [](void* pl) {
    if (!coinShower.enabled) return;
    for (int i = 0; i < 25; i++) gd::call<void>(gd::addr::PlayLayer::spawnFirework, pl);
});

// ------------------------------------------------------------------ icon randomiser
MH_ON(ev::plQuit, [](void*) {
    if (!iconRandomiser.enabled) return;
    auto gm = gd::gameManager();
    if (!gm) return;
    static std::mt19937 rng{std::random_device{}()};
    auto rnd = [](int lo, int hi) { return std::uniform_int_distribution<int>(lo, hi)(rng); };
    using namespace gd::off;
    if (s_randIcons) {
        gd::setSeedValue(&GDF(gm, GameManager, m_playerFrame, char), rnd(1, 480));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerShip, char), rnd(1, 160));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerBall, char), rnd(1, 110));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerBird, char), rnd(1, 145));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerDart, char), rnd(1, 95));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerRobot, char), rnd(1, 65));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerSpider, char), rnd(1, 65));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerSwing, char), rnd(1, 40));
    }
    if (s_randColours) {
        gd::setSeedValue(&GDF(gm, GameManager, m_playerColor, char), rnd(0, 106));
        gd::setSeedValue(&GDF(gm, GameManager, m_playerColor2, char), rnd(0, 106));
    }
});

// ------------------------------------------------------------------ hook locali
namespace {
void (*o_lightning)(void*, cc::Point, cc::Point, cc::Color3B, float, float, int, bool, float);
void h_lightning(void* self, cc::Point a, cc::Point b, cc::Color3B c, float w, float d, int disp, bool flash, float op) {
    if (noPortalLightning.enabled) return;
    o_lightning(self, a, b, c, w, d, disp, flash, op);
}

void (*o_spawnPortalCircle)(void*, cc::Color3B, float);
void h_spawnPortalCircle(void* self, cc::Color3B c, float r) {
    if (noPortalCircle.enabled || noCircles.enabled) return;
    o_spawnPortalCircle(self, c, r);
}

void (*o_playSpawnEffect)(void*);
void h_playSpawnEffect(void* self) {
    if (noRespawnFlash.enabled) return;
    o_playSpawnEffect(self);
}

void (*o_updateSwingFire)(void*);
void h_updateSwingFire(void* self) {
    if (noSwingFire.enabled) return;
    o_updateSwingFire(self);
}

void (*o_triggerShader)(void*, void*);
void h_triggerShader(void* self, void* obj) {
    if (noShaders.enabled && self == gd::playLayer()) return;
    o_triggerShader(self, obj);
}

// No Camera: blocca i trigger della camera
void (*o_staticCam)(void*, cc::Point, bool, bool, bool, float, int, float);
void h_staticCam(void* self, cc::Point p, bool a, bool b, bool c, float t, int e, float r) {
    if (noCamera.enabled && self == gd::playLayer()) return;
    o_staticCam(self, p, a, b, c, t, e, r);
}
void (*o_camOffX)(void*, float, float, int, float, int, int);
void h_camOffX(void* self, float o, float d, int e, float r, int u, int c) {
    if (noCamera.enabled && self == gd::playLayer()) return;
    o_camOffX(self, o, d, e, r, u, c);
}
void (*o_camOffY)(void*, float, float, int, float, int, int);
void h_camOffY(void* self, float o, float d, int e, float r, int u, int c) {
    if (noCamera.enabled && self == gd::playLayer()) return;
    o_camOffY(self, o, d, e, r, u, c);
}
void (*o_screenRot)(void*, float, bool, bool, float, int, float, int, int);
void h_screenRot(void* self, float rot, bool add, bool conv, float d, int e, float r, int u, int c) {
    if (noCamera.enabled && self == gd::playLayer()) return;
    o_screenRot(self, rot, add, conv, d, e, r, u, c);
}

// Stop Triggers on Death
bool deadStop(void* self) {
    if (!stopTriggersOnDeath.enabled || self != gd::playLayer()) return false;
    auto p1 = gd::player1(self);
    return p1 && GDF(p1, PlayerObject, m_isDead, bool);
}
void (*o_moveStep)(void*, float, bool);
void h_moveStep(void* self, float dt, bool vis) {
    if (deadStop(self)) return;
    o_moveStep(self, dt, vis);
}
void (*o_rotActions)(void*);
void h_rotActions(void* self) {
    if (deadStop(self)) return;
    o_rotActions(self);
}
void (*o_transformActions)(void*, bool);
void h_transformActions(void* self, bool vis) {
    if (deadStop(self)) return;
    o_transformActions(self, vis);
}

// Wave Trail on Death: non fermare la scia dell'onda mentre il giocatore e' morto
void (*o_stopStroke)(void*);
void h_stopStroke(void* self) {
    if (waveTrailOnDeath.enabled) {
        auto pl = gd::playLayer();
        auto p1 = pl ? gd::player1(pl) : nullptr;
        if (p1 && GDF(p1, PlayerObject, m_isDead, bool)) return;
    }
    o_stopStroke(self);
}
} // namespace

MH_INSTALL(cosmetic) {
    using namespace gd;
    hooks::gd("GJBaseGameLayer::lightningFlash", addr::GJBaseGameLayer::lightningFlash, h_lightning, &o_lightning);
    hooks::gd("PlayerObject::spawnPortalCircle", addr::PlayerObject::spawnPortalCircle, h_spawnPortalCircle, &o_spawnPortalCircle);
    hooks::gd("PlayerObject::playSpawnEffect", addr::PlayerObject::playSpawnEffect, h_playSpawnEffect, &o_playSpawnEffect);
    hooks::gd("PlayerObject::updateSwingFire", addr::PlayerObject::updateSwingFire, h_updateSwingFire, &o_updateSwingFire);
    hooks::gd("GJBaseGameLayer::triggerShaderCommand", addr::GJBaseGameLayer::triggerShaderCommand, h_triggerShader, &o_triggerShader);
    hooks::gd("GJBaseGameLayer::updateStaticCameraPos", addr::GJBaseGameLayer::updateStaticCameraPos, h_staticCam, &o_staticCam);
    hooks::gd("GJBaseGameLayer::updateCameraOffsetX", addr::GJBaseGameLayer::updateCameraOffsetX, h_camOffX, &o_camOffX);
    hooks::gd("GJBaseGameLayer::updateCameraOffsetY", addr::GJBaseGameLayer::updateCameraOffsetY, h_camOffY, &o_camOffY);
    hooks::gd("GJBaseGameLayer::updateScreenRotation", addr::GJBaseGameLayer::updateScreenRotation, h_screenRot, &o_screenRot);
    hooks::gd("GJBaseGameLayer::processMoveActionsStep", addr::GJBaseGameLayer::processMoveActionsStep, h_moveStep, &o_moveStep);
    hooks::gd("GJBaseGameLayer::processRotationActions", addr::GJBaseGameLayer::processRotationActions, h_rotActions, &o_rotActions);
    hooks::gd("GJBaseGameLayer::processTransformActions", addr::GJBaseGameLayer::processTransformActions, h_transformActions, &o_transformActions);
    hooks::gd("HardStreak::stopStroke", addr::HardStreak::stopStroke, h_stopStroke, &o_stopStroke);
}
