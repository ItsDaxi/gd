// Finestra REPLAY: registrazione e riproduzione di macro (input per frame)
#include "common.hpp"
#include "core/paths.hpp"

#include <nlohmann/json.hpp>
#include <shellapi.h>
#include <windows.h>

#include <fstream>
#include <unordered_map>

using namespace mh;
using json = nlohmann::json;

namespace {

struct Input {
    int frame;
    int button;
    bool down;
    bool p2;
};

std::vector<Input> s_inputs;
std::string s_fileName = "macro";
std::string s_loadedName;
bool s_autoSave = false, s_allowRandom = false, s_blockInputs = true;
bool s_dirty = false;
int s_frame = 0;
size_t s_playIndex = 0;
bool s_injecting = false;
std::unordered_map<void*, int> s_checkpointFrames;

std::filesystem::path replayDir() { return paths::dataDir() / "replays"; }

bool saveMacro(const std::string& name) {
    if (name.empty()) return false;
    json j;
    j["version"] = 1;
    j["framework"] = "megahack-gd";
    j["level"] = gd::levelName(gd::level());
    auto& arr = j["inputs"] = json::array();
    for (auto& in : s_inputs) arr.push_back({{"frame", in.frame}, {"btn", in.button}, {"down", in.down}, {"2p", in.p2}});
    std::ofstream f(replayDir() / (name + ".json"));
    if (!f) return false;
    f << j.dump();
    s_dirty = false;
    s_loadedName = name;
    return true;
}

bool loadMacro(const std::filesystem::path& p) {
    std::ifstream f(p);
    if (!f) return false;
    try {
        auto j = json::parse(f);
        s_inputs.clear();
        for (auto& in : j.at("inputs")) {
            Input i{};
            i.frame = in.value("frame", 0);
            i.button = in.value("btn", 1);
            i.down = in.value("down", false);
            i.p2 = in.value("2p", false);
            s_inputs.push_back(i);
        }
        std::sort(s_inputs.begin(), s_inputs.end(), [](const Input& a, const Input& b) { return a.frame < b.frame; });
        s_loadedName = p.stem().string();
        s_dirty = false;
        return true;
    } catch (const std::exception& e) {
        MH_WARN("macro non valida %s: %s", p.string().c_str(), e.what());
        return false;
    }
}

Hack& record = hack("REPLAY", "REPLAY/STATE/RECORD").isCheat();
Hack& play = hack("REPLAY", "REPLAY/STATE/PLAY").isCheat();

std::string s_error;

void replayUI() {
    gui::inputRow("##macroname", T("NX/PICKER/INPUT"), &s_fileName);
    if (gui::beginComboRow("##macrofile", s_loadedName.empty() ? T("REPLAY/NONE") : s_loadedName.c_str())) {
        std::error_code ec;
        bool any = false;
        for (auto& e : std::filesystem::directory_iterator(replayDir(), ec)) {
            if (e.path().extension() != ".json") continue;
            any = true;
            auto stem = e.path().stem().string();
            if (ImGui::Selectable(stem.c_str(), stem == s_loadedName)) {
                if (loadMacro(e.path())) s_fileName = stem;
                else s_error = lang::tr("ERROR/REPLAY/FILE_NOT_FOUND");
            }
        }
        if (!any) ImGui::TextDisabled("%s", T("NX/PICKER/EMPTY"));
        ImGui::EndCombo();
    }
    int r = gui::buttonPair(T("REPLAY/SAVE"), T("REPLAY/OPEN"));
    if (r == 1) {
        if (s_fileName.empty()) s_error = lang::tr("ERROR/REPLAY/EMPTY_FILENAME");
        else if (s_inputs.empty()) s_error = lang::tr("ERROR/REPLAY/NO_INPUTS");
        else saveMacro(s_fileName);
    } else if (r == 2) {
        if (!loadMacro(replayDir() / (s_fileName + ".json"))) s_error = lang::tr("ERROR/REPLAY/FILE_NOT_FOUND");
    }
    r = gui::buttonPair(T("REPLAY/END_LAYOUT/CLEAR"), T("REPLAY/DELETE"));
    if (r == 1) {
        if (s_dirty) {
            confirm(lang::tr("MESSAGE/REPLAY/CONFIRM_CLEAR"), [](void*) {
                s_inputs.clear();
                s_loadedName.clear();
                s_dirty = false;
            }, nullptr);
        } else {
            s_inputs.clear();
            s_loadedName.clear();
        }
    } else if (r == 2 && !s_loadedName.empty()) {
        confirm(lang::tr("MESSAGE/REPLAY/CONFIRM_DELETE"), [](void*) {
            std::error_code ec;
            std::filesystem::remove(replayDir() / (s_loadedName + ".json"), ec);
            s_loadedName.clear();
        }, nullptr);
    }
    gui::checkboxRow(T("REPLAY/SAVE_LAYOUT/AUTO_SAVE"), &s_autoSave);
    gui::checkboxRow(T("REPLAY/GAMEPLAY/ALLOW_RANDOMNESS"), &s_allowRandom);
    gui::checkboxRow(T("REPLAY/GAMEPLAY/BLOCK_INPUTS"), &s_blockInputs);
    if (gui::buttonRow(T("REPLAY/CONVERT"))) {
        // .gdr in formato JSON: {"inputs":[{"frame":..,"btn":..,"2p":..,"down":..}]}
        auto src = replayDir() / (s_fileName + ".gdr.json");
        if (!std::filesystem::exists(src)) src = replayDir() / (s_fileName + ".gdr");
        if (loadMacro(src) && saveMacro(s_fileName + "_converted")) s_error = lang::tr("MESSAGE/REPLAY/CONVERT_SUCCESS");
        else s_error = lang::tr("ERROR/REPLAY/FILE_NOT_FOUND") + " (.gdr JSON)";
    }
    if (gui::buttonRow(T("REPLAY/OPEN_DIR")))
        ShellExecuteW(nullptr, L"open", replayDir().c_str(), nullptr, nullptr, SW_SHOWNORMAL);
    char info[64];
    snprintf(info, sizeof(info), "%zu inputs", s_inputs.size());
    gui::labelRow(info);
    if (!s_error.empty() && gui::buttonRow(s_error.c_str())) s_error.clear();
}

[[maybe_unused]] bool s_init = [] {
    record.toggled([](bool on) {
        if (on) {
            play.enabled = false;
            s_dirty = true;
        } else if (s_autoSave && !s_inputs.empty()) {
            saveMacro(s_fileName);
        }
    });
    play.toggled([](bool on) {
        if (on) {
            record.enabled = false;
            if (s_inputs.empty()) {
                s_error = lang::tr("ERROR/REPLAY/NO_INPUTS");
                play.enabled = false;
            }
        }
    });
    hack("_settings", "REPLAY/SETTINGS")
        .optText("REPLAY/FILE", &s_fileName)
        .optBool("REPLAY/SAVE_LAYOUT/AUTO_SAVE", &s_autoSave)
        .optBool("REPLAY/GAMEPLAY/ALLOW_RANDOMNESS", &s_allowRandom)
        .optBool("REPLAY/GAMEPLAY/BLOCK_INPUTS", &s_blockInputs);
    return true;
}();

} // namespace

