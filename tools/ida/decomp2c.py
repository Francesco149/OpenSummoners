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


OVERRIDES = {0x4766a0: "view_box_visible"}   # ported functions whose header line is not a `FUN_ name` table


def port_names():
    names = dict(OVERRIDES)
    protos = prototypes()
    for fn in sorted(os.listdir(os.path.join(REPO, "src"))):
        if not fn.endswith(".h"):
            continue
        for m in re.finditer(r"FUN_00([0-9a-f]{6})\s+([A-Za-z_]\w*)", open(os.path.join(REPO, "src", fn), errors="replace").read()):
            if m.group(2) in protos:   # a real port function (prose like "FUN_0044e680 call (...)" is not)
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
            PROTO_HDR.setdefault(name, fn)
    return protos


def wrappers(used, protos):
    out = []
    for n in sorted(used):
        if n not in protos:
            out.append(f"/* no prototype found for {n} */")
            continue
        ret, types = protos[n]
        args = ", ".join(f"intptr_t a{i}" for i in range(len(types)))
        args = (args + ", ...") if args else "void"   # IDA sometimes counts arguments a callee never reads
        call = ", ".join(f"({t})a{i}" for i, t in enumerate(types))
        body = f"{n}({call})"
        if ret == "void":
            out.append(f"static inline intptr_t W_{n}({args}) {{ {body}; return 0; }}")
        else:
            out.append(f"static inline intptr_t W_{n}({args}) {{ return (intptr_t){body}; }}")
    return "\n".join(out)


def data_symbols(edition):
    syms = {}
    for line in open(os.path.join(IDA, edition, "data_xrefs.tsv")):
        f = line.rstrip("\n").split("\t")
        if len(f) > 1 and re.match(r"^[0-9a-f]{8}$", f[0]) and re.match(r"^[A-Za-z_]\w{2,}$", f[1]):
            syms.setdefault(f[1], int(f[0], 16))
    return syms


def class_widths(edition, cls):
    for line in open(os.path.join(IDA, edition, "classes_ida.tsv")):
        f = line.rstrip("\n").split("\t")
        if f[0] == cls and len(f) > 6:
            return {int(k, 16): int(v) for k, v in (x.split(":") for x in f[6].split(",") if x)}
    return {}


def extract(edition, va):
    src = open(os.path.join(IDA, edition, "all_ida.c"), errors="replace").read()
    m = re.search(r"\n([^\n;]*\bsub_%X\([^;{]*?\)\n\{.*?\n\})" % va, src, re.S)
    if not m:
        raise SystemExit(f"sub_{va:X} not found")
    return m.group(1)


USED = set()
INTVARS = []
PROTO_HDR = {}
LOCAL = {}
SYMS = {}
PTRVARS = set()   # typed-pointer locals / params: an offset off them scales by the element
CLSIDX = {}       # class-pointer locals (kept as integers): name -> (class, size)
_CLS_SIZE = {}


def class_size(cls):
    if not _CLS_SIZE:
        for line in open(os.path.join(IDA, EDITION[0], "classes_ida.tsv")):
            f = line.rstrip("\n").split("\t")
            if len(f) > 1 and f[0].startswith("cls_"):
                _CLS_SIZE[f[0]] = int(f[1], 16)
    return _CLS_SIZE.get(cls, 0)


def accx(f, base, off):
    """the F32/F16/S16/F8 accessor at base + off: bytes, unless base is a typed pointer (its C arithmetic)"""
    if base.strip() in PTRVARS:
        return f"{f}({base.strip()} + ({off}), 0)"
    return f"{f}({base}, {off})"


EDITION = ["enold"]
DECLS = []
ARRAYS = []
CLSPTR = []


