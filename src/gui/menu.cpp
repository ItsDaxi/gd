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
#include <windows.h>

namespace mh::gui {

std::string g_search;
static bool g_open = false;
static float g_alpha = 0.f;
static std::map<std::string, WindowState> g_windows;
static std::map<std::string, float> g_heights;
static std::vector<std::string> g_favourites;
static bool g_needSort = false;

WindowState& windowState(const std::string& name) { return g_windows[name]; }
std::map<std::string, WindowState>& windowStates() { return g_windows; }
std::vector<std::string>& favourites() { return g_favourites; }
bool isFavourite(const std::string& id) {
    return std::find(g_favourites.begin(), g_favourites.end(), id) != g_favourites.end();
}

bool menuOpen() { return g_open; }
void toggleMenu() { g_open = !g_open; }
void sortWindows() { g_needSort = true; }

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
    if (!h.displayName.empty() && !lang::has(h.id)) return h.displayName;
    return lang::tr(h.id);
}

static bool matchesSearch(const Hack& h) {
    if (g_search.empty()) return true;
    return lower(hackName(h)).find(lower(g_search)) != std::string::npos;
}

static float S(float v) { return v * config::ui().scale; }

// ------------------------------------------------------------ widget
static bool rowBase(const char* id, const char* label, bool active, bool hasOptions, bool* openOptions,
                    bool dot = true) {
    auto* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    float h = S(24);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(id);
    bool clicked = ImGui::InvisibleButton("##row", ImVec2(w, h), ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    bool hovered = ImGui::IsItemHovered();
    bool right = ImGui::IsItemClicked(ImGuiMouseButton_Right);
    ImGui::PopID();

    float optW = hasOptions ? S(18) : 0;
    bool onArrow = hasOptions && hovered && ImGui::GetIO().MousePos.x > p.x + w - optW;
    ImU32 bg = ImGui::GetColorU32(hovered ? rowHover() : rowColor());
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), bg);
    // separatore sottile tra le righe
    dl->AddLine(ImVec2(p.x, p.y + h - 1), ImVec2(p.x + w, p.y + h - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.25f)));

    ImU32 txt = ImGui::GetColorU32(active ? accent() : textColor());
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

bool buttonRow(const char* label) {
    auto* dl = ImGui::GetWindowDrawList();
    float w = ImGui::GetContentRegionAvail().x;
    float h = S(24);
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::PushID(label);
    bool clicked = ImGui::InvisibleButton("##btn", ImVec2(w, h));
    bool hovered = ImGui::IsItemHovered();
    bool held = ImGui::IsItemActive();
    ImGui::PopID();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), ImGui::GetColorU32(held ? ImVec4(accent().x, accent().y, accent().z, 0.4f) : hovered ? rowHover() : rowColor()));
    dl->AddLine(ImVec2(p.x, p.y + h - 1), ImVec2(p.x + w, p.y + h - 1), ImGui::GetColorU32(ImVec4(0, 0, 0, 0.25f)));
    ImVec2 ts = ImGui::CalcTextSize(label);
    dl->AddText(ImVec2(p.x + (w - ts.x) * 0.5f, p.y + (h - ts.y) * 0.5f), ImGui::GetColorU32(textColor()), label);
    return clicked;
}

