#!/usr/bin/env python3
"""gen_knowledge.py — the repo's RE knowledge as an IDA "knowledge pack" (JSON) for
tools/ida/oss_ida_export.py (kb=...). EN-old VAs only (the port's target edition).

  python3 tools/ida/gen_knowledge.py -o knowledge.json

Sources (the repo is the source of truth; nothing here is hand-maintained twice):
  names   tools/flow/retail_fields.json (curated engine names) + port provenance: a C function
          in src/*.c whose leading comment names exactly one `FUN_<va>` → `<c_name>` (a VA claimed
          by two port functions is skipped); retail fields win.
  globals GLOBALS below (well-established singletons from docs/findings) + retail_fields globals
  decls   retail-layout structs from src/*.h (headers with _Static_assert(offsetof ..)):
          preprocessed with the mingw32 cc, only declarations from src/ kept, typedef/struct/
          enum/union only (prototypes, inline bodies and asserts dropped)
  protos  thiscall prototypes from tools/ghidra-scripts/TagThiscallFunctions.java TAGS
          (Ghidra type words mapped to C)
Every applied name keeps the VA: `<name>_<VA>` so grep-by-address still works.
"""
import argparse
import json
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SRC = os.path.join(ROOT, "src")

# Well-established engine singletons (docs/findings/winmain-and-bootstrap.md, audio-init.md,
# input.md; rhost.c edition profile). (name, C type or None)
GLOBALS = {
    0x8a93cc: ("g_zdd", "zdd *"),          # DirectDraw wrapper (ZDD) object pointer
    0x8a93d0: ("g_zdi", None),             # DirectInput7 wrapper (ZDI)
    0x8a93d4: ("g_zds", None),             # DirectSound wrapper (ZDS)
    0x8a93d8: ("g_zdi_keyboard", None),    # ZDI keyboard sub-device
    0x8a93dc: ("g_zdi_joystick", None),    # [2] joystick sub-devices
    0x8a93e4: ("g_zdm", None),             # music manager (ZDM, DirectShow BGM)
    0x8a93ec: ("g_hwnd", "HWND"),          # game window
    0x8a952c: ("g_app_active", "int"),     # WM_ACTIVATEAPP wParam; the pump waits while 0
    0x8a9b64: ("g_app_state", None),       # app/run state (+0x1c pump wait flag, +0x0c timer)
    0x8a4f94: ("g_rand_seed", "unsigned int"),   # MSVC rand LCG state
    0x8a6e80: ("g_settings", None),        # in-memory config.dat (key map, sound, …)
    0x8a9a40: ("g_launcher_ok", "int"),    # launcher Launch clicked
    0x8a9b48: ("g_cfg_screen", "int"),     # LOWORD screen mode 3/4/5 | HIWORD VRAM
    0x8a9b4c: ("g_cfg_sound", "int"),
    0x8a9410: ("g_gamma_lut", "unsigned char[256]"),
}

GHIDRA_TYPES = [(r"\bundefined4\b", "int"), (r"\bundefined2\b", "short"), (r"\bundefined1\b", "char"),
                (r"\bundefined\b", "char"), (r"\buint\b", "unsigned int"), (r"\bushort\b", "unsigned short"),
                (r"\bbyte\b", "unsigned char"), (r"\bdword\b", "unsigned int"), (r"\bbool\b", "char")]


