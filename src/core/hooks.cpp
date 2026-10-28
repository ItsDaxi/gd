#include "hooks.hpp"
#include "log.hpp"
#include "memory.hpp"

#include <windows.h>
#include <MinHook.h>

namespace mh::hooks {

static std::vector<Info> g_hooks;
static bool g_init = false;

bool init() {
    if (g_init) return true;
    MH_STATUS s = MH_Initialize();
    if (s != MH_OK && s != MH_ERROR_ALREADY_INITIALIZED) {
        MH_ERR("MH_Initialize fallito: %s", MH_StatusToString(s));
        return false;
    }
    g_init = true;
    return true;
}

void shutdown() {
    if (!g_init) return;
    MH_DisableHook(MH_ALL_HOOKS);
    MH_Uninitialize();
    g_init = false;
}

uintptr_t gdBase() { return mem::base(); }

void* resolveExport(const char* module, const char* symbol) {
    HMODULE m = GetModuleHandleA(module);
    if (!m) m = LoadLibraryA(module);
    if (!m) return nullptr;
    return reinterpret_cast<void*>(GetProcAddress(m, symbol));
}

bool create(const char* name, void* target, void* detour, void** original) {
    if (!init()) return false;
    if (!target) {
        MH_WARN("hook %s: target nullo", name);
        g_hooks.push_back({name, 0, false});
        return false;
    }
    MH_STATUS s = MH_CreateHook(target, detour, original);
    bool ok = s == MH_OK;
    if (!ok) MH_WARN("hook %s @ %p fallito: %s", name, target, MH_StatusToString(s));
    g_hooks.push_back({name, reinterpret_cast<uintptr_t>(target), ok});
    return ok;
}

bool skipped(const char* name) {
    MH_WARN("hook %s: indirizzo non disponibile per questa versione, saltato", name);
    g_hooks.push_back({name, 0, false});
    return false;
}

void enableAll() {
    MH_STATUS s = MH_EnableHook(MH_ALL_HOOKS);
    if (s != MH_OK) MH_ERR("MH_EnableHook: %s", MH_StatusToString(s));
    size_t ok = 0;
    for (auto& h : g_hooks) ok += h.ok;
    MH_LOG("hook attivi: %zu/%zu", ok, g_hooks.size());
}

const std::vector<Info>& list() { return g_hooks; }

} // namespace mh::hooks