TYPEMAP = [("unsigned __int16", "uint16_t"), ("__int16", "int16_t"), ("unsigned int", "uint32_t"),
           ("unsigned __int8", "uint8_t"), ("_DWORD", "int32_t"), ("_WORD", "uint16_t"), ("_BYTE", "uint8_t"),
           ("__int64", "int64_t"), ("unsigned __int64", "uint64_t"), ("char", "char"), ("bool", "int"), ("int", "int32_t")]


def map_type(t):
    t = t.strip()
    if t.startswith("cls_") or t.startswith("struct ") or t.startswith("_UNKNOWN"):
        n = t.count("*")
        if n >= 2:   # a pointer to class pointers: a table of 4-byte slots
            return "intptr_t " + "*" * (n - 1)
        return "intptr_t"   # class pointers: integers (a class-sized index then fails to compile: fix by hand)
    stars = t.count("*")
    base = t.replace("*", "").strip()
    for a, b in TYPEMAP:
        if base == a:
            base = b
            break
    else:
        if base not in ("void", "int8_t", "int16_t", "int32_t", "uint8_t", "uint16_t", "uint32_t", "int64_t", "uint64_t",
                        "double", "float", "long double"):
            return "intptr_t"   # Windows / CRT types IDA guessed (FILE *, LPCSTR, HWND ...): integers
    if stars == 0 and base in ("int32_t", "uint32_t"):
        return "intptr_t"
    return base + (" " + "*" * stars if stars else "")


