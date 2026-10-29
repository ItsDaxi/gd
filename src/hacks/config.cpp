#include "config.hpp"
#include "registry.hpp"
#include "lang.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"
#include "gui/menu.hpp"

#include <nlohmann/json.hpp>

#include <chrono>
#include <fstream>

namespace mh::config {

using json = nlohmann::json;

static UI g_ui;
static bool g_dirty = false;
static auto g_lastChange = std::chrono::steady_clock::now();

UI& ui() { return g_ui; }

static void readColor(const json& j, const char* k, float* out) {
    if (!j.contains(k) || !j[k].is_array() || j[k].size() != 4) return;
    for (int i = 0; i < 4; i++) out[i] = j[k][i].get<float>();
}

static json colorJson(const float* c) { return json::array({c[0], c[1], c[2], c[3]}); }

static void readOption(const json& opts, Option& o) {
    if (o.key.empty() || !opts.contains(o.key)) return;
    const auto& v = opts[o.key];
    try {
        switch (o.type) {
        case OptType::Bool: *static_cast<bool*>(o.ptr) = v.get<bool>(); break;
        case OptType::Int:
        case OptType::Combo:
        case OptType::Key: *static_cast<int*>(o.ptr) = v.get<int>(); break;
        case OptType::Float: *static_cast<float*>(o.ptr) = v.get<float>(); break;
        case OptType::Text: *static_cast<std::string*>(o.ptr) = v.get<std::string>(); break;
        case OptType::Color: readColor(opts, o.key.c_str(), static_cast<float*>(o.ptr)); break;
        default: break;
        }
    } catch (...) {
    }
}

static json writeOption(const Option& o) {
    switch (o.type) {
    case OptType::Bool: return *static_cast<bool*>(o.ptr);
    case OptType::Int:
    case OptType::Combo:
    case OptType::Key: return *static_cast<int*>(o.ptr);
    case OptType::Float: return *static_cast<float*>(o.ptr);
    case OptType::Text: return *static_cast<std::string*>(o.ptr);
    case OptType::Color: return colorJson(static_cast<float*>(o.ptr));
    default: return nullptr;
    }
}

void load() {
    auto path = paths::dataDir() / "config.json";
    std::ifstream f(path);
    if (!f) {
        MH_LOG("nessun config, uso i valori predefiniti");
        lang::set(g_ui.language);
        return;
    }
    json j;
    try {
        j = json::parse(f, nullptr, true, true);
    } catch (const std::exception& e) {
        MH_WARN("config.json non valido: %s", e.what());
        return;
    }
    auto& u = j["ui"];
    g_ui.scale = u.value("scale", g_ui.scale);
    g_ui.animMs = u.value("animMs", g_ui.animMs);
    g_ui.autoSave = u.value("autoSave", g_ui.autoSave);
    g_ui.toggleKey = u.value("toggleKey", g_ui.toggleKey);
    g_ui.altToggleKey = u.value("altToggleKey", g_ui.altToggleKey);
    g_ui.iconHotkey = u.value("iconHotkey", g_ui.iconHotkey);
    g_ui.indicatorDot = u.value("indicatorDot", g_ui.indicatorDot);
    g_ui.theme = u.value("theme", g_ui.theme);
    g_ui.darkText = u.value("darkText", g_ui.darkText);
    g_ui.language = u.value("language", g_ui.language);
    g_ui.keybindsDisabledInEditor = u.value("keybindsDisabledInEditor", g_ui.keybindsDisabledInEditor);
    g_ui.hideOnToggle = u.value("hideOnToggle", g_ui.hideOnToggle);
    g_ui.autoSelectSearch = u.value("autoSelectSearch", g_ui.autoSelectSearch);
    g_ui.ruleset = u.value("ruleset", g_ui.ruleset);
    readColor(u, "accent", g_ui.accent);
    readColor(u, "titleAccent", g_ui.titleAccent);
    readColor(u, "background", g_ui.background);
    lang::set(g_ui.language);

    auto& hk = j["hacks"];
    auto& opts = j["options"];
    auto& binds = j["keybinds"];
    for (auto& h : hacks()) {
        for (auto& o : h.options) readOption(opts, o);
        if (binds.contains(h.id)) h.keybind = binds[h.id].get<int>();
        // lo stato viene applicato (onToggle) al primo frame, sul thread del gioco
        if (!h.button && hk.contains(h.id) && hk[h.id].is_boolean()) h.enabled = hk[h.id].get<bool>();
    }
    if (j.contains("favourites")) {
        for (auto& id : j["favourites"]) gui::favourites().push_back(id.get<std::string>());
    }
    // layout v3 (colonne di Mega Hack v9): le posizioni salvate con le versioni precedenti vengono ignorate
    if (j.value("layout", 0) == 3 && j.contains("windows")) {
        for (auto& [name, w] : j["windows"].items()) {
            auto& st = gui::windowState(name);
            st.x = w.value("x", st.x);
            st.y = w.value("y", st.y);
            st.collapsed = w.value("collapsed", st.collapsed);
            st.userMoved = w.value("moved", false);
        }
    }
    MH_LOG("config caricato");
}

void save() {
    json j;
    auto& u = j["ui"];
    u["scale"] = g_ui.scale;
    u["animMs"] = g_ui.animMs;
    u["autoSave"] = g_ui.autoSave;
    u["toggleKey"] = g_ui.toggleKey;
    u["altToggleKey"] = g_ui.altToggleKey;
    u["iconHotkey"] = g_ui.iconHotkey;
    u["indicatorDot"] = g_ui.indicatorDot;
    u["theme"] = g_ui.theme;
    u["darkText"] = g_ui.darkText;
    u["language"] = g_ui.language;
    u["keybindsDisabledInEditor"] = g_ui.keybindsDisabledInEditor;
    u["hideOnToggle"] = g_ui.hideOnToggle;
    u["autoSelectSearch"] = g_ui.autoSelectSearch;
    u["ruleset"] = g_ui.ruleset;
    u["accent"] = colorJson(g_ui.accent);
    u["titleAccent"] = colorJson(g_ui.titleAccent);
    u["background"] = colorJson(g_ui.background);

    for (auto& h : hacks()) {
        if (!h.button) j["hacks"][h.id] = h.enabled;
        for (auto& o : h.options) {
            if (o.key.empty() || !o.ptr) continue;
            j["options"][o.key] = writeOption(o);
        }
        if (h.keybind) j["keybinds"][h.id] = h.keybind;
    }
    j["favourites"] = gui::favourites();
    j["layout"] = 3;
    for (auto& [name, st] : gui::windowStates()) {
        j["windows"][name] = {{"x", st.x}, {"y", st.y}, {"collapsed", st.collapsed}, {"moved", st.userMoved}};
    }
    auto path = paths::dataDir() / "config.json";
    auto tmp = path;
    tmp += ".tmp";
    {
        std::ofstream f(tmp);
        f << j.dump(2);
    }
    std::error_code ec;
    std::filesystem::rename(tmp, path, ec);
    if (ec) {
        std::filesystem::remove(path, ec);
        std::filesystem::rename(tmp, path, ec);
    }
    g_dirty = false;
}

void markDirty() {
    g_dirty = true;
    g_lastChange = std::chrono::steady_clock::now();
}

void tick() {
    if (!g_dirty || !g_ui.autoSave) return;
    if (std::chrono::steady_clock::now() - g_lastChange > std::chrono::seconds(2)) save();
}

} // namespace mh::config
