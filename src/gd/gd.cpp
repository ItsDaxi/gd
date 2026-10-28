#include "gd.hpp"
#include "core/log.hpp"

#include <windows.h>
#include <atomic>

namespace gd {

std::atomic<void*> g_fmodEngine{nullptr}; // catturato da hacks/core_hooks (FMODAudioEngine::update)
std::atomic<void*> g_editorUI{nullptr};   // catturato da hacks/core_hooks (EditorUI::init)

void* editorUI() { return editorLayer() ? g_editorUI.load() : nullptr; }

void* gameManager() {
    static void* gm = nullptr;
    if (!gm) gm = call<void*>(addr::GameManager::sharedState);
    return gm;
}

void* gameStats() {
    static void* gs = nullptr;
    if (!gs) gs = call<void*>(addr::GameStatsManager::sharedState);
    return gs;
}

void* fmod() { return g_fmodEngine.load(); }

void* playLayer() {
    auto gm = gameManager();
    return gm ? GDF(gm, GameManager, m_playLayer, void*) : nullptr;
}

void* editorLayer() {
    auto gm = gameManager();
    return gm ? GDF(gm, GameManager, m_levelEditorLayer, void*) : nullptr;
}

void* gameLayer() {
    auto gm = gameManager();
    return gm ? GDF(gm, GameManager, m_gameLayer, void*) : nullptr;
}

void* level() {
    auto l = gameLayer();
    return l ? GDF(l, GJBaseGameLayer, m_level, void*) : nullptr;
}

void* player1(void* layer) { return layer ? GDF(layer, GJBaseGameLayer, m_player1, void*) : nullptr; }
void* player2(void* layer) { return layer ? GDF(layer, GJBaseGameLayer, m_player2, void*) : nullptr; }

bool isPractice() {
    auto l = gameLayer();
    return l && GDF(l, GJBaseGameLayer, m_isPracticeMode, bool);
}

bool isPlatformer() {
    auto l = gameLayer();
    return l && GDF(l, GJBaseGameLayer, m_isPlatformer, bool);
}

bool isDualMode() {
    auto l = gameLayer();
    if (!l) return false;
    return mh::mem::at<bool>(l, off::GJBaseGameLayer::m_gameState + off::GJGameState::m_isDualMode);
}

float percent() {
    auto pl = playLayer();
    if (!pl) return 0.f;
    return call<float>(addr::PlayLayer::getCurrentPercent, pl);
}

double levelTime() {
    auto l = gameLayer();
    if (!l) return 0.0;
    return mh::mem::at<double>(l, off::GJBaseGameLayer::m_gameState + off::GJGameState::m_levelTime);
}

int attempts() {
    auto pl = playLayer();
    return pl ? GDF(pl, GJBaseGameLayer, m_attempts, int) : 0;
}

int seedValue(const void* p) {
    auto v = reinterpret_cast<const int*>(p);
    return v[0] - v[1]; // SeedValueRSV: random - seed
}

void setSeedValue(void* p, int value) {
    auto v = reinterpret_cast<int*>(p);
    v[2] = value;
    v[0] = value + v[1];
}

int levelID(void* lvl) { return lvl ? seedValue(&GDF(lvl, GJGameLevel, m_levelID, char)) : 0; }

std::string levelName(void* lvl) {
    if (!lvl) return {};
    // std::string MSVC: union{buf[16], ptr} + size + capacity
    auto base = reinterpret_cast<const char*>(lvl) + off::GJGameLevel::m_levelName;
    size_t size = *reinterpret_cast<const size_t*>(base + 16);
    size_t cap = *reinterpret_cast<const size_t*>(base + 24);
    const char* data = cap >= 16 ? *reinterpret_cast<const char* const*>(base) : base;
    if (size > 4096) return {};
    return std::string(data, size);
}

int levelNormalPercent(void* lvl) {
    if (!lvl) return 0;
    // SeedValueVSR: value, seed, random
    auto v = reinterpret_cast<const int*>(reinterpret_cast<const char*>(lvl) + off::GJGameLevel::m_normalPercent);
    return v[2] - v[1];
}

bool getGameVar(const char* key) {
    auto gm = gameManager();
    return gm && call<bool>(addr::GameManager::getGameVariable, gm, key);
}

void setGameVar(const char* key, bool v) {
    auto gm = gameManager();
    if (gm) call<void>(addr::GameManager::setGameVariable, gm, key, v);
}

// --- verifica della versione --------------------------------------------
// Ogni indirizzo delle bindings dovrebbe cadere all'inizio di una funzione:
// MSVC allinea le funzioni a 16 byte e riempie lo spazio prima con int3 (0xCC).
static bool looksLikeFunctionStart(uintptr_t rva) {
    auto p = reinterpret_cast<const uint8_t*>(mh::mem::base() + rva);
    if (!mh::mem::readable(p - 1, 4)) return false;
    uint8_t before = p[-1];
    bool aligned = (rva & 0xF) == 0;
    bool padded = before == 0xCC || before == 0xC3 || before == 0x90;
    return aligned && padded;
}

bool versionMatches() {
    static int result = -1;
    if (result >= 0) return result == 1;
    const uintptr_t probes[] = {
        addr::PlayLayer::init,           addr::PlayLayer::destroyPlayer, addr::PlayLayer::resetLevel,
        addr::GJBaseGameLayer::update,   addr::PlayerObject::pushButton, addr::PlayerObject::releaseButton,
        addr::GameManager::sharedState,  addr::PlayLayer::levelComplete, addr::PauseLayer::customSetup,
        addr::EditorUI::init,
    };
    int ok = 0, total = 0;
    for (auto rva : probes) {
        if (!rva) continue;
        total++;
        ok += looksLikeFunctionStart(rva);
    }
    MH_LOG("verifica versione %s: %d/%d funzioni riconosciute", GD_VERSION_STR, ok, total);
    result = total > 0 && ok * 10 >= total * 8 ? 1 : 0; // almeno l'80%
    return result == 1;
}

const char* versionString() { return GD_VERSION_STR; }

} // namespace gd
