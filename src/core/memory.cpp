#include "memory.hpp"

#include <windows.h>

namespace mh::mem {

uintptr_t base() {
    static uintptr_t b = reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    return b;
}

uintptr_t cocosBase() {
    static uintptr_t b = reinterpret_cast<uintptr_t>(GetModuleHandleW(L"libcocos2d.dll"));
    return b;
}

bool readable(const void* p, size_t size) {
    if (!p) return false;
    MEMORY_BASIC_INFORMATION mbi{};
    if (!VirtualQuery(p, &mbi, sizeof(mbi))) return false;
    if (mbi.State != MEM_COMMIT) return false;
    if (mbi.Protect & (PAGE_NOACCESS | PAGE_GUARD)) return false;
    auto end = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
    return reinterpret_cast<uintptr_t>(p) + size <= end;
}

bool write(uintptr_t addr, const void* data, size_t size, std::vector<uint8_t>* original) {
    DWORD old;
    if (!VirtualProtect(reinterpret_cast<void*>(addr), size, PAGE_EXECUTE_READWRITE, &old)) return false;
    if (original) {
        original->resize(size);
        std::memcpy(original->data(), reinterpret_cast<void*>(addr), size);
    }
    std::memcpy(reinterpret_cast<void*>(addr), data, size);
    VirtualProtect(reinterpret_cast<void*>(addr), size, old, &old);
    FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(addr), size);
    return true;
}

bool Patch::apply() {
    if (m_applied || !m_addr) return m_applied;
    m_applied = write(m_addr, m_bytes.data(), m_bytes.size(), &m_original);
    return m_applied;
}

bool Patch::restore() {
    if (!m_applied) return true;
    if (write(m_addr, m_original.data(), m_original.size())) m_applied = false;
    return !m_applied;
}

} // namespace mh::mem