namespace mh::replay {
const char* stateText() {
    if (record.enabled) return "Recording";
    if (play.enabled) return "Playing";
    return "";
}
} // namespace mh::replay

static gui::WindowContent s_replayUI("REPLAY", replayUI);

MH_ON(ev::plReset, [](void* pl) {
    int frame = 0;
    if (GDF(pl, GJBaseGameLayer, m_isPracticeMode, bool)) {
        auto cps = GDF(pl, PlayLayer, m_checkpointArray, void*);
        unsigned n = cc::arrayCount(cps);
        if (n > 0) {
            auto it = s_checkpointFrames.find(cc::arrayAt(cps, n - 1));
            if (it != s_checkpointFrames.end()) frame = it->second;
        }
    }
    s_frame = frame;
    if (record.enabled) {
        // elimina gli input dopo il punto di ripartenza
        s_inputs.erase(std::remove_if(s_inputs.begin(), s_inputs.end(), [&](const Input& i) { return i.frame >= frame; }),
                       s_inputs.end());
    }
    s_playIndex = 0;
    while (s_playIndex < s_inputs.size() && s_inputs[s_playIndex].frame < frame) s_playIndex++;
    if (!s_allowRandom && (record.enabled || play.enabled)) srand(0);
});

MH_ON(ev::plCheckpoint, [](void*, void* cp) { s_checkpointFrames[cp] = s_frame; });
MH_ON(ev::plQuit, [](void*) {
    s_checkpointFrames.clear();
    if (record.enabled && s_autoSave && !s_inputs.empty()) saveMacro(s_fileName);
});

MH_ON(ev::bglTick, [](void* layer) {
    if (layer != gd::playLayer()) return;
    if (play.enabled) {
        s_injecting = true;
        while (s_playIndex < s_inputs.size() && s_inputs[s_playIndex].frame <= s_frame) {
            auto& in = s_inputs[s_playIndex++];
            if (in.frame == s_frame)
                gd::call<void>(gd::addr::GJBaseGameLayer::handleButton, layer, in.down, in.button, !in.p2);
        }
        s_injecting = false;
    }
    s_frame++;
});

MH_ON(ev::bglButton, [](void* layer, bool down, int button, bool p1, bool& allow) {
    if (layer != gd::playLayer() || s_injecting) return;
    if (play.enabled && s_blockInputs) {
        allow = false;
        return;
    }
    if (record.enabled && allow) {
        s_inputs.push_back({s_frame, button, down, !p1});
        s_dirty = true;
    }
});
