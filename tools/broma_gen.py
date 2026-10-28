#!/usr/bin/env python3
"""
broma_gen.py - genera header C++ (indirizzi + offset membri) per Geometry Dash
Windows x64 partendo dai file .bro di https://github.com/geode-sdk/bindings

Non usa Geode: legge solo i file di testo delle bindings e calcola il layout
delle classi secondo le regole di MSVC x64. Se sono disponibili i size-check
ufficiali (test/members/<ver>/Windows.cpp) ogni classe viene validata e gli
offset delle classi che non combaciano NON vengono emessi.

Uso:
    python3 tools/broma_gen.py <bindings_repo> <versione> <output.hpp> [--classes a,b,c]
"""
from __future__ import annotations

import re
import sys
import argparse
from dataclasses import dataclass, field
from pathlib import Path

# --------------------------------------------------------------------------
# Parsing
# --------------------------------------------------------------------------


def strip_comments(src: str) -> str:
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    src = re.sub(r"//[^\n]*", "", src)
    return src


@dataclass
class Func:
    name: str
    ret: str
    params: str
    is_static: bool
    is_virtual: bool
    is_const: bool
    win: int | None
    raw: str


@dataclass
class Member:
    type: str
    name: str


@dataclass
class Pad:
    size: int


@dataclass
class Klass:
    name: str
    bases: list[str]
    items: list = field(default_factory=list)  # Member | Pad
    funcs: list[Func] = field(default_factory=list)
    has_virtual_decl: bool = False


def split_top(s: str, sep: str) -> list[str]:
    out, depth, cur = [], 0, ""
    for ch in s:
        if ch in "<([{":
            depth += 1
        elif ch in ">)]}":
            depth -= 1
        if ch == sep and depth == 0:
            out.append(cur)
            cur = ""
        else:
            cur += ch
    if cur.strip():
        out.append(cur)
    return [x.strip() for x in out]


def strip_templates(s: str) -> str:
    out, depth = "", 0
    for ch in s:
        if ch == "<":
            depth += 1
            continue
        if ch == ">":
            depth -= 1
            continue
        if depth == 0:
            out += ch
    return out


def parse_binding(spec: str) -> int | None:
    m = re.search(r"\bwin\s+(0x[0-9a-fA-F]+)", spec)
    return int(m.group(1), 16) if m else None


def parse_statement(stmt: str, k: Klass):
    stmt = stmt.strip()
    if not stmt:
        return
    stmt = re.sub(r"\[\[.*?\]\]", "", stmt).strip()
    if stmt.startswith("PAD"):
        m = re.search(r"\bwin\s+(0x[0-9a-fA-F]+|\d+)", stmt)
        if m:
            k.items.append(Pad(int(m.group(1), 0)))
        return
    flat = strip_templates(stmt)
    if "(" in flat:
        # funzione
        spec = ""
        head = stmt
        # binding "= win 0x..., m1 ..." dopo la parentesi di chiusura dei parametri
        depth = 0
        close = -1
        for i, ch in enumerate(stmt):
            if ch == "(":
                depth += 1
            elif ch == ")":
                depth -= 1
                if depth == 0:
                    close = i
        if close == -1:
            return
        # la prima parentesi top-level (fuori dai template) apre i parametri
        depth_t = 0
        open_ = -1
        for i, ch in enumerate(stmt):
            if ch == "<":
                depth_t += 1
            elif ch == ">":
                depth_t -= 1
            elif ch == "(" and depth_t == 0:
                open_ = i
                break
        if open_ == -1:
            return
        # trova la chiusura corrispondente
        depth = 0
        for i in range(open_, len(stmt)):
            if stmt[i] == "(":
                depth += 1
            elif stmt[i] == ")":
                depth -= 1
                if depth == 0:
                    close = i
                    break
        head = stmt[:open_].strip()
        params = stmt[open_ + 1:close].strip()
        tail = stmt[close + 1:]
        is_const = bool(re.match(r"\s*const\b", tail))
        if "=" in tail:
            spec = tail.split("=", 1)[1]
        is_virtual = bool(re.match(r"virtual\b", head))
        is_static = bool(re.match(r"static\b", head))
        head = re.sub(r"^(virtual|static|inline|explicit)\s+", "", head)
        head = re.sub(r"^(virtual|static|inline|explicit)\s+", "", head)
        parts = head.rsplit(None, 1)
        if len(parts) == 2:
            ret, name = parts
        else:
            ret, name = "", parts[0]
        while name.startswith("*") or name.startswith("&"):
            ret += name[0]
            name = name[1:]
        if is_virtual:
            k.has_virtual_decl = True
        k.funcs.append(Func(name, ret.strip(), params, is_static, is_virtual, is_const, parse_binding(spec), stmt))
        return
    # membro
    m = re.match(r"(.+?)\s*\b(m_[A-Za-z0-9_]+|[A-Za-z_][A-Za-z0-9_]*)\s*$", stmt)
    if not m:
        return
    k.items.append(Member(m.group(1).strip(), m.group(2)))


