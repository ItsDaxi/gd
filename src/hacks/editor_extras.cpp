// CREATOR: Accurate Save, Smooth Editor Trail, Multiple Editor Trails, Object Trajectory
#include "common.hpp"

#include <climits>
#include <cstdio>
#include <cstdlib>
#include <deque>

using namespace mh;

namespace {

Hack& H(const char* id) { return hack("CREATOR", id); }

// ============================================================== Accurate Save
// GameObject::getSaveString scrive la posizione con pochi decimali: si riscrivono le chiavi 2 (X)
// e 3 (Y) con la posizione double dell'oggetto (m_positionX/Y).
Hack& accurateSave = H("CREATOR/HACK_ACCURATE_SAVE");

std::string fmtCoord(double v) {
    char buf[48];
    snprintf(buf, sizeof(buf), "%.4f", v);
    std::string s = buf;
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s.empty() || s == "-" ? "0" : s;
}

std::string accurateString(const std::string& in, double x, double y) {
    std::string out;
    out.reserve(in.size() + 16);
    size_t pos = 0;
    int index = 0;
    int key = -1;
    bool changed = false;
    while (pos <= in.size()) {
        size_t end = in.find(',', pos);
        if (end == std::string::npos) end = in.size();
        std::string tok = in.substr(pos, end - pos);
        if (index % 2 == 0) {
            key = std::atoi(tok.c_str());
        } else if (key == 2 || key == 3) {
            double want = key == 2 ? x : y;
            char* e = nullptr;
            double have = std::strtod(tok.c_str(), &e);
            // solo se il valore salvato e' davvero la stessa posizione arrotondata
            if (e && *e == 0 && std::abs(have - want) < 1.0) {
                tok = fmtCoord(want);
                changed = true;
            }
        }
        if (index) out += ',';
        out += tok;
        index++;
        if (end == in.size()) break;
        pos = end + 1;
    }
    return changed ? out : in;
}

void* (*o_saveString)(void*, void*, void*);
void* h_saveString(void* self, void* ret, void* layer) {
    void* r = o_saveString(self, ret, layer);
    if (accurateSave.enabled && r) {
        std::string s = gd::readStdString(r);
        if (!s.empty()) {
            std::string fixed = accurateString(s, GDF(self, GameObject, m_positionX, double),
                                               GDF(self, GameObject, m_positionY, double));
            if (fixed != s) gd::writeStdString(r, fixed);
        }
    }
    return r;
}

// ============================================================== scie dell'editor
// La scia del playtest e' disegnata da DrawGridLayer con i punti in LevelEditorLayer::m_playerPoints,
// aggiunti a 30 fps. Qui si registrano le posizioni a ogni frame e si disegnano con dei CCDrawNode.
Hack& smoothTrail = H("CREATOR/HACK_SMOOTH_EDITOR_TRAIL");
float s_trailTransparency = 50.f;
void clearHistory(int which);
Hack& multiTrails = H("CREATOR/MULTIPLE_EDITOR_TRAILS/HACK_MULTIPLE_EDITOR_TRAILS")
    .optFloat("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/TRANSPARENCY", &s_trailTransparency, 0, 95, "%.0f%%")
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_ALL", [] { clearHistory(0); })
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_LASTEST", [] { clearHistory(1); })
    .optButton("CREATOR/MULTIPLE_EDITOR_TRAILS/DROPDOWN/CLEAR_OLDEST", [] { clearHistory(2); });

constexpr cc::Color4F kTrailColor{0.4f, 1.f, 0.4f, 1.f}; // la linea verde del playtest
using Path = std::vector<cc::Point>;
std::deque<std::vector<Path>> s_history; // ogni playtest: percorso P1 (+ P2)
Path s_cur[2];
void* s_owner = nullptr; // LevelEditorLayer dei nodi
void* s_histNode = nullptr;
void* s_curNode = nullptr;
void* s_objTrajNode = nullptr;
bool s_histDirty = false;
float s_lastTransparency = -1.f;

void clearHistory(int which) {
    if (s_history.empty()) return;
    if (which == 0) s_history.clear();
    else if (which == 1) s_history.pop_back();
    else s_history.pop_front();
    s_histDirty = true;
}

bool ensureEditorNodes(void* lel) {
    if (s_owner == lel && s_histNode && s_curNode && s_objTrajNode) return true;
    s_owner = nullptr;
    void* objLayer = GDF(lel, GJBaseGameLayer, m_objectLayer, void*);
    if (!objLayer) return false;
    s_histNode = cc::drawNodeCreate();
    s_curNode = cc::drawNodeCreate();
    s_objTrajNode = cc::drawNodeCreate();
    if (!s_histNode || !s_curNode || !s_objTrajNode) return false;
    cc::addChild(objLayer, s_histNode, 999);
    cc::addChild(objLayer, s_curNode, 1000);
    cc::addChild(objLayer, s_objTrajNode, 1001);
    s_owner = lel;
    s_histDirty = true;
    return true;
}

void drawPath(void* node, const Path& p, cc::Color4F c) {
    for (size_t i = 1; i < p.size(); i++) cc::drawSegment(node, p[i - 1], p[i], 1.f, c);
}

void redrawHistory() {
    cc::drawNodeClear(s_histNode);
    if (multiTrails.enabled) {
        cc::Color4F c = kTrailColor;
        c.a = 1.f - std::clamp(s_trailTransparency, 0.f, 95.f) / 100.f;
        for (auto& run : s_history)
            for (auto& p : run) drawPath(s_histNode, p, c);
    }
    s_histDirty = false;
    s_lastTransparency = s_trailTransparency;
}

void redrawCurrent() {
    cc::drawNodeClear(s_curNode);
    if (!smoothTrail.enabled) return;
    for (auto& p : s_cur) drawPath(s_curNode, p, kTrailColor);
}

// LevelEditorLayer non e' validato dai size-check: si ritrova m_editorUI a runtime e si usa lo
// stesso scostamento per m_playerPoints (sono vicini nella classe)
intptr_t lelShift(void* lel) {
    static void* cached = nullptr;
    static intptr_t shift = INTPTR_MIN;
    if (lel == cached) return shift;
    void* ui = gd::editorUI();
    if (!ui) return INTPTR_MIN;
    cached = lel;
    shift = INTPTR_MIN;
    namespace lo = gdg::off_unchecked::LevelEditorLayer;
    for (intptr_t d = 0; d <= 0x80 && shift == INTPTR_MIN; d += 8) {
        for (intptr_t s : {d, -d}) {
            if (mem::at<void*>(lel, static_cast<uintptr_t>(lo::m_editorUI + s)) == ui) {
                shift = s;
                break;
            }
        }
    }
    MH_LOG("LevelEditorLayer: scostamento m_editorUI = %lld", static_cast<long long>(shift));
    return shift;
}

struct RawPoints {
    cc::Point* first;
    cc::Point* last;
    cc::Point* end;
};

RawPoints* gdPlayerPoints(void* lel) {
    intptr_t s = lelShift(lel);
    if (s == INTPTR_MIN) return nullptr;
    auto v = &mem::at<RawPoints>(lel, static_cast<uintptr_t>(gdg::off_unchecked::LevelEditorLayer::m_playerPoints + s));
    if (v->first > v->last || v->last > v->end) return nullptr;
    if ((v->first == nullptr) != (v->end == nullptr)) return nullptr;
    return v;
}

int playbackMode(void* lel) { return GDF(lel, GJBaseGameLayer, m_playbackMode, int); }

void record(void* lel) {
    void* players[2] = {gd::player1(lel), gameState<bool>(lel, gd::off::GJGameState::m_isDualMode) ? gd::player2(lel) : nullptr};
    for (int i = 0; i < 2; i++) {
        if (!players[i]) continue;
        cc::Point p = cc::position(players[i]);
        auto& path = s_cur[i];
        if (!path.empty()) {
            const cc::Point& q = path.back();
            float dx = p.x - q.x, dy = p.y - q.y;
            if (dx * dx + dy * dy < 0.25f) continue;
            // salto (teletrasporto, respawn): nuovo tratto non collegato
            if (dx * dx + dy * dy > 300.f * 300.f) path.clear();
        }
        path.push_back(p);
        if (smoothTrail.enabled && path.size() >= 2)
            cc::drawSegment(s_curNode, path[path.size() - 2], path.back(), 1.f, kTrailColor);
    }
}

void (*o_lelPostUpdate)(void*, float);
void h_lelPostUpdate(void* self, float dt) {
    o_lelPostUpdate(self, dt);
    if (!smoothTrail.enabled && !multiTrails.enabled) return;
    if (playbackMode(self) != 1 || !ensureEditorNodes(self)) return;
    record(self);
    // con la scia fluida quella di GD (a 30 fps) viene svuotata per non vederne due
    if (smoothTrail.enabled) {
        if (auto v = gdPlayerPoints(self)) v->last = v->first;
    }
}

void (*o_lelPlaytest)(void*);
void h_lelPlaytest(void* self) {
    // nuovo playtest (non ripresa dalla pausa): il percorso precedente va nella cronologia
    if (playbackMode(self) == 0) {
        if (multiTrails.enabled && (!s_cur[0].empty() || !s_cur[1].empty())) {
            std::vector<Path> run;
            for (auto& p : s_cur)
                if (p.size() >= 2) run.push_back(p);
            if (!run.empty()) s_history.push_back(std::move(run));
            while (s_history.size() > 100) s_history.pop_front();
            s_histDirty = true;
        }
        s_cur[0].clear();
        s_cur[1].clear();
        if (s_curNode && s_owner == self) cc::drawNodeClear(s_curNode);
    }
    o_lelPlaytest(self);
}

// ============================================================== Object Trajectory
// Percorso degli oggetti selezionati in base ai trigger move che puntano ai loro gruppi.
// Ogni trigger parte quando la linea del tempo arriva alla sua X (velocita' iniziale del livello).
Hack& objectTrajectory = H("CREATOR/HACK_OBJECT_TRAJECTORY")
    .partial("Considera i trigger move attivati dalla linea del tempo (non spawn/touch, non 'target' o "
             "'lock to player') con la velocita' iniziale del livello; i trigger rotate non sono simulati");

constexpr int kMoveTrigger = 901;

float easeBounceOut(float t) {
    if (t < 1 / 2.75f) return 7.5625f * t * t;
    if (t < 2 / 2.75f) { t -= 1.5f / 2.75f; return 7.5625f * t * t + 0.75f; }
    if (t < 2.5f / 2.75f) { t -= 2.25f / 2.75f; return 7.5625f * t * t + 0.9375f; }
    t -= 2.625f / 2.75f;
    return 7.5625f * t * t + 0.984375f;
}

// easing come in cocos2d (EasingType di GD: 0 nessuno, 1-3 ease, 4-6 elastic, 7-9 bounce,
// 10-12 exponential, 13-15 sine, 16-18 back; ognuno InOut, In, Out)
float ease(int type, float rate, float t) {
    t = std::clamp(t, 0.f, 1.f);
    const float pi = 3.14159265f;
    int kind = (type - 1) / 3, mode = (type - 1) % 3; // mode: 0 InOut, 1 In, 2 Out
    if (type <= 0 || type > 18) return t;
    auto in = [&](float x) -> float {
        switch (kind) {
        case 0: return std::pow(x, rate > 0 ? rate : 2.f);
        case 1: {
            float period = rate > 0 ? rate : 0.3f;
            if (x <= 0 || x >= 1) return x;
            float s = period / 4.f;
            x -= 1;
            return -std::pow(2.f, 10 * x) * std::sin((x - s) * 2 * pi / period);
        }
        case 2: return 1 - easeBounceOut(1 - x);
        case 3: return x <= 0 ? 0 : std::pow(2.f, 10 * (x - 1));
        case 4: return 1 - std::cos(x * pi / 2);
        default: { const float s = 1.70158f; return x * x * ((s + 1) * x - s); }
        }
    };
    if (mode == 1) return in(t);
    if (mode == 2) return 1 - in(1 - t);
    return t < 0.5f ? in(t * 2) / 2 : 1 - in((1 - t) * 2) / 2;
}

struct MoveInfo {
    float start, duration;
    cc::Point offset;
    int easing;
    float rate;
};

float levelSpeedPx(void* lel) {
    static const float speeds[] = {311.58f, 251.16f, 387.42f, 468.f, 576.f}; // 1x, 0.5x, 2x, 3x, 4x
    int sp = 0;
    if (auto settings = GDF(lel, GJBaseGameLayer, m_levelSettings, void*))
        sp = mem::at<int>(settings, gdg::off_unchecked::LevelSettingsObject::m_startSpeed);
    return speeds[(sp >= 0 && sp <= 4) ? sp : 0];
}

bool hasGroup(void* obj, int group) {
    if (group <= 0) return false;
    auto groups = GDF(obj, GameObject, m_groups, short*);
    int n = GDF(obj, GameObject, m_groupCount, short);
    if (!groups) return false;
    for (int i = 0; i < n && i < 10; i++)
        if (groups[i] == group) return true;
    return false;
}

std::vector<void*> s_moveTriggers;
float s_moveScanTimer = 0.f;

void scanMoveTriggers(void* lel) {
    s_moveTriggers.clear();
    forEach(GDF(lel, GJBaseGameLayer, m_objects, void*), [](void* o) {
        if (o && GDF(o, GameObject, m_objectID, int) == kMoveTrigger) s_moveTriggers.push_back(o);
    });
}

void drawObjectTrajectory(void* lel, void* obj, float speed) {
    std::vector<MoveInfo> moves;
    for (void* t : s_moveTriggers) {
        if (GDF(t, EffectGameObject, m_isSpawnTriggered, bool) || GDF(t, EffectGameObject, m_isTouchTriggered, bool)) continue;
        if (GDF(t, EffectGameObject, m_useMoveTarget, bool) || GDF(t, EffectGameObject, m_lockToPlayerX, bool) ||
            GDF(t, EffectGameObject, m_lockToPlayerY, bool) || GDF(t, EffectGameObject, m_lockToCameraX, bool) ||
            GDF(t, EffectGameObject, m_lockToCameraY, bool))
            continue;
        if (!hasGroup(obj, GDF(t, EffectGameObject, m_targetGroupID, int))) continue;
        cc::Point off = GDF(t, EffectGameObject, m_moveOffset, cc::Point);
        // nel trigger 10 unita' = 1 blocco = 30 punti
        moves.push_back({cc::position(t).x / speed, std::max(0.f, GDF(t, EffectGameObject, m_duration, float)),
                         {off.x * 3.f, off.y * 3.f}, GDF(t, EffectGameObject, m_easingType, int),
                         GDF(t, EffectGameObject, m_easingRate, float)});
    }
    if (moves.empty()) return;
    float t0 = 1e30f, t1 = -1e30f;
    for (auto& m : moves) {
        t0 = std::min(t0, m.start);
        t1 = std::max(t1, m.start + m.duration);
    }
    cc::Point base = cc::position(obj);
    auto at = [&](float t) {
        cc::Point p = base;
        for (auto& m : moves) {
            float k = m.duration <= 0 ? (t >= m.start ? 1.f : 0.f) : ease(m.easing, m.rate, (t - m.start) / m.duration);
            p.x += m.offset.x * k;
            p.y += m.offset.y * k;
        }
        return p;
    };
    int samples = std::clamp(static_cast<int>((t1 - t0) * 60.f), 2, 2000);
    cc::Color4F col{0.3f, 0.85f, 1.f, 0.9f};
    cc::Point prev = at(t0);
    for (int i = 1; i <= samples; i++) {
        cc::Point cur = at(t0 + (t1 - t0) * i / samples);
        cc::drawSegment(s_objTrajNode, prev, cur, 1.f, col);
        prev = cur;
    }
    // fine percorso
    cc::Color4F endCol{1.f, 1.f, 1.f, 0.9f};
    cc::drawSegment(s_objTrajNode, {prev.x - 4, prev.y}, {prev.x + 4, prev.y}, 1.f, endCol);
    cc::drawSegment(s_objTrajNode, {prev.x, prev.y - 4}, {prev.x, prev.y + 4}, 1.f, endCol);
}

void updateObjectTrajectory(void* lel) {
    cc::drawNodeClear(s_objTrajNode);
    if (!objectTrajectory.enabled || playbackMode(lel) != 0) return;
    void* ui = gd::editorUI();
    if (!ui) return;
    s_moveScanTimer -= cc::deltaTime();
    if (s_moveScanTimer <= 0.f) {
        scanMoveTriggers(lel);
        s_moveScanTimer = 0.5f;
    }
    if (s_moveTriggers.empty()) return;
    float speed = levelSpeedPx(lel);
    std::vector<void*> selected;
    if (auto o = GDF(ui, EditorUI, m_selectedObject, void*)) selected.push_back(o);
    forEach(GDF(ui, EditorUI, m_selectedObjects, void*), [&](void* o) {
        if (o && selected.size() < 50) selected.push_back(o);
    });
    for (void* o : selected) drawObjectTrajectory(lel, o, speed);
}

} // namespace

