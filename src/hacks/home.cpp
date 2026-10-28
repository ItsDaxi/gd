// Finestre HOME ("Mega Hack"), KEYBINDS, GM_VAR ("Variables") e FAVOURITES
#include "common.hpp"
#include "core/hooks.hpp"
#include "gui/theme.hpp"

#include <cstring>

using namespace mh;

// ================================================================== HOME
static gui::WindowContent s_homeUI("HOME", [] {
    auto& ui = config::ui();
    float s = ui.scale;
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4 * s, 3 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6 * s, 4 * s));
    ImGui::PushItemWidth(w);

    // ricerca
    static bool focusSearch = false;
    if (focusSearch) {
        ImGui::SetKeyboardFocusHere();
        focusSearch = false;
    }
    char buf[128];
    snprintf(buf, sizeof(buf), "%s", gui::g_search.c_str());
    if (ImGui::InputTextWithHint("##search", T("HOME/SEARCH/SEARCH"), buf, sizeof(buf))) gui::g_search = buf;
    if (ImGui::IsWindowAppearing() && ui.autoSelectSearch) focusSearch = true;

    // tempo delle animazioni e scala
    if (ImGui::DragInt("##anim", &ui.animMs, 1.f, 0, 2000, (std::string(T("HOME/ANIM_SPEED/PRE")) + ": %dms").c_str()))
        config::markDirty();
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(w * 0.6f);
    static float pendingScale = ui.scale;
    ImGui::DragFloat("##scale", &pendingScale, 0.01f, 0.5f, 3.f, "%.2fx");
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (ImGui::Button("Set Scale", ImVec2(-1, 0))) {
        ui.scale = pendingScale;
        config::markDirty();
    }
    if (gui::buttonRow(T("HOME/SORT_TABS"))) gui::sortWindows();

    // auto-save / save
    bool as = ui.autoSave;
    ImGui::PushStyleColor(ImGuiCol_Text, as ? gui::accent() : gui::textColor());
    if (ImGui::Button("Auto Save", ImVec2(w * 0.5f - 2 * s, 0))) {
        ui.autoSave = !ui.autoSave;
        config::markDirty();
    }
    ImGui::PopStyleColor();
    ImGui::SameLine();
    if (ImGui::Button(T("REPLAY/SAVE_LAYOUT/SAVE"), ImVec2(-1, 0))) config::save();

    // lingua
    auto langs = lang::available();
    ImGui::PushItemWidth(w);
    if (ImGui::BeginCombo("##lang", (std::string(T("HOME/LANGUAGE")) + ": " + lang::current()).c_str())) {
        for (auto& l : langs) {
            if (ImGui::Selectable(l.c_str(), l == lang::current())) {
                ui.language = l;
                lang::set(l);
                config::markDirty();
            }
        }
        ImGui::EndCombo();
    }
    // tema
    const char* themes[] = {T("HOME/THEME/0/TOGGLE"), T("HOME/THEME/1/TOGGLE"), T("HOME/THEME/2/TOGGLE"),
                            T("HOME/THEME/3/TOGGLE"), T("HOME/THEME/4/TOGGLE"), T("HOME/THEME/5/TOGGLE")};
    int theme = ui.theme;
    if (ImGui::Combo("##theme", &theme, themes, gui::presetCount())) {
        gui::applyPreset(theme);
        config::markDirty();
    }
    ImGui::PopItemWidth();
    if (ImGui::TreeNode(T("HOME/THEME/ADVANCED"))) {
        bool changed = false;
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/ACCENT"), ui.accent, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/TITLE_ACCENT"), ui.titleAccent, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::ColorEdit4(T("HOME/THEME/ADVANCED/BACKGROUND"), ui.background, ImGuiColorEditFlags_NoInputs);
        changed |= ImGui::Checkbox(T("HOME/THEME/ADVANCED/DARK_TEXT"), &ui.darkText);
        if (changed) {
            gui::applyTheme();
            config::markDirty();
        }
        ImGui::TreePop();
    }
    gui::checkboxRow(T("HOME/DOT"), &ui.indicatorDot);
    gui::checkboxRow(T("HOME/SEARCH/AUTO_SELECT"), &ui.autoSelectSearch);
    gui::checkboxRow("Hide on Toggle", &ui.hideOnToggle);
    gui::keyButton("Menu", &ui.toggleKey);
    gui::keyButton(T("HOME/SHORTCUT/ALT"), &ui.altToggleKey);

    if (ImGui::TreeNode(T("HOME/MISC"))) {
        if (ImGui::Button(T("HOME/MISC/LOAD_LANG"), ImVec2(-1, 0))) {
            lang::init();
            lang::set(ui.language);
        }
        ImGui::TextDisabled("%s", T("HOME/MISC/LICENSES"));
        ImGui::TextWrapped("Dear ImGui (MIT), MinHook (BSD-2), nlohmann/json (MIT). Indirizzi: geode-sdk/bindings. "
                           "Testi: absolllute/Mega-Hack-Translate.");
        ImGui::TreePop();
    }
    size_t ok = 0;
    for (auto& h : hooks::list()) ok += h.ok;
    ImGui::TextDisabled("GD %s%s | hook %zu/%zu", gd::versionString(), gd::versionMatches() ? "" : " (!)", ok, hooks::list().size());
    ImGui::PopStyleVar(2);
}, true);

