#pragma once
#include <string>

namespace mh::config {

void load();
void save();
// salva se e' attivo l'auto-save e ci sono modifiche
void markDirty();
void tick();

// Impostazioni dell'interfaccia
struct UI {
    float scale = 1.0f;
    int animMs = 250;
    bool autoSave = true;
    int toggleKey = 0x09;     // VK_TAB
    int altToggleKey = 0x2D;  // VK_INSERT
    int iconHotkey = 0;
    bool indicatorDot = true;
    int theme = 0;
    float accent[4] = {0.55f, 0.30f, 0.95f, 1.0f};
    float titleAccent[4] = {0.62f, 0.38f, 1.0f, 1.0f};
    float background[4] = {0.11f, 0.11f, 0.12f, 0.97f};
    bool darkText = false;
    std::string language = "en-GB";
    bool keybindsDisabledInEditor = false;
    bool hideOnToggle = false;
    bool autoSelectSearch = true;
    int ruleset = 0;
};
UI& ui();

} // namespace mh::config
