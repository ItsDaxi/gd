#pragma once
// Piccolo scanner di istruzioni x64 dentro una funzione nota.
// Serve per funzioni che richiedono di cambiare una costante del codice di GD senza avere
// l'eseguibile: si cercano gli accessi RIP-relativi a costanti (es. 240.0) e si reindirizzano
// verso una copia modificabile allocata vicino al modulo.
#include <cstdint>
#include <vector>

namespace mh::scan {

struct RipRef {
    uintptr_t instr;   // inizio istruzione
    uintptr_t disp;    // indirizzo del displacement a 32 bit
    uintptr_t target;  // indirizzo puntato
    int length;        // lunghezza istruzione
};

// Istruzioni SSE (movss/movsd/mulsd/divsd/addsd/subsd/comisd/...) con operando [rip+disp32]
// nell'intervallo [func, func+maxLen). La scansione si ferma sul padding int3 tra le funzioni.
std::vector<RipRef> sseRipRefs(uintptr_t func, size_t maxLen = 0x3000);

// Memoria eseguibile/scrivibile entro +-2GB da `near` (per i nuovi target dei displacement)
void* allocNear(uintptr_t origin, size_t size);

// Riscrive il displacement di un'istruzione RIP-relativa per puntare a `newTarget`
bool redirect(const RipRef& ref, uintptr_t newTarget);
bool restore(const RipRef& ref);

// Cerca "cmp r32, imm8" (83 F8+r ib) con un dato immediato; restituisce gli indirizzi dell'immediato
std::vector<uintptr_t> cmpImm8(uintptr_t func, int8_t imm, size_t maxLen = 0x3000);

} // namespace mh::scan