def port_names():
    """VA -> port C function name from provenance comments directly above definitions"""
    defn = re.compile(r"^(?!static\s+inline)(?:static\s+)?[A-Za-z_][\w\s\*]*?\b([a-z_][a-z0-9_]*)\s*\([^;]*$")
    claims = {}
    for fn in sorted(os.listdir(SRC)):
        if not fn.endswith(".c"):
            continue
        lines = open(os.path.join(SRC, fn), errors="replace").read().split("\n")
        for i, l in enumerate(lines):
            if not l or l[0] in " \t#/{}*" or l.startswith("typedef") or l.startswith("struct"):
                continue
            mm = defn.match(l)
            if not mm or mm.group(1) in ("if", "while", "for", "switch", "return", "sizeof"):
                continue
            # the definition must open a body within 3 lines
            if not any(x.startswith("{") or x.rstrip().endswith("{") for x in lines[i:i + 4]):
                continue
            # leading comment block
            j = i - 1; block = []
            while j >= 0 and j >= i - 30 and (lines[j].strip().startswith(("/*", "*", "//")) or lines[j].strip().endswith("*/")):
                block.append(lines[j]); j -= 1
            if not block:
                continue
            head = "\n".join(reversed(block))
            vas = set(re.findall(r"FUN_00([0-9a-f]{6})", head[:400]))
            if len(vas) == 1:
                va = int(vas.pop(), 16)
                claims.setdefault(va, set()).add(mm.group(1))
    return {va: next(iter(n)) for va, n in claims.items() if len(n) == 1}


def retail_fields():
    d = json.load(open(os.path.join(ROOT, "tools/flow/retail_fields.json")))["fields"]
    names, gl = {}, {}
    for k, v in d.items():
        va = int(k, 0)
        if "name" in v:
            names[va] = v["name"]
        for f in v.get("fields", []) if isinstance(v.get("fields"), list) else []:
            if f.get("src") == "global" and "global" in f and "name" in f:
                gl[int(str(f["global"]), 0)] = f["name"]
    return names, gl


def struct_decls():
    hdrs = [h for h in sorted(os.listdir(SRC)) if h.endswith(".h") and "_Static_assert(offsetof" in open(os.path.join(SRC, h), errors="replace").read()]
    cc = os.environ.get("RHOST_CC", "i686-w64-mingw32-gcc")
    out, seen = [], set()
    for h in hdrs:
        r = subprocess.run([cc, "-E", "-I", SRC, "-D_Static_assert(...)=", os.path.join(SRC, h)], capture_output=True, text=True)
        if r.returncode:
            print(f"warn: {h}: preprocess failed", file=sys.stderr)
            continue
        cur_src = False; text = []
        for line in r.stdout.split("\n"):
            if line.startswith("# "):
                m = re.match(r'# \d+ "([^"]+)"', line)
                if m:
                    cur_src = os.path.abspath(m.group(1)).startswith(SRC)
                continue
            if cur_src:
                text.append(line)
        body = "\n".join(text)
        # split into top-level declarations
        depth = 0; start = 0; i = 0; n = len(body)
        while i < n:
            c = body[i]
            if c == "{":
                depth += 1
            elif c == "}":
                depth -= 1
                if depth == 0:
                    # function body? (")" right before the matching "{")
                    k = body.rfind("{", start, i)
                    pre = body[start:body.find("{", start)].strip() if body.find("{", start) >= 0 else ""
                    if pre.endswith(")"):
                        start = i + 1
            elif c == ";" and depth == 0:
                d = body[start:i + 1].strip()
                start = i + 1
                if re.match(r"^(typedef\b|struct\s+\w+\s*\{|union\s+\w+\s*\{|enum\b)", d) and "__attribute__((visibility" not in d:
                    key = re.sub(r"\s+", " ", d)
                    if key not in seen:
                        seen.add(key); out.append(d)
            i += 1
    return out


def thiscall_protos(names):
    j = open(os.path.join(ROOT, "tools/ghidra-scripts/TagThiscallFunctions.java")).read()
    rows = re.findall(r'\{\s*0x([0-9a-fA-F]+)L,\s*"(\w+)",\s*((?:"[^"]*"\s*\+?\s*)+)\}', j)
    protos = {}
    for va, cls, parts in rows:
        p = "".join(re.findall(r'"([^"]*)"', parts))
        for a, b in GHIDRA_TYPES:
            p = re.sub(a, b, p)
        m = re.match(r"\s*(.+?)\s+FUN_([0-9a-f]+)\s*\((.*)\)\s*$", p)
        if not m:
            continue
        ret, args = m.group(1), m.group(3).strip()
        args = "" if args in ("", "void") else ", " + args
        v = int(va, 16)
        nm = names.get(v, "FUN_%08x" % v)
        protos[v] = f"{ret} __thiscall f({cls} *this{args});"
        if v not in names:
            names[v] = f"{cls}__m"
    return protos


