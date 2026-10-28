#include "lang.hpp"
#include "core/log.hpp"
#include "core/paths.hpp"

#include <embedded_langs.hpp>
#include <nlohmann/json.hpp>

#include <fstream>
#include <map>
#include <unordered_map>

namespace mh::lang {

using Dict = std::unordered_map<std::string, std::string>;

static std::map<std::string, Dict> g_langs;
static std::string g_current = "en-GB";
static Dict g_live;
static std::unordered_map<std::string, std::string> g_pretty; // cache dei nomi generati

static void parseInto(Dict& d, const std::string& text, const std::string& name) {
    try {
        auto j = nlohmann::json::parse(text, nullptr, true, true);
        for (auto& [k, v] : j.items()) {
            if (v.is_string()) d[k] = v.get<std::string>();
        }
    } catch (const std::exception& e) {
        MH_WARN("lingua %s non valida: %s", name.c_str(), e.what());
    }
}

void init() {
    for (size_t i = 0; i < embedded_langs::count; i++) {
        auto& l = embedded_langs::all[i];
        std::string name = l.name;
        std::string text(l.data, l.size);
        if (name == "LIVE") parseInto(g_live, text, name);
        else parseInto(g_langs[name], text, name);
    }
    // lingue aggiuntive dell'utente (formato Mega-Hack-Translate)
    std::error_code ec;
    for (auto& e : std::filesystem::directory_iterator(paths::dataDir() / "langs", ec)) {
        if (e.path().extension() != ".json") continue;
        std::ifstream f(e.path(), std::ios::binary);
        std::string text((std::istreambuf_iterator<char>(f)), {});
        auto code = e.path().stem().string();
        if (code == "LIVE") parseInto(g_live, text, code);
        else parseInto(g_langs[code], text, code);
    }
    // __BASE__: una lingua puo' derivare da un'altra (es. en-US da en-GB)
    for (auto& [code, d] : g_langs) {
        auto it = d.find("__BASE__");
        if (it == d.end()) continue;
        auto base = g_langs.find(it->second);
        if (base == g_langs.end()) continue;
        for (auto& [k, v] : base->second) d.try_emplace(k, v);
    }
    MH_LOG("lingue caricate: %zu", g_langs.size());
}

void set(const std::string& code) {
    if (g_langs.count(code)) g_current = code;
    else g_current = "en-GB";
}

const std::string& current() { return g_current; }

std::vector<std::string> available() {
    std::vector<std::string> v;
    for (auto& [k, _] : g_langs) v.push_back(k);
    return v;
}

static const std::string* lookup(const std::string& key) {
    auto c = g_langs.find(g_current);
    if (c != g_langs.end()) {
        auto it = c->second.find(key);
        if (it != c->second.end() && !it->second.empty()) return &it->second;
    }
    auto en = g_langs.find("en-GB");
    if (en != g_langs.end()) {
        auto it = en->second.find(key);
        if (it != en->second.end()) return &it->second;
    }
    auto it = g_live.find(key);
    if (it != g_live.end()) return &it->second;
    return nullptr;
}

bool has(const std::string& key) { return lookup(key) != nullptr; }

const std::string& tr(const std::string& key) {
    if (auto s = lookup(key)) return *s;
    auto it = g_pretty.find(key);
    if (it != g_pretty.end()) return it->second;
    // "LEVEL/HACK_NO_SPIKES" -> "No Spikes"
    std::string last = key.substr(key.find_last_of('/') + 1);
    if (last.rfind("HACK_", 0) == 0) last = last.substr(5);
    std::string out;
    bool up = true;
    for (char ch : last) {
        if (ch == '_') {
            out += ' ';
            up = true;
            continue;
        }
        out += up ? ch : static_cast<char>(tolower(static_cast<unsigned char>(ch)));
        up = false;
    }
    return g_pretty.emplace(key, out).first->second;
}

const std::string& tip(const std::string& key) {
    static const std::string empty;
    if (auto s = lookup(key + "/TIP")) return *s;
    return empty;
}

} // namespace mh::lang
