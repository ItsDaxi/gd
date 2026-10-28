// Dialog di conferma disegnato con ImGui sopra al gioco
#include "common.hpp"

using namespace mh;

namespace {
struct Pending {
    std::string message;
    void (*onYes)(void*) = nullptr;
    void* ctx = nullptr;
    bool active = false;
};
Pending s_confirm;
} // namespace

void mh::confirm(const std::string& message, void (*onYes)(void*), void* ctx) {
    s_confirm = {message, onYes, ctx, true};
}

static gui::Overlay s_confirmOverlay([] {
    if (!s_confirm.active) return;
    auto& io = ImGui::GetIO();
    float s = config::ui().scale;
    ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f), ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(320 * s, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12 * s, 10 * s));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8 * s, 8 * s));
    ImGui::Begin("##mh_confirm", nullptr,
                 ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::SetWindowFocus();
    ImGui::TextColored(ImGui::GetStyle().Colors[ImGuiCol_CheckMark], "%s", T("MESSAGE"));
    ImGui::Separator();
    ImGui::PushTextWrapPos(0);
    ImGui::TextUnformatted(s_confirm.message.c_str());
    ImGui::PopTextWrapPos();
    float w = (ImGui::GetContentRegionAvail().x - 8 * s) * 0.5f;
    bool yes = ImGui::Button(T("MESSAGE/YES"), ImVec2(w, 0)) || ImGui::IsKeyPressed(ImGuiKey_Enter);
    ImGui::SameLine();
    bool no = ImGui::Button(T("MESSAGE/NO"), ImVec2(w, 0)) || ImGui::IsKeyPressed(ImGuiKey_Escape);
    ImGui::End();
    ImGui::PopStyleVar(2);
    if (yes || no) {
        auto p = s_confirm;
        s_confirm.active = false;
        if (yes && p.onYes) p.onYes(p.ctx);
    }
});
