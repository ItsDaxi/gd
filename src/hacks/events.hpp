#pragma once
// Eventi emessi dagli hook centrali (hacks/core_hooks.cpp).
// Ogni funzione di gioco viene agganciata una sola volta (MinHook non permette
// due hook sullo stesso indirizzo); le varie funzioni del menu si iscrivono qui.

#include <vector>

#include "registry.hpp"

namespace mh::ev {

template <class... A>
struct Event {
    std::vector<void (*)(A...)> subs;
    void add(void (*f)(A...)) { subs.push_back(f); }
    void operator()(A... a) const {
        for (auto f : subs) f(a...);
    }
    bool empty() const { return subs.empty(); }
};

#define MH_EVENT(name, ...)                     \
    inline Event<__VA_ARGS__>& name() {         \
        static Event<__VA_ARGS__> e;            \
        return e;                               \
    }

// Iscrizione statica: MH_ON(ev::plReset, [](void* pl) { ... });
struct Sub {
    template <class E, class F>
    Sub(E& e, F f) {
        e.add(+f);
    }
};

// ------------------------------------------------------------- PlayLayer
MH_EVENT(plInit, void* /*pl*/)                       // dopo PlayLayer::init
MH_EVENT(plStartGame, void*)                         // dopo startGame
MH_EVENT(plPostUpdate, void*, float)                 // dopo PlayLayer::postUpdate
MH_EVENT(plPreReset, void*)                          // prima di resetLevel
MH_EVENT(plReset, void*)                             // dopo resetLevel
MH_EVENT(plDestroyPlayer, void*, void* /*player*/, void* /*obj*/, bool& /*allow*/)
MH_EVENT(plDeath, void*, void* /*player*/)           // il giocatore e' morto davvero
MH_EVENT(plNoclipDeath, void*, void* /*player*/)     // morte evitata (noclip)
MH_EVENT(plPreComplete, void*, bool& /*allow*/)      // prima di levelComplete
MH_EVENT(plComplete, void*)
MH_EVENT(plQuit, void*)
MH_EVENT(plPause, void*, bool& /*allow*/)
MH_EVENT(plTogglePractice, void*, bool)
MH_EVENT(plCheckpoint, void*, void* /*checkpoint*/)
MH_EVENT(plNewBest, void*, bool& /*allow*/)
MH_EVENT(plDrawHitboxes, void*, bool& /*draw*/)
MH_EVENT(plUpdateProgress, void*)
MH_EVENT(plUpdateAttempts, void*)
MH_EVENT(plLevelEndAnim, void*, bool& /*allow*/)

// ------------------------------------------------------------- GJBaseGameLayer
MH_EVENT(bglPreUpdate, void*, float& /*dt*/, bool& /*run*/)
MH_EVENT(bglTick, void*)                             // update che verra' eseguito davvero
MH_EVENT(bglPostUpdate, void*, float)
MH_EVENT(bglButton, void*, bool /*down*/, int /*button*/, bool /*p1*/, bool& /*allow*/)
MH_EVENT(bglCollisionPre, void*, void* /*player*/, void** /*objs*/, int& /*count*/)
MH_EVENT(bglCollisionPost, void*, void* /*player*/, void** /*objs*/, int /*count*/)
MH_EVENT(bglShake, void*, bool& /*allow*/)
MH_EVENT(bglZoom, void*, float& /*zoom*/, bool& /*allow*/)
MH_EVENT(bglFlip, void*, bool& /*flip*/, bool& /*allow*/)
MH_EVENT(bglShader, void*, bool& /*allow*/)
MH_EVENT(bglCamera, void*, bool& /*allow*/)
MH_EVENT(bglSpawnGroup, void*, int, bool& /*allow*/)
MH_EVENT(bglToggleGroup, void*, int, bool, bool& /*allow*/)

// ------------------------------------------------------------- PlayerObject
MH_EVENT(poPreUpdate, void*, float)
MH_EVENT(poPostUpdate, void*, float)
MH_EVENT(poPushButton, void*, int, bool& /*allow*/)
MH_EVENT(poReleaseButton, void*, int, bool& /*allow*/)
MH_EVENT(poDeathEffect, void*, bool& /*allow*/)
MH_EVENT(poGhost, void*, int& /*type*/)
MH_EVENT(poSpiderDash, void*, bool& /*allow*/)
MH_EVENT(poStreak, void*, bool& /*allow*/)
MH_EVENT(poFadeStreak, void*, bool& /*allow*/)
MH_EVENT(poRingJump, void*, void*, bool& /*allow*/)

// ------------------------------------------------------------- vari
MH_EVENT(circleWave, void* /*wave*/)                 // CCCircleWave creato
MH_EVENT(itemUnlocked, int /*type*/, int /*id*/, bool& /*result*/)
MH_EVENT(iconUnlocked, int /*id*/, int /*type*/, bool& /*result*/)
MH_EVENT(colorUnlocked, int /*id*/, int /*type*/, bool& /*result*/)
MH_EVENT(pauseSetupPre, void* /*pauseLayer*/)
MH_EVENT(pauseSetup, void* /*pauseLayer*/)
MH_EVENT(endLevelSetup, void* /*endLevelLayer*/)
MH_EVENT(uiLayerInit, void* /*uiLayer*/)
MH_EVENT(menuLayerInit, void*)
MH_EVENT(menuGameUpdate, void*, float)
MH_EVENT(editorInit, void* /*editorUI*/)
MH_EVENT(schedulerUpdate, float& /*dt*/)
MH_EVENT(fmodUpdate, void*)
MH_EVENT(musicFade, bool& /*allow*/)
MH_EVENT(transition, float& /*duration*/, int& /*type*/)
MH_EVENT(particleDraw, void*, bool& /*allow*/)
MH_EVENT(frame) // ogni frame, sul thread principale

} // namespace mh::ev

#define MH_ON(event, ...) static ::mh::ev::Sub MH_CONCAT(mh_sub_, __LINE__)(event(), __VA_ARGS__)
