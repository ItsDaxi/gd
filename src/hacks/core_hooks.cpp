// Hook centrali sulle funzioni di Geometry Dash.
// Ogni hook emette eventi (hacks/events.hpp) a cui si iscrivono le funzioni del menu.
#include "events.hpp"
#include "registry.hpp"
#include "core/hooks.hpp"
#include "core/log.hpp"
#include "gd/cocos.hpp"
#include "gd/gd.hpp"

#include <atomic>
#include <vector>

namespace gd {
extern std::atomic<void*> g_fmodEngine;
extern std::atomic<void*> g_editorUI;
}

namespace mh::gui {
void* g_editorLayerForKeys() { return gd::editorLayer(); }
} // namespace mh::gui

using namespace mh;

namespace mh {
bool g_simulating = false;
bool g_simDied = false;
bool g_inDestroyPlayer = false;
} // namespace mh

namespace {

// ============================================================== PlayLayer
bool (*o_plInit)(void*, void*, bool);
bool h_plInit(void* self, void* level, bool dontCreate) {
    g_cheatedThisAttempt = anyCheatEnabled();
    bool r = o_plInit(self, level, dontCreate);
    if (r) ev::plInit()(self);
    return r;
}

void (*o_plStartGame)(void*);
void h_plStartGame(void* self) {
    o_plStartGame(self);
    ev::plStartGame()(self);
}

void (*o_plPostUpdate)(void*, float);
void h_plPostUpdate(void* self, float dt) {
    o_plPostUpdate(self, dt);
    ev::plPostUpdate()(self, dt);
}

void (*o_plResetLevel)(void*);
void h_plResetLevel(void* self) {
    ev::plPreReset()(self);
    o_plResetLevel(self);
    g_cheatedThisAttempt = anyCheatEnabled();
    ev::plReset()(self);
}

void (*o_plDestroyPlayer)(void*, void*, void*);
void h_plDestroyPlayer(void* self, void* player, void* obj) {
    if (g_simulating) {
        g_simDied = true;
        return;
    }
    // lo spike anti-cheat di GD deve sempre poter "uccidere" il giocatore
    void* anticheat = GDF(self, GJBaseGameLayer, m_anticheatSpike, void*);
    if (obj && obj == anticheat) {
        o_plDestroyPlayer(self, player, obj);
        return;
    }
    bool allow = true;
    ev::plDestroyPlayer()(self, player, obj, allow);
    if (!allow) {
        ev::plNoclipDeath()(self, player);
        return;
    }
    g_inDestroyPlayer = true;
    o_plDestroyPlayer(self, player, obj);
    g_inDestroyPlayer = false;
    if (player && GDF(player, PlayerObject, m_isDead, bool)) ev::plDeath()(self, player);
}

void (*o_plLevelComplete)(void*);
void h_plLevelComplete(void* self) {
    if (g_simulating) return;
    bool allow = true;
    ev::plPreComplete()(self, allow);
    if (!allow) return;
    o_plLevelComplete(self);
    ev::plComplete()(self);
}

void (*o_plOnQuit)(void*);
void h_plOnQuit(void* self) {
    ev::plQuit()(self);
    o_plOnQuit(self);
}

void (*o_plPauseGame)(void*, bool);
void h_plPauseGame(void* self, bool unfocused) {
    bool allow = true;
    ev::plPause()(self, allow);
    if (allow) o_plPauseGame(self, unfocused);
}

void (*o_plTogglePractice)(void*, bool);
void h_plTogglePractice(void* self, bool on) {
    o_plTogglePractice(self, on);
    ev::plTogglePractice()(self, on);
}

void (*o_plShowNewBest)(void*, bool, int, int, bool, bool, bool);
void h_plShowNewBest(void* self, bool a, int b, int c, bool d, bool e, bool f) {
    bool allow = true;
    ev::plNewBest()(self, allow);
    if (allow) o_plShowNewBest(self, a, b, c, d, e, f);
}

bool (*o_plShouldDrawHitboxes)(void*);
bool h_plShouldDrawHitboxes(void* self) {
    bool draw = o_plShouldDrawHitboxes(self);
    ev::plDrawHitboxes()(self, draw);
    return draw;
}

void (*o_plUpdateProgressbar)(void*);
void h_plUpdateProgressbar(void* self) {
    o_plUpdateProgressbar(self);
    ev::plUpdateProgress()(self);
}

void (*o_plUpdateAttempts)(void*);
void h_plUpdateAttempts(void* self) {
    o_plUpdateAttempts(self);
    ev::plUpdateAttempts()(self);
}

void (*o_plStoreCheckpoint)(void*, void*);
void h_plStoreCheckpoint(void* self, void* cp) {
    if (g_simulating) return;
    o_plStoreCheckpoint(self, cp);
    ev::plCheckpoint()(self, cp);
}

void (*o_plEndAnim)(void*, cc::Point);
void h_plEndAnim(void* self, cc::Point pos) {
    if (g_simulating) return;
    bool allow = true;
    ev::plLevelEndAnim()(self, allow);
    if (allow) o_plEndAnim(self, pos);
}

// ============================================================== GJBaseGameLayer
void (*o_bglUpdate)(void*, float);
void h_bglUpdate(void* self, float dt) {
    bool run = true;
    ev::bglPreUpdate()(self, dt, run);
    if (!run) return;
    ev::bglTick()(self);
    o_bglUpdate(self, dt);
    ev::bglPostUpdate()(self, dt);
}

void (*o_bglHandleButton)(void*, bool, int, bool);
void h_bglHandleButton(void* self, bool down, int button, bool p1) {
    bool allow = true;
    ev::bglButton()(self, down, button, p1, allow);
    if (allow) o_bglHandleButton(self, down, button, p1);
}

struct RawVector {
    void** first;
    void** last;
    void** end;
};

void (*o_bglCollisionCheck)(void*, void*, void*, int, float);
void h_bglCollisionCheck(void* self, void* player, void* vec, int count, float dt) {
    if (g_simulating || ev::bglCollisionPre().empty() || !vec) {
        o_bglCollisionCheck(self, player, vec, count, dt);
        return;
    }
    static thread_local std::vector<void*> buf;
    auto* v = static_cast<RawVector*>(vec);
    size_t size = static_cast<size_t>(v->last - v->first);
    if (count < 0 || static_cast<size_t>(count) > size) count = static_cast<int>(size);
    buf.assign(v->first, v->first + size);
    int n = count;
    ev::bglCollisionPre()(self, player, buf.data(), n);
    RawVector fake{buf.data(), buf.data() + n, buf.data() + buf.size()};
    o_bglCollisionCheck(self, player, &fake, n, dt);
    ev::bglCollisionPost()(self, player, buf.data(), n);
}

void (*o_bglShake)(void*, float, float, float);
void h_bglShake(void* self, float a, float b, float c) {
    if (g_simulating) return;
    bool allow = true;
    ev::bglShake()(self, allow);
    if (allow) o_bglShake(self, a, b, c);
}

void (*o_bglUpdateZoom)(void*, float, float, int, float, int, int);
void h_bglUpdateZoom(void* self, float zoom, float dur, int easing, float rate, int uid, int cid) {
    if (g_simulating) return;
    bool allow = true;
    ev::bglZoom()(self, zoom, allow);
    if (allow) o_bglUpdateZoom(self, zoom, dur, easing, rate, uid, cid);
}

void (*o_bglToggleFlipped)(void*, bool, bool);
void h_bglToggleFlipped(void* self, bool flip, bool noEffects) {
    if (g_simulating) return;
    bool allow = true;
    ev::bglFlip()(self, flip, allow);
    if (allow) o_bglToggleFlipped(self, flip, noEffects);
}

void (*o_bglUpdateShader)(void*, float);
void h_bglUpdateShader(void* self, float dt) {
    bool allow = true;
    ev::bglShader()(self, allow);
    if (allow) o_bglUpdateShader(self, dt);
}

void (*o_bglUpdateCamera)(void*, float);
void h_bglUpdateCamera(void* self, float dt) {
    bool allow = true;
    ev::bglCamera()(self, allow);
    if (allow) o_bglUpdateCamera(self, dt);
}

void (*o_bglToggleGroup)(void*, int, bool);
void h_bglToggleGroup(void* self, int id, bool on) {
    if (g_simulating) return;
    bool allow = true;
    ev::bglToggleGroup()(self, id, on, allow);
    if (allow) o_bglToggleGroup(self, id, on);
}

// ============================================================== PlayerObject
void (*o_poUpdate)(void*, float);
void h_poUpdate(void* self, float dt) {
    if (g_simulating) return o_poUpdate(self, dt);
    ev::poPreUpdate()(self, dt);
    o_poUpdate(self, dt);
    ev::poPostUpdate()(self, dt);
}

bool (*o_poPush)(void*, int);
bool h_poPush(void* self, int button) {
    if (g_simulating) return o_poPush(self, button);
    bool allow = true;
    ev::poPushButton()(self, button, allow);
    return allow ? o_poPush(self, button) : false;
}

bool (*o_poRelease)(void*, int);
bool h_poRelease(void* self, int button) {
    if (g_simulating) return o_poRelease(self, button);
    bool allow = true;
    ev::poReleaseButton()(self, button, allow);
    return allow ? o_poRelease(self, button) : false;
}

void (*o_poDeathEffect)(void*);
void h_poDeathEffect(void* self) {
    if (g_simulating) return;
    bool allow = true;
    ev::poDeathEffect()(self, allow);
    if (allow) o_poDeathEffect(self);
}

void (*o_poGhost)(void*, int);
void h_poGhost(void* self, int type) {
    ev::poGhost()(self, type);
    o_poGhost(self, type);
}

void (*o_poSpiderDash)(void*, cc::Point, cc::Point);
void h_poSpiderDash(void* self, cc::Point a, cc::Point b) {
    if (g_simulating) return;
    bool allow = true;
    ev::poSpiderDash()(self, allow);
    if (allow) o_poSpiderDash(self, a, b);
}

void (*o_poActivateStreak)(void*);
void h_poActivateStreak(void* self) {
    if (g_simulating) return;
    bool allow = true;
    ev::poStreak()(self, allow);
    if (allow) o_poActivateStreak(self);
}

void (*o_poFadeStreak)(void*, float);
void h_poFadeStreak(void* self, float d) {
    if (g_simulating) return;
    bool allow = true;
    ev::poFadeStreak()(self, allow);
    if (allow) o_poFadeStreak(self, d);
}

void (*o_poRingJump)(void*, void*, bool);
void h_poRingJump(void* self, void* ring, bool skip) {
    if (g_simulating) return o_poRingJump(self, ring, skip);
    bool allow = true;
    ev::poRingJump()(self, ring, allow);
    if (allow) o_poRingJump(self, ring, skip);
}

// HardStreak::updateStroke viene chiamato da GJBaseGameLayer::update dopo PlayerObject::update
// (che ricalcola m_pulseSize dalla musica) e prima di postUpdate: e' l'unico punto in cui
// cambiare pulse e spessore ha effetto sul disegno
void (*o_hsUpdateStroke)(void*, float);
void h_hsUpdateStroke(void* self, float dt) {
    if (!g_simulating) ev::waveStroke()(self);
    o_hsUpdateStroke(self, dt);
}

// ============================================================== vari
bool (*o_circleInit)(void*, float, float, float, bool, bool);
bool h_circleInit(void* self, float a, float b, float c, bool d, bool e) {
    bool r = o_circleInit(self, a, b, c, d, e);
    if (r) ev::circleWave()(self);
    return r;
}

bool (*o_isItemUnlocked)(void*, int, int);
bool h_isItemUnlocked(void* self, int type, int id) {
    bool r = o_isItemUnlocked(self, type, id);
    ev::itemUnlocked()(type, id, r);
    return r;
}

bool (*o_isIconUnlocked)(void*, int, int);
bool h_isIconUnlocked(void* self, int id, int type) {
    bool r = o_isIconUnlocked(self, id, type);
    ev::iconUnlocked()(id, type, r);
    return r;
}

bool (*o_isColorUnlocked)(void*, int, int);
bool h_isColorUnlocked(void* self, int id, int type) {
    bool r = o_isColorUnlocked(self, id, type);
    ev::colorUnlocked()(id, type, r);
    return r;
}

void (*o_pauseSetup)(void*);
void h_pauseSetup(void* self) {
    ev::pauseSetupPre()(self);
    o_pauseSetup(self);
    ev::pauseSetup()(self);
}

void (*o_endLevelSetup)(void*);
void h_endLevelSetup(void* self) {
    o_endLevelSetup(self);
    ev::endLevelSetup()(self);
}

bool (*o_uiLayerInit)(void*, void*);
bool h_uiLayerInit(void* self, void* layer) {
    bool r = o_uiLayerInit(self, layer);
    if (r) ev::uiLayerInit()(self);
    return r;
}

bool (*o_menuLayerInit)(void*);
bool h_menuLayerInit(void* self) {
    bool r = o_menuLayerInit(self);
    if (r) ev::menuLayerInit()(self);
    return r;
}

void (*o_menuGameUpdate)(void*, float);
void h_menuGameUpdate(void* self, float dt) {
    o_menuGameUpdate(self, dt);
    ev::menuGameUpdate()(self, dt);
}

bool (*o_editorUIInit)(void*, void*);
bool h_editorUIInit(void* self, void* lel) {
    bool r = o_editorUIInit(self, lel);
    if (r) {
        gd::g_editorUI.store(self);
        ev::editorInit()(self);
    }
    return r;
}

void (*o_fmodUpdate)(void*, float);
void h_fmodUpdate(void* self, float dt) {
    gd::g_fmodEngine.store(self);
    o_fmodUpdate(self, dt);
    ev::fmodUpdate()(self);
}

void (*o_fadeOutMusic)(void*, float, int);
void h_fadeOutMusic(void* self, float d, int ch) {
    bool allow = true;
    ev::musicFade()(allow);
    if (allow) o_fadeOutMusic(self, d, ch);
}

// ============================================================== cocos2d
void (*o_schedUpdate)(void*, float);
void h_schedUpdate(void* self, float dt) {
    ev::schedulerUpdate()(dt);
    o_schedUpdate(self, dt);
}

void* (*o_fadeCreate)(float, void*);
thread_local bool t_inFade = false;
void* h_fadeCreate(float duration, void* scene) {
    if (t_inFade) return o_fadeCreate(duration, scene);
    int type = 0;
    ev::transition()(duration, type);
    if (type == 0) return o_fadeCreate(duration, scene);
    t_inFade = true;
    void* r = cc::transitionCreate(type, duration, scene);
    t_inFade = false;
    return r ? r : o_fadeCreate(duration, scene);
}

void (*o_particleDraw)(void*);
void h_particleDraw(void* self) {
    bool allow = true;
    ev::particleDraw()(self, allow);
    if (allow) o_particleDraw(self);
}

} // namespace

