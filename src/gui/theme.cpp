#include "theme.hpp"
#include "hacks/config.hpp"

#include <algorithm>
#include <cstring>

namespace mh::gui {

struct Preset {
    float accent[4];
    float title[4];
    float bg[4];
    bool darkText;
};

// Nona, Nona Lux, Octavus, Septima, Sextus, Festive (nomi da LIVE.json)
static const Preset kPresets[] = {
    {{0.55f, 0.30f, 0.95f, 1}, {0.62f, 0.38f, 1.00f, 1}, {0.11f, 0.11f, 0.12f, 0.97f}, false},
    {{0.55f, 0.30f, 0.95f, 1}, {0.62f, 0.38f, 1.00f, 1}, {0.93f, 0.93f, 0.95f, 0.97f}, true},
    {{0.20f, 0.55f, 1.00f, 1}, {0.25f, 0.60f, 1.00f, 1}, {0.09f, 0.10f, 0.13f, 0.97f}, false},
    {{0.20f, 0.80f, 0.45f, 1}, {0.25f, 0.85f, 0.50f, 1}, {0.09f, 0.12f, 0.10f, 0.97f}, false},
    {{0.95f, 0.30f, 0.35f, 1}, {1.00f, 0.38f, 0.40f, 1}, {0.12f, 0.09f, 0.09f, 0.97f}, false},
    {{0.85f, 0.15f, 0.20f, 1}, {0.15f, 0.65f, 0.30f, 1}, {0.10f, 0.10f, 0.10f, 0.97f}, false},
};

int presetCount() { return static_cast<int>(sizeof(kPresets) / sizeof(kPresets[0])); }

void applyPreset(int preset) {
    preset = std::clamp(preset, 0, presetCount() - 1);
    auto& ui = config::ui();
    ui.theme = preset;
    std::memcpy(ui.accent, kPresets[preset].accent, sizeof(ui.accent));
    std::memcpy(ui.titleAccent, kPresets[preset].title, sizeof(ui.titleAccent));
    std::memcpy(ui.background, kPresets[preset].bg, sizeof(ui.background));
    ui.darkText = kPresets[preset].darkText;
    applyTheme();
}

static ImVec4 v4(const float* c) { return ImVec4(c[0], c[1], c[2], c[3]); }

ImVec4 accent() { return v4(config::ui().accent); }
ImVec4 titleAccent() { return v4(config::ui().titleAccent); }
ImU32 accentU32(float alpha) {
    auto a = accent();
    a.w *= alpha;
    return ImGui::ColorConvertFloat4ToU32(a);
}
ImVec4 textColor() { return config::ui().darkText ? ImVec4(0.08f, 0.08f, 0.1f, 1) : ImVec4(0.95f, 0.95f, 0.97f, 1); }
ImVec4 dimText() { return config::ui().darkText ? ImVec4(0.4f, 0.4f, 0.45f, 1) : ImVec4(0.55f, 0.55f, 0.6f, 1); }
ImVec4 rowColor() {
    auto b = v4(config::ui().background);
    return ImVec4(b.x, b.y, b.z, b.w);
}
ImVec4 rowHover() {
    auto b = v4(config::ui().background);
    float d = config::ui().darkText ? -0.06f : 0.06f;
    return ImVec4(b.x + d, b.y + d, b.z + d, b.w);
}

void applyTheme() {
    auto& s = ImGui::GetStyle();
    s = ImGuiStyle();
    float scale = config::ui().scale;
    s.WindowPadding = ImVec2(0, 0);
    s.WindowRounding = 0;
    s.WindowBorderSize = 0;
    s.FramePadding = ImVec2(6, 3);
    s.FrameRounding = 2;
    s.ItemSpacing = ImVec2(4, 0);
    s.ItemInnerSpacing = ImVec2(4, 2);
    s.ScrollbarSize = 6;
    s.ScrollbarRounding = 3;
    s.PopupRounding = 2;
    s.PopupBorderSize = 1;
    s.GrabRounding = 2;
    s.GrabMinSize = 8;
    s.ScaleAllSizes(scale);

    auto* c = s.Colors;
    auto acc = accent();
    auto bg = rowColor();
    auto txt = textColor();
    c[ImGuiCol_Text] = txt;
    c[ImGuiCol_TextDisabled] = dimText();
    c[ImGuiCol_WindowBg] = bg;
    c[ImGuiCol_ChildBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_PopupBg] = ImVec4(bg.x, bg.y, bg.z, 0.98f);
    c[ImGuiCol_Border] = ImVec4(acc.x, acc.y, acc.z, 0.5f);
    c[ImGuiCol_FrameBg] = rowHover();
    c[ImGuiCol_FrameBgHovered] = ImVec4(acc.x, acc.y, acc.z, 0.25f);
    c[ImGuiCol_FrameBgActive] = ImVec4(acc.x, acc.y, acc.z, 0.4f);
    c[ImGuiCol_TitleBg] = titleAccent();
    c[ImGuiCol_TitleBgActive] = titleAccent();
    c[ImGuiCol_CheckMark] = acc;
    c[ImGuiCol_SliderGrab] = acc;
    c[ImGuiCol_SliderGrabActive] = titleAccent();
    c[ImGuiCol_Button] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
    c[ImGuiCol_ButtonHovered] = ImVec4(acc.x, acc.y, acc.z, 0.6f);
    c[ImGuiCol_ButtonActive] = acc;
    c[ImGuiCol_Header] = ImVec4(acc.x, acc.y, acc.z, 0.35f);
    c[ImGuiCol_HeaderHovered] = ImVec4(acc.x, acc.y, acc.z, 0.5f);
    c[ImGuiCol_HeaderActive] = acc;
    c[ImGuiCol_ScrollbarBg] = ImVec4(0, 0, 0, 0);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(acc.x, acc.y, acc.z, 0.6f);
    c[ImGuiCol_Separator] = ImVec4(acc.x, acc.y, acc.z, 0.4f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(acc.x, acc.y, acc.z, 0.4f);
}

} // namespace mh::gui
