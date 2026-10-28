#pragma once
#include <cstdint>
#include <cstring>
#include <vector>

namespace mh::mem {

// Base di GeometryDash.exe
uintptr_t base();
// Base di libcocos2d.dll
uintptr_t cocosBase();

// Puntatore valido e leggibile? (protezione contro offset sbagliati)
bool readable(const void* p, size_t size = 8);

template <class T>
inline T& at(const void* obj, uintptr_t off) {
    return *reinterpret_cast<T*>(reinterpret_cast<uintptr_t>(obj) + off);
}

// Scrive byte in memoria protetta (restituisce i byte originali)
bool write(uintptr_t addr, const void* data, size_t size, std::vector<uint8_t>* original = nullptr);

// Patch reversibile
class Patch {
public:
    Patch() = default;
    Patch(uintptr_t addr, std::vector<uint8_t> bytes) : m_addr(addr), m_bytes(std::move(bytes)) {}
    bool apply();
    bool restore();
    void set(bool on) { on ? (void)apply() : (void)restore(); }
    bool applied() const { return m_applied; }

private:
    uintptr_t m_addr = 0;
    std::vector<uint8_t> m_bytes;
    std::vector<uint8_t> m_original;
    bool m_applied = false;
};

} // namespace mh::mem
