// Mega Hack GD - mod menu standalone per Geometry Dash 2.209 / 2.2091 (senza Geode)
#include "core/hooks.hpp"
#include "core/log.hpp"
#include "gd/gd.hpp"
#include "gui/gui.hpp"
#include "hacks/config.hpp"
#include "hacks/lang.hpp"
#include "hacks/registry.hpp"

#include <windows.h>

namespace {

bool g_firstFrameDone = false;

DWORD WINAPI initThread(LPVOID) {
    mh::log::init();
    MH_LOG("Mega Hack GD %s per Geometry Dash %s", MH_VERSION_STR, gd::versionString());

    // libcocos2d.dll e' un import statico di GD, ma con l'injector potremmo arrivare prima
    for (int i = 0; i < 200 && !GetModuleHandleW(L"libcocos2d.dll"); i++) Sleep(50);
    if (!GetModuleHandleW(L"libcocos2d.dll")) {
        MH_ERR("libcocos2d.dll non trovato: questo non sembra Geometry Dash");
        return 0;
    }

    mh::lang::init();
    mh::config::load();
    if (!mh::hooks::init()) return 0;

    mh::gui::install();

    if (gd::versionMatches()) {
        mh::runInstallers();
    } else {
        MH_ERR("versione di GD diversa da %s: hook di gioco disattivati", gd::versionString());
        MessageBoxW(nullptr,
                    L"Mega Hack GD: la versione di Geometry Dash non corrisponde a quella per cui e' stato compilato "
                    L"il mod.\nIl menu si apre comunque (Tab) ma le funzioni di gioco sono disattivate per evitare crash.\n\n"
                    L"Usa la DLL corretta (2.209 o 2.2091).",
                    L"Mega Hack GD", MB_ICONWARNING | MB_OK);
    }
    mh::hooks::enableAll();
    MH_LOG("inizializzazione completata");
    return 0;
}

} // namespace

// primo frame: applica gli hack caricati dal config sul thread del gioco
MH_EVERY_FRAME(applyConfig) {
    if (g_firstFrameDone) return;
    g_firstFrameDone = true;
    mh::applyLoadedState();
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(module);
        HANDLE t = CreateThread(nullptr, 0, initThread, nullptr, 0, nullptr);
        if (t) CloseHandle(t);
    } else if (reason == DLL_PROCESS_DETACH) {
        mh::config::save();
    }
    return TRUE;
}
