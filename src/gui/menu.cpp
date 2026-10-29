#include "menu.hpp"
#include "gui.hpp"
#include "theme.hpp"
#include "hacks/config.hpp"
#include "hacks/lang.hpp"
#include "hacks/registry.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <windows.h>

namespace mh::gui {

std::string g_search;
static bool g_open = false;
static float g_anim = 0.f; // 0 = chiuso, 1 = aperto (avanza linearmente in animMs)
static std::map<std::string, WindowState> g_windows;
static std::map<std::string, float> g_contentH; // altezza misurata del contenuto di ogni finestra
static std::vector<std::string> g_favourites;

WindowState& windowState(const std::string& name) { return g_windows[name]; }
std::map<std::string, WindowState>& windowStates() { return g_windows; }
std::vector<std::string>& favourites() { return g_favourites; }
bool isFavourite(const std::string& id) {
    return std::find(g_favourites.begin(), g_favourites.end(), id) != g_favourites.end();
}

bool menuOpen() { return g_open; }
void toggleMenu() { g_open = !g_open; }
void sortWindows() {
    for (auto& [_, st] : g_windows) {
        st.userMoved = false;
        st.collapsed = false;
    }
    config::markDirty();
}

// ------------------------------------------------------------ registrazioni
struct ContentEntry {
    std::string window;
    void (*fn)();
    bool before;
};
static std::vector<ContentEntry>& contents() {
    static std::vector<ContentEntry> v;
    return v;
}
WindowContent::WindowContent(const char* window, void (*fn)(), bool before) { contents().push_back({window, fn, before}); }

static std::vector<void (*)()>& overlays() {
    static std::vector<void (*)()> v;
    return v;
}
Overlay::Overlay(void (*fn)()) { overlays().push_back(fn); }

static std::vector<bool (*)(int, bool)>& keyHandlers() {
    static std::vector<bool (*)(int, bool)> v;
    return v;
}
KeyHandler::KeyHandler(bool (*fn)(int, bool)) { keyHandlers().push_back(fn); }

void* g_editorLayerForKeys(); // definito in hacks/core_hooks.cpp

bool dispatchKey(int vk, bool down) {
    bool swallow = false;
    for (auto fn : keyHandlers()) swallow |= fn(vk, down);
    if (!down) return swallow;
    if (config::ui().keybindsDisabledInEditor && g_editorLayerForKeys()) return swallow;
    for (auto& h : hacks()) {
        if (h.keybind == vk) {
            h.toggle();
            config::markDirty();
            swallow = true;
        }
    }
    return swallow;
}

ImFont* bigFont() {
    auto& io = ImGui::GetIO();
    return io.Fonts->Fonts.Size > 1 ? io.Fonts->Fonts[1] : io.Fonts->Fonts[0];
}

void drawOverlays() {
    for (auto fn : overlays()) fn();
}

// ------------------------------------------------------------ utilita'
std::string keyName(int vk) {
    if (vk <= 0) return lang::tr("KEYBINDS/EMPTY");
    UINT sc = MapVirtualKeyW(static_cast<UINT>(vk), MAPVK_VK_TO_VSC);
    switch (vk) {
    case VK_LEFT: case VK_UP: case VK_RIGHT: case VK_DOWN: case VK_PRIOR: case VK_NEXT:
    case VK_END: case VK_HOME: case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK:
        sc |= 0x100;
        break;
    }
    char name[64]{};
    if (GetKeyNameTextA(static_cast<LONG>(sc << 16), name, sizeof(name)) > 0) return name;
    char buf[16];
    snprintf(buf, sizeof(buf), "0x%02X", vk);
    return buf;
}

static std::string lower(std::string s) {
    for (auto& c : s) c = static_cast<char>(tolower(static_cast<unsigned char>(c)));
    return s;
}

static const std::string& hackName(const Hack& h) {
    if (!h.displayName.empty()) return h.displayName;
    return lang::tr(h.id);
}

static bool matchesSearch(const Hack& h) {
    if (g_search.empty()) return true;
    return lower(hackName(h)).find(lower(g_search)) != std::string::npos;
}

static float S(float v) { return v * config::ui().scale; }
float rowHeight() { return S(24); }

static void rowBackground(ImVec2 p, float w, float h, bool hovered, bool held = false) {
    auto* dl = ImGui::GetWindowDrawList();
    ImVec4 c = held ? ImVec4(accent().x, accent().y, accent().z, 0.35f) : hovered ? rowHover() : rowColor();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::GetColorU32(c));
    dl->AddLine(ImVec2(p.x, p.y + h - 1), ImVec2(p.x + w, p.y + h - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.25f)));
}

