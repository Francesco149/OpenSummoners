#!/usr/bin/env python3
"""decomp2c.py — a FIRST PASS from an IDA decompile function to port-style C (the big transcriptions).

    python3 tools/ida/decomp2c.py 461790 [--edition enold] [--this c] > /tmp/x.c

What it does (mechanically; the result is then finished + checked against the asm BY HAND):
  * keeps the decompile's control flow verbatim (labels, gotos into cases — function-scoped in C too);
  * `*(_DWORD *)(X + N)` / `_WORD` / `_BYTE` / `__int16` -> F32(X, N) / F16 / F8 / S16 lvalue accessors
    (the module is compiled with no-strict-aliasing: see the generated prologue);
  * `this->field_X` -> F32/F16/F8(c, 0xX) by the width IDA's class table gives the field;
  * `sub_XXXXXX(` -> the port function named for FUN_00xxxxxx in src/*.h (else `/*TODO 0xXXXXXX*/sub_XXXXXX(`);
  * `rand_draw_5BF505()` -> rnd(); `p_cls_40BE00` -> W; `&unk_X` / `&byte_X` / `&dword_X` -> D(0xX);
    `stru_8A6B60._pad000[N]` -> SYS(N) (the system singleton's bytes);
  * the variable declarations are dropped (re-declare what the body uses as int32_t / pointers).
The output is NOT final code: casts of IDA class pointers, compound expressions over pointer-typed locals
(IDA's _DWORD * arithmetic scales by 4!) and every call's argument types need review.
"""
import argparse
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
IDA = os.environ.get("IDA_DIR", "/mnt/ephemeral/opensummoners/decompiled-ida")


def port_names():
    names = {}
    for fn in sorted(os.listdir(os.path.join(REPO, "src"))):
        if not fn.endswith(".h"):
            continue
        for m in re.finditer(r"FUN_00([0-9a-f]{6})\s+([A-Za-z_]\w*)", open(os.path.join(REPO, "src", fn), errors="replace").read()):
            names.setdefault(int(m.group(1), 16), m.group(2))
    return names


def prototypes():
    """name -> (return type, [param types]) from the src/*.h declarations"""
    protos = {}
    for fn in sorted(os.listdir(os.path.join(REPO, "src"))):
        if not fn.endswith(".h"):
            continue
        t = open(os.path.join(REPO, "src", fn), errors="replace").read()
        t = re.sub(r"/\*.*?\*/", " ", t, flags=re.S)
        for m in re.finditer(r"(^|\n)\s*([A-Za-z_][\w\s\*]*?)\b([A-Za-z_]\w*)\s*\(([^;{}()]*(?:\([^()]*\)[^;{}()]*)*)\)\s*;", t):
            ret, name, params = m.group(2).strip(), m.group(3), m.group(4)
            if ret.startswith(("typedef", "return", "#")) or "(" in params:
                continue
            ps = [p.strip() for p in params.split(",")] if params.strip() not in ("", "void") else []
            types = []
            for p in ps:
                if re.search(r"(\*|\w\s+)\s*[A-Za-z_]\w*$", p) and len(re.findall(r"[A-Za-z_]\w*", p)) > 1:
                    p = re.sub(r"[A-Za-z_]\w*$", "", p).strip()
                types.append(p or "int32_t")
            protos.setdefault(name, (ret, types))
    return protos


def wrappers(used, protos):
    out = []
    for n in sorted(used):
        if n not in protos:
            out.append(f"/* no prototype found for {n} */")
            continue
        ret, types = protos[n]
        args = ", ".join(f"intptr_t a{i}" for i in range(len(types))) or "void"
        call = ", ".join(f"({t})a{i}" for i, t in enumerate(types))
        body = f"{n}({call})"
        if ret == "void":
            out.append(f"static inline intptr_t W_{n}({args}) {{ {body}; return 0; }}")
        else:
            out.append(f"static inline intptr_t W_{n}({args}) {{ return (intptr_t){body}; }}")
    return "\n".join(out)


def class_widths(edition, cls):
    for line in open(os.path.join(IDA, edition, "classes_ida.tsv")):
        f = line.rstrip("\n").split("\t")
        if f[0] == cls and len(f) > 6:
            return {int(k, 16): int(v) for k, v in (x.split(":") for x in f[6].split(",") if x)}
    return {}


def extract(edition, va):
    src = open(os.path.join(IDA, edition, "all_ida.c"), errors="replace").read()
    m = re.search(r"\n([^\n;]*\bsub_%X\([^\n]*\)\n\{.*?\n\})" % va, src, re.S)
    if not m:
        raise SystemExit(f"sub_{va:X} not found")
    return m.group(1)