def parse_class_body(body: str, k: Klass):
    # spezza in statement: ';' al livello 0, oppure blocchi {} di funzioni inline
    i, depth, cur = 0, 0, ""
    while i < len(body):
        ch = body[i]
        if ch == "{":
            if depth == 0:
                # corpo inline di una funzione: lo saltiamo ma chiudiamo lo statement
                d = 0
                while i < len(body):
                    if body[i] == "{":
                        d += 1
                    elif body[i] == "}":
                        d -= 1
                        if d == 0:
                            break
                    i += 1
                parse_statement(cur, k)
                cur = ""
                i += 1
                continue
        if ch == ";" and depth == 0:
            parse_statement(cur, k)
            cur = ""
        else:
            if ch in "<(":
                depth += 1
            elif ch in ">)":
                depth -= 1
            cur += ch
        i += 1


def parse_bro(text: str, classes: dict[str, Klass], free_funcs: list[Func]):
    text = strip_comments(text)
    text = re.sub(r"^\s*#[^\n]*", "", text, flags=re.M)
    pos = 0
    n = len(text)
    while pos < n:
        m = re.compile(r"(\[\[[^\]]*\]\]\s*)*class\s+([A-Za-z0-9_:]+)\s*(:\s*([^{]+))?\{").search(text, pos)
        f = None
        # funzioni libere tra le classi
        seg_end = m.start() if m else n
        seg = text[pos:seg_end]
        if seg.strip():
            tmp = Klass("<free>", [])
            parse_class_body(seg, tmp)
            free_funcs.extend(tmp.funcs)
        if not m:
            break
        name = m.group(2)
        bases = [b.strip() for b in split_top(m.group(4) or "", ",") if b.strip()]
        bases = [re.sub(r"^(public|private|protected)\s+", "", b) for b in bases]
        # trova la graffa di chiusura
        i = m.end()
        depth = 1
        while i < n and depth:
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
            i += 1
        body = text[m.end():i - 1]
        k = Klass(name, bases)
        parse_class_body(body, k)
        if name in classes:
            # unisce (es. dichiarazioni ripetute)
            classes[name].funcs.extend(k.funcs)
            if not classes[name].items:
                classes[name].items = k.items
        else:
            classes[name] = k
        pos = i


# --------------------------------------------------------------------------
# Layout MSVC x64
# --------------------------------------------------------------------------

PRIM = {
    "bool": (1, 1), "char": (1, 1), "signed char": (1, 1), "unsigned char": (1, 1),
    "int8_t": (1, 1), "uint8_t": (1, 1), "byte": (1, 1), "GLubyte": (1, 1),
    "short": (2, 2), "unsigned short": (2, 2), "int16_t": (2, 2), "uint16_t": (2, 2), "wchar_t": (2, 2),
    "int": (4, 4), "unsigned": (4, 4), "unsigned int": (4, 4), "int32_t": (4, 4), "uint32_t": (4, 4),
    "long": (4, 4), "unsigned long": (4, 4), "float": (4, 4), "GLuint": (4, 4), "GLint": (4, 4),
    "GLenum": (4, 4), "GLfloat": (4, 4), "GLsizei": (4, 4), "GLushort": (2, 2), "GLshort": (2, 2),
    "long long": (8, 8), "unsigned long long": (8, 8), "int64_t": (8, 8), "uint64_t": (8, 8),
    "double": (8, 8), "size_t": (8, 8), "intptr_t": (8, 8), "uintptr_t": (8, 8), "time_t": (8, 8),
    "ptrdiff_t": (8, 8), "long double": (8, 8),
    "std::atomic_bool": (1, 1), "std::atomic<bool>": (1, 1), "std::atomic_int": (4, 4),
    "FMOD_RESULT": (4, 4), "FMOD_MODE": (4, 4),
}

