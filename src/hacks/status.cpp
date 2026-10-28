// Finestra STATUS: etichette sovrapposte al gioco
#include "common.hpp"

#include <chrono>
#include <ctime>
#include <deque>

using namespace mh;

namespace mh::stats {
extern int noclipDeaths;
float noclipAccuracy();
} // namespace mh::stats
namespace mh::replay {
const char* stateText();
}

namespace {

float s_opacity = 1.f, s_scale = 1.f;
bool s_hideAll = false;

struct Field {
    const char* key;      // es. "STATUS/FPS" -> hack "STATUS/FPS/ENABLED"
    int position = 0;     // 0..5 (STATUS/*/POSITION/n)
    bool prefix = true;
    bool units = true;
    Hack* hack = nullptr;
};

std::vector<std::string> positionItems() {
    std::vector<std::string> v;
    for (int i = 0; i < 6; i++) v.push_back("STATUS/*/POSITION/" + std::to_string(i));
    return v;
}

Field s_fields[] = {
    {"STATUS/MESSAGE"}, {"STATUS/TESTMODE"}, {"STATUS/CHEAT_INDICATOR"}, {"STATUS/FPS"}, {"STATUS/CPS"},
    {"STATUS/BEST_RUN"}, {"STATUS/NOCLIP_ACCURACY"}, {"STATUS/NOCLIP_DEATHS"}, {"STATUS/ATTEMPTS"},
    {"STATUS/JUMPS"}, {"STATUS/LEVEL_TIME"}, {"STATUS/PERCENTAGE"}, {"STATUS/CLOCK"}, {"STATUS/FRAME"},
    {"STATUS/POSITION"}, {"STATUS/VELOCITY"}, {"STATUS/DEAD"}, {"STATUS/REPLAY_STATE"}, {"STATUS/SESSION_TIME"},
};

std::string s_message = "Mega Hack GD";
bool s_hideWhenLegit = false, s_cpsMax = true, s_cpsTotal = false, s_bestAccurate = false, s_bestOnlyCurrent = false;
bool s_twelveHour = false;

// --- dati
std::deque<double> s_clicks;
int s_cpsMaxValue = 0, s_clicksTotal = 0;
float s_runStart = 0.f, s_bestFrom = 0.f, s_bestTo = 0.f;
int s_frame = 0;
auto s_sessionStart = std::chrono::steady_clock::now();

double nowSec() {
    using namespace std::chrono;
    return duration<double>(steady_clock::now().time_since_epoch()).count();
}

bool registerFields() {
    for (auto& f : s_fields) {
        std::string id = std::string(f.key) + "/ENABLED";
        auto& h = hack("STATUS", id);
        h.optCombo("STATUS/*/POSITION", &f.position, positionItems())
            .optBool("STATUS/*/PREFIX", &f.prefix)
            .optBool("STATUS/*/UNITS", &f.units);
        f.hack = &h;
    }
    s_fields[0].hack->optText("STATUS/MESSAGE/DROPDOWN/MESSAGE", &s_message);
    s_fields[2].hack->optBool("STATUS/CHEAT_INDICATOR/DROPDOWN/HIDE_WHEN_LEGIT", &s_hideWhenLegit);
    s_fields[4].hack->optBool("STATUS/CPS/DROPDOWN/MAX", &s_cpsMax).optBool("STATUS/CPS/DROPDOWN/TOTAL", &s_cpsTotal);
    s_fields[5].hack->optBool("STATUS/BEST_RUN/DROPDOWN/ACCURATE_PERCENTAGE", &s_bestAccurate)
        .optBool("STATUS/BEST_RUN/DROPDOWN/ONLY_CURRENT", &s_bestOnlyCurrent);
    s_fields[12].hack->optBool("STATUS/CLOCK/DROPDOWN/TWELVE_HOUR", &s_twelveHour);
    // valori predefiniti: alcune etichette in alto a destra
    s_fields[3].position = 1;
    s_fields[12].position = 1;
    s_fields[18].position = 1;
    return true;
}
[[maybe_unused]] bool s_registered = registerFields();
// impostazioni generali salvate nel config (finestra nascosta "_settings")
[[maybe_unused]] Hack& s_statusSettings = hack("_settings", "STATUS/SETTINGS")
    .optFloat("STATUS/COSMETIC/OPACITY", &s_opacity, 0.f, 1.f)
    .optFloat("STATUS/COSMETIC/SCALE", &s_scale, 0.25f, 4.f)
    .optBool("STATUS/HIDE", &s_hideAll);

} // namespace

