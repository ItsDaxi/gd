#!/usr/bin/env python3
"""Controlla gli indirizzi usati dal mod su un GeometryDash.exe locale.

Per ogni funzione citata nei sorgenti (gd::addr::Classe::metodo) verifica che l'indirizzo cada
all'inizio di una funzione (preceduto da int3/ret) e mostra le prime istruzioni.

    pip install pefile capstone
    python3 tools/verify_exe.py <GeometryDash.exe> [src/gd/generated/gd_2_2091.hpp]

L'eseguibile non va messo nel repository.
"""
import glob
import re
import sys
from pathlib import Path

import pefile
from capstone import CS_ARCH_X86, CS_MODE_64, Cs

root = Path(__file__).resolve().parent.parent
exe = sys.argv[1]
header = sys.argv[2] if len(sys.argv) > 2 else str(root / "src/gd/generated/gd_2_2091.hpp")

pe = pefile.PE(exe, fast_load=True)
data = open(exe, "rb").read()


def offset(rva):
    for s in pe.sections:
        if s.VirtualAddress <= rva < s.VirtualAddress + s.Misc_VirtualSize:
            return rva - s.VirtualAddress + s.PointerToRawData
    return None


addrs, ns = {}, None
for line in open(header):
    if line.startswith("namespace off"):
        break
    m = re.match(r"namespace (\w+) \{$", line.strip())
    if m:
        ns = m.group(1)
        continue
    m = re.search(r"uintptr_t (\w+) = (0x[0-9a-f]+);", line)
    if m and ns:
        addrs[f"{ns}::{m.group(1)}"] = int(m.group(2), 16)

used = set()
for f in glob.glob(str(root / "src/**/*.cpp"), recursive=True):
    used.update(f"{a}::{b}" for a, b in re.findall(r"addr::(\w+)::(\w+)", open(f).read()))

md = Cs(CS_ARCH_X86, CS_MODE_64)
bad = 0
for name in sorted(used):
    rva = addrs.get(name)
    if not rva:
        print(f"{name:55s} assente in questa versione")
        continue
    o = offset(rva)
    if o is None:
        print(f"{name:55s} {rva:#x} fuori dall'eseguibile")
        bad += 1
        continue
    ins = list(md.disasm(data[o:o + 16], 0x140000000 + rva))[:2]
    text = "; ".join(f"{i.mnemonic} {i.op_str}" for i in ins)
    start = data[o - 1] in (0xCC, 0xC3)
    if not start:
        bad += 1
    print(f"{name:55s} {rva:#10x} {text}{'' if start else '   <-- non sembra un inizio di funzione'}")
print(f"\n{len(used)} funzioni usate, {bad} da controllare")