// Stile comune per i widget ImGui dentro le righe: altezza = riga, nessuno spazio tra le righe
struct RowStyle {
    RowStyle() {
        float fh = ImGui::GetFontSize();
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(7), std::max(0.f, (rowHeight() - fh) * 0.5f)));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.f);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.f);
        ImGui::PushStyleColor(ImGuiCol_FrameBg, rowColor());
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, rowHover());
        ImGui::PushStyleColor(ImGuiCol_FrameBgActive, rowHover());
        ImGui::PushStyleColor(ImGuiCol_Button, rowColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, rowHover());
    }
    ~RowStyle() {
        ImGui::PopStyleColor(5);
        ImGui::PopStyleVar(4);
    }
};

// ------------------------------------------------------------ widget
static bool rowBase(const char* id, const char* label, bool active, bool hasOptions, bool* openOptions, bool dot = true,
                    bool dim = false) {
    auto* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    float h = rowHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##row", ImVec2(w, h), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool hovered = ImGui::IsItemHovered();
    bool right = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    ImGui::PopID();

    float optW = hasOptions ? S(18) : 0;
    bool onArrow = hasOptions && hovered && ImGui::GetIO().MousePos.x > p.x + w - optW;
    rowBackground(p, w, h, hovered);

    ImU32 txt = ImGui::GetColorU32(active ? accent() : dim ? dimText() : textColor());
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->PushClipRect(p, ImVec2(p.x + w - optW - S(8), p.y + h), true);
    dl->AddText(ImVec2(p.x + S(7), p.y + (h - ts.y) * 0.5f), txt, label);
    dl->PopClipRect();

    // indicatore a destra (come Mega Hack): barretta verticale
    if (dot && config::ui().indicatorDot) {
        float bx = p.x + w - optW - S(6);
        ImU32 ic = active ? accentU32() : ImGui::GetColorU32(ImVec4(1, 1, 1, 0.12f));
        dl->AddRectFilled(ImVec2(bx, p.y + S(5)), ImVec2(bx + S(3), p.y + h - S(5)), ic);
    }
    // triangolo delle opzioni in basso a destra
    if (hasOptions) {
        ImU32 tc = ImGui::GetColorU32(onArrow ? accent() : ImVec4(1, 1, 1, 0.35f));
        ImVec2 br(p.x + w - S(4), p.y + h - S(4));
        dl->AddTriangleFilled(ImVec2(br.x, br.y - S(9)), br, ImVec2(br.x - S(9), br.y), tc);
    }

    if (openOptions) *openOptions = hasOptions && ((clicked && onArrow) || right);
    if (hasOptions && ((clicked && onArrow) || right)) return false;
    return clicked && !right;
}

bool checkboxRow(const char* label, bool* v) {
    bool clicked = rowBase(label, label, *v, false, nullptr);
    if (clicked) {
        *v = !*v;
        config::markDirty();
    }
    return clicked;
}

static bool centeredButton(const char* label, float w, bool active) {
    auto* dl = ImGui::GetWindowDrawList();
    float h = rowHeight();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##btn", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    ImGui::PopID();
    rowBackground(p, w, h, hovered, held);
    // nome visibile = testo prima di "##"
    const char* end = strstr(label, "##");
    ImVec2 ts = ImGui::CalcTextSize(label, end);
    dl->PushClipRect(p, ImVec2(p.x + w, p.y + h), true);
    dl->AddText(ImVec2(p.x + std::max(S(4), (w - ts.x) * 0.5f), p.y + (h - ts.y) * 0.5f),
                ImGui::GetColorU32(active ? accent() : textColor()), label, end);
    dl->PopClipRect();
    return clicked;
}

bool buttonRow(const char* label, bool active) { return centeredButton(label, ImGui::GetContentRegionAvail().x, active); }

int buttonPair(const char* a, const char* b, bool activeA, bool activeB) {
    float w = ImGui::GetContentRegionAvail().x;
    float half = std::floor(w * 0.5f);
    int r = 0;
    if (centeredButton(a, half, activeA)) r = 1;
    ImGui::SameLine(0, 0);
    if (centeredButton(b, w - half, activeB)) r = 2;
    return r;
}

