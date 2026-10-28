// Finestra BYPASS
#include "common.hpp"

#include <cstring>

using namespace mh;

namespace {

Hack& antiKick = hack("BYPASS", "BYPASS/HACK_ANTI_KICK");
Hack& challenge = hack("BYPASS", "BYPASS/HACK_CHALLENGE").partial("Sblocca tramite le variabili di gioco (UGV) e le statistiche");
Hack& keymaster = hack("BYPASS", "BYPASS/HACK_KEYMASTER").partial("Sblocca tramite le variabili di gioco (UGV) e le statistiche");
Hack& mainLevels = hack("BYPASS", "BYPASS/HACK_MAIN_LEVELS");
Hack& musicUnlocker = hack("BYPASS", "BYPASS/HACK_MUSIC_UNLOCKER");
Hack& sliderLimit = hack("BYPASS", "BYPASS/HACK_SLIDER_LIMIT").partial("Non disponibile: il limite e' applicato dentro SliderTouchLogic");
Hack& textLength = hack("BYPASS", "BYPASS/HACK_TEXT_LENGTH");
Hack& treasureRoom = hack("BYPASS", "BYPASS/HACK_TREASURE_ROOM").partial("Sblocca tramite le variabili di gioco (UGV) e le statistiche");
Hack& unlockIcons = hack("BYPASS", "BYPASS/HACK_UNLOCK_ICONS");
Hack& unlockShops = hack("BYPASS", "BYPASS/HACK_UNLOCK_SHOPS").partial("Mostra statistiche (monete) sufficienti ad aprire i negozi; gli orbs non vengono toccati");
Hack& unlockVaults = hack("BYPASS", "BYPASS/HACK_UNLOCK_VAULTS").partial("Sblocca tramite le variabili di gioco (UGV) e le statistiche");

// --- Anti-Kick: GD esce dai livelli "hackerati" se questa funzione restituisce true
bool (*o_shouldExit)(void*);
bool h_shouldExit(void* self) {
    if (antiKick.enabled) return false;
    return o_shouldExit(self);
}

// --- statistiche "finte" per superare i requisiti di sblocco (non vengono salvate)
int (*o_getStat)(void*, const char*);
int h_getStat(void* self, const char* key) {
    int v = o_getStat(self, key);
    if (!key) return v;
    if (mainLevels.enabled && std::strcmp(key, "6") == 0) return std::max(v, 9999);                 // stelle
    if ((unlockShops.enabled || unlockVaults.enabled) && (std::strcmp(key, "8") == 0 || std::strcmp(key, "12") == 0))
        return std::max(v, 9999);                                                                    // monete
    if ((unlockVaults.enabled || treasureRoom.enabled) && std::strcmp(key, "13") == 0) return std::max(v, 9999); // diamanti
    if ((keymaster.enabled || treasureRoom.enabled) && std::strcmp(key, "21") == 0) return std::max(v, 999);     // chiavi demon
    return v;
}

// --- variabili utente: porte/stanze segrete
bool (*o_getUGV)(void*, const char*);
bool h_getUGV(void* self, const char* key) {
    if (challenge.enabled || keymaster.enabled || treasureRoom.enabled || unlockVaults.enabled) return true;
    return o_getUGV(self, key);
}

bool (*o_hasCompletedMain)(void*, int);
bool h_hasCompletedMain(void* self, int id) {
    if (mainLevels.enabled) return true;
    return o_hasCompletedMain(self, id);
}

// --- Text Length: niente limite di caratteri
bool (*o_textInit)(void*, float, float, const char*, const char*, int, const char*);
bool h_textInit(void* self, float w, float h, const char* ph, const char* tf, int fs, const char* lf) {
    bool r = o_textInit(self, w, h, ph, tf, fs, lf);
    if (r && textLength.enabled) GDF(self, CCTextInputNode, m_maxLabelLength, int) = 0x7FFFFFFF;
    return r;
}

} // namespace

MH_ON(ev::itemUnlocked, [](int type, int id, bool& r) {
    if (unlockIcons.enabled && type != static_cast<int>(gd::UnlockType::GJItem)) r = true;
    if (musicUnlocker.enabled && type == static_cast<int>(gd::UnlockType::GJItem) && (id == 16 || id == 17)) r = true;
});
MH_ON(ev::iconUnlocked, [](int, int, bool& r) {
    if (unlockIcons.enabled) r = true;
});
MH_ON(ev::colorUnlocked, [](int, int, bool& r) {
    if (unlockIcons.enabled) r = true;
});

MH_INSTALL(bypass) {
    using namespace gd;
    hooks::gd("GJBaseGameLayer::shouldExitHackedLevel", addr::GJBaseGameLayer::shouldExitHackedLevel, h_shouldExit, &o_shouldExit);
    hooks::gd("GameStatsManager::getStat", addr::GameStatsManager::getStat, h_getStat, &o_getStat);
    hooks::gd("GameManager::getUGV", addr::GameManager::getUGV, h_getUGV, &o_getUGV);
    hooks::gd("GameStatsManager::hasCompletedMainLevel", addr::GameStatsManager::hasCompletedMainLevel, h_hasCompletedMain, &o_hasCompletedMain);
    hooks::gd("CCTextInputNode::init", addr::CCTextInputNode::init, h_textInit, &o_textInit);
}