# tipi valore cocos2d / geode con layout noto (MSVC x64)
VALUE_TYPES = {
    "cocos2d::CCPoint": (8, 4), "cocos2d::CCSize": (8, 4), "cocos2d::CCRect": (16, 4),
    "cocos2d::ccColor3B": (3, 1), "cocos2d::ccColor4B": (4, 1), "cocos2d::ccColor4F": (16, 4),
    "cocos2d::ccBlendFunc": (8, 4), "cocos2d::ccHSVValue": (16, 4), "cocos2d::CCAffineTransform": (24, 4),
    "cocos2d::ccTex2F": (8, 4), "cocos2d::ccVertex2F": (8, 4), "cocos2d::ccVertex3F": (12, 4),
    "cocos2d::ccV3F_C4B_T2F": (24, 4), "cocos2d::ccV3F_C4B_T2F_Quad": (96, 4),
    "cocos2d::ccV2F_C4B_T2F": (20, 4), "cocos2d::ccFontDefinition": (None, None),
    "kmMat4": (64, 4), "cocos2d::_ccHSVValue": (16, 4), "cocos2d::_ccColor3B": (3, 1),
    "geode::SeedValueRSV": (12, 4), "geode::SeedValueVRS": (12, 4), "geode::SeedValueVSR": (12, 4),
    "geode::SeedValueRVS": (12, 4), "geode::SeedValueSVR": (12, 4), "geode::SeedValueSRV": (12, 4),
    "geode::SeedValueRS": (8, 4), "geode::SeedValueSR": (8, 4),
    "cocos2d::CCAffineTransform": (24, 4), "std::string": (32, 8), "gd::string": (32, 8),
    "std::string_view": (16, 8), "std::mutex": (80, 8), "std::recursive_mutex": (80, 8),
    "std::condition_variable": (72, 8), "std::thread": (16, 8),
    "cocos2d::ccTexParams": (16, 4), "cocos2d::SEL_MenuHandler": (8, 8), "cocos2d::SEL_CallFunc": (8, 8),
    "cocos2d::SEL_CallFuncN": (8, 8), "cocos2d::SEL_CallFuncO": (8, 8), "cocos2d::SEL_SCHEDULE": (8, 8),
    "cocos2d::TextureQuality": (4, 4), "cocos2d::cc_timeval": (8, 4), "cocos2d::CCKeyboardHandler": (None, None),
}

STD_FIXED = {
    "std::vector": (24, 8), "gd::vector": (24, 8), "std::map": (16, 8), "gd::map": (16, 8),
    "std::set": (16, 8), "gd::set": (16, 8), "std::unordered_map": (64, 8), "gd::unordered_map": (64, 8),
    "std::unordered_set": (64, 8), "gd::unordered_set": (64, 8), "std::list": (16, 8),
    "std::deque": (40, 8), "std::function": (64, 8), "std::shared_ptr": (16, 8),
    "std::unique_ptr": (8, 8), "std::weak_ptr": (16, 8), "geode::Ref": (8, 8), "cocos2d::CCSet": (None, None),
    "std::multimap": (16, 8), "std::multiset": (16, 8), "std::queue": (40, 8), "std::stack": (40, 8),
    "std::basic_string": (32, 8), "std::bitset": (None, None), "std::priority_queue": (32, 8),
}


def parse_enums(path: Path) -> dict[str, tuple[int, int]]:
    out = {}
    if not path.exists():
        return out
    txt = strip_comments(path.read_text(encoding="utf-8", errors="ignore"))
    for m in re.finditer(r"enum\s+(class\s+|struct\s+)?([A-Za-z0-9_]+)\s*(:\s*([A-Za-z0-9_ :]+))?\s*\{", txt):
        name = m.group(2)
        under = (m.group(4) or "int").strip()
        sz = PRIM.get(under, (4, 4))
        out[name] = sz
    return out