void keyButton(const char* label, int* vk) {
    std::string text = std::string(label) + ": " + (capturing(vk) ? std::string("...") : keyName(*vk));
    ImGui::PushID(vk);
    if (centeredButton(text.c_str(), ImGui::GetContentRegionAvail().x, capturing(vk))) beginKeyCapture(vk);
    ImGui::PopID();
}

static void sliderFill(float t) {
    ImVec2 a = ImGui::GetItemRectMin(), b = ImGui::GetItemRectMax();
    t = std::clamp(t, 0.f, 1.f);
    ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(a.x, b.y - S(3)), ImVec2(a.x + (b.x - a.x) * t, b.y - S(1)), accentU32(0.8f));
}

bool sliderIntRow(const char* id, int* v, int mn, int mx, const char* fmt) {
    RowStyle rs;
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool c = ImGui::SliderInt(id, v, mn, mx, fmt, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleColor(2);
    sliderFill(mx > mn ? static_cast<float>(*v - mn) / static_cast<float>(mx - mn) : 0.f);
    if (c) config::markDirty();
    return c;
}

bool sliderFloatRow(const char* id, float* v, float mn, float mx, const char* fmt) {
    RowStyle rs;
    ImGui::PushStyleColor(ImGuiCol_SliderGrab, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_SliderGrabActive, ImVec4(0, 0, 0, 0));
    ImGui::SetNextItemWidth(-FLT_MIN);
    bool c = ImGui::SliderFloat(id, v, mn, mx, fmt, ImGuiSliderFlags_AlwaysClamp);
    ImGui::PopStyleColor(2);
    sliderFill(mx > mn ? (*v - mn) / (mx - mn) : 0.f);
    if (c) config::markDirty();
    return c;
}

bool inputRow(const char* id, const char* hint, std::string* value) {
    RowStyle rs;
    char buf[256];
    snprintf(buf, sizeof(buf), "%s", value->c_str());
    ImGui::SetNextItemWidth(-FLT_MIN);
    if (ImGui::InputTextWithHint(id, hint, buf, sizeof(buf))) {
        *value = buf;
        return true;
    }
    return false;
}

bool beginComboRow(const char* id, const char* preview) {
    RowStyle rs;
    ImGui::SetNextItemWidth(-FLT_MIN);
    return ImGui::BeginCombo(id, preview, ImGuiComboFlags_HeightLarge);
}

void labelRow(const char* text) {
    float w = ImGui::GetContentRegionAvail().x;
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(w, rowHeight()));
    rowBackground(p, w, rowHeight(), false);
    ImVec2 ts = ImGui::CalcTextSize(text);
    ImGui::GetWindowDrawList()->AddText(ImVec2(p.x + S(7), p.y + (rowHeight() - ts.y) * 0.5f), ImGui::GetColorU32(dimText()), text);
}

static void tooltip(const Hack& h) {
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal)) return;
    const auto& tip = lang::tip(h.id);
    if (tip.empty() && h.extraTip.empty() && !h.keybind) return;
    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(S(320));
    if (!tip.empty()) ImGui::TextUnformatted(tip.c_str());
    if (!h.extraTip.empty()) ImGui::TextColored(ImVec4(1, 0.75f, 0.3f, 1), "%s", h.extraTip.c_str());
    if (h.cheat) ImGui::TextColored(ImVec4(1, 0.35f, 0.35f, 1), "Cheat");
    if (h.keybind) ImGui::TextDisabled("[%s]", keyName(h.keybind).c_str());
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