void keyButton(const char* label, int* vk) {
    std::string text = capturingKey() ? std::string("...") : keyName(*vk);
    ImGui::TextUnformatted(label);
    ImGui::SameLine();
    ImGui::PushID(label);
    if (ImGui::Button(text.c_str(), ImVec2(-1, 0))) beginKeyCapture(vk);
    ImGui::PopID();
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
    ImGui::PushItemWidth(S(150));
    switch (o.type) {
    case OptType::Bool: changed = ImGui::Checkbox(label, static_cast<bool*>(o.ptr)); break;
    case OptType::Int:
        changed = ImGui::DragInt(label, static_cast<int*>(o.ptr), 0.25f, static_cast<int>(o.min), static_cast<int>(o.max), o.fmt ? o.fmt : "%d", ImGuiSliderFlags_AlwaysClamp);
        break;
    case OptType::Float:
        changed = ImGui::DragFloat(label, static_cast<float*>(o.ptr), (o.max - o.min) / 400.f, o.min, o.max, o.fmt ? o.fmt : "%.2f", ImGuiSliderFlags_AlwaysClamp);
        break;
    case OptType::Text: {
        auto* s = static_cast<std::string*>(o.ptr);
        char buf[256];
        snprintf(buf, sizeof(buf), "%s", s->c_str());
        if (ImGui::InputText(label, buf, sizeof(buf))) {
            *s = buf;
            changed = true;
        }
        break;
    }
    case OptType::Combo: {
        int* v = static_cast<int*>(o.ptr);
        int n = static_cast<int>(o.items.size());
        const char* preview = (*v >= 0 && *v < n) ? T(o.items[*v]) : "";
        if (ImGui::BeginCombo(label, preview)) {
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
    case OptType::Key: keyButton(label, static_cast<int*>(o.ptr)); break;
    case OptType::Color:
        changed = ImGui::ColorEdit4(label, static_cast<float*>(o.ptr), ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_AlphaBar);
        break;
    case OptType::Button:
        if (ImGui::Button(label, ImVec2(S(150), 0)) && o.onClick) o.onClick();
        break;
    case OptType::Label: ImGui::TextDisabled("%s", label); break;
    }
    ImGui::PopItemWidth();
    ImGui::PopID();
    if (changed) {
        config::markDirty();
        if (o.onChange) o.onChange();
    }
}

static void optionsPopup(Hack& h) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(S(8), S(8)));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(S(6), S(5)));
    if (ImGui::BeginPopup(("opts_" + h.id).c_str())) {
        ImGui::TextColored(titleAccent(), "%s", hackName(h).c_str());
        ImGui::Separator();
        const auto& tip = lang::tip(h.id);
        if (!tip.empty()) {
            ImGui::PushTextWrapPos(S(300));
            ImGui::TextDisabled("%s", tip.c_str());
            ImGui::PopTextWrapPos();
        }
        if (!h.extraTip.empty()) {
            ImGui::PushTextWrapPos(S(300));
            ImGui::TextColored(ImVec4(1, 0.75f, 0.3f, 1), "%s", h.extraTip.c_str());
            ImGui::PopTextWrapPos();
        }
        for (auto& o : h.options) drawOption(o);
        if (h.customUI) h.customUI();
        ImGui::Separator();
        keyButton(T("KEYBINDS/SHORTCUT"), &h.keybind);
        bool fav = isFavourite(h.id);
        if (ImGui::Button(fav ? "- Favourite" : "+ Favourite", ImVec2(-1, 0))) {
            if (fav) g_favourites.erase(std::remove(g_favourites.begin(), g_favourites.end(), h.id), g_favourites.end());
            else g_favourites.push_back(h.id);
            config::markDirty();
        }
        ImGui::EndPopup();
    }
    ImGui::PopStyleVar(2);
}

bool hackRow(Hack& h) {
    bool hasOptions = true; // keybind e preferiti sono sempre disponibili
    bool openOpts = false;
    std::string label = hackName(h);
    if (h.notice) label += " *";
    bool clicked = rowBase(h.id.c_str(), label.c_str(), h.enabled, hasOptions && (!h.options.empty() || h.customUI), &openOpts, !h.button);
    tooltip(h);
    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) openOpts = true;
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
    std::sort(out.begin(), out.end(), [](Hack* a, Hack* b) { return lower(hackName(*a)) < lower(hackName(*b)); });
    return out;
}

static void layoutDefault(const std::vector<std::string>& windows) {
    float x = S(8), y = S(8);
    float maxH = ImGui::GetIO().DisplaySize.y - S(8);
    float colW = S(200) + S(6);
    for (auto& name : windows) {
        float h = g_heights.count(name) ? g_heights[name] : S(24) * 6;
        if (y > S(8) && y + h > maxH) {
            x += colW;
            y = S(8);
        }
        auto& st = g_windows[name];
        st.x = x;
        st.y = y;
        st.placed = true;
        y += std::min(h, maxH - S(8)) + S(6);
    }
}