// ================================================================== KEYBINDS
static gui::WindowContent s_keybindsUI("KEYBINDS", [] {
    float s = config::ui().scale;
    float w = ImGui::GetContentRegionAvail().x;
    static int selected = -1;
    static int pendingKey = 0;
    static char filter[64] = "";
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4 * s, 3 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6 * s, 4 * s));
    auto& list = hacks();
    std::string preview = selected >= 0 && selected < static_cast<int>(list.size()) ? lang::tr(list[selected].id) : T("KEYBINDS/SET");
    ImGui::PushItemWidth(w);
    if (ImGui::BeginCombo("##kbhack", preview.c_str(), ImGuiComboFlags_HeightLarge)) {
        ImGui::InputTextWithHint("##kbfilter", T("HOME/SEARCH/SEARCH"), filter, sizeof(filter));
        std::string f = filter;
        for (auto& c : f) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
        for (int i = 0; i < static_cast<int>(list.size()); i++) {
            if (list[i].window.rfind("_", 0) == 0) continue;
            std::string name = lang::tr(list[i].id);
            std::string low = name;
            for (auto& c : low) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
            if (!f.empty() && low.find(f) == std::string::npos) continue;
            ImGui::PushID(i);
            if (ImGui::Selectable(name.c_str(), selected == i)) {
                selected = i;
                pendingKey = list[i].keybind;
            }
            ImGui::PopID();
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    gui::keyButton(T("KEYBINDS/SHORTCUT"), &pendingKey);
    float half = (w - 4 * s) * 0.5f;
    bool valid = selected >= 0 && selected < static_cast<int>(list.size());
    if (ImGui::Button(T("KEYBINDS/REMOVE"), ImVec2(half, 0)) && valid) {
        list[selected].keybind = 0;
        config::markDirty();
    }
    ImGui::SameLine();
    if (ImGui::Button("Set", ImVec2(half, 0)) && valid) {
        if (!pendingKey) confirm(lang::tr("MESSAGE/KEYBINDS/EMPTY"), nullptr, nullptr);
        else {
            list[selected].keybind = pendingKey;
            config::markDirty();
        }
    }
    if (gui::buttonRow("Reset All")) {
        for (auto& h : list) h.keybind = 0;
        config::markDirty();
    }
    gui::checkboxRow(T("KEYBINDS/HACK_DISABLE_IN_EDITOR"), &config::ui().keybindsDisabledInEditor);
    if (ImGui::TreeNode(T("KEYBINDS/VIEW"))) {
        for (auto& h : list)
            if (h.keybind) ImGui::Text("%s: %s", gui::keyName(h.keybind).c_str(), T(h.id));
        ImGui::TreePop();
    }
    ImGui::PopStyleVar(2);
}, true);

// ================================================================== GM_VAR (variabili di gioco)
namespace {
struct GmVar {
    const char* key;
    const char* label;
};
// Variabili di GameManager note (alcune dal file di lingua, le altre con nome descrittivo)
const GmVar kVars[] = {
    {"0060", "GM_VAR/0060"}, {"0061", "GM_VAR/0061"}, {"0062", "GM_VAR/0062"}, {"0096", "GM_VAR/0096"},
    {"0024", "Show Cursor In-Game"}, {"0026", "Auto-Retry"}, {"0027", "Auto-Checkpoints"}, {"0040", "Show Percentage"},
    {"0010", "Flip 2-Player Controls"}, {"0011", "Always Limit Controls"}, {"0028", "Disable Thumbstick"},
    {"0052", "Fast Practice Reset"}, {"0074", "Show Restart Button"}, {"0109", "Show Info Label"},
    {"0126", "Decimal Percentage"}, {"0127", "Save Gauntlets"}, {"0134", "Hide Attempts"},
    {"0135", "Hide Attempts in Practice"}, {"0166", "Show Hitboxes in Practice"}, {"0168", "Fast Menu"},
    {"0174", "Hide Playtest Text"},
};
int s_varSel = 0;
} // namespace

static gui::WindowContent s_varsUI("GM_VAR", [] {
    float s = config::ui().scale;
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6 * s, 4 * s));
    ImGui::PushItemWidth(w);
    const int n = static_cast<int>(sizeof(kVars) / sizeof(kVars[0]));
    auto label = [](int i) {
        static std::string tmp;
        tmp = std::string(kVars[i].key) + " " + (lang::has(kVars[i].label) ? lang::tr(kVars[i].label) : std::string(kVars[i].label));
        return tmp.c_str();
    };
    if (ImGui::BeginCombo("##gmvar", label(s_varSel))) {
        for (int i = 0; i < n; i++)
            if (ImGui::Selectable(label(i), i == s_varSel)) s_varSel = i;
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    if (!gd::gameManager()) return;
    bool v = gd::getGameVar(kVars[s_varSel].key);
    if (gui::checkboxRow(v ? "Enabled" : "Disabled", &v)) gd::setGameVar(kVars[s_varSel].key, v);
    // le quattro variabili del file di lingua sempre visibili
    for (int i = 0; i < 4; i++) {
        bool cur = gd::getGameVar(kVars[i].key);
        if (gui::checkboxRow(T(kVars[i].label), &cur)) gd::setGameVar(kVars[i].key, cur);
    }
}, true);

// ================================================================== FAVOURITES
static gui::WindowContent s_favUI("FAVOURITES", [] {
    if (gui::favourites().empty()) {
        ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x);
        ImGui::TextDisabled("  Tasto destro su una funzione -> + Favourite");
        ImGui::PopTextWrapPos();
    }
});