static void drawOption(Option& o) {
    const char* label = T(o.label);
    bool changed = false;
    ImGui::PushID(o.key.empty() ? o.label.c_str() : o.key.c_str());
    float w = S(220);
    switch (o.type) {
    case OptType::Bool: changed = ImGui::Checkbox(label, static_cast<bool*>(o.ptr)); break;
    case OptType::Int: {
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(w);
        changed = ImGui::SliderInt("##v", static_cast<int*>(o.ptr), static_cast<int>(o.min), static_cast<int>(o.max),
                                   o.fmt ? o.fmt : "%d", ImGuiSliderFlags_AlwaysClamp);
        break;
    }
    case OptType::Float: {
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(w);
        changed = ImGui::SliderFloat("##v", static_cast<float*>(o.ptr), o.min, o.max, o.fmt ? o.fmt : "%.2f",
                                     ImGuiSliderFlags_AlwaysClamp);
        break;
    }
    case OptType::Text: {
        auto* s = static_cast<std::string*>(o.ptr);
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", s->c_str());
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(w);
        if (ImGui::InputText("##v", buf, sizeof(buf))) {
            *s = buf;
            changed = true;
        }
        break;
    }
    case OptType::Combo: {
        int* v = static_cast<int*>(o.ptr);
        int n = static_cast<int>(o.items.size());
        const char* preview = (*v >= 0 && *v < n) ? T(o.items[*v]) : "";
        ImGui::TextUnformatted(label);
        ImGui::SetNextItemWidth(w);
        if (ImGui::BeginCombo("##v", preview, ImGuiComboFlags_HeightLarge)) {
            for (int i = 0; i < n; i++) {
                if (ImGui::Selectable(T(o.items[i]), *v == i)) {
                    *v = i;
                    changed = true;
                }
            }
            ImGui::EndCombo();
        }
        break;
    }
    case OptType::Key: {
        int* vk = static_cast<int*>(o.ptr);
        std::string t = std::string(label) + ": " + (capturing(vk) ? std::string("...") : keyName(*vk));
        if (ImGui::Button(t.c_str(), ImVec2(w, 0))) beginKeyCapture(vk);
        break;
    }
    case OptType::Color:
        changed = ImGui::ColorEdit4(label, static_cast<float*>(o.ptr), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        break;
    case OptType::Button:
        if (ImGui::Button(label, ImVec2(w, 0)) && o.onClick) o.onClick();
        break;
    case OptType::Label: ImGui::TextDisabled("%s", label); break;
    }
    ImGui::PopID();
    if (changed) {
        config::markDirty();
        if (o.onChange) o.onChange();
    }
}

static void optionsPopup(Hack& h) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(10), S(10)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(6), S(6)));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(S(6), S(4)));
    if (ImGui::BeginPopup(("opts_" + h.id).c_str())) {
        ImGui::TextColored(titleAccent(), "%s", hackName(h).c_str());
        ImGui::Separator();
        const auto& tip = lang::tip(h.id);
        ImGui::PushTextWrapPos(S(300));
        if (!tip.empty()) ImGui::TextDisabled("%s", tip.c_str());
        if (!h.extraTip.empty()) ImGui::TextColored(ImVec4(1, 0.75f, 0.3f, 1), "%s", h.extraTip.c_str());
        ImGui::PopTextWrapPos();
        for (auto& o : h.options) drawOption(o);
        if (h.customUI) h.customUI();
        ImGui::Separator();
        std::string kb = std::string(T("KEYBINDS/SHORTCUT")) + ": " + (capturing(&h.keybind) ? std::string("...") : keyName(h.keybind));
        if (ImGui::Button(kb.c_str(), ImVec2(S(220), 0))) beginKeyCapture(&h.keybind);
        bool fav = isFavourite(h.id);
        if (ImGui::Button(fav ? "- Favourites" : "+ Favourites", ImVec2(S(220), 0))) {
            if (fav) g_favourites.erase(std::remove(g_favourites.begin(), g_favourites.end(), h.id), g_favourites.end());
            else g_favourites.push_back(h.id);
            config::markDirty();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(3);
}

bool hackRow(Hack& h) {
    bool openOpts = false;
    const std::string& label = hackName(h);
    bool hasOptions = !h.options.empty() || h.customUI;
    bool clicked = rowBase(h.id.c_str(), label.c_str(), h.enabled, hasOptions, &openOpts, !h.button, h.unavailable);
    tooltip(h);
    if (openOpts) ImGui::OpenPopup(("opts_" + h.id).c_str());
    if (clicked) {
        h.set(!h.enabled);
        config::markDirty();
        if (config::ui().hideOnToggle && !h.button) g_open = false;
    }
    optionsPopup(h);
    return clicked;
}

// ------------------------------------------------------------ finestre
static std::vector<Hack*> hacksFor(const std::string& window) {
    std::vector<Hack*> out;
    if (window == "FAVOURITES") {
        for (auto& id : g_favourites)
            if (auto h = find(id)) out.push_back(h);
        return out;
    }
    for (auto& h : hacks())
        if (h.window == window && matchesSearch(h)) out.push_back(&h);
    std::sort(out.begin(), out.end(), [](Hack* a, Hack* b) {
        // le funzioni non disponibili vanno in fondo
        if (a->unavailable != b->unavailable) return b->unavailable;
        return lower(hackName(*a)) < lower(hackName(*b));
    });
    return out;
}

static bool hasCustom(const std::string& name) {
    for (auto& c : contents())
        if (c.window == name) return true;
    return false;
}

static bool windowVisible(const std::string& name, size_t rows) {
    if (!g_search.empty()) return rows > 0 || name == "HOME";
    return rows > 0 || hasCustom(name);
}

static float titleHeight() { return S(24); }

static float estimatedHeight(const std::string& name, size_t rows) {
    auto& st = g_windows[name];
    if (st.collapsed) return titleHeight();
    auto it = g_contentH.find(name);
    float content = it != g_contentH.end() ? it->second : rows * rowHeight() + (hasCustom(name) ? S(24) * 6 : 0);
    return titleHeight() + content;
}

// Disposizione predefinita a colonne, come in Mega Hack v9:
// Mega Hack+Keybinds | Bypass+Replay | Creator | Cosmetic | Level | Status | Universal+Screenshot |
// Cheat Safety+Utility | Display+Interface+Speedhack. Le finestre che Mega Hack non ha (Iconic,
// Variables, Favourites) vanno sotto le colonne corte.
static const std::vector<std::vector<std::string>>& presetColumns() {
    static const std::vector<std::vector<std::string>> cols = {
        {"HOME", "KEYBINDS", "FAVOURITES"},
        {"BYPASS", "REPLAY"},
        {"CREATOR", "ICONIC", "GM_VAR"},
        {"COSMETIC"},
        {"LEVEL"},
        {"STATUS"},
        {"UNIVERSAL", "SCREENSHOT"},
        {"CHEAT_SAFETY", "UTILITY"},
        {"DISPLAY", "INTERFACE", "SPEEDHACK"},
    };
    return cols;
}

// Larghezza delle finestre: come in Mega Hack tutte le colonne stanno nello schermo
static float g_winWidth = 200.f;
static float windowWidth() { return g_winWidth; }

static void autoLayout(const std::map<std::string, size_t>& rows) {
    auto& io = ImGui::GetIO();
    float margin = S(4), gap = S(3);
    float screenH = io.DisplaySize.y - margin * 2;
    auto cols = presetColumns();
    float n = static_cast<float>(cols.size());
    // le colonne riempiono tutta la larghezza (a 1920x1080: 9 finestre da ~210 px)
    float w = std::clamp((io.DisplaySize.x - margin * 2 - gap * (n - 1)) / n, S(130), S(260));
    g_winWidth = w;
    int maxCols = std::max(1, static_cast<int>((io.DisplaySize.x - margin + gap) / (w + gap)));
    if (static_cast<int>(cols.size()) > maxCols) {
        // schermo stretto: ridistribuisce le finestre nelle colonne disponibili
        std::vector<std::string> flat;
        for (auto& c : cols) flat.insert(flat.end(), c.begin(), c.end());
        cols.assign(maxCols, {});
        size_t per = (flat.size() + maxCols - 1) / maxCols;
        for (size_t i = 0; i < flat.size(); i++) cols[std::min(i / per, cols.size() - 1)].push_back(flat[i]);
    }
    for (size_t c = 0; c < cols.size(); c++) {
        std::vector<std::string> vis;
        for (auto& n : cols[c]) {
            auto it = rows.find(n);
            if (windowVisible(n, it == rows.end() ? 0 : it->second)) vis.push_back(n);
        }
        if (vis.empty()) continue;
        // "water filling": le finestre corte prendono la loro altezza, le lunghe si dividono il resto
        float avail = screenH - gap * static_cast<float>(vis.size() - 1);
        std::vector<float> want(vis.size()), got(vis.size(), 0.f);
        for (size_t i = 0; i < vis.size(); i++) want[i] = estimatedHeight(vis[i], rows.count(vis[i]) ? rows.at(vis[i]) : 0);
        std::vector<size_t> order(vis.size());
        for (size_t i = 0; i < order.size(); i++) order[i] = i;
        std::sort(order.begin(), order.end(), [&](size_t a, size_t b) { return want[a] < want[b]; });
        float left = avail;
        for (size_t k = 0; k < order.size(); k++) {
            float share = left / static_cast<float>(order.size() - k);
            size_t i = order[k];
            got[i] = std::max(titleHeight() + rowHeight() * 3, std::min(want[i], share));
            left -= got[i];
        }
        float y = margin;
        for (size_t i = 0; i < vis.size(); i++) {
            auto& st = g_windows[vis[i]];
            st.maxH = got[i];
            if (!st.userMoved) {
                st.x = margin + static_cast<float>(c) * (w + gap);
                st.y = y;
            }
            y += got[i] + gap;
        }
    }
}

static void drawWindow(const std::string& name, const std::vector<Hack*>& list, float slide) {
    if (!windowVisible(name, list.size())) return;
    auto& st = g_windows[name];
    auto& io = ImGui::GetIO();
    float width = windowWidth();
    float th = titleHeight();
    float maxH = st.userMoved || st.maxH <= 0 ? io.DisplaySize.y - st.y - S(6) : st.maxH;
    maxH = std::max(maxH, th + rowHeight() * 2);
    float content = estimatedHeight(name, list.size()) - th;
    float height = st.collapsed ? th : std::min(th + content, maxH);

    ImGui::SetNextWindowPos(ImVec2(st.x, st.y + slide), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(width, height), ImGuiCond_Always);
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar |
                             ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse |
                             ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove;
    std::string wid = "##mh_" + name;
    if (!ImGui::Begin(wid.c_str(), nullptr, flags)) {
        ImGui::End();
        return;
    }

    // barra del titolo: trascinando si sposta la finestra, "-" la chiude
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::InvisibleButton("##title", ImVec2(width, th));
    bool titleHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 2.f)) {
        st.x += io.MouseDelta.x;
        st.y += io.MouseDelta.y;
        st.x = std::clamp(st.x, 0.f, io.DisplaySize.x - width);
        st.y = std::clamp(st.y, 0.f, io.DisplaySize.y - th);
        st.userMoved = true;
        config::markDirty();
    }
    bool onCollapse = titleHovered && io.MousePos.x < p.x + S(24);
    if (ImGui::IsItemDeactivated() && io.MouseDragMaxDistanceSqr[0] < 16.f && onCollapse) {
        st.collapsed = !st.collapsed;
        config::markDirty();
    }
    dl->AddRectFilled(p, ImVec2(p.x + width, p.y + th), ImGui::GetColorU32(titleAccent()));
    const char* title = T(name);
    ImVec2 ts = ImGui::CalcTextSize(title);
    dl->AddText(ImVec2(p.x + (width - ts.x) * 0.5f, p.y + (th - ts.y) * 0.5f), IM_COL32(255, 255, 255, 255), title);
    const char* sym = st.collapsed ? "+" : "-";
    ImVec2 ss = ImGui::CalcTextSize(sym);
    dl->AddText(ImVec2(p.x + S(9), p.y + (th - ss.y) * 0.5f), onCollapse ? IM_COL32(255, 255, 255, 255) : IM_COL32(255, 255, 255, 190), sym);

    if (!st.collapsed) {
        // contenuto in una child scrollabile con la rotella se non entra nell'altezza assegnata
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
        ImGui::BeginChild("##content", ImVec2(width, height - th), ImGuiChildFlags_None, ImGuiWindowFlags_None);
        float startY = ImGui::GetCursorScreenPos().y;
        for (auto& c : contents())
            if (c.window == name && c.before) c.fn();
        for (auto* h : list) hackRow(*h);
        for (auto& c : contents())
            if (c.window == name && !c.before) c.fn();
        g_contentH[name] = ImGui::GetCursorScreenPos().y - startY;
        ImGui::EndChild();
        ImGui::PopStyleVar();
    }
    ImGui::End();
}

