// Finestra BYPASS
#include "common.hpp"

#include <cstring>

using namespace mh;

namespace {

Hack& antiKick = hack("BYPASS", "BYPASS/HACK_ANTI_KICK");
Hack& challenge = hack("BYPASS", "BYPASS/HACK_CHALLENGE").partial("Sblocca tramite le variabili di gioco (UGV)");
Hack& keymaster = hack("BYPASS", "BYPASS/HACK_KEYMASTER").partial("Sblocca tramite le variabili di gioco (UGV); le chiavi non vengono toccate");
Hack& mainLevels = hack("BYPASS", "BYPASS/HACK_MAIN_LEVELS");
Hack& musicUnlocker = hack("BYPASS", "BYPASS/HACK_MUSIC_UNLOCKER");
Hack& textLength = hack("BYPASS", "BYPASS/HACK_TEXT_LENGTH");
Hack& treasureRoom = hack("BYPASS", "BYPASS/HACK_TREASURE_ROOM").partial("Apre le porte (UGV e diamanti richiesti dai dialoghi); i forzieri restano quelli veri");
Hack& unlockIcons = hack("BYPASS", "BYPASS/HACK_UNLOCK_ICONS");
Hack& unlockShops = hack("BYPASS", "BYPASS/HACK_UNLOCK_SHOPS").partial("Sblocca tramite le variabili di gioco (UGV); monete e orbs non vengono toccati");
Hack& unlockVaults = hack("BYPASS", "BYPASS/HACK_UNLOCK_VAULTS").partial("Sblocca tramite le variabili di gioco (UGV) e i requisiti di ingresso");

// --- Anti-Kick: GD esce dai livelli "hackerati" se questa funzione restituisce true
bool (*o_shouldExit)(void*);
bool h_shouldExit(void* self) {
    if (antiKick.enabled) return false;
    return o_shouldExit(self);
}

// --- requisiti di sblocco
// ATTENZIONE: GameStatsManager::getStat e' letto anche da incrementStat (che poi salva), da
// checkAchievement e da GameLevelManager::updateUserScore (che manda le statistiche al server).
// Un valore finto restituito sempre finirebbe nel salvataggio e online. Per questo le statistiche
// finte valgono SOLO dentro le funzioni dell'interfaccia che controllano un requisito (gate),
// e mentre una gate e' attiva ogni scrittura delle statistiche viene bloccata.
// Chiavi lette dalle gate (disassemblato 2.2091): LevelPage "8" (monete segrete),
// OptionsLayer::onSecretVault "12" (monete utente), SecretLayer2 e SecretRewardsLayer "13" (diamanti).
enum Gate { GateNone, GateMainLevels, GateVault, GateTreasure };
thread_local Gate t_gate = GateNone;

struct GateScope {
    Gate prev;
    explicit GateScope(Gate g) : prev(t_gate) { t_gate = g; }
    ~GateScope() { t_gate = prev; }
};

int (*o_getStat)(void*, const char*);
int h_getStat(void* self, const char* key) {
    int v = o_getStat(self, key);
    if (!key || t_gate == GateNone) return v;
    if (t_gate == GateMainLevels && mainLevels.enabled && std::strcmp(key, "8") == 0) return std::max(v, 9999);
    if (t_gate == GateVault && unlockVaults.enabled && (std::strcmp(key, "12") == 0 || std::strcmp(key, "13") == 0))
        return std::max(v, 9999);
    if (t_gate == GateTreasure && treasureRoom.enabled && std::strcmp(key, "13") == 0) return std::max(v, 9999);
    return v;
}

// dentro una gate non si scrive nessuna statistica
void (*o_setStat)(void*, const char*, int);
void h_setStat(void* self, const char* key, int value) {
    if (t_gate != GateNone) {
        MH_LOG("Bypass: scrittura della statistica %s bloccata dentro una gate", key ? key : "?");
        return;
    }
    o_setStat(self, key, value);
}
void (*o_incrementStat)(void*, const char*, int);
void h_incrementStat(void* self, const char* key, int amount) {
    if (t_gate != GateNone) {
        MH_LOG("Bypass: incremento della statistica %s bloccato dentro una gate", key ? key : "?");
        return;
    }
    o_incrementStat(self, key, amount);
}

// gate: void f(void* self, void* arg) oppure void f(void* self)
#define MH_GATE1(name, gate)                       \
    void (*o_##name)(void*, void*);                \
    void h_##name(void* self, void* a) {           \
        GateScope g(gate);                         \
        o_##name(self, a);                         \
    }
#define MH_GATE0(name, gate, R)                    \
    R (*o_##name)(void*);                          \
    R h_##name(void* self) {                       \
        GateScope g(gate);                         \
        return o_##name(self);                     \
    }
MH_GATE1(levelPageUpdate, GateMainLevels)
MH_GATE1(levelPagePlay, GateMainLevels)
MH_GATE1(secretVault, GateVault)
MH_GATE0(secretLayer2Init, GateVault, bool)
MH_GATE1(secretLevel, GateVault)
MH_GATE0(dialog01, GateTreasure, void)
MH_GATE0(dialog03, GateTreasure, void)
MH_GATE0(dialogMechanic, GateTreasure, void)
MH_GATE0(dialogDiamond, GateTreasure, void)

// --- variabili utente: porte/stanze segrete
bool (*o_getUGV)(void*, const char*);
bool h_getUGV(void* self, const char* key) {
    if (challenge.enabled || keymaster.enabled || treasureRoom.enabled || unlockVaults.enabled) return true;
    return o_getUGV(self, key);
}

bool (*o_hasCompletedMain)(void*, int);
bool h_hasCompletedMain(void* self, int id) {
    // solo per il lucchetto della pagina dei livelli (non per premi e achievement)
    if (mainLevels.enabled && t_gate == GateMainLevels) return true;
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
    hooks::gd("GameStatsManager::setStat", addr::GameStatsManager::setStat, h_setStat, &o_setStat);
    hooks::gd("GameStatsManager::incrementStat", addr::GameStatsManager::incrementStat, h_incrementStat, &o_incrementStat);
    hooks::gd("LevelPage::updateDynamicPage", addr::LevelPage::updateDynamicPage, h_levelPageUpdate, &o_levelPageUpdate);
    hooks::gd("LevelPage::onPlay", addr::LevelPage::onPlay, h_levelPagePlay, &o_levelPagePlay);
    hooks::gd("OptionsLayer::onSecretVault", addr::OptionsLayer::onSecretVault, h_secretVault, &o_secretVault);
    hooks::gd("SecretLayer2::init", addr::SecretLayer2::init, h_secretLayer2Init, &o_secretLayer2Init);
    hooks::gd("SecretLayer2::onSecretLevel", addr::SecretLayer2::onSecretLevel, h_secretLevel, &o_secretLevel);
    hooks::gd("SecretRewardsLayer::showDialog01", addr::SecretRewardsLayer::showDialog01, h_dialog01, &o_dialog01);
    hooks::gd("SecretRewardsLayer::showDialog03", addr::SecretRewardsLayer::showDialog03, h_dialog03, &o_dialog03);
    hooks::gd("SecretRewardsLayer::showDialogMechanic", addr::SecretRewardsLayer::showDialogMechanic, h_dialogMechanic, &o_dialogMechanic);
    hooks::gd("SecretRewardsLayer::showDialogDiamond", addr::SecretRewardsLayer::showDialogDiamond, h_dialogDiamond, &o_dialogDiamond);
    hooks::gd("GameManager::getUGV", addr::GameManager::getUGV, h_getUGV, &o_getUGV);
    hooks::gd("GameStatsManager::hasCompletedMainLevel", addr::GameStatsManager::hasCompletedMainLevel, h_hasCompletedMain, &o_hasCompletedMain);
    hooks::gd("CCTextInputNode::init", addr::CCTextInputNode::init, h_textInit, &o_textInit);
}
