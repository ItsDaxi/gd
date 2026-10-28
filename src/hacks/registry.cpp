#include "registry.hpp"
#include "core/log.hpp"

#include <unordered_map>

namespace mh {

bool g_cheatedThisAttempt = false;

std::deque<Hack>& hacks() {
    static std::deque<Hack> list;
    return list;
}

static std::unordered_map<std::string, Hack*>& index() {
    static std::unordered_map<std::string, Hack*> map;
    return map;
}

Hack& hack(const std::string& window, const std::string& id) {
    auto it = index().find(id);
    if (it != index().end()) return *it->second;
    auto& h = hacks().emplace_back();
    h.id = id;
    h.window = window;
    index()[id] = &h;
    return h;
}

Hack& action(const std::string& window, const std::string& id, std::function<void()> f) {
    auto& h = hack(window, id);
    h.button = true;
    h.onPress = std::move(f);
    return h;
}

Hack* find(const std::string& id) {
    auto it = index().find(id);
    return it == index().end() ? nullptr : it->second;
}

const std::vector<std::string>& windowOrder() {
    static const std::vector<std::string> order = {
        "BYPASS", "COSMETIC", "CREATOR", "LEVEL", "UNIVERSAL", "STATUS", "ICONIC",
        "DISPLAY", "SPEEDHACK", "UTILITY", "REPLAY", "CHEAT_SAFETY", "INTERFACE",
        "SCREENSHOT", "GM_VAR", "KEYBINDS", "HOME", "FAVOURITES",
    };
    return order;
}

void Hack::set(bool on) {
    if (button) {
        if (onPress) onPress();
        return;
    }
    if (enabled == on) return;
    enabled = on;
    if (cheat && on) g_cheatedThisAttempt = true;
    if (onToggle) onToggle(on);
}

Hack& Hack::optBool(const std::string& label, bool* p, std::function<void()> changed) {
    Option o;
    o.type = OptType::Bool;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    o.onChange = std::move(changed);
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optInt(const std::string& label, int* p, int mn, int mx, const char* fmt) {
    Option o;
    o.type = OptType::Int;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    o.min = static_cast<float>(mn);
    o.max = static_cast<float>(mx);
    o.fmt = fmt;
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optFloat(const std::string& label, float* p, float mn, float mx, const char* fmt) {
    Option o;
    o.type = OptType::Float;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    o.min = mn;
    o.max = mx;
    o.fmt = fmt;
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optText(const std::string& label, std::string* p) {
    Option o;
    o.type = OptType::Text;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optCombo(const std::string& label, int* p, std::vector<std::string> items) {
    Option o;
    o.type = OptType::Combo;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    o.items = std::move(items);
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optKey(const std::string& label, int* p) {
    Option o;
    o.type = OptType::Key;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = p;
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optColor(const std::string& label, float* rgba4) {
    Option o;
    o.type = OptType::Color;
    o.key = id + "#" + label;
    o.label = label;
    o.ptr = rgba4;
    options.push_back(std::move(o));
    return *this;
}

Hack& Hack::optButton(const std::string& label, std::function<void()> f) {
    Option o;
    o.type = OptType::Button;
    o.label = label;
    o.onClick = std::move(f);
    options.push_back(std::move(o));
    return *this;
}

// ---------------------------------------------------------------- installer
struct InstallerEntry {
    const char* name;
    void (*fn)();
};
static std::vector<InstallerEntry>& installers() {
    static std::vector<InstallerEntry> v;
    return v;
}

Installer::Installer(const char* name, void (*fn)()) { installers().push_back({name, fn}); }

void runInstallers() {
    for (auto& i : installers()) {
        MH_LOG("installo hook: %s", i.name);
        i.fn();
    }
}

static std::vector<void (*)()>& frameCallbacks() {
    static std::vector<void (*)()> v;
    return v;
}

FrameCallback::FrameCallback(void (*fn)()) { frameCallbacks().push_back(fn); }

void runFrameCallbacks() {
    for (auto fn : frameCallbacks()) fn();
}

void applyLoadedState() {
    for (auto& h : hacks()) {
        if (h.enabled && !h.button && h.onToggle) h.onToggle(true);
    }
}

bool anyCheatEnabled() {
    for (auto& h : hacks()) {
        if (h.cheat && h.enabled) return true;
    }
    return false;
}

} // namespace mh
