// Show Trajectory: disegna il percorso che farebbe il giocatore tenendo premuto o lasciando il tasto.
// Come nei mod Geode (Eclipse, GDH): due PlayerObject finti, nascosti, copiano lo stato del giocatore
// vero e vengono fatti avanzare tick per tick con checkCollisions + update. Durante la simulazione
// (mh::g_simulating) gli hook bloccano tutto cio' che cambierebbe il livello: morte, trigger, monete,
// checkpoint, fine livello, dual, camera, effetti.
#include "common.hpp"

#include <windows.h>

using namespace mh;

namespace {

int s_trajIter = 312;
float s_trajThick = 1.f;
bool s_trajP2 = true;
Hack& showTrajectory = hack("LEVEL", "LEVEL/SHOW_TRAJECTORY/HACK_SHOW_TRAJECTORY")
    .optInt("LEVEL/SHOW_TRAJECTORY/DROPDOWN/ITERATIONS", &s_trajIter, 1, 5000)
    .optFloat("LEVEL/SHOW_TRAJECTORY/DROPDOWN/THICKNESS", &s_trajThick, 0.1f, 5.f, "%.2fx")
    .optBool("Player 2", &s_trajP2)
    .partial("Sperimentale: simula il giocatore con le funzioni di fisica di GD (checkCollisions/update). "
             "I trigger toccati durante la simulazione non vengono attivati, quindi il percorso puo' "
             "divergere dopo trigger, portali di modalita' o teletrasporti");

void* s_layer = nullptr;      // PlayLayer a cui appartengono i nodi qui sotto
void* s_container = nullptr;  // nodo invisibile con i giocatori finti (e le loro particelle)
void* s_fake[2] = {};
void* s_drawNode = nullptr;
void* s_simReal = nullptr;    // giocatore vero che si sta simulando
bool s_crashed = false;       // disattivata per questo livello
int s_failures = 0;           // eccezioni consecutive
bool s_rebuild = false;       // ricreare i giocatori finti (stato corrotto)

void resetNodes() {
    s_layer = s_container = s_drawNode = nullptr;
    s_fake[0] = s_fake[1] = nullptr;
}

// toglie i nodi dal livello (usato quando i giocatori finti vanno ricreati)
void destroyNodes() {
    if (s_container) cc::removeFromParent(s_container);
    if (s_drawNode) cc::removeFromParent(s_drawNode);
    resetNodes();
}

bool ready() {
    using namespace gd::addr;
    return PlayerObject::create && PlayerObject::copyAttributes && PlayerObject::update &&
           PlayerObject::pushButton && PlayerObject::releaseButton && GJBaseGameLayer::checkCollisions;
}

bool ensureNodes(void* pl) {
    if (s_layer == pl && s_drawNode && s_fake[0] && s_fake[1]) return true;
    resetNodes();
    void* objLayer = GDF(pl, GJBaseGameLayer, m_objectLayer, void*);
    if (!objLayer) return false;
    s_container = cc::nodeCreate();
    s_drawNode = cc::drawNodeCreate();
    if (!s_container || !s_drawNode) {
        s_crashed = true;
        return false;
    }
    cc::setVisible(s_container, false);
    cc::addChild(objLayer, s_container, -1000);
    cc::addChild(objLayer, s_drawNode, 1000);
    for (auto& f : s_fake) {
        f = gd::call<void*>(gd::addr::PlayerObject::create, 1, 1, pl, s_container, true);
        if (!f) {
            // non riprovare a ogni frame (i nodi gia' aggiunti resterebbero nel livello)
            s_crashed = true;
            MH_LOG("Show Trajectory: PlayerObject::create non riuscito");
            return false;
        }
        cc::addChild(s_container, f, 0);
    }
    s_layer = pl;
    return true;
}

cc::Color4F withAlpha(cc::Color4F c, float a) {
    c.a *= a;
    return c;
}

struct SimArgs {
    void* pl;
    void* fake;
    void* real;
    bool hold;
    cc::Color4F color;
    bool broken = false; // posizione non valida: lo stato del giocatore finto e' da buttare
};

void drawDeath(cc::Point p, float r) {
    cc::Color4F red{1.f, 0.2f, 0.2f, 1.f};
    float w = 0.8f * s_trajThick;
    cc::drawSegment(s_drawNode, {p.x - r, p.y - r}, {p.x + r, p.y - r}, w, red);
    cc::drawSegment(s_drawNode, {p.x + r, p.y - r}, {p.x + r, p.y + r}, w, red);
    cc::drawSegment(s_drawNode, {p.x + r, p.y + r}, {p.x - r, p.y + r}, w, red);
    cc::drawSegment(s_drawNode, {p.x - r, p.y + r}, {p.x - r, p.y - r}, w, red);
}

void simulate(SimArgs* a) {
    using namespace gd::addr;
    void* fake = a->fake;
    gd::call<void>(PlayerObject::copyAttributes, fake, a->real);
    GDF(fake, PlayerObject, m_position, cc::Point) = GDF(a->real, PlayerObject, m_position, cc::Point);
    cc::setPosition(fake, cc::position(a->real));
    // copyAttributes non copia lo stato del nodo: rotazione e scala restavano quelle della simulazione
    // precedente e si accumulavano
    cc::setRotation(fake, cc::rotation(a->real));
    cc::setScale(fake, cc::scale(a->real));
    GDF(fake, PlayerObject, m_isDead, bool) = false;
    // gli orb toccati nella simulazione precedente non devono restare "toccati"
    if (void* rings = GDF(fake, PlayerObject, m_touchingRings, void*))
        cc::call<void>("?removeAllObjects@CCArray@cocos2d@@QEAAXXZ", rings);
    if (a->hold) gd::call<bool>(PlayerObject::pushButton, fake, 1);
    else gd::call<bool>(PlayerObject::releaseButton, fake, 1);

    const float dt = 0.25f; // un tick a 240 Hz, in unita' di 1/60 s come fa GD
    int n = std::clamp(s_trajIter, 1, 5000);
    float radius = 0.6f * s_trajThick;
    for (int i = 0; i < n; i++) {
        cc::Point prev = cc::position(fake);
        g_simDied = false;
        gd::call<int>(GJBaseGameLayer::checkCollisions, a->pl, fake, dt, false);
        if (g_simDied) {
            drawDeath(prev, 10.f);
            break;
        }
        gd::call<void>(PlayerObject::update, fake, dt);
        if (PlayerObject::updateRotation) gd::call<void>(PlayerObject::updateRotation, fake, dt);
        cc::Point cur = cc::position(fake);
        if (!std::isfinite(cur.x) || !std::isfinite(cur.y) || std::abs(cur.x - prev.x) > 1000.f || std::abs(cur.y - prev.y) > 1000.f) {
            a->broken = true;
            break;
        }
        // sfuma l'ultimo tratto
        float fade = n - i < 40 ? (n - i) / 40.f : 1.f;
        cc::drawSegment(s_drawNode, prev, cur, radius, withAlpha(a->color, fade));
    }
}

#ifdef _MSC_VER
// una lettura sbagliata durante la simulazione non deve chiudere il gioco; codice e indirizzo
// finiscono nel log per capire quale funzione di GD ha dato problemi
DWORD s_excCode = 0;
uintptr_t s_excAddr = 0;
int excFilter(EXCEPTION_POINTERS* e) {
    s_excCode = e->ExceptionRecord->ExceptionCode;
    s_excAddr = reinterpret_cast<uintptr_t>(e->ExceptionRecord->ExceptionAddress);
    return EXCEPTION_EXECUTE_HANDLER;
}
bool guardedSimulate(SimArgs* a) {
    __try {
        simulate(a);
        return true;
    } __except (excFilter(GetExceptionInformation())) {
        return false;
    }
}
#else
bool guardedSimulate(SimArgs* a) {
    simulate(a);
    return true;
}
#endif

void runPlayer(void* pl, int fakeIdx, void* real) {
    if (!real || GDF(real, PlayerObject, m_isDead, bool)) return;
    s_simReal = real;
    SimArgs hold{pl, s_fake[fakeIdx], real, true, {0.3f, 1.f, 0.3f, 1.f}};
    SimArgs release{pl, s_fake[fakeIdx], real, false, {1.f, 0.55f, 0.2f, 1.f}};
    g_simulating = true;
    bool ok = guardedSimulate(&hold) && guardedSimulate(&release);
    g_simulating = false;
    g_simDied = false;
    s_simReal = nullptr;
    if (hold.broken || release.broken) {
        MH_LOG("Show Trajectory: posizione non valida nella simulazione, giocatori finti ricreati");
        s_rebuild = true;
    }
    if (ok) {
        s_failures = 0;
        return;
    }
    s_rebuild = true;
#ifdef _MSC_VER
    uintptr_t base = mem::base();
    MH_LOG("Show Trajectory: eccezione 0x%08lX a %p (GeometryDash.exe+0x%llX)", static_cast<unsigned long>(s_excCode),
           reinterpret_cast<void*>(s_excAddr),
           static_cast<unsigned long long>(s_excAddr >= base ? s_excAddr - base : 0));
#endif
    if (++s_failures >= 10) {
        s_crashed = true;
        MH_LOG("Show Trajectory: troppe eccezioni, disattivata fino al prossimo livello");
    }
}

// ------------------------------------------------------------- hook attivi solo durante la simulazione
void (*o_dualMode)(void*, void*, bool, void*, bool);
void h_dualMode(void* self, void* obj, bool dual, void* player, bool noEffects) {
    if (!g_simulating) o_dualMode(self, obj, dual, player, noEffects);
}
void (*o_willSwitch)(void*, void*, void*);
void h_willSwitch(void* self, void* player, void* obj) {
    if (!g_simulating) o_willSwitch(self, player, obj);
}
void (*o_gameEvent)(void*, int, int, int);
void h_gameEvent(void* self, int e, int material, int playerID) {
    if (!g_simulating) o_gameEvent(self, e, material, playerID);
}
void (*o_sfxTrigger)(void*, void*);
void h_sfxTrigger(void* self, void* obj) {
    if (!g_simulating) o_sfxTrigger(self, obj);
}
void (*o_destroyObject)(void*, void*);
void h_destroyObject(void* self, void* obj) {
    if (!g_simulating) o_destroyObject(self, obj);
}
void (*o_touchedTrigger)(void*, void*, void*);
void h_touchedTrigger(void* self, void* player, void* obj) {
    if (!g_simulating) o_touchedTrigger(self, player, obj);
}
void (*o_spawnGroup)(void*, int, bool, double, const void*, int, int);
void h_spawnGroup(void* self, int group, bool ordered, double delay, const void* remap, int trig, int ctrl) {
    if (!g_simulating) o_spawnGroup(self, group, ordered, delay, remap, trig, ctrl);
}
void (*o_checkpointActivated)(void*, void*);
void h_checkpointActivated(void* self, void* obj) {
    if (!g_simulating) o_checkpointActivated(self, obj);
}
void (*o_triggerObject)(void*, void*, int, const void*);
void h_triggerObject(void* self, void* layer, int uid, const void* remap) {
    if (!g_simulating) o_triggerObject(self, layer, uid, remap);
}
void (*o_activatedBy)(void*, void*);
void h_activatedBy(void* self, void* player) {
    if (!g_simulating) o_activatedBy(self, player);
}
bool (*o_hasBeenActivatedBy)(void*, void*);
bool h_hasBeenActivatedBy(void* self, void* player) {
    // il giocatore finto vede gli oggetti come li vede quello vero
    if (g_simulating && s_simReal) return o_hasBeenActivatedBy(self, s_simReal);
    return o_hasBeenActivatedBy(self, player);
}
void (*o_incrementJumps)(void*);
void h_incrementJumps(void* self) {
    if (!g_simulating) o_incrementJumps(self);
}

} // namespace

