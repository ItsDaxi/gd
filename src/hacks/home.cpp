// Finestre HOME ("Mega Hack"), KEYBINDS, GM_VAR ("Variables") e FAVOURITES
#include "common.hpp"
#include "core/hooks.hpp"
#include "gui/theme.hpp"

#include <cstring>

using namespace mh;

namespace {
std::string lowerStr(std::string s) {
    for (auto& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}
} // namespace

// ================================================================== HOME
static gui::WindowContent s_homeUI("HOME", [] {
    auto& ui = config::ui();

    // ricerca (filtra tutte le finestre); con Auto-Select riceve il focus all'apertura del menu
    static bool wasOpen = false;
    if (gui::menuOpen() && !wasOpen && ui.autoSelectSearch) ImGui::SetKeyboardFocusHere();
    wasOpen = gui::menuOpen();
    gui::inputRow("##search", T("HOME/SEARCH/SEARCH"), &gui::g_search);

    std::string animFmt = std::string(T("HOME/ANIM_SPEED/PRE")) + ": %dms";
    gui::sliderIntRow("##anim", &ui.animMs, 0, 1000, animFmt.c_str());

    // scala: applicata con "Set Scale" (cambia la dimensione dei font)
    static float pendingScale = ui.scale;
    gui::sliderFloatRow("##scale", &pendingScale, 0.5f, 2.5f, "Scale: %.2fx");
    if (gui::buttonRow("Set Scale")) {
        ui.scale = pendingScale;
        config::markDirty();
    }
    if (gui::buttonRow(T("HOME/SORT_TABS"))) gui::sortWindows();

    int r = gui::buttonPair("Auto Save", T("REPLAY/SAVE_LAYOUT/SAVE"), ui.autoSave, false);
    if (r == 1) {
        ui.autoSave = !ui.autoSave;
        config::markDirty();
    } else if (r == 2) {
        config::save();
    }

    // lingua
    std::string langPreview = std::string(T("HOME/LANGUAGE")) + ": " + lang::current();
    if (gui::beginComboRow("##lang", langPreview.c_str())) {
        for (auto& l : lang::available()) {
            if (ImGui::Selectable(l.c_str(), l == lang::current())) {
                ui.language = l;
                lang::set(l);
                config::markDirty();
            }
        }
        ImGui::EndCombo();
    }
    // tema
    std::string themePreview = std::string(T("HOME/THEME")) + ": " + T("HOME/THEME/" + std::to_string(ui.theme) + "/TOGGLE");
    if (gui::beginComboRow("##theme", themePreview.c_str())) {
        for (int i = 0; i < gui::presetCount(); i++) {
            if (ImGui::Selectable(T("HOME/THEME/" + std::to_string(i) + "/TOGGLE"), ui.theme == i)) {
                gui::applyPreset(i);
                config::markDirty();
            }
        }
        ImGui::EndCombo();
    }
    static bool advanced = false;
    gui::checkboxRow(T("HOME/THEME/ADVANCED"), &advanced);
    if (advanced) {
        bool changed = false;
        ImGui::Indent(8);
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 4));
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/ACCENT"), ui.accent, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/TITLE_ACCENT"), ui.titleAccent, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/BACKGROUND"), ui.background, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::Checkbox(T("HOME/THEME/ADVANCED/DARK_TEXT"), &ui.darkText);
        ImGui::PopStyleVar();
        ImGui::Unindent(8);
        if (changed) {
            gui::applyTheme();
            config::markDirty();
        }
    }
    gui::checkboxRow(T("HOME/DOT"), &ui.indicatorDot);
    gui::checkboxRow(T("HOME/SEARCH/AUTO_SELECT"), &ui.autoSelectSearch);
    gui::checkboxRow("Hide on Toggle", &ui.hideOnToggle);
    gui::keyButton("Menu", &ui.toggleKey);
    gui::keyButton(T("HOME/SHORTCUT/ALT"), &ui.altToggleKey);
    if (gui::buttonRow(T("HOME/MISC/LOAD_LANG"))) {
        lang::init();
        lang::set(ui.language);
    }
    size_t ok = 0;
    for (auto& h : hooks::list()) ok += h.ok;
    char info[96];
    snprintf(info, sizeof(info), "GD %s%s | hook %zu/%zu", gd::versionString(), gd::versionMatches() ? "" : " (!)", ok,
             hooks::list().size());
    gui::labelRow(info);
}, true);

