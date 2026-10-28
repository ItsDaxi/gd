// Finestre CHEAT_SAFETY e INTERFACE
#include "common.hpp"

using namespace mh;

namespace {

Hack& safeMode = hack("CHEAT_SAFETY", "CHEAT_SAFETY/HACK_SAFE_MODE");
Hack& autoSafeMode = hack("CHEAT_SAFETY", "CHEAT_SAFETY/HACK_AUTO_SAFE_MODE");
Hack& safeModePopup = hack("CHEAT_SAFETY", "CHEAT_SAFETY/HACK_SAFE_MODE_POPUP");
Hack& disableCheats = action("CHEAT_SAFETY", "CHEAT_SAFETY/DISABLE_CHEATS", [] {
    confirm(lang::tr("MESSAGE/CHEAT_SAFETY/CONFIRM_DISABLE"), [](void*) {
        for (auto& h : hacks())
            if (h.cheat && h.enabled) h.set(false);
        config::markDirty();
    }, nullptr);
});

bool shouldBeSafe() {
    if (safeMode.enabled) return true;
    if (autoSafeMode.enabled && (g_cheatedThisAttempt || anyCheatEnabled())) return true;
    return false;
}

} // namespace

// Safe mode = "test mode" di GD (come con le StartPos): nessun progresso salvato
MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (shouldBeSafe()) GDF(pl, GJBaseGameLayer, m_isTestMode, bool) = true;
});
MH_ON(ev::plPreComplete, [](void* pl, bool&) {
    if (shouldBeSafe()) GDF(pl, GJBaseGameLayer, m_isTestMode, bool) = true;
});
MH_ON(ev::plDestroyPlayer, [](void* pl, void*, void*, bool&) {
    if (shouldBeSafe()) GDF(pl, GJBaseGameLayer, m_isTestMode, bool) = true;
});
MH_ON(ev::plInit, [](void*) {
    if (!safeModePopup.enabled || !anyCheatEnabled()) return;
    std::string list;
    for (auto& h : hacks())
        if (h.cheat && h.enabled) list += "\n- " + lang::tr(h.id);
    confirm(lang::tr("CHEAT_SAFETY/HACK_SAFE_MODE_POPUP/TIP") + ":" + list + "\n\n" + lang::tr("MESSAGE/CHEAT_SAFETY/CONFIRM_DISABLE"),
            [](void*) {
                for (auto& h : hacks())
                    if (h.cheat && h.enabled) h.set(false);
                config::markDirty();
            },
            nullptr);
});

static gui::WindowContent s_rulesetUI("CHEAT_SAFETY", [] {
    auto& ui = config::ui();
    std::string preview = std::string(T("CHEAT_SAFETY/RULESET")) + ": " + T("*/RULESET*/" + std::to_string(ui.ruleset));
    if (gui::beginComboRow("##ruleset", preview.c_str())) {
        for (int i = 0; i < 3; i++) {
            if (ImGui::Selectable(T("*/RULESET*/" + std::to_string(i)), ui.ruleset == i)) {
                ui.ruleset = i;
                config::markDirty();
            }
        }
        ImGui::EndCombo();
    }
}, true);

// ------------------------------------------------------------------ INTERFACE
namespace {
Hack& hideEndCheats = hack("INTERFACE", "INTERFACE/HACK_HIDE_ENDSCREEN_CHEATS")
    .partial("Questa versione non aggiunge testo alla schermata di fine livello: nessun effetto");
Hack& hideEndExtras = hack("INTERFACE", "INTERFACE/HACK_HIDE_ENDSCREEN_EXTRAS")
    .partial("Questa versione non aggiunge extra alla schermata di fine livello: nessun effetto");
Hack& hideSnow = hack("INTERFACE", "INTERFACE/HACK_HIDE_MENU_SNOW")
    .partial("Nessuna neve nel menu di GD 2.209: nessun effetto");
Hack& hideIconicPause = hack("INTERFACE", "INTERFACE/HACK_HIDE_PAUSE_ICONIC")
    .partial("Iconic qui e' una finestra del menu, non un pulsante in pausa: nessun effetto");
Hack& hideRobsVault = hack("INTERFACE", "INTERFACE/HACK_HIDE_ROBSVAULT");

void (*o_addSecretDoor)(void*);
void h_addSecretDoor(void* self) {
    if (hideRobsVault.enabled) return;
    o_addSecretDoor(self);
}
} // namespace

MH_INSTALL(interface) {
    hooks::gd("LevelPage::addSecretDoor", gd::addr::LevelPage::addSecretDoor, h_addSecretDoor, &o_addSecretDoor);
}
