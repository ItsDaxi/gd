#!/usr/bin/env python3
"""Genera docs/FUNZIONI.md: elenco di tutte le funzioni registrate nel codice,
con nome (en-GB e it-IT da Mega-Hack-Translate), finestra e stato."""
import glob, json, re
from pathlib import Path

root = Path(__file__).resolve().parent.parent
en = json.loads((root / "resources/langs/en-GB.json").read_text(encoding="utf-8"))
it = json.loads((root / "resources/langs/it-IT.json").read_text(encoding="utf-8"))
live = json.loads((root / "resources/langs/LIVE.json").read_text(encoding="utf-8"))

rows = []
for f in sorted(glob.glob(str(root / "src/hacks/*.cpp"))):
    src = open(f, encoding="utf-8").read()
    # hack("WINDOW", "ID") / action("WINDOW", "ID", ...) / H("ID") (finestra dal file)
    default_win = None
    m = re.search(r'Hack& H\(const char\* id\) \{ return hack\("([A-Z_]+)"', src)
    if m:
        default_win = m.group(1)
    def until_semicolon(pos):
        out, q, i = "", False, pos
        while i < len(src):
            c = src[i]
            if c == '"' and src[i - 1] != "\\":
                q = not q
            if c == ";" and not q:
                break
            out += c
            i += 1
        return out

    for m in re.finditer(r'(?:(hack|action)\("([A-Z_]+)",\s*"([A-Z0-9_/*]+)"|\bH\("([A-Z0-9_/*]+)"\))', src):
        kind, win, hid, hid2 = m.groups()
        rest = until_semicolon(m.end())
        if hid2:
            win, hid, kind = default_win, hid2, "hack"
        if win and win.startswith("_"):
            continue
        name = re.search(r'\.name\("([^"]+)"\)', rest)
        partial = re.search(r'\.partial\("((?:[^"\\]|\\.)*)"\)', rest)
        cheat = ".isCheat()" in rest
        rows.append((win, hid, name.group(1) if name else None, partial.group(1) if partial else None, cheat, kind == "action"))
    # campi dello Status
    for m in re.finditer(r'\{"(STATUS/[A-Z_]+)"\}', src):
        rows.append(("STATUS", m.group(1) + "/ENABLED", None, None, False, False))

order = ["BYPASS", "COSMETIC", "CREATOR", "LEVEL", "UNIVERSAL", "STATUS", "ICONIC", "DISPLAY", "SPEEDHACK",
         "UTILITY", "REPLAY", "CHEAT_SAFETY", "INTERFACE", "SCREENSHOT"]
def tr(d, k):
    return d.get(k) or en.get(k) or live.get(k) or ""

out = ["# Elenco funzioni", "",
       "Generato da `tools/gen_features.py` leggendo il codice sorgente. Nomi e descrizioni vengono da "
       "[Mega-Hack-Translate](https://github.com/absolllute/Mega-Hack-Translate) (`en-GB.json` / `it-IT.json`).", "",
       "Legenda stato: ✅ implementata (compilata, non ancora provata in gioco) · ⚠️ parziale / con limitazioni · ❌ non disponibile senza Geode o senza patch del binario · "
       "🔴 = conta come cheat (Safe Mode / Cheat Indicator)", ""]
total = {"ok": 0, "partial": 0, "na": 0}
for w in order:
    items = [r for r in rows if r[0] == w]
    if not items:
        continue
    out += [f"## {tr(en, w) or w}", "", "| Funzione | Italiano | Stato | Note |", "|---|---|---|---|"]
    seen = set()
    for _, hid, name, partial, cheat, action in sorted(items, key=lambda r: (tr(en, r[1]) or r[2] or r[1]).lower()):
        if hid in seen:
            continue
        seen.add(hid)
        n_en = tr(en, hid) or name or hid
        n_it = it.get(hid, "") or ""
        if partial and partial.startswith("Non disponibile"):
            st, total["na"] = "❌", total["na"] + 1
        elif partial:
            st, total["partial"] = "⚠️", total["partial"] + 1
        else:
            st, total["ok"] = "✅", total["ok"] + 1
        note = (partial or tr(en, hid + "/TIP")).replace("|", "/").replace("\n", " ")
        out.append(f"| {n_en}{' 🔴' if cheat else ''} | {n_it} | {st} | {note} |")
    out.append("")
out.insert(6, f"**Totale:** {sum(total.values())} funzioni — ✅ {total['ok']} · ⚠️ {total['partial']} · ❌ {total['na']}\n")
(root / "docs").mkdir(exist_ok=True)
(root / "docs/FUNZIONI.md").write_text("\n".join(out) + "\n", encoding="utf-8")
print(total)