// --- raccolta dati
MH_ON(ev::bglButton, [](void* layer, bool down, int button, bool, bool& allow) {
    if (!allow || !down || button != 1 || layer != gd::playLayer()) return;
    s_clicks.push_back(nowSec());
    s_clicksTotal++;
});
MH_ON(ev::plReset, [](void* pl) {
    s_frame = 0;
    s_clicksTotal = 0;
    s_runStart = gd::percent();
    (void)pl;
});
MH_ON(ev::bglPostUpdate, [](void* layer, float) {
    if (layer == gd::playLayer()) s_frame++;
});
MH_ON(ev::plDeath, [](void*, void*) {
    float end = gd::percent();
    if (end - s_runStart > s_bestTo - s_bestFrom) {
        s_bestFrom = s_runStart;
        s_bestTo = end;
    }
});
MH_ON(ev::plInit, [](void*) {
    s_bestFrom = s_bestTo = 0.f;
    s_cpsMaxValue = 0;
});

namespace {

std::string fieldText(Field& f, ImU32& color) {
    char buf[160];
    auto pl = gd::playLayer();
    auto p1 = pl ? gd::player1(pl) : nullptr;
    auto prefix = [&](const char* p) { return f.prefix ? std::string(p) : std::string(); };
    std::string k = f.key;
    color = IM_COL32(255, 255, 255, 255);
    if (k == "STATUS/MESSAGE") return s_message;
    if (k == "STATUS/TESTMODE") return (pl && GDF(pl, GJBaseGameLayer, m_isTestMode, bool)) ? "Testmode" : "";
    if (k == "STATUS/CHEAT_INDICATOR") {
        bool cheat = anyCheatEnabled() || g_cheatedThisAttempt;
        if (!cheat && s_hideWhenLegit) return "";
        color = cheat ? IM_COL32(255, 60, 60, 255) : IM_COL32(60, 255, 90, 255);
        return ".";
    }
    if (k == "STATUS/FPS") {
        snprintf(buf, sizeof(buf), "%s%.0f", prefix("FPS: ").c_str(), ImGui::GetIO().Framerate);
        return buf;
    }
    if (k == "STATUS/CPS") {
        double t = nowSec();
        while (!s_clicks.empty() && t - s_clicks.front() > 1.0) s_clicks.pop_front();
        int cps = static_cast<int>(s_clicks.size());
        s_cpsMaxValue = std::max(s_cpsMaxValue, cps);
        std::string s = prefix("CPS: ") + std::to_string(cps);
        if (s_cpsMax) s += "/" + std::to_string(s_cpsMaxValue);
        if (s_cpsTotal) s += " (" + std::to_string(s_clicksTotal) + ")";
        return s;
    }
    if (k == "STATUS/BEST_RUN") {
        if (s_bestTo <= 0.f) return "";
        if (s_bestOnlyCurrent && s_bestFrom > 0.5f) return "";
        if (s_bestAccurate) snprintf(buf, sizeof(buf), "%s%.2f-%.2f%%", prefix("Best Run: ").c_str(), s_bestFrom, s_bestTo);
        else snprintf(buf, sizeof(buf), "%s%d-%d%%", prefix("Best Run: ").c_str(), static_cast<int>(s_bestFrom), static_cast<int>(s_bestTo));
        return buf;
    }
    if (k == "STATUS/NOCLIP_ACCURACY") {
        snprintf(buf, sizeof(buf), "%s%.2f%s", prefix("Accuracy: ").c_str(), stats::noclipAccuracy(), f.units ? "%" : "");
        if (stats::noclipDeaths > 0) color = IM_COL32(255, 120, 120, 255);
        return buf;
    }
    if (k == "STATUS/NOCLIP_DEATHS") {
        snprintf(buf, sizeof(buf), "%s%d", prefix("Deaths: ").c_str(), stats::noclipDeaths);
        return buf;
    }
    if (k == "STATUS/ATTEMPTS") return prefix("Attempt ") + std::to_string(gd::attempts());
    if (k == "STATUS/JUMPS") return prefix("Jumps: ") + std::to_string(pl ? GDF(pl, PlayLayer, m_jumps, int) : 0);
    if (k == "STATUS/LEVEL_TIME") {
        double t = gd::levelTime();
        snprintf(buf, sizeof(buf), "%s%d:%05.2f", prefix("Time: ").c_str(), static_cast<int>(t / 60), std::fmod(t, 60.0));
        return buf;
    }
    if (k == "STATUS/PERCENTAGE") {
        snprintf(buf, sizeof(buf), "%s%.2f%s", prefix("Progress: ").c_str(), gd::percent(), f.units ? "%" : "");
        return buf;
    }
    if (k == "STATUS/CLOCK") {
        std::time_t t = std::time(nullptr);
        std::tm tmv{};
        localtime_s(&tmv, &t);
        strftime(buf, sizeof(buf), s_twelveHour ? "%I:%M:%S %p" : "%H:%M:%S", &tmv);
        return buf;
    }
    if (k == "STATUS/FRAME") return prefix("Frame: ") + std::to_string(s_frame);
    if (k == "STATUS/POSITION") {
        if (!p1) return "";
        auto p = cc::position(p1);
        snprintf(buf, sizeof(buf), "%sX %.2f  Y %.2f", prefix("Pos: ").c_str(), p.x, p.y);
        return buf;
    }
    if (k == "STATUS/VELOCITY") {
        if (!p1) return "";
        snprintf(buf, sizeof(buf), "%s%.3f", prefix("Velocity: ").c_str(), GDF(p1, PlayerObject, m_yVelocity, double));
        return buf;
    }
    if (k == "STATUS/DEAD") {
        bool dead = p1 && GDF(p1, PlayerObject, m_isDead, bool);
        return prefix("Dead: ") + (dead ? "true" : "false");
    }
    if (k == "STATUS/REPLAY_STATE") return replay::stateText();
    if (k == "STATUS/SESSION_TIME") {
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - s_sessionStart).count();
        snprintf(buf, sizeof(buf), "%s%02lld:%02lld:%02lld", prefix("Session: ").c_str(), static_cast<long long>(secs / 3600),
                 static_cast<long long>((secs / 60) % 60), static_cast<long long>(secs % 60));
        return buf;
    }
    return "";
}

} // namespace

