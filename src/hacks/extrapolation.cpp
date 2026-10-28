// Frame Extrapolation: GD aggiorna la fisica a 240 tick al secondo, quindi sopra i 240 fps alcuni frame
// mostrano la stessa immagine. Qui, dopo ogni GJBaseGameLayer::update, giocatori e camera vengono
// spostati in avanti della frazione di tick non ancora simulata (m_extraDelta) e rimessi al loro
// posto prima dell'update successivo, cosi' la fisica non cambia.
#include "common.hpp"

using namespace mh;

namespace {

Hack& frameExtrapolation = hack("DISPLAY", "DISPLAY/HACK_FRAME_EXTRAPOLATION")
    .partial("Estrapola giocatori e camera; gli altri oggetti in movimento restano a 240 Hz");

constexpr double kTick = 1.0 / 240.0;
constexpr const char* kSetPos = "?setPosition@CCNode@cocos2d@@UEAAXAEBVCCPoint@2@@Z";

// CCNode::setPosition di base: non tocca la posizione "fisica" di PlayerObject
void rawSetPosition(void* node, cc::Point p) { cc::call<void>(kSetPos, node, static_cast<const cc::Point*>(&p)); }

struct Track {
    void* node = nullptr;
    cc::Point last{};     // posizione reale all'ultimo frame
    cc::Point vel{};      // spostamento per tick
    cc::Point applied{};  // posizione reale salvata mentre e' applicata l'estrapolazione
    bool valid = false;
    bool active = false;
};
Track s_tracks[3]; // P1, P2, camera (m_objectLayer)
void* s_layer = nullptr;
int s_ticks = 0;

void restoreAll() {
    for (auto& t : s_tracks) {
        if (t.active && t.node) rawSetPosition(t.node, t.applied);
        t.active = false;
    }
}

void resetAll() {
    restoreAll();
    for (auto& t : s_tracks) t = Track{};
    s_ticks = 0;
}

} // namespace

MH_ON(ev::plInit, [](void*) { resetAll(); s_layer = nullptr; });
MH_ON(ev::plQuit, [](void*) { resetAll(); s_layer = nullptr; });
MH_ON(ev::plPreReset, [](void*) { restoreAll(); });
MH_ON(ev::plReset, [](void*) { resetAll(); });

MH_ON(ev::bglPreUpdate, [](void* layer, float&, bool&) {
    if (layer == s_layer) restoreAll();
});

MH_ON(ev::poPostUpdate, [](void* player, float) {
    if (s_layer && player == gd::player1(s_layer)) s_ticks++;
});

MH_ON(ev::bglPostUpdate, [](void* layer, float) {
    if (!frameExtrapolation.enabled || layer != gd::playLayer()) {
        if (layer == s_layer) resetAll();
        return;
    }
    if (s_layer != layer) {
        resetAll();
        s_layer = layer;
    }
    void* nodes[3] = {gd::player1(layer), gd::player2(layer), GDF(layer, GJBaseGameLayer, m_objectLayer, void*)};
    int ticks = s_ticks;
    s_ticks = 0;
    for (int i = 0; i < 3; i++) {
        auto& t = s_tracks[i];
        if (t.node != nodes[i]) t = Track{};
        t.node = nodes[i];
        if (!t.node) continue;
        cc::Point cur = cc::position(t.node);
        if (ticks > 0) {
            if (t.valid) {
                t.vel = {(cur.x - t.last.x) / ticks, (cur.y - t.last.y) / ticks};
                // salti (respawn, teletrasporti): niente estrapolazione
                if (std::abs(t.vel.x) > 60.f || std::abs(t.vel.y) > 60.f) t.vel = {0, 0};
            }
            t.last = cur;
            t.valid = true;
        }
    }
    bool dead = GDF(nodes[0], PlayerObject, m_isDead, bool);
    if (dead || GDF(layer, PlayLayer, m_isPaused, bool)) return;
    double extra = GDF(layer, GJBaseGameLayer, m_extraDelta, double);
    if (!(extra > 0.0) || extra > 0.1) return;
    float alpha = static_cast<float>(std::min(1.0, extra / kTick));
    bool dual = gameState<bool>(layer, gd::off::GJGameState::m_isDualMode);
    for (int i = 0; i < 3; i++) {
        auto& t = s_tracks[i];
        if (!t.node || !t.valid || (i == 1 && !dual)) continue;
        cc::Point cur = cc::position(t.node);
        t.applied = cur;
        t.active = true;
        rawSetPosition(t.node, {cur.x + t.vel.x * alpha, cur.y + t.vel.y * alpha});
    }
});
