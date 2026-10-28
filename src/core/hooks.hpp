#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace mh::hooks {

bool init();
void shutdown();

// Crea un hook (non ancora attivo). Restituisce false se fallisce.
bool create(const char* name, void* target, void* detour, void** original);

// Hook su una funzione di GeometryDash.exe (RVA dalle bindings)
template <class F>
inline bool gd(const char* name, uintptr_t rva, F* detour, F** original);

// Hook su un export di una DLL (es. libcocos2d.dll, simbolo mangled MSVC)
template <class F>
inline bool exported(const char* module, const char* symbol, F* detour, F** original);

// Attiva tutti gli hook creati
void enableAll();

struct Info {
    std::string name;
    uintptr_t target;
    bool ok;
};
const std::vector<Info>& list();

void* resolveExport(const char* module, const char* symbol);
bool skipped(const char* name);
uintptr_t gdBase();

template <class F>
inline bool gd(const char* name, uintptr_t rva, F* detour, F** original) {
    if (rva == 0) return skipped(name); // non disponibile per questa versione di GD
    return create(name, reinterpret_cast<void*>(gdBase() + rva), reinterpret_cast<void*>(detour),
                  reinterpret_cast<void**>(original));
}

template <class F>
inline bool exported(const char* module, const char* symbol, F* detour, F** original) {
    void* target = resolveExport(module, symbol);
    if (!target) return false;
    return create(symbol, target, reinterpret_cast<void*>(detour), reinterpret_cast<void**>(original));
}

} // namespace mh::hooks