class Layout:
    def __init__(self, classes: dict[str, Klass], enums, size_checks: dict[str, int]):
        self.classes = classes
        self.enums = enums
        self.size_checks = size_checks
        self.cache: dict[str, tuple] = {}
        self.unknown: set[str] = set()
        self.cocos_sizes: dict[str, tuple[int, int, bool]] = {}

    # ritorna (size, align) del tipo o None
    def type_size(self, t: str):
        t = t.strip()
        t = re.sub(r"\b(const|volatile|struct|class|enum|mutable)\b", "", t).strip()
        t = re.sub(r"\s+", " ", t)
        if t.endswith("*") or t.endswith("&"):
            return (8, 8)
        if t in PRIM:
            return PRIM[t]
        if t in VALUE_TYPES:
            v = VALUE_TYPES[t]
            return v if v[0] is not None else None
        # template
        m = re.match(r"([A-Za-z0-9_:]+)\s*<(.*)>$", t)
        if m:
            base, args = m.group(1), split_top(m.group(2), ",")
            if base in ("std::array",):
                inner = self.type_size(args[0])
                if inner is None:
                    return None
                return (inner[0] * int(args[1], 0), inner[1])
            if base in ("std::pair",):
                a, b = self.type_size(args[0]), self.type_size(args[1])
                if not a or not b:
                    return None
                al = max(a[1], b[1])
                off = (a[0] + b[1] - 1) // b[1] * b[1]
                sz = off + b[0]
                return ((sz + al - 1) // al * al, al)
            if base == "std::optional":
                inner = self.type_size(args[0])
                if not inner:
                    return None
                sz = inner[0] + 1
                al = inner[1]
                return ((sz + al - 1) // al * al, al)
            if base in ("std::atomic",):
                return self.type_size(args[0])
            if base in STD_FIXED and STD_FIXED[base][0] is not None:
                return STD_FIXED[base]
            if base in ("geode::SeedValue",):
                return (12, 4)
            return None
        bare = t.split("::")[-1]
        if t in self.enums or bare in self.enums:
            return self.enums.get(t, self.enums.get(bare))
        if t in self.classes or bare in self.classes:
            lay = self.layout(t if t in self.classes else bare)
            if lay:
                return (lay["size"], lay["align"])
            return None
        return None

    def is_polymorphic(self, name: str) -> bool:
        if name.startswith("cocos2d::") or name.startswith("cocos2d::extension::"):
            return True
        k = self.classes.get(name)
        if not k:
            return True  # delegate/protocol sconosciuti: quasi sempre polimorfici
        if k.has_virtual_decl or any(f.name.startswith("~") and f.is_virtual for f in k.funcs):
            return True
        return any(self.is_polymorphic(b) for b in k.bases)

    def base_size(self, b: str):
        if b in self.cocos_sizes:
            s, a, _ = self.cocos_sizes[b]
            return (s, a)
        if b.startswith("cocos2d::"):
            return None
        if b in self.size_checks and b not in self.classes:
            return (self.size_checks[b], 8)
        if b in self.classes:
            lay = self.layout(b)
            if lay:
                return (lay["size"], lay["align"])
            return None
        bare = b.split("::")[-1]
        if bare in self.size_checks:
            return (self.size_checks[bare], 8)
        return None

    def layout(self, name: str):
        if name in self.cache:
            return self.cache[name]
        self.cache[name] = None  # anti-ricorsione
        k = self.classes.get(name)
        if k is None:
            return None
        off = 0
        align = 1
        bases = list(k.bases)
        poly = self.is_polymorphic(name)
        # MSVC: la prima base polimorfica diventa primaria (offset 0)
        if poly:
            pbases = [b for b in bases if self.is_polymorphic(b)]
            if pbases:
                first = pbases[0]
                bases.remove(first)
                bases.insert(0, first)
            else:
                off = 8
                align = 8
        base_offsets = {}
        for b in bases:
            bs = self.base_size(b)
            if bs is None:
                self.unknown.add(f"base {b} (in {name})")
                return None
            sz, al = bs
            if sz == 0:
                continue
            off = (off + al - 1) // al * al
            base_offsets[b] = off
            off += sz
            align = max(align, al)
        if poly:
            align = max(align, 8)
        members = []
        for it in k.items:
            if isinstance(it, Pad):
                members.append(("PAD", "", off))
                off += it.size
                continue
            ts = self.type_size(it.type)
            if ts is None:
                self.unknown.add(f"{it.type} (in {name}::{it.name})")
                return None
            sz, al = ts
            off = (off + al - 1) // al * al
            members.append((it.name, it.type, off))
            off += sz
            align = max(align, al)
        size = (off + align - 1) // align * align
        if size == 0:
            size = 1
        res = {"size": size, "align": align, "members": members, "bases": base_offsets}
        self.cache[name] = res
        return res


def load_size_checks(path: Path) -> dict[str, int]:
    out = {}
    if not path.exists():
        return out
    txt = path.read_text()
    m = re.search(r"#ifdef GEODE_IS_WINDOWS(.*?)#endif", txt, re.S)
    body = m.group(1) if m else txt
    for mm in re.finditer(r"GEODE_SIZE_CHECK\(([A-Za-z0-9_:]+),\s*(0x[0-9a-fA-F]+)\)", body):
        out[mm.group(1)] = int(mm.group(2), 16)
    return out


# dimensioni delle classi cocos2d (Windows x64, libcocos2d.dll di GD 2.2)
# ricavate e verificate con i size-check di classi GD che ne ereditano
COCOS_SIZES = {
    "cocos2d::CCObject": (0x38, 8),
    "cocos2d::CCNode": (0x140, 8),
    "cocos2d::CCNodeRGBA": (0x158, 8),
    "cocos2d::CCLayer": (0x1d8, 8),
    "cocos2d::CCLayerRGBA": (0x200, 8),
    "cocos2d::CCLayerColor": (0x220, 8),
    "cocos2d::CCLayerGradient": (0x248, 8),
    "cocos2d::CCSprite": (0x260, 8),
    "cocos2d::CCSpriteBatchNode": (0x168, 8),
    "cocos2d::CCMenu": (0x1e8, 8),
    "cocos2d::CCMenuItem": (0x180, 8),
    "cocos2d::CCMenuItemSprite": (0x198, 8),
    "cocos2d::CCMenuItemImage": (0x198, 8),
    "cocos2d::CCScene": (0x148, 8),
    "cocos2d::CCLabelBMFont": (0x1e0, 8),
    "cocos2d::CCParticleSystemQuad": (0x3d0, 8),
    "cocos2d::CCDrawNode": (0x188, 8),
    "cocos2d::CCAction": (0x50, 8),
    "cocos2d::CCActionInterval": (0x60, 8),
    "cocos2d::CCDictionary": (0x50, 8),
    "cocos2d::CCArray": (0x48, 8),
    "cocos2d::CCString": (0x58, 8),
    "cocos2d::CCTransitionScene": (0x168, 8),
    "cocos2d::extension::CCScale9Sprite": (0x2a8, 8),
    "cocos2d::CCTouchDelegate": (0x10, 8),
    "cocos2d::CCKeypadDelegate": (0x8, 8),
    "cocos2d::CCKeyboardDelegate": (0x8, 8),
    "cocos2d::CCMouseDelegate": (0x8, 8),
    "cocos2d::CCAccelerometerDelegate": (0x8, 8),
    "cocos2d::CCIMEDelegate": (0x8, 8),
    "cocos2d::CCTextFieldDelegate": (0x8, 8),
    "cocos2d::CCRGBAProtocol": (0x8, 8),
    "cocos2d::CCBlendProtocol": (0x8, 8),
    "cocos2d::CCTextureProtocol": (0x8, 8),
    "cocos2d::CCLabelProtocol": (0x8, 8),
    "cocos2d::extension::CCScrollViewDelegate": (0x8, 8),
    "cocos2d::extension::CCTableViewDelegate": (0x8, 8),
    "cocos2d::extension::CCTableViewDataSource": (0x8, 8),
    "cocos2d::CCSceneDelegate": (0x8, 8),
}


def infer_cocos_sizes(lay: Layout, verbose=False):
    """Prova a dedurre dimensioni mancanti/errate delle basi cocos dai size-check."""
    lay.cocos_sizes = {k: (v[0], v[1], True) for k, v in COCOS_SIZES.items()}
    cocos_bases = set()
    for k in lay.classes.values():
        for b in k.bases:
            if b.startswith("cocos2d::"):
                cocos_bases.add(b)
    # due passate: alcune classi dipendono da piu' basi cocos
    for _ in range(3):
        for b in sorted(cocos_bases):
            children = [k for k in lay.classes.values()
                        if b in k.bases and k.name in lay.size_checks and not k.name.startswith("cocos2d")]
            if not children:
                continue
            best, best_ok = None, 0
            for cand in range(8, 0x800, 8):
                lay.cocos_sizes[b] = (cand, 8, True)
                lay.cache.clear()
                ok = 0
                for c in children:
                    l = lay.layout(c.name)
                    if l is not None and l["size"] == lay.size_checks[c.name]:
                        ok += 1
                if ok > best_ok:
                    best, best_ok = cand, ok
            if best is not None:
                lay.cocos_sizes[b] = (best, 8, True)
            elif b in COCOS_SIZES:
                lay.cocos_sizes[b] = (*COCOS_SIZES[b], True)
            else:
                lay.cocos_sizes.pop(b, None)
    if verbose:
        for b in sorted(cocos_bases):
            if b in lay.cocos_sizes and COCOS_SIZES.get(b, (None,))[0] != lay.cocos_sizes[b][0]:
                print(f"[infer] {b} = {hex(lay.cocos_sizes[b][0])} (tabella: {COCOS_SIZES.get(b)})", file=sys.stderr)
    lay.cache.clear()


# --------------------------------------------------------------------------
# Output
# --------------------------------------------------------------------------

def cpp_ident(s: str) -> str:
    return re.sub(r"[^A-Za-z0-9_]", "_", s)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("bindings")
    ap.add_argument("version")
    ap.add_argument("out")
    ap.add_argument("--checks-version", default=None, help="versione dei size-check da usare per la validazione")
    ap.add_argument("--classes", default="", help="lista classi da emettere (default: tutte)")
    ap.add_argument("--classes-file", default=None, help="file con una classe per riga")
    ap.add_argument("--cocos-ref", default=None, help="versione da cui dedurre le dimensioni cocos2d (se mancano i size-check)")
    ap.add_argument("--verbose", action="store_true")
    a = ap.parse_args()

    root = Path(a.bindings)
    bro_dir = root / "bindings" / a.version
    classes: dict[str, Klass] = {}
    free: list[Func] = []
    for f in ["Cocos2d.bro", "Extras.bro", "FMOD.bro", "Kazmath.bro", "GeometryDash.bro"]:
        p = bro_dir / f
        if p.exists():
            parse_bro(p.read_text(encoding="utf-8", errors="ignore"), classes, free)
    enums = parse_enums(root / "bindings" / "include" / "Geode" / "Enums.hpp")
    checks_ver = a.checks_version or a.version
    checks = load_size_checks(root / "test" / "members" / checks_ver / "Windows.cpp")

    lay = Layout(classes, enums, checks)
    if not checks and a.cocos_ref:
        ref_classes: dict[str, Klass] = {}
        ref_free: list[Func] = []
        for f in ["Cocos2d.bro", "Extras.bro", "FMOD.bro", "Kazmath.bro", "GeometryDash.bro"]:
            p = root / "bindings" / a.cocos_ref / f
            if p.exists():
                parse_bro(p.read_text(encoding="utf-8", errors="ignore"), ref_classes, ref_free)
        ref_checks = load_size_checks(root / "test" / "members" / a.cocos_ref / "Windows.cpp")
        ref = Layout(ref_classes, enums, ref_checks)
        infer_cocos_sizes(ref, a.verbose)
        lay.cocos_sizes = dict(ref.cocos_sizes)
    else:
        infer_cocos_sizes(lay, a.verbose)

    wanted = [c for c in a.classes.split(",") if c]
    if a.classes_file:
        wanted += [l.strip() for l in Path(a.classes_file).read_text().splitlines() if l.strip() and not l.startswith("#")]
    wanted_set = set(wanted)
    ok_classes, bad_classes, unk_classes = [], [], []
    for name in sorted(classes):
        if name.startswith("cocos2d") or name.startswith("FMOD::") or name == "<free>":
            continue
        l = lay.layout(name)
        if l is None:
            unk_classes.append(name)
            continue
        if name in checks:
            if l["size"] == checks[name]:
                ok_classes.append(name)
            else:
                bad_classes.append((name, l["size"], checks[name]))

    print(f"[{a.version}] classi: {len(classes)}  validate OK: {len(ok_classes)}  "
          f"mismatch: {len(bad_classes)}  layout ignoto: {len(unk_classes)}  size-check: {len(checks)}",
          file=sys.stderr)
    if a.verbose:
        for n, got, exp in bad_classes[:80]:
            print(f"  MISMATCH {n}: calcolato {hex(got)} atteso {hex(exp)}", file=sys.stderr)
        for u in sorted(lay.unknown)[:80]:
            print(f"  UNKNOWN {u}", file=sys.stderr)

    bad_set = {b[0] for b in bad_classes}
    out = []
    ver_id = cpp_ident(a.version)
    out.append("// GENERATO AUTOMATICAMENTE da tools/broma_gen.py - NON MODIFICARE A MANO")
    out.append(f"// Sorgente: geode-sdk/bindings, bindings/{a.version} (solo Windows x64)")
    out.append(f"// Size-check usati per la validazione: {checks_ver if checks else 'nessuno'}")
    out.append("#pragma once")
    out.append("#include <cstdint>\n")
    out.append(f"namespace gdv{ver_id} {{")
    out.append("")
    out.append("// --- indirizzi funzioni (RVA rispetto a GeometryDash.exe) ---")
    out.append("namespace addr {")
    emitted = set()
    for name in sorted(classes):
        k = classes[name]
        if name.startswith("cocos2d") or name.startswith("FMOD::") or name == "<free>":
            continue
        fl = [f for f in k.funcs if f.win is not None]
        if not fl or (wanted_set and name not in wanted_set):
            continue
        out.append(f"namespace {cpp_ident(name)} {{")
        counts: dict[str, int] = {}
        for f in fl:
            base = cpp_ident(f.name.replace("~", "dtor_"))
            if f.name == name:
                base = "ctor"
            n = counts.get(base, 0)
            counts[base] = n + 1
            ident = base if n == 0 else f"{base}_{n}"
            sig = re.sub(r"\s+", " ", f"{'static ' if f.is_static else ''}{'virtual ' if f.is_virtual else ''}{f.ret} {f.name}({f.params}){' const' if f.is_const else ''}")
            out.append(f"    inline constexpr uintptr_t {ident} = {hex(f.win)}; // {sig}")
        out.append("}")
    ffl = [f for f in free if f.win is not None]
    if ffl:
        out.append("namespace free_fn {")
        for f in ffl:
            out.append(f"    inline constexpr uintptr_t {cpp_ident(f.name)} = {hex(f.win)};")
        out.append("}")
    out.append("} // namespace addr\n")

    out.append("// --- offset dei membri (validati contro i size-check se disponibili) ---")
    out.append("namespace off {")
    for name in sorted(classes):
        if wanted_set and name not in wanted_set:
            continue
        if name.startswith("cocos2d") or name.startswith("FMOD::") or name == "<free>":
            continue
        if name in bad_set:
            out.append(f"// {name}: layout NON valido (size mismatch) -> vedi namespace off_unchecked")
            continue
        l = lay.cache.get(name)
        if not l:
            continue
        validated = name in checks and name not in bad_set
        if not l["members"]:
            continue
        out.append(f"namespace {cpp_ident(name)} {{ // sizeof = {hex(l['size'])}{' (validato)' if validated else ''}")
        out.append(f"    inline constexpr uintptr_t _size = {hex(l['size'])};")
        seen = set()
        for mname, mtype, moff in l["members"]:
            if mname == "PAD" or mname in seen:
                continue
            seen.add(mname)
            out.append(f"    inline constexpr uintptr_t {mname} = {hex(moff)}; // {mtype}")
        out.append("}")
    out.append("} // namespace off")
    out.append("")
    out.append("// --- offset NON validati (la dimensione calcolata non combacia col size-check) ---")
    out.append("// I membri iniziali sono spesso corretti, quelli finali possono essere sbagliati.")
    out.append("namespace off_unchecked {")
    for name in sorted(bad_set):
        if wanted_set and name not in wanted_set:
            continue
        l = lay.cache.get(name)
        if not l or not l["members"]:
            continue
        exp = checks.get(name, 0)
        out.append(f"namespace {cpp_ident(name)} {{ // calcolato {hex(l['size'])}, atteso {hex(exp)}")
        seen = set()
        for mname, mtype, moff in l["members"]:
            if mname == "PAD" or mname in seen:
                continue
            seen.add(mname)
            out.append(f"    inline constexpr uintptr_t {mname} = {hex(moff)}; // {mtype}")
        out.append("}")
    out.append("} // namespace off_unchecked")
    out.append("")
    out.append("} // namespace")
    Path(a.out).write_text("\n".join(out) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