// ================================================================== KEYBINDS
static gui::WindowContent s_keybindsUI("KEYBINDS", [] {
    static int selected = -1;
    static int pendingKey = 0;
    static std::string filter;
    auto& list = hacks();
    bool valid = selected >= 0 && selected < static_cast<int>(list.size());
    std::string preview = valid ? lang::tr(list[selected].id) : T("KEYBINDS/SET");
    if (gui::beginComboRow("##kbhack", preview.c_str())) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s", filter.c_str());
        if (ImGui::IsWindowAppearing()) ImGui::SetKeyboardFocusHere();
        if (ImGui::InputTextWithHint("##kbfilter", T("HOME/SEARCH/SEARCH"), buf, sizeof(buf))) filter = buf;
        std::string f = lowerStr(filter);
        for (int i = 0; i < static_cast<int>(list.size()); i++) {
            if (list[i].window.rfind("_", 0) == 0) continue;
            const std::string& name = lang::tr(list[i].id);
            if (!f.empty() && lowerStr(name).find(f) == std::string::npos) continue;
            ImGui::PushID(i);
            if (ImGui::Selectable(name.c_str(), selected == i)) {
                selected = i;
                pendingKey = list[i].keybind;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    gui::keyButton(T("KEYBINDS/SHORTCUT"), &pendingKey);
    int r = gui::buttonPair("Remove", "Set");
    if (r == 1 && valid) {
        list[selected].keybind = 0;
        pendingKey = 0;
        config::markDirty();
    } else if (r == 2 && valid) {
        if (!pendingKey) confirm(lang::tr("MESSAGE/KEYBINDS/EMPTY"), nullptr, nullptr);
        else {
            list[selected].keybind = pendingKey;
            config::markDirty();
        }
    }
    if (gui::buttonRow("Reset All")) {
        confirm("Reset All?", [](void*) {
            for (auto& h : hacks()) h.keybind = 0;
            config::markDirty();
        }, nullptr);
    }
    gui::checkboxRow(T("KEYBINDS/HACK_DISABLE_IN_EDITOR"), &config::ui().keybindsDisabledInEditor);
    // elenco dei tasti assegnati
    for (auto& h : list) {
        if (!h.keybind) continue;
        std::string t = gui::keyName(h.keybind) + "  " + lang::tr(h.id);
        gui::labelRow(t.c_str());
    }
}, true);

// ================================================================== GM_VAR (variabili di gioco)
namespace {
struct GmVar {
    const char* key;
    const char* label;
};
// Le prime quattro vengono dal file di lingua di Mega Hack, le altre sono impostazioni note di GD
const GmVar kVars[] = {
    {"0060", "GM_VAR/0060"}, {"0061", "GM_VAR/0061"}, {"0062", "GM_VAR/0062"}, {"0096", "GM_VAR/0096"},
    {"0024", "Show Cursor In-Game"}, {"0026", "Auto-Retry"}, {"0027", "Auto-Checkpoints"}, {"0040", "Show Percentage"},
    {"0010", "Flip 2-Player Controls"}, {"0011", "Always Limit Controls"}, {"0028", "Disable Thumbstick"},
    {"0052", "Fast Practice Reset"}, {"0074", "Show Restart Button"}, {"0109", "Show Info Label"},
    {"0126", "Decimal Percentage"}, {"0127", "Save Gauntlets"}, {"0134", "Hide Attempts"},
    {"0135", "Hide Attempts in Practice"}, {"0166", "Show Hitboxes in Practice"}, {"0168", "Fast Menu"},
    {"0174", "Hide Playtest Text"},
};
constexpr int kVarCount = static_cast<int>(sizeof(kVars) / sizeof(kVars[0]));
int s_varSel = 0;
std::string varLabel(int i) {
    std::string name = lang::has(kVars[i].label) ? lang::tr(kVars[i].label) : std::string(kVars[i].label);
    return std::string(kVars[i].key) + " " + name;
}
} // namespace

static gui::WindowContent s_varsUI("GM_VAR", [] {
    if (gui::beginComboRow("##gmvar", varLabel(s_varSel).c_str())) {
        for (int i = 0; i < kVarCount; i++)
            if (ImGui::Selectable(varLabel(i).c_str(), i == s_varSel)) s_varSel = i;
        ImGui::EndCombo();
    }
    if (!gd::gameManager()) return;
    bool v = gd::getGameVar(kVars[s_varSel].key);
    if (gui::checkboxRow(v ? "Enabled" : "Disabled", &v)) gd::setGameVar(kVars[s_varSel].key, v);
    for (int i = 0; i < 4; i++) {
        bool cur = gd::getGameVar(kVars[i].key);
        if (gui::checkboxRow(T(kVars[i].label), &cur)) gd::setGameVar(kVars[i].key, cur);
    }
}, true);

// ================================================================== FAVOURITES
static gui::WindowContent s_favUI("FAVOURITES", [] {
    if (gui::favourites().empty()) gui::labelRow("Tasto destro -> + Favourites");
});