def convert(text, names, widths, this):
    lines = text.split("\n")
    k = lines.index("{")
    head, body = " ".join(l.strip() for l in lines[:k]), lines[k + 1:-1]
    hm = re.search(r"\((.*)\)\s*$", head)
    for p in (hm.group(1).split(",") if hm else []):
        pm = re.match(r"(.*?)\s*\b(\w+)$", p.strip())
        if pm and "*" in pm.group(1) and "*" in map_type(pm.group(1)):
            PTRVARS.add(pm.group(2))
    out, decls = [], []
    for ln in body:
        m = re.match(r"^  ([A-Za-z_][\w\s\*]*?)\s*\b([A-Za-z_]\w*)(\[\d+\])?; // ", ln)
        if m and m.group(1).strip().startswith(("cls_", "_UNKNOWN", "struct ")) and "*" in m.group(1):
            CLSPTR.append(m.group(2))
        if m:
            fo = re.search(r"\[ebp-([0-9A-F]+)h\]", ln)
            if m.group(3) and fo:   # a stack array: laid out in one frame buffer as in retail (writers overrun them)
                ARRAYS.append((int(fo.group(1), 16), map_type(m.group(1)), m.group(2), int(m.group(3)[1:-1])))
                PTRVARS.add(m.group(2))
                continue
            mt = map_type(m.group(1))
            if mt == "intptr_t" and not m.group(3):
                INTVARS.append(m.group(2))
            if "*" in mt:
                PTRVARS.add(m.group(2))
            cm = re.match(r"(cls_\w+) \*$", m.group(1).strip())
            if cm and class_size(cm.group(1)):
                CLSIDX[m.group(2)] = (cm.group(1), class_size(cm.group(1)))
            decls.append(f"    {mt} {m.group(2)}{m.group(3) or ''};")
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
    s = re.sub(r"\*\*\(_DWORD \*\*\)&stru_8A6B60\._pad000\[800\]", "CFG()", s)
    s = re.sub(r"\*\*\((?:_DWORD|int|cls_\w+|_UNKNOWN) \*\*\)&stru_8A6B60\._pad000\[([^\]]+)\]", r"F32(SYS32(\1), 0)", s)
    s = re.sub(r"\*\((?:cls_\w+ \*+|_DWORD \*|int \*|_UNKNOWN \*\*|unsigned __int16 \*\*|__int16 \*\*|_WORD \*\*|char \*\*|_BYTE \*\*|_DWORD \*\*)\)&stru_8A6B60\._pad000\[([^\]]+)\]", r"SYS32(\1)", s)
    s = re.sub(r"\*\(_WORD \*\)&stru_8A6B60\._pad000\[([^\]]+)\]", r"F16(g_eng.sys, \1)", s)
    for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("_BYTE", "F8")):
        s = re.sub(r"\*\(%s \*\)&W->gap([0-9A-F]+)\[([^\]]+)\]" % t, lambda m: f"{f}(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
        s = re.sub(r"\*\(%s \*\)&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]" % t, lambda m: f"{f}(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
        s = re.sub(r"\*\(%s \*\)&(\w+)\[([^\]]+)\]" % t, r"%s(&\1[\2], 0)" % f, s)
    s = re.sub(r"&p_cls_40BE00->gap([0-9A-F]+)\[(\d+)\]", lambda m: f"(W + 0x{int(m.group(1), 16):x} + {m.group(2)})", s)
    for t, f in (("_BYTE", "F8"), ("_WORD", "F16"), ("_DWORD", "F32"), ("char", "F8"), ("__int16", "S16")):
        s = re.sub(r"\*\*\(%s \*\*\)\(%s \+ (\d+)\)" % (t, inner), lambda m, f=f: f"{f}({accx("F32", m.group(1), m.group(2))}, 0)", s)
    s = re.sub(r"\*\(\(_DWORD \*\)(\w+) \+ ([^()]+?)\)", r"F32(\1, 4 * (\2))", s)
    s = re.sub(r"\*\(\((?:_DWORD|cls_\w+|int|_WORD|_UNKNOWN) \*\*\)(\w+) \+ ([^()]+?)\)", r"F32(\1, 4 * (\2))", s)
    s = re.sub(r"\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int|_WORD|_BYTE|char|__int16|unsigned __int16) \*\*+\)\(%s \+ ([^()]+?)\)" % inner, lambda m: accx("F32", m.group(1), m.group(2)), s)
    for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("unsigned __int16", "F16"), ("_BYTE", "F8")):
        s = re.sub(r"\*\(%s \*\)(\w+)->gap([0-9A-F]+)\b(?!\s*\[)" % t,
                   lambda m, f=f: f"{f}({'W' if m.group(1) == 'p_cls_40BE00' else m.group(1)}, 0x{int(m.group(2), 16):x})", s)
    s = re.sub(r"\*\((?:cls_\w+|_UNKNOWN|void) \*\*+\)(\w+)\b(?!\s*[\[(])", r"F32(\1, 0)", s)
    for _ in range(4):
        for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("unsigned __int16", "F16"),
                     ("_BYTE", "F8"), ("char", "F8"), ("unsigned __int8", "F8"), ("unsigned int", "F32"), ("_UNKNOWN \\*", "F32")):
            s = re.sub(r"\*\(%s \*\)\(%s \+ (\d+)\)" % (t, inner), lambda m, f=f: accx(f, m.group(1), m.group(2)), s)
            s = re.sub(r"\*\(%s \*\)(\w+)\b(?!\s*[\[(])" % t, r"%s(\1, 0)" % f, s)
            s = re.sub(r"\*\(%s \*\)\(%s\)" % (t, inner), r"%s((\1), 0)" % f, s)
        s = re.sub(r"\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int|_WORD|_BYTE|char|__int16|unsigned __int16) \*\*\)\(%s\)" % inner, r"F32((\1), 0)", s)
    for v, (cls, n) in CLSIDX.items():   # a class pointer kept as an integer, indexed: by the class size
        s = re.sub(r"(?:\(char \*\))?&%s\[([^\]]+)\]" % v, lambda m: f"({v} + ({m.group(1)}) * 0x{n:x})", s)
    for v in INTVARS:   # an integer local used as a pointer by IDA's cast-free deref: a dword load
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b(?!\s*[\[(])|(?<=[=(,!])\*%s\b(?!\s*[\[(])" % (v, v), f"F32({v}, 0)", s)
    for v in CLSPTR:   # a class pointer kept as an integer: its unary deref is a dword load
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b|(?<=[=(,!])\*%s\b" % (v, v), f"F32({v}, 0)", s)
    s = re.sub(r"\b(\w+)->field_([0-9A-F]+)", lambda m: f"F32({m.group(1)}, 0x{int(m.group(2), 16):x})", s)
    s = re.sub(r"\*\*\(_DWORD \*\*\)&stru_8A6B60\._pad000\[800\]", "CFG()", s)
    s = re.sub(r"\*\((?:_DWORD|int|cls_\w+) \*\*\)\(%s \+ (\d+)\)" % r"((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?)", r"F32(\1, \2)", s)
    s = s.replace("rand_draw_5BF505()", "rnd()")
    s = s.replace("p_cls_40BE00", "W")
    s = re.sub(r"\*\(_DWORD \*\)&stru_8A6B60\._pad000\[(\d+)\]", r"SYS32(\1)", s)
    s = re.sub(r"stru_8A6B60\._pad000\[(\d+)\]", r"SYSB(\1)", s)
    s = re.sub(r"\bdbl_([0-9A-F]{6})\b", lambda m: f"(*(const double *)D(0x{int(m.group(1), 16):x}))", s)
    s = re.sub(r"\bflt_([0-9A-F]{6})\b", lambda m: f"(*(const float *)D(0x{int(m.group(1), 16):x}))", s)
    for kind, f in (("dword", "int32_t"), ("word", "uint16_t"), ("byte", "uint8_t")):
        s = re.sub(r"\b%s_([0-9A-F]{6})\[" % kind, lambda m: f"(({f} *)D(0x{int(m.group(1), 16):x}))[", s)
    for name, va in SYMS.items():
        if not re.search(r"\b%s\b" % re.escape(name), s) or name.startswith(("sub_", "p_cls_")) or name in ("Format", "Buffer", "stru_8A6B60") \
                or re.match(r"^a\d+$", name) or re.search(r"\b%s\b" % re.escape(name), head):
            continue
        s = re.sub(r"&%s\b" % re.escape(name), f"D(0x{va:x})", s)
        if name.startswith(("dword_", "word_", "byte_")):
            acc = {"dword_": "F32", "word_": "F16", "byte_": "F8"}[name.split("_")[0] + "_"]
            s = re.sub(r"(?<![\w.])%s\b(?!\s*\[)" % re.escape(name), f"{acc}(D(0x{va:x}), 0)", s)
        elif re.match(r"^(a[A-Z0-9]\w*|trailer|stru_\w+|unk_\w+|off_\w+|asc_\w+)$", name):
            s = re.sub(r"(?<![\w.&])%s\b(?!\s*[\[.])" % re.escape(name), f"D(0x{va:x})", s)
    s = re.sub(r"&(?:unk|byte|dword|word|asc|off|stru|o_cls_\w+?)_(?!8A6B60\b)([0-9A-F]{6})\b", lambda m: f"D(0x{int(m.group(1), 16):x})", s)

    DSB = {0x5bb870: "g_eng.dsb_volume_5bb870", 0x5bb880: "g_eng.dsb_pan_5bb880", 0x5bb890: "g_eng.dsb_freq_5bb890",
           0x5bb8c0: "g_eng.dsb_play_5bb8c0", 0x5bb8e0: "g_eng.dsb_stop_5bb8e0",
           0x406440: "g_eng.log_fail_406440", 0x408b90: "g_eng.h408b90", 0x5bf3ee: "g_eng.h5bf3ee",
           0x426110: "g_eng.h426110", 0x5bf5db: "g_eng.h5bf5db", 0x407f30: "g_eng.h407f30",
           0x471080: "ftol_round_"}   # platform / log slots

    def call(m):
        va = int(m.group(1), 16)
        if va in DSB:
            return f"{DSB[va]}("
        if va in LOCAL:
            return f"{LOCAL[va]}("
        n = names.get(va)
        if n:
            USED.add(n)
            return f"W_{n}("
        return f"/*TODO 0x{va:x}*/sub_{m.group(1)}("
    s = re.sub(r"\b(?:sub|[A-Za-z_]+?)_([0-9A-F]{6})\(", lambda m: call(m) if re.match(r"(sub|\w+)_[0-9A-F]{6}\(", m.group(0)) else m.group(0), s)
    s = re.sub(r"\(cls_\w+ \*+\)", "", s)
    # a pointer cast under arithmetic scales it (`(_BYTE *)v + 2` = 2 bytes): kept, as the C type
    CT = {"_DWORD": "int32_t", "_WORD": "uint16_t", "_BYTE": "uint8_t", "int": "int32_t", "char": "uint8_t",
          "unsigned __int16": "uint16_t", "__int16": "int16_t", "unsigned int": "uint32_t", "void": "uint8_t"}
    s = re.sub(r"\((?:const )?(_DWORD|_WORD|_BYTE|int|char|unsigned __int16|__int16|unsigned int) (\*\*?)\)(?=(?:\w+|\((?:[^()]|\([^()]*\))*\)) [-+] )",
               lambda m: f"({CT[m.group(1)] if m.group(2) == '*' else 'int32_t'} *)", s)
    s = re.sub(r"\((?:const )?(?:_DWORD|_WORD|_BYTE|int|char|unsigned __int16|__int16|unsigned int|void) \*\*?\)(?=\w|\()", "", s)
    s = re.sub(r"\*&SYSB\((\d+)\)", r"SYS32(\1)", s)
    s = s.replace("operator new(", "(intptr_t)g_eng.op_new(").replace("operator delete(", "g_eng.op_delete((void *)")
    s = re.sub(r"&stru_8A6B60\._pad000\[([^\]]+)\]", r"((uint8_t *)g_eng.sys + (\1))", s)
    s = re.sub(r"\*\(_DWORD \*\)&W->gap([0-9A-F]+)\[(\d+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + {m.group(2)})", s)
    s = re.sub(r"W->field_([0-9A-F]+)", lambda m: f"F32(W, 0x{int(m.group(1), 16):x})", s)
    SETS = {"LOWORD": "SETLO", "HIWORD": "SETHI", "LOBYTE": "SETLB", "LODWORD": "SETLODW", "HIDWORD": "SETHIDW"}
    out, pos = [], 0
    for m in re.finditer(r"\b(LOWORD|HIWORD|LOBYTE|LODWORD|HIDWORD)\((\w+)\) = ", s):
        if m.start() < pos:
            continue
        k, depth = m.end(), 0
        while k < len(s):
            ch = s[k]
            if ch == "(":
                depth += 1
            elif ch == ")":
                if depth == 0:
                    break
                depth -= 1
            elif ch == ";" and depth == 0:
                break
            k += 1
        out.append(s[pos:m.start()] + f"{SETS[m.group(1)]}({m.group(2)}, {s[m.end():k]})")
        pos = k
    s = "".join(out) + s[pos:]
    s = re.sub(r"\bLOWORD\((\w+)\)", r"((uint16_t)(\1))", s)
    s = re.sub(r"\bLOBYTE\((\w+)\)", r"((uint8_t)(\1))", s)
    s = s.replace("unsigned __int64", "uint64_t").replace("__int64", "int64_t")
    s = re.sub(r"\((?:FILE|HWND|HANDLE|HMODULE|LPCSTR|LPSTR|LPVOID|LPCVOID|DWORD|BOOL|LONG) \*?\)", "", s)
    s = re.sub(r"\(_WORD\)", "(uint16_t)", s)
    s = re.sub(r"\(__int16\)", "(int16_t)", s)
    s = re.sub(r"\(_BYTE\)", "(uint8_t)", s)
    s = re.sub(r"\(_DWORD\)", "(int32_t)", s)
    s = re.sub(r"\(unsigned __int16\)", "(uint16_t)", s)
    # retail's log 0x406440 inlined (OutputDebugStringA x 3 + the dated line appended to the log file)
    s = re.sub(r"OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\(OutputString\);\s*"
               r"(\w+) = fopen\(stru_8A9530\.path, Mode\);\s*if \( \3 != 0 \)\s*\{\s*GetLocalTime\(&SystemTime\);\s*"
               r"fprintf\(.*?\);\s*fclose\(\3\);\s*\}",
               r"g_eng.log_fail_406440(g_eng.log, (const char *)(\1), (const char *)(\2));", s, flags=re.S)
    return head, s


PROLOGUE = """#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
static inline int ofsub32_(int32_t a, int32_t b) { int32_t r; return __builtin_sub_overflow(a, b, &r); }
static inline int ofadd32_(int32_t a, int32_t b) { int32_t r; return __builtin_add_overflow(a, b, &r); }
#define __OFSUB__(a, b) ofsub32_((int32_t)(a), (int32_t)(b))
#define __OFADD__(a, b) ofadd32_((int32_t)(a), (int32_t)(b))
#define __CFADD__(a, b) ((uint32_t)(a) + (uint32_t)(b) < (uint32_t)(a))
#define __CFSUB__(a, b) ((uint32_t)(a) < (uint32_t)(b))
#define __SETS__(x) ((int32_t)(x) < 0)
#define LODWORD(x) ((uint32_t)(uint64_t)(x))
#define HIDWORD(x) ((uint32_t)((uint64_t)(x) >> 32))
#define WORD1(x) ((uint16_t)((uint64_t)(x) >> 16))
#define WORD2(x) ((uint16_t)((uint64_t)(x) >> 32))
#define BYTE1(x) ((uint8_t)((uint64_t)(x) >> 8))
#define BYTE2(x) ((uint8_t)((uint64_t)(x) >> 16))
#define HIBYTE(x) ((uint8_t)((uint32_t)(x) >> 24))
#define HIWORD(x) ((uint16_t)((uint32_t)(x) >> 16))
#define SHIDWORD(x) ((int32_t)((uint64_t)(x) >> 32))
#define SLODWORD(x) ((int32_t)(x))
#include <math.h>
/* 0x471080: rounded half away from zero (fcomp 0.0, then fsub / fadd 0.5 and __ftol's truncation) */
static inline int64_t ftol_round_(double a) { return a < 0.0 ? (int64_t)(a - 0.5) : (int64_t)(a + 0.5); }
static inline int32_t abs32(int32_t x) { return x < 0 ? (int32_t)(0u - (uint32_t)x) : x; }
#ifndef __cplusplus
#include <stdbool.h>
#endif
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
"""


def signature(head, name):
    """IDA header -> (C signature of the port function `name`, the param prologue)"""
    m = re.match(r"\s*(.*?)\s*(?:__thiscall|__stdcall|__fastcall|__cdecl)?\s*\bsub_[0-9A-F]+\((.*)\)\s*$", head)
    ret, params = m.group(1).replace("__thiscall", "").replace("__stdcall", "").strip(), m.group(2)
    ret = ("void" if ret == "void" else "double" if ret in ("long double", "double", "float")   # st0
           else "int64_t" if "__int64" in ret else "int32_t")                                    # edx:eax
    out, pro = [], []
    for p in [x.strip() for x in params.split(",") if x.strip()]:
        pm = re.match(r"(.*?)\s*\b(\w+)$", p)
        t, n = pm.group(1), pm.group(2)
        if n == "this":
            out.append("wl_char *self")
            pro.append("    intptr_t c = (intptr_t)self;")
            continue
        mt = map_type(t)
        out.append(f"{'int32_t' if mt == 'intptr_t' else mt} {n}")
    return f"{ret} {name}({', '.join(out) or 'void'})", "\n".join(pro)


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("va", nargs="+", help="VA, or VA:port_name (complete named functions, shared wrappers)")
    ap.add_argument("--edition", default="enold")
    ap.add_argument("--cls", default="cls_4022A0", help="the `this` class (field widths)")
    ap.add_argument("--this", default="c")
    a = ap.parse_args()
    names, widths, funcs, todo = port_names(), class_widths(a.edition, a.cls), [], set()
    SYMS.update(data_symbols(a.edition))
    EDITION[0] = a.edition
    for spec in a.va:
        va_s, _, pname = spec.partition(":")
        if pname:
            LOCAL[int(va_s, 16)] = pname
    protos_local = []
    for spec in a.va:
        va_s, _, pname = spec.partition(":")
        va = int(va_s, 16)
        DECLS.clear(); ARRAYS.clear(); CLSPTR.clear(); INTVARS.clear(); PTRVARS.clear(); CLSIDX.clear()
        head, s = convert(extract(a.edition, va), names, widths, a.this)
        out = []
        if pname:
            sig, pro = signature(head, pname)
            protos_local.append(sig + ";")
            out.append(f"/* 0x{va:x}: {head} */")
            out.append(sig)
            out.append("{")
            out.append(pro)
        else:
            out.append(f"/* {head} */")
            out.append("{")
        out.append("\n".join(DECLS))
        if ARRAYS:
            top = max(o for o, *_ in ARRAYS)
            out.append(f"    _Alignas(4) uint8_t frame_[0x{top:x} + 0x1000];   /* the stack arrays at their ebp offsets */")
            out.append("    memset(frame_, 0, sizeof frame_);")
            for o, t, n, cnt in sorted(ARRAYS, reverse=True):
                out.append(f"    {t} *{n} = ({t} *)(frame_ + 0x{top - o:x});   /* [{cnt}] at ebp-0x{o:x} */")
        out.append(s)
        out.append("}")
        funcs.append("\n".join(out))
        todo |= set(re.findall(r"TODO 0x([0-9a-f]+)", s))
    # a thiscall callee whose `this` IDA left out (its prototype refined after the caller was decompiled; the asm
    # passes the caller's ecx): the caller's `this` is passed
    nparm = {}
    for sig in protos_local:
        m = re.match(r"\w+ (\w+)\((.*)\);$", sig)
        nparm[m.group(1)] = (0 if m.group(2) == "void" else m.group(2).count(",") + 1, m.group(2).startswith("wl_char *self"))
    for k, f in enumerate(funcs if nparm else []):
        has_this = "intptr_t c = (intptr_t)self;" in f
        out, pos = [], 0
        for m in re.finditer(r"(?<![\w.])(%s)\(" % "|".join(map(re.escape, nparm)), f):
            if m.start() < pos or re.match(r"\s*(int32_t|void) $", f[max(0, m.start() - 8):m.start()]):
                continue
            j, depth, args, top = m.end(), 0, 0, False
            while j < len(f) and not (f[j] == ")" and depth == 0):
                if f[j] in "([":
                    depth += 1
                elif f[j] in ")]":
                    depth -= 1
                elif f[j] == "," and depth == 0:
                    args += 1
                elif not f[j].isspace():
                    top = True
                j += 1
            n, thisp = nparm[m.group(1)]
            if top and args + 1 == n - 1 and thisp:
                out.append(f[pos:m.end()] + ("c, " if has_this else "/*ECX: the caller's ecx*/ "))
                pos = m.end()
        funcs[k] = "".join(out) + f[pos:]
    protos = prototypes()
    print("\n".join(f'#include "{h}"' for h in sorted({PROTO_HDR[n] for n in USED if n in PROTO_HDR})))
    print(PROLOGUE)
    print(wrappers(USED, protos))
    print("\n".join(protos_local))
    print()
    print("\n\n".join(funcs))
    s = " ".join(sorted(todo))
    print(f"/* unported callees: {' '.join('0x' + t for t in s.split()) or 'none'} */")


if __name__ == "__main__":
    main()