MH_ON(ev::plInit, [](void*) {
    resetNodes();
    s_crashed = false;
    s_failures = 0;
    s_rebuild = false;
});
MH_ON(ev::plQuit, [](void*) { resetNodes(); });

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (s_layer == pl && s_drawNode) cc::drawNodeClear(s_drawNode);
    if (!showTrajectory.enabled || s_crashed || !ready()) return;
    if (GDF(pl, PlayLayer, m_isPaused, bool)) return;
    if (s_rebuild && s_layer == pl) {
        destroyNodes();
        s_rebuild = false;
    }
    if (!ensureNodes(pl)) return;
    runPlayer(pl, 0, gd::player1(pl));
    if (s_trajP2 && gameState<bool>(pl, gd::off::GJGameState::m_isDualMode)) runPlayer(pl, 1, gd::player2(pl));
});

MH_INSTALL(trajectory) {
    using namespace gd;
    hooks::gd("GJBaseGameLayer::toggleDualMode", addr::GJBaseGameLayer::toggleDualMode, h_dualMode, &o_dualMode);
    hooks::gd("GJBaseGameLayer::playerWillSwitchMode", addr::GJBaseGameLayer::playerWillSwitchMode, h_willSwitch, &o_willSwitch);
    hooks::gd("GJBaseGameLayer::gameEventTriggered", addr::GJBaseGameLayer::gameEventTriggered, h_gameEvent, &o_gameEvent);
    hooks::gd("GJBaseGameLayer::activateSFXTrigger", addr::GJBaseGameLayer::activateSFXTrigger, h_sfxTrigger, &o_sfxTrigger);
    hooks::gd("GJBaseGameLayer::destroyObject", addr::GJBaseGameLayer::destroyObject, h_destroyObject, &o_destroyObject);
    hooks::gd("GJBaseGameLayer::playerTouchedTrigger", addr::GJBaseGameLayer::playerTouchedTrigger, h_touchedTrigger, &o_touchedTrigger);
    hooks::gd("GJBaseGameLayer::spawnGroup", addr::GJBaseGameLayer::spawnGroup, h_spawnGroup, &o_spawnGroup);
    hooks::gd("PlayLayer::checkpointActivated", addr::PlayLayer::checkpointActivated, h_checkpointActivated, &o_checkpointActivated);
    hooks::gd("EffectGameObject::triggerObject", addr::EffectGameObject::triggerObject, h_triggerObject, &o_triggerObject);
    hooks::gd("EnhancedGameObject::activatedByPlayer", addr::EnhancedGameObject::activatedByPlayer, h_activatedBy, &o_activatedBy);
    hooks::gd("EnhancedGameObject::hasBeenActivatedByPlayer", addr::EnhancedGameObject::hasBeenActivatedByPlayer, h_hasBeenActivatedBy, &o_hasBeenActivatedBy);
    hooks::gd("PlayerObject::incrementJumps", addr::PlayerObject::incrementJumps, h_incrementJumps, &o_incrementJumps);
}