def translate(kb, edition):
    """EN-old VAs → another edition via tools/ida/crosswalk.py outputs: names/comments for every
    matched function or global; prototypes only where the code is byte- or mnemonic-identical
    (a moved struct offset would make an applied prototype lie)."""
    D = os.environ.get("OSS_IDA_LOCAL", "/mnt/ephemeral/opensummoners/decompiled-ida")
    base = os.path.join(D, "plain") if os.path.isdir(os.path.join(D, "plain")) else D
    fn = {}; strict = set()
    for l in open(os.path.join(base, f"crosswalk_enold_{edition}.tsv")).readlines()[1:]:
        a, b, how = l.split("\t")[:3]
        fn[int(a, 16)] = int(b, 16)
        if how in ("bytes", "mnem"):
            strict.add(int(a, 16))
    dv = {}
    p = os.path.join(base, f"crosswalk_data_enold_{edition}.tsv")
    if os.path.exists(p):
        for l in open(p).readlines()[1:]:
            a, b = l.split("\t")[:2]
            dv[int(a, 16)] = int(b, 16)
    out = {"names": {}, "protos": {}, "gtypes": {}, "cmts": {}, "decls": kb["decls"]}
    def tr(va, allow_strict=False):
        v = int(va, 16)
        if v in fn and (not allow_strict or v in strict):
            return fn[v]
        return dv.get(v)
    for k in ("names", "cmts"):
        for va, x in kb[k].items():
            t = tr(va)
            if t:
                out[k]["%x" % t] = re.sub(r"_[0-9A-F]{6}$", "_%X" % t, x) if k == "names" else x + f" (EN-old {va})"
    for va, x in kb["protos"].items():
        t = tr(va, allow_strict=True)
        if t:
            out["protos"]["%x" % t] = x
    for va, x in kb["gtypes"].items():
        t = dv.get(int(va, 16))
        if t:
            out["gtypes"]["%x" % t] = x
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", required=True)
    ap.add_argument("--edition", default="enold", help="enold (native) or ense-en / ense-jp (translated through the crosswalk)")
    a = ap.parse_args()
    pn = port_names()
    rn, rg = retail_fields()
    names = dict(pn); names.update(rn)
    decls = struct_decls()
    protos = thiscall_protos(names)
    kb = {"names": {}, "protos": {}, "gtypes": {}, "cmts": {}, "decls": "\n".join(decls) + "\n"}
    for va, nm in names.items():
        if va not in protos:
            kb["names"]["%x" % va] = f"{nm}_{va:X}"
        kb["cmts"]["%x" % va] = f"port: {pn[va]} (src/)" if va in pn else f"retail_fields: {nm}"
    for va, p in protos.items():
        kb["protos"]["%x" % va] = p
    for va, (nm, ty) in GLOBALS.items():
        kb["names"]["%x" % va] = nm
        if ty:
            kb["gtypes"]["%x" % va] = ty
    for va, nm in rg.items():
        kb["names"].setdefault("%x" % va, f"g_{nm}")
    if a.edition != "enold":
        kb = translate(kb, a.edition)
    json.dump(kb, open(a.out, "w"), indent=0)
    print(f"knowledge: {len(pn)} port names, {len(rn)} retail_fields names, {len(protos)} thiscall protos, "
          f"{len(GLOBALS) + len(rg)} globals, {len(decls)} type decls → {a.out}"
          + (f" (translated to {a.edition}: {len(kb['names'])} names, {len(kb['protos'])} protos)" if a.edition != "enold" else ""))


if __name__ == "__main__":
    main()
