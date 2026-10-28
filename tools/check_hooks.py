#!/usr/bin/env python3
"""Controlla che ogni funzione agganciata (hooks::gd) abbia un indirizzo Windows unico
nelle bindings: un indirizzo condiviso (es. uno stub "return false" usato da molte classi)
renderebbe l'hook attivo anche in punti non voluti."""
import re, sys, glob, collections
header = sys.argv[1] if len(sys.argv) > 1 else "src/gd/generated/gd_2_2091.hpp"
txt = open(header, encoding="utf-8").read()
addr_part = txt.split("namespace off {")[0]
cls = None
table = {}
counts = collections.Counter()
for line in addr_part.splitlines():
    m = re.match(r"namespace ([A-Za-z0-9_]+) \{$", line)
    if m:
        cls = m.group(1)
        continue
    m = re.match(r"\s+inline constexpr uintptr_t ([A-Za-z0-9_]+) = (0x[0-9a-f]+);", line)
    if m and cls:
        table[(cls, m.group(1))] = m.group(2)
        counts[m.group(2)] += 1
src = "".join(open(f, encoding="utf-8").read() for f in glob.glob("src/**/*.cpp", recursive=True))
used = set(re.findall(r'hooks::gd\("[^"]*",\s*addr::([A-Za-z0-9_]+)::([A-Za-z0-9_]+)', src))
bad = 0
for c, f in sorted(used):
    a = table.get((c, f))
    if a is None:
        print(f"MANCANTE {c}::{f}")
        bad += 1
    elif a == "0":
        print(f"NON DISPONIBILE (0) {c}::{f}")
        bad += 1
    elif counts[a] > 1:
        others = [f"{k[0]}::{k[1]}" for k, v in table.items() if v == a and k != (c, f)]
        print(f"CONDIVISO {c}::{f} = {a} anche da: {', '.join(others[:6])}")
        bad += 1
print(f"{len(used)} hook controllati, {bad} problemi")
sys.exit(1 if bad else 0)
