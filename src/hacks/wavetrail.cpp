// Custom Wave Trail: colore, spessore, pulse e opacita' della scia dell'onda.
// In GD 2.2 la scia e' un HardStreak (CCDrawNode -> CCNodeRGBA): il colore si imposta come per
// gli sprite, lo spessore e' m_waveSize e la pulsazione m_pulseSize (come Better Trail / QOLMod).
#include "common.hpp"

#include <unordered_map>

using namespace mh;

namespace {

int s_colorMode = 0; // 0 default, 1 fisso, 2 arcobaleno, 3 colore 1 del giocatore, 4 colore 2
float s_color[4] = {1.f, 1.f, 1.f, 1.f};
float s_rainbowSpeed = 1.f;
float s_thickness = 1.f;
int s_pulseMode = 0; // 0 default, 1 nessuna pulsazione, 2 moltiplicatore, 3 fissa
float s_pulseMul = 1.f;
float s_pulseFixed = 1.f;
float s_opacity = 1.f;
bool s_solid = false;
bool s_bothPlayers = true;

Hack& customWave = hack("COSMETIC", "COSMETIC/HACK_CUSTOM_WAVE_TRAIL").name("Custom Wave Trail")
    .optCombo("Colour", &s_colorMode, {"Default", "Static", "Rainbow", "Player Colour 1", "Player Colour 2"})
    .optColor("NX/COLOUR", s_color)
    .optFloat("Rainbow Speed", &s_rainbowSpeed, 0.05f, 10.f, "%.2fx")
    .optFloat("Thickness", &s_thickness, 0.1f, 5.f, "%.2fx")
    .optCombo("Pulse", &s_pulseMode, {"Default", "No Pulse", "Multiplier", "Fixed"})
    .optFloat("Pulse Multiplier", &s_pulseMul, 0.f, 5.f, "%.2fx")
    .optFloat("Fixed Pulse", &s_pulseFixed, 0.f, 3.f, "%.2f")
    .optFloat("Opacity", &s_opacity, 0.f, 1.f, "%.2f")
    .optBool("COSMETIC/HACK_SOLID_WAVE_TRAIL", &s_solid)
    .optBool("Player 2 too", &s_bothPlayers);

// GD riscrive spessore e pulse durante il gioco: si moltiplicano solo i valori nuovi
struct Tracked {
    float base = 0.f, written = -1.f;
};
std::unordered_map<void*, Tracked> s_size, s_pulse;

float scaled(std::unordered_map<void*, Tracked>& map, void* key, float current, float mul) {
    auto& t = map[key];
    if (current != t.written) t.base = current;
    t.written = t.base * mul;
    return t.written;
}

float s_time = 0.f;

cc::Color3B to3B(const float* c) {
    return {static_cast<uint8_t>(c[0] * 255.f), static_cast<uint8_t>(c[1] * 255.f), static_cast<uint8_t>(c[2] * 255.f)};
}

void apply(void* player, float phase) {
    if (!player) return;
    void* wave = GDF(player, PlayerObject, m_waveTrail, void*);
    if (!wave) return;

    // colore
    cc::Color3B col{};
    bool setCol = true;
    switch (s_colorMode) {
    case 1: col = to3B(s_color); break;
    case 2: {
        float r, g, b;
        ImGui::ColorConvertHSVtoRGB(std::fmod(s_time * 0.25f * s_rainbowSpeed + phase, 1.f), 1.f, 1.f, r, g, b);
        float c[4] = {r, g, b, 1.f};
        col = to3B(c);
        break;
    }
    case 3: col = GDF(player, PlayerObject, m_playerColor1, cc::Color3B); break;
    case 4: col = GDF(player, PlayerObject, m_playerColor2, cc::Color3B); break;
    default: setCol = false;
    }
    if (setCol) cc::setColor(wave, col);
    if (s_opacity < 0.999f) cc::setOpacity(wave, static_cast<uint8_t>(std::clamp(s_opacity, 0.f, 1.f) * 255.f));

    // spessore
    float& size = GDF(wave, HardStreak, m_waveSize, float);
    if (s_thickness != 1.f) size = scaled(s_size, wave, size, s_thickness);

    // pulsazione
    float& pulse = GDF(wave, HardStreak, m_pulseSize, float);
    switch (s_pulseMode) {
    case 1: pulse = 0.f; break;
    case 2: pulse = scaled(s_pulse, wave, pulse, s_pulseMul); break;
    case 3: pulse = s_pulseFixed; break;
    default: break;
    }

    if (s_solid) GDF(wave, HardStreak, m_isSolid, bool) = true;
}

} // namespace

MH_EVERY_FRAME(waveTrailTime) { s_time += ImGui::GetIO().DeltaTime; }

// dopo l'update del layer (gioco ed editor), prima che la scia venga disegnata
MH_ON(ev::bglPostUpdate, [](void* layer, float) {
    if (!customWave.enabled) return;
    apply(gd::player1(layer), 0.f);
    if (s_bothPlayers) apply(gd::player2(layer), 0.5f);
});

MH_ON(ev::plQuit, [](void*) {
    s_size.clear();
    s_pulse.clear();
});