void drawMenu() {
    auto& io = ImGui::GetIO();
    float ms = static_cast<float>(std::max(0, config::ui().animMs));
    float step = ms > 0.f ? io.DeltaTime * 1000.f / ms : 1.f;
    g_anim = std::clamp(g_anim + (g_open ? step : -step), 0.f, 1.f);
    if (!g_open && (io.WantCaptureKeyboard || io.WantTextInput)) {
        // menu chiuso: nessun campo/finestra deve tenere la tastiera, cosi' i tasti tornano al gioco
        ImGui::ClearActiveID();
        ImGui::SetWindowFocus(nullptr);
    }
    if (g_anim <= 0.f) return;
    // ease-out cubica: le finestre scendono dall'alto e compaiono in dissolvenza
    float e = 1.f - std::pow(1.f - g_anim, 3.f);
    float slide = (1.f - e) * -S(40);

    std::map<std::string, std::vector<Hack*>> lists;
    std::map<std::string, size_t> rows;
    for (auto& name : windowOrder()) {
        lists[name] = hacksFor(name);
        rows[name] = lists[name].size();
    }
    autoLayout(rows);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, e);
    for (auto& name : windowOrder()) drawWindow(name, lists[name], slide);
    ImGui::PopStyleVar();
}

} // namespace mh::gui