static void drawWindow(const std::string& name) {
    auto list = hacksFor(name);
    bool hasBefore = false, hasAfter = false;
    for (auto& c : contents()) {
        if (c.window != name) continue;
        (c.before ? hasBefore : hasAfter) = true;
    }
    if (!g_search.empty() && list.empty() && name != "HOME") return;
    if (list.empty() && !hasBefore && !hasAfter && name != "FAVOURITES") return;

    auto& st = g_windows[name];
    float width = S(200);
    ImGui::SetNextWindowPos(ImVec2(st.x, st.y), g_needSort ? ImGuiCond_Always : ImGuiCond_Once);
    ImGui::SetNextWindowSizeConstraints(ImVec2(width, 0), ImVec2(width, ImGui::GetIO().DisplaySize.y - S(16)));
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize |
                             ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings;
    std::string wid = "##mh_" + name;
    if (!ImGui::Begin(wid.c_str(), nullptr, flags)) {
        ImGui::End();
        return;
    }
    auto pos = ImGui::GetWindowPos();
    st.x = pos.x;
    st.y = pos.y;

    // barra del titolo
    auto* dl = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    float th = S(24);
    ImGui::InvisibleButton("##title", ImVec2(width, th));
    bool titleHovered = ImGui::IsItemHovered();
    if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
        auto d = ImGui::GetIO().MouseDelta;
        ImGui::SetWindowPos(ImVec2(pos.x + d.x, pos.y + d.y));
        config::markDirty();
    }
    bool onCollapse = titleHovered && ImGui::GetIO().MousePos.x < p.x + S(24);
    if (ImGui::IsItemDeactivated() && onCollapse && ImGui::GetIO().MouseDragMaxDistanceSqr[0] < 16.f) {
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
        float maxH = ImGui::GetIO().DisplaySize.y - st.y - th - S(12);
        float rows = static_cast<float>(list.size()) * S(24);
        bool scroll = !hasBefore && !hasAfter && rows > maxH && maxH > S(48);
        if (scroll) ImGui::BeginChild("##rows", ImVec2(width, maxH), ImGuiChildFlags_None, ImGuiWindowFlags_NoScrollbar);
        for (auto& c : contents())
            if (c.window == name && c.before) c.fn();
        for (auto* h : list) hackRow(*h);
        for (auto& c : contents())
            if (c.window == name && !c.before) c.fn();
        if (scroll) ImGui::EndChild();
    }
    g_heights[name] = ImGui::GetWindowHeight();
    ImGui::End();
}

void drawMenu() {
    float target = g_open ? 1.f : 0.f;
    float dt = ImGui::GetIO().DeltaTime;
    float speed = config::ui().animMs > 0 ? 1000.f / static_cast<float>(config::ui().animMs) : 1000.f;
    g_alpha += (target - g_alpha) * std::min(1.f, dt * speed * 1.5f);
    if (std::abs(g_alpha - target) < 0.01f) g_alpha = target;
    if (g_alpha <= 0.f) {
        // menu chiuso: nessuna finestra deve tenere il focus della tastiera
        if (ImGui::GetIO().WantCaptureKeyboard && !ImGui::GetIO().WantTextInput) ImGui::SetWindowFocus(nullptr);
        return;
    }

    std::vector<std::string> windows;
    for (auto& name : windowOrder()) windows.push_back(name);

    bool anyUnplaced = false;
    for (auto& n : windows)
        if (!g_windows[n].placed) anyUnplaced = true;
    if (anyUnplaced || g_needSort) layoutDefault(windows);

    ImGui::PushStyleVar(ImGuiStyleVar_Alpha, g_alpha);
    for (auto& name : windows) drawWindow(name);
    ImGui::PopStyleVar();
    if (g_needSort) {
        g_needSort = false;
        config::markDirty();
    }
}

} // namespace mh::gui