MH_ON(ev::editorInit, [](void*) {
    // nuovo editor: i nodi del precedente non esistono piu'
    s_owner = s_histNode = s_curNode = s_objTrajNode = nullptr;
    s_history.clear();
    s_cur[0].clear();
    s_cur[1].clear();
    s_moveTriggers.clear();
});

MH_EVERY_FRAME(editorExtras) {
    void* lel = gd::editorLayer();
    if (!lel) return;
    bool any = smoothTrail.enabled || multiTrails.enabled || objectTrajectory.enabled;
    if (!any && !s_owner) return;
    if (!ensureEditorNodes(lel)) return;
    static bool lastSmooth = false, lastMulti = false;
    if (lastSmooth != smoothTrail.enabled) redrawCurrent();
    if (s_histDirty || lastMulti != multiTrails.enabled || s_lastTransparency != s_trailTransparency) redrawHistory();
    lastSmooth = smoothTrail.enabled;
    lastMulti = multiTrails.enabled;
    updateObjectTrajectory(lel);
}

MH_INSTALL(editorExtras) {
    using namespace gd;
    hooks::gd("GameObject::getSaveString", addr::GameObject::getSaveString, h_saveString, &o_saveString);
    hooks::gd("LevelEditorLayer::postUpdate", addr::LevelEditorLayer::postUpdate, h_lelPostUpdate, &o_lelPostUpdate);
    hooks::gd("LevelEditorLayer::onPlaytest", addr::LevelEditorLayer::onPlaytest, h_lelPlaytest, &o_lelPlaytest);
}
