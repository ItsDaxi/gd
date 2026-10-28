// Finestra ICONIC ("Icons"): effetti di colore per le icone del giocatore
#include "common.hpp"

using namespace mh;

namespace {

enum Mode { NoEffect = 0, Static, Rainbow, Fade, Count };
const char* modeName(int m) {
    static const char* names[] = {"No Effect", "Static", "Rainbow", "Fade"};
    return names[std::clamp(m, 0, Count - 1)];
}

struct Channel {
    const char* label;    // chiave di traduzione
    int mode = NoEffect;
    float a[4] = {1, 1, 1, 1};
    float b[4] = {0, 0, 0, 1};
    float speed = 1.f;
};

enum { Col1, Col2, Glow, Trail, ShipTrail, ChannelCount };
struct PlayerFx {
    Channel ch[ChannelCount] = {{"ICONIC/PREVIEW/COL1"}, {"ICONIC/PREVIEW/COL2"}, {"ICONIC/PREVIEW/GLOW"},
                                {"ICONIC/PREVIEW/TRAIL"}, {"ICONIC/PREVIEW/SHIP_FIRE"}};
};
PlayerFx s_fx[2];
int s_selected = 0;
bool s_sameDual = false;
float s_cycleSpeed = 1.f;
float s_time = 0.f;

Hack& effects = hack("ICONIC", "ICONIC/EFFECTS").name("Icon Effects");

cc::Color3B evaluate(const Channel& c, float phase) {
    auto to8 = [](float v) { return static_cast<uint8_t>(std::clamp(v, 0.f, 1.f) * 255.f); };
    float t = s_time * c.speed * s_cycleSpeed + phase;
    switch (c.mode) {
    case Static: return {to8(c.a[0]), to8(c.a[1]), to8(c.a[2])};
    case Rainbow: {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(std::fmod(t * 0.25f, 1.f), 1.f, 1.f, r, g, b);
        return {to8(r), to8(g), to8(b)};
    }
    case Fade: {
        float k = 0.5f + 0.5f * std::sin(t * 3.14159f);
        return {to8(c.a[0] + (c.b[0] - c.a[0]) * k), to8(c.a[1] + (c.b[1] - c.a[1]) * k), to8(c.a[2] + (c.b[2] - c.a[2]) * k)};
    }
    default: return {255, 255, 255};
    }
}

void apply(void* player, const PlayerFx& fx, float phase) {
    if (!player) return;
    for (int i = 0; i < ChannelCount; i++) {
        const auto& c = fx.ch[i];
        if (c.mode == NoEffect) continue;
        cc::Color3B col = evaluate(c, phase);
        switch (i) {
        case Col1: gd::call<void>(gd::addr::PlayerObject::setColor, player, static_cast<const cc::Color3B*>(&col)); break;
        case Col2: gd::call<void>(gd::addr::PlayerObject::setSecondColor, player, static_cast<const cc::Color3B*>(&col)); break;
        case Glow:
            GDF(player, PlayerObject, m_hasCustomGlowColor, bool) = true;
            GDF(player, PlayerObject, m_glowColor, cc::Color3B) = col;
            gd::call<void>(gd::addr::PlayerObject::updateGlowColor, player);
            break;
        case Trail: cc::setColor(GDF(player, PlayerObject, m_regularTrail, void*), col); break;
        case ShipTrail: cc::setColor(GDF(player, PlayerObject, m_shipStreak, void*), col); break;
        }
    }
}

void channelUI(Channel& c, int idx) {
    ImGui::PushID(idx);
    char label[96];
    snprintf(label, sizeof(label), "%s: %s", T(c.label), modeName(c.mode));
    if (gui::buttonRow(label)) {
        c.mode = (c.mode + 1) % Count;
        config::markDirty();
    }
    if (c.mode == Static || c.mode == Fade) {
        ImGui::Indent(6);
        if (ImGui::ColorEdit3("A", c.a, ImGuiColorEditFlags_NoInputs)) config::markDirty();
        if (c.mode == Fade) {
            ImGui::SameLine();
            if (ImGui::ColorEdit3("B", c.b, ImGuiColorEditFlags_NoInputs)) config::markDirty();
        }
        ImGui::Unindent(6);
    }
    if (c.mode == Rainbow || c.mode == Fade) {
        ImGui::PushItemWidth(-1);
        if (ImGui::DragFloat("##spd", &c.speed, 0.01f, 0.05f, 10.f, "Speed: %.2fx")) config::markDirty();
        ImGui::PopItemWidth();
    }
    ImGui::PopID();
}

void iconicUI() {
    float w = ImGui::GetContentRegionAvail().x;
    // selettore Player 1 / Player 2 come nell'immagine di riferimento
    for (int p = 0; p < 2; p++) {
        if (p) ImGui::SameLine(0, 0);
        bool sel = s_selected == p;
        ImGui::PushStyleColor(ImGuiCol_Button, sel ? ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] : ImGui::GetStyle().Colors[ImGuiCol_FrameBg]);
        if (ImGui::Button(p ? "Player 2" : "Player 1", ImVec2(w * 0.5f, 22 * config::ui().scale))) s_selected = p;
        ImGui::PopStyleColor();
    }
    auto& fx = s_fx[s_selected];
    for (int i = 0; i < ChannelCount; i++) channelUI(fx.ch[i], i);
    ImGui::PushItemWidth(-1);
    if (ImGui::DragFloat("##cycle", &s_cycleSpeed, 0.01f, 0.05f, 10.f, "Cycle Speed: %.2fx")) config::markDirty();
    ImGui::PopItemWidth();
    gui::checkboxRow("Same Dual Colour", &s_sameDual);
}

[[maybe_unused]] bool s_init = [] {
    auto& settings = hack("_settings", "ICONIC/SETTINGS");
    settings.optBool("Same Dual Colour", &s_sameDual).optFloat("Cycle Speed", &s_cycleSpeed, 0.05f, 10.f);
    for (int p = 0; p < 2; p++) {
        for (int i = 0; i < ChannelCount; i++) {
            auto& c = s_fx[p].ch[i];
            std::string base = "ICONIC/P" + std::to_string(p + 1) + "/" + c.label;
            settings.optInt(base + "/MODE", &c.mode, 0, Count - 1)
                .optColor(base + "/A", c.a)
                .optColor(base + "/B", c.b)
                .optFloat(base + "/SPEED", &c.speed, 0.05f, 10.f);
        }
    }
    return true;
}();

} // namespace

static gui::WindowContent s_iconicUI("ICONIC", iconicUI);

MH_EVERY_FRAME(iconic) { s_time += ImGui::GetIO().DeltaTime; }

MH_ON(ev::plPostUpdate, [](void* pl, float) {
    if (!effects.enabled) return;
    apply(gd::player1(pl), s_fx[0], 0.f);
    apply(gd::player2(pl), s_sameDual ? s_fx[0] : s_fx[1], s_sameDual ? 0.f : 0.5f);
});