static gui::Overlay s_statusOverlay([] {
    if (s_hideAll || !gd::playLayer()) return;
    auto* dl = ImGui::GetBackgroundDrawList();
    auto* font = gui::bigFont();
    auto sz = ImGui::GetIO().DisplaySize;
    float fontSize = 18.f * s_scale * std::max(0.6f, sz.y / 900.f);
    float pad = 6.f;
    float y[6] = {pad, pad, sz.y - pad, sz.y - pad, pad, sz.y - pad};
    uint8_t alpha = static_cast<uint8_t>(std::clamp(s_opacity, 0.f, 1.f) * 255);
    for (auto& f : s_fields) {
        if (!f.hack || !f.hack->enabled) continue;
        ImU32 color;
        std::string text = fieldText(f, color);
        if (text.empty()) continue;
        float size = (std::string(f.key) == "STATUS/CHEAT_INDICATOR") ? fontSize * 2.f : fontSize;
        ImVec2 ts = font->CalcTextSizeA(size, FLT_MAX, 0, text.c_str());
        int p = std::clamp(f.position, 0, 5);
        bool bottom = p == 2 || p == 3 || p == 5;
        float x = (p == 0 || p == 2) ? pad : (p == 1 || p == 3) ? sz.x - ts.x - pad : (sz.x - ts.x) * 0.5f;
        float yy = bottom ? y[p] - ts.y : y[p];
        color = (color & 0x00FFFFFF) | (static_cast<ImU32>(alpha) << 24);
        dl->AddText(font, size, ImVec2(x + 1.5f, yy + 1.5f), IM_COL32(0, 0, 0, alpha / 2), text.c_str());
        dl->AddText(font, size, ImVec2(x, yy), color, text.c_str());
        y[p] += bottom ? -ts.y : ts.y;
    }
});

// Riga in cima alla finestra Status: Opacity / Scale / Hide (come nell'immagine di riferimento)
static gui::WindowContent s_statusHeader("STATUS", [] {
    float w = ImGui::GetContentRegionAvail().x;
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
    ImGui::PushItemWidth(w * 0.5f);
    if (ImGui::DragFloat("##op", &s_opacity, 0.01f, 0.f, 1.f, "Opacity: %.2fx")) config::markDirty();
    ImGui::SameLine(0, 0);
    if (ImGui::DragFloat("##sc", &s_scale, 0.01f, 0.25f, 4.f, "Scale: %.2fx")) config::markDirty();
    ImGui::PopItemWidth();
    ImGui::PopStyleVar();
    gui::checkboxRow(T("STATUS/HIDE"), &s_hideAll);
}, true);
