#include "scan.hpp"
#include "memory.hpp"

#include <windows.h>
#include <cstring>

namespace mh::scan {

static size_t functionLength(const uint8_t* p, size_t maxLen) {
    // fine funzione: almeno 3 int3 consecutivi di padding
    for (size_t i = 0; i + 3 < maxLen; i++) {
        if (p[i] == 0xCC && p[i + 1] == 0xCC && p[i + 2] == 0xCC) return i;
    }
    return maxLen;
}

std::vector<RipRef> sseRipRefs(uintptr_t func, size_t maxLen) {
    std::vector<RipRef> out;
    auto p = reinterpret_cast<const uint8_t*>(func);
    if (!mem::readable(p, maxLen)) maxLen = 0x400;
    if (!mem::readable(p, maxLen)) return out;
    size_t len = functionLength(p, maxLen);
    for (size_t i = 0; i + 8 < len; i++) {
        uint8_t pre = p[i];
        if (pre != 0xF2 && pre != 0xF3 && pre != 0x66) continue;
        size_t j = i + 1;
        if ((p[j] & 0xF0) == 0x40) j++; // REX
        if (p[j] != 0x0F) continue;
        uint8_t op = p[j + 1];
        // 10 mov, 51 sqrt, 58 add, 59 mul, 5C sub, 5D min, 5E div, 5F max, 2E ucomis, 2F comis, 5A cvt
        bool sse = op == 0x10 || op == 0x51 || op == 0x58 || op == 0x59 || op == 0x5C || op == 0x5D || op == 0x5E ||
                   op == 0x5F || op == 0x2E || op == 0x2F || op == 0x5A;
        if (!sse) continue;
        uint8_t modrm = p[j + 2];
        if ((modrm & 0xC7) != 0x05) continue; // [rip+disp32]
        int32_t disp;
        std::memcpy(&disp, p + j + 3, 4);
        int length = static_cast<int>(j + 7 - i);
        uintptr_t next = func + i + length;
        out.push_back({func + i, func + j + 3, next + static_cast<intptr_t>(disp), length});
    }
    return out;
}

void* allocNear(uintptr_t origin, size_t size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    uintptr_t gran = si.dwAllocationGranularity;
    uintptr_t start = origin & ~(gran - 1);
    for (uintptr_t off = gran; off < 0x70000000; off += gran) {
        for (int dir = -1; dir <= 1; dir += 2) {
            uintptr_t addr = dir < 0 ? start - off : start + off;
            void* p = VirtualAlloc(reinterpret_cast<void*>(addr), size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (p) return p;
        }
    }
    return nullptr;
}

bool redirect(const RipRef& ref, uintptr_t newTarget) {
    intptr_t next = static_cast<intptr_t>(ref.instr + ref.length);
    intptr_t d = static_cast<intptr_t>(newTarget) - next;
    if (d > INT32_MAX || d < INT32_MIN) return false;
    int32_t disp = static_cast<int32_t>(d);
    return mem::write(ref.disp, &disp, 4);
}

bool restore(const RipRef& ref) { return redirect(ref, ref.target); }

std::vector<uintptr_t> cmpImm8(uintptr_t func, int8_t imm, size_t maxLen) {
    std::vector<uintptr_t> out;
    auto p = reinterpret_cast<const uint8_t*>(func);
    if (!mem::readable(p, maxLen)) maxLen = 0x400;
    if (!mem::readable(p, maxLen)) return out;
    size_t len = functionLength(p, maxLen);
    for (size_t i = 0; i + 2 < len; i++) {
        // 83 /7 ib = cmp r/m32, imm8 con registro diretto (modrm F8..FF)
        if (p[i] == 0x83 && p[i + 1] >= 0xF8 && static_cast<int8_t>(p[i + 2]) == imm) out.push_back(func + i + 2);
    }
    return out;
}

} // namespace mh::scan