USED = set()
DECLS = []
ARRAYS = []
CLSPTR = []


TYPEMAP = [("unsigned __int16", "uint16_t"), ("__int16", "int16_t"), ("unsigned int", "uint32_t"),
           ("unsigned __int8", "uint8_t"), ("_DWORD", "int32_t"), ("_WORD", "uint16_t"), ("_BYTE", "uint8_t"),
           ("__int64", "int64_t"), ("unsigned __int64", "uint64_t"), ("char", "char"), ("bool", "int"), ("int", "int32_t")]


def map_type(t):
    t = t.strip()
    if t.startswith("cls_") or t.startswith("struct ") or t.startswith("_UNKNOWN"):
        return "intptr_t"   # class pointers: integers (a class-sized index then fails to compile: fix by hand)
    stars = t.count("*")
    base = t.replace("*", "").strip()
    for a, b in TYPEMAP:
        if base == a:
            base = b
            break
    if stars == 0 and base in ("int32_t", "uint32_t"):
        return "intptr_t"
    return base + (" " + "*" * stars if stars else "")


def convert(text, names, widths, this):
    lines = text.split("\n")
    head, body = lines[0], lines[2:-1]
    out, decls = [], []
    for ln in body:
        m = re.match(r"^  ([A-Za-z_][\w\s\*]*?)\s*\b([va]\d+|i)(\[\d+\])?; // ", ln)
        if m and m.group(1).strip().startswith(("cls_", "_UNKNOWN", "struct ")) and "*" in m.group(1):
            CLSPTR.append(m.group(2))
        if m:
            fo = re.search(r"\[ebp-([0-9A-F]+)h\]", ln)
            if m.group(3) and fo:   # a stack array: laid out in one frame buffer as in retail (writers overrun them)
                ARRAYS.append((int(fo.group(1), 16), map_type(m.group(1)), m.group(2), int(m.group(3)[1:-1])))
                continue
            decls.append(f"    {map_type(m.group(1))} {m.group(2)}{m.group(3) or ''};")
            continue
        out.append(ln)
    DECLS.extend(decls)
    s = "\n".join(out)
    W = {1: "F8", 2: "F16", 4: "F32"}
    s = re.sub(r"this->field_([0-9A-F]+)", lambda m: f"{W.get(widths.get(int(m.group(1), 16), 4), 'F32')}({this}, 0x{int(m.group(1), 16):x})", s)
    s = re.sub(r"this->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"GAP({this}, 0x{int(m.group(1), 16):x}, {m.group(2)})", s)
    s = re.sub(r"\bthis\b", this, s)
    s = s.replace("nullptr", "0")
    s = re.sub(r"\*\(\(_DWORD \*\)(\w+) \+ (\d+)\)", r"F32(\1, 4 * \2)", s)
    s = re.sub(r"\*\(\(_WORD \*\)(\w+) \+ (\d+)\)", r"F16(\1, 2 * \2)", s)
    inner = r"((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?)"
    s = re.sub(r"\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int) \*\*\)\(%s \+ (\d+)\)" % inner, r"F32(\1, \2)", s)
    for _ in range(4):
        for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("unsigned __int16", "F16"),
                     ("_BYTE", "F8"), ("char", "F8"), ("unsigned int", "F32"), ("_UNKNOWN \\*", "F32")):
            s = re.sub(r"\*\(%s \*\)\(%s \+ (\d+)\)" % (t, inner), r"%s(\1, \2)" % f, s)
            s = re.sub(r"\*\(%s \*\)(\w+)\b(?!\s*[\[(])" % t, r"%s(\1, 0)" % f, s)
            s = re.sub(r"\*\(%s \*\)\(%s\)" % (t, inner), r"%s((\1), 0)" % f, s)
        s = re.sub(r"\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int) \*\*\)\(%s\)" % inner, r"F32((\1), 0)", s)
    for v in CLSPTR:   # a class pointer kept as an integer: its unary deref is a dword load
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b|(?<=[=(,!])\*%s\b" % (v, v), f"F32({v}, 0)", s)
    s = re.sub(r"\b(\w+)->field_([0-9A-F]+)", lambda m: f"F32({m.group(1)}, 0x{int(m.group(2), 16):x})", s)
    s = re.sub(r"\*\*\(_DWORD \*\*\)&stru_8A6B60\._pad000\[800\]", "CFG()", s)
    s = re.sub(r"\*\((?:_DWORD|int|cls_\w+) \*\*\)\(%s \+ (\d+)\)" % r"((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?)", r"F32(\1, \2)", s)
    s = s.replace("rand_draw_5BF505()", "rnd()")
    s = s.replace("p_cls_40BE00", "W")
    s = re.sub(r"\*\(_DWORD \*\)&stru_8A6B60\._pad000\[(\d+)\]", r"SYS32(\1)", s)
    s = re.sub(r"stru_8A6B60\._pad000\[(\d+)\]", r"SYSB(\1)", s)
    s = re.sub(r"&(?:unk|byte|dword|word|asc|off|stru|o_cls_\w+?)_([0-9A-F]{6})\b", lambda m: f"D(0x{int(m.group(1), 16):x})", s)

    DSB = {0x5bb870: "g_eng.dsb_volume_5bb870", 0x5bb880: "g_eng.dsb_pan_5bb880", 0x5bb890: "g_eng.dsb_freq_5bb890",
           0x5bb8c0: "g_eng.dsb_play_5bb8c0", 0x5bb8e0: "g_eng.dsb_stop_5bb8e0"}

    def call(m):
        va = int(m.group(1), 16)
        if va in DSB:
            return f"{DSB[va]}("
        n = names.get(va)
        if n:
            USED.add(n)
            return f"W_{n}("
        return f"/*TODO 0x{va:x}*/sub_{m.group(1)}("
    s = re.sub(r"\b(?:sub|[A-Za-z_]+?)_([0-9A-F]{6})\(", lambda m: call(m) if re.match(r"(sub|\w+)_[0-9A-F]{6}\(", m.group(0)) else m.group(0), s)
    s = re.sub(r"\(cls_\w+ \*\*?\)", "", s)
    s = re.sub(r"\((?:const )?(?:_DWORD|_WORD|_BYTE|int|char|unsigned __int16|__int16|unsigned int|void) \*\*?\)(?=\w|\()", "", s)
    s = re.sub(r"\*&SYSB\((\d+)\)", r"SYS32(\1)", s)
    s = re.sub(r"\*\(_DWORD \*\)&W->gap([0-9A-F]+)\[(\d+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + {m.group(2)})", s)
    s = re.sub(r"W->field_([0-9A-F]+)", lambda m: f"F32(W, 0x{int(m.group(1), 16):x})", s)
    s = re.sub(r"\b(LOWORD|HIWORD|LOBYTE)\((\w+)\) = ([^;]+);", lambda m: {"LOWORD": "SETLO", "HIWORD": "SETHI", "LOBYTE": "SETLB"}[m.group(1)] + f"({m.group(2)}, {m.group(3)});", s)
    s = re.sub(r"\bLOWORD\((\w+)\)", r"((uint16_t)(\1))", s)
    s = re.sub(r"\bLOBYTE\((\w+)\)", r"((uint8_t)(\1))", s)
    s = s.replace("unsigned __int64", "uint64_t").replace("__int64", "int64_t")
    s = re.sub(r"\(_WORD\)", "(uint16_t)", s)
    s = re.sub(r"\(__int16\)", "(int16_t)", s)
    s = re.sub(r"\(_BYTE\)", "(uint8_t)", s)
    s = re.sub(r"\(_DWORD\)", "(int32_t)", s)
    s = re.sub(r"\(unsigned __int16\)", "(uint16_t)", s)
    return head, s


PROLOGUE = """#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
"""


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("va")
    ap.add_argument("--edition", default="enold")
    ap.add_argument("--cls", default="cls_4022A0", help="the `this` class (field widths)")
    ap.add_argument("--this", default="c")
    a = ap.parse_args()
    va = int(a.va, 16)
    head, s = convert(extract(a.edition, va), port_names(), class_widths(a.edition, a.cls), a.this)
    print(PROLOGUE)
    print(wrappers(USED, prototypes()))
    print(f"/* {head} */")
    print("{")
    print("\n".join(DECLS))
    if ARRAYS:
        top = max(o for o, *_ in ARRAYS)
        print(f"    _Alignas(4) uint8_t frame_[0x{top:x} + 0x1000];   /* the stack arrays at their ebp offsets */")
        print("    memset(frame_, 0, sizeof frame_);")
        for o, t, n, cnt in sorted(ARRAYS, reverse=True):
            print(f"    {t} *{n} = ({t} *)(frame_ + 0x{top - o:x});   /* [{cnt}] at ebp-0x{o:x} */")
    print(s)
    print("}")
    todo = sorted(set(re.findall(r"TODO 0x([0-9a-f]+)", s)))
    print(f"/* unported callees: {' '.join('0x' + t for t in todo) or 'none'} */")


if __name__ == "__main__":
    main()