MH_INSTALL(core) {
    using namespace gd;
    hooks::gd("PlayLayer::init", addr::PlayLayer::init, h_plInit, &o_plInit);
    hooks::gd("PlayLayer::startGame", addr::PlayLayer::startGame, h_plStartGame, &o_plStartGame);
    hooks::gd("PlayLayer::postUpdate", addr::PlayLayer::postUpdate, h_plPostUpdate, &o_plPostUpdate);
    hooks::gd("PlayLayer::resetLevel", addr::PlayLayer::resetLevel, h_plResetLevel, &o_plResetLevel);
    hooks::gd("PlayLayer::destroyPlayer", addr::PlayLayer::destroyPlayer, h_plDestroyPlayer, &o_plDestroyPlayer);
    hooks::gd("PlayLayer::levelComplete", addr::PlayLayer::levelComplete, h_plLevelComplete, &o_plLevelComplete);
    hooks::gd("PlayLayer::onQuit", addr::PlayLayer::onQuit, h_plOnQuit, &o_plOnQuit);
    hooks::gd("PlayLayer::pauseGame", addr::PlayLayer::pauseGame, h_plPauseGame, &o_plPauseGame);
    hooks::gd("PlayLayer::togglePracticeMode", addr::PlayLayer::togglePracticeMode, h_plTogglePractice, &o_plTogglePractice);
    hooks::gd("PlayLayer::showNewBest", addr::PlayLayer::showNewBest, h_plShowNewBest, &o_plShowNewBest);
    hooks::gd("PlayLayer::shouldDrawDebugHitboxes", addr::PlayLayer::shouldDrawDebugHitboxes, h_plShouldDrawHitboxes, &o_plShouldDrawHitboxes);
    hooks::gd("PlayLayer::updateProgressbar", addr::PlayLayer::updateProgressbar, h_plUpdateProgressbar, &o_plUpdateProgressbar);
    hooks::gd("PlayLayer::updateAttempts", addr::PlayLayer::updateAttempts, h_plUpdateAttempts, &o_plUpdateAttempts);
    hooks::gd("PlayLayer::storeCheckpoint", addr::PlayLayer::storeCheckpoint, h_plStoreCheckpoint, &o_plStoreCheckpoint);
    hooks::gd("PlayLayer::playEndAnimationToPos", addr::PlayLayer::playEndAnimationToPos, h_plEndAnim, &o_plEndAnim);

    hooks::gd("GJBaseGameLayer::update", addr::GJBaseGameLayer::update, h_bglUpdate, &o_bglUpdate);
    hooks::gd("GJBaseGameLayer::handleButton", addr::GJBaseGameLayer::handleButton, h_bglHandleButton, &o_bglHandleButton);
    hooks::gd("GJBaseGameLayer::collisionCheckObjects", addr::GJBaseGameLayer::collisionCheckObjects, h_bglCollisionCheck, &o_bglCollisionCheck);
    hooks::gd("GJBaseGameLayer::shakeCamera", addr::GJBaseGameLayer::shakeCamera, h_bglShake, &o_bglShake);
    hooks::gd("GJBaseGameLayer::updateZoom", addr::GJBaseGameLayer::updateZoom, h_bglUpdateZoom, &o_bglUpdateZoom);
    hooks::gd("GJBaseGameLayer::toggleFlipped", addr::GJBaseGameLayer::toggleFlipped, h_bglToggleFlipped, &o_bglToggleFlipped);
    hooks::gd("GJBaseGameLayer::updateShaderLayer", addr::GJBaseGameLayer::updateShaderLayer, h_bglUpdateShader, &o_bglUpdateShader);
    hooks::gd("GJBaseGameLayer::updateCamera", addr::GJBaseGameLayer::updateCamera, h_bglUpdateCamera, &o_bglUpdateCamera);
    hooks::gd("GJBaseGameLayer::toggleGroup", addr::GJBaseGameLayer::toggleGroup, h_bglToggleGroup, &o_bglToggleGroup);

    hooks::gd("PlayerObject::update", addr::PlayerObject::update, h_poUpdate, &o_poUpdate);
    hooks::gd("PlayerObject::pushButton", addr::PlayerObject::pushButton, h_poPush, &o_poPush);
    hooks::gd("PlayerObject::releaseButton", addr::PlayerObject::releaseButton, h_poRelease, &o_poRelease);
    hooks::gd("PlayerObject::playDeathEffect", addr::PlayerObject::playDeathEffect, h_poDeathEffect, &o_poDeathEffect);
    hooks::gd("PlayerObject::toggleGhostEffect", addr::PlayerObject::toggleGhostEffect, h_poGhost, &o_poGhost);
    hooks::gd("PlayerObject::playSpiderDashEffect", addr::PlayerObject::playSpiderDashEffect, h_poSpiderDash, &o_poSpiderDash);
    hooks::gd("PlayerObject::activateStreak", addr::PlayerObject::activateStreak, h_poActivateStreak, &o_poActivateStreak);
    hooks::gd("PlayerObject::fadeOutStreak2", addr::PlayerObject::fadeOutStreak2, h_poFadeStreak, &o_poFadeStreak);
    hooks::gd("PlayerObject::ringJump", addr::PlayerObject::ringJump, h_poRingJump, &o_poRingJump);
    hooks::gd("HardStreak::updateStroke", addr::HardStreak::updateStroke, h_hsUpdateStroke, &o_hsUpdateStroke);

    hooks::gd("CCCircleWave::init", addr::CCCircleWave::init, h_circleInit, &o_circleInit);
    hooks::gd("GameStatsManager::isItemUnlocked", addr::GameStatsManager::isItemUnlocked, h_isItemUnlocked, &o_isItemUnlocked);
    hooks::gd("GameManager::isIconUnlocked", addr::GameManager::isIconUnlocked, h_isIconUnlocked, &o_isIconUnlocked);
    hooks::gd("GameManager::isColorUnlocked", addr::GameManager::isColorUnlocked, h_isColorUnlocked, &o_isColorUnlocked);
    hooks::gd("PauseLayer::customSetup", addr::PauseLayer::customSetup, h_pauseSetup, &o_pauseSetup);
    hooks::gd("EndLevelLayer::customSetup", addr::EndLevelLayer::customSetup, h_endLevelSetup, &o_endLevelSetup);
    hooks::gd("UILayer::init", addr::UILayer::init, h_uiLayerInit, &o_uiLayerInit);
    hooks::gd("MenuLayer::init", addr::MenuLayer::init, h_menuLayerInit, &o_menuLayerInit);
    hooks::gd("MenuGameLayer::update", addr::MenuGameLayer::update, h_menuGameUpdate, &o_menuGameUpdate);
    hooks::gd("EditorUI::init", addr::EditorUI::init, h_editorUIInit, &o_editorUIInit);
    hooks::gd("FMODAudioEngine::update", addr::FMODAudioEngine::update, h_fmodUpdate, &o_fmodUpdate);
    hooks::gd("FMODAudioEngine::fadeOutMusic", addr::FMODAudioEngine::fadeOutMusic, h_fadeOutMusic, &o_fadeOutMusic);

    hooks::exported("libcocos2d.dll", "?update@CCScheduler@cocos2d@@UEAAXM@Z", h_schedUpdate, &o_schedUpdate);
    hooks::exported("libcocos2d.dll", "?create@CCTransitionFade@cocos2d@@SAPEAV12@MPEAVCCScene@2@@Z", h_fadeCreate, &o_fadeCreate);
    hooks::exported("libcocos2d.dll", "?draw@CCParticleSystemQuad@cocos2d@@UEAAXXZ", h_particleDraw, &o_particleDraw);
}
