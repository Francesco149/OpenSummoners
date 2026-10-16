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
                p = re.sub(r"\s*\b([A-Za-z_]\w*)\s*\[\s*\d*\s*\]$", r" *\1", p)   # `int32_t box[4]` is a pointer
                if re.search(r"(\*|\w\s+)\s*[A-Za-z_]\w*$", p) and len(re.findall(r"[A-Za-z_]\w*", p)) > 1:
                    p = re.sub(r"[A-Za-z_]\w*$", "", p).strip()
                types.append(p or "int32_t")
            protos.setdefault(name, (ret, types))
            PROTO_HDR.setdefault(name, fn)
    return protos


_ASM = {}


def ecx_in(edition, va, size):
    """an IDA-__stdcall function that reads ecx before writing it (linear scan of its asm, past returns): its
    caller's ecx — always the caller's `this` — is an input ('read'), or goes on to a thiscall ('call')"""
    if edition not in _ASM:
        lines = open(os.path.join(IDA, edition, "all_ida.asm"), errors="replace").read().splitlines()
        idx = {}
        for i, l in enumerate(lines):
            m = re.match(r"^([0-9a-f]{8})\s", l)
            if m:
                idx.setdefault(int(m.group(1), 16), i)
        _ASM[edition] = (lines, idx)
    lines, idx = _ASM[edition]
    cc = ida_cc(edition)
    i, end = idx.get(va), va + size
    reg = lambda t: re.search(r"\b(ecx|cx|cl|ch)\b", t)
    while i is not None and i < len(lines):
        m = re.match(r"^([0-9a-f]{8})\s+(\w+)\s*(.*)$", lines[i]); i += 1
        if not m:
            continue
        if int(m.group(1), 16) >= end:
            break
        op, args = m.group(2), m.group(3).split(";")[0]
        dst, _, src = args.partition(",")
        if op == "call":
            cm = re.search(r"sub_([0-9A-F]+)|\w+_([0-9A-F]{6})\b", args)
            cv = int(cm.group(1) or cm.group(2), 16) if cm else None
            return "call" if cv in cc and cc[cv][0] == "thiscall" else None
        if op.startswith("j") or op in ("retn", "ret") or (op == "push" and re.match(r"\s*ecx\s*$", args)):
            continue   # (push ecx: MSVC's 4-byte local reservation)
        if op in ("xor", "sub") and re.match(r"\s*ecx,\s*ecx\s*$", args):
            return None
        if reg(src) or re.search(r"\[[^\]]*\becx\b", dst) or (op not in ("mov", "lea", "pop", "movzx", "movsx", "xor") and reg(dst)) \
                or (op == "xor" and reg(dst) and not re.match(r"\s*ecx,\s*ecx", args)):
            return "read"
        if reg(dst):
            return None
    return None


_CC = {}


def ida_cc(edition):
    """va -> (calling convention, IDA argument count) from functions_ida.tsv"""
    if edition not in _CC:
        out, pro = {}, {}
        for line in open(os.path.join(IDA, edition, "functions_ida.tsv")):
            f = line.rstrip("\n").split("\t")
            if len(f) > 5 and re.match(r"^[0-9a-f]{8}$", f[0]):
                out[int(f[0], 16)] = (f[4], int(f[5] or 0))
                pro[int(f[0], 16)] = f[8] if len(f) > 8 else ""
        _CC[edition] = (out, pro)
    return _CC[edition][0]


def retail_calls(edition):
    """the typed pointers to retail's own (unported) callees: RC_xxxxxx(...) = ((rt_xxxxxx)g_eng.retail(va))(...)"""
    cc = ida_cc(edition)
    pro = _CC[edition][1]
    out = []
    for va in sorted(RETAIL):
        conv, n = cc.get(va, ("cdecl", 0))
        ret = pro.get(va, "")
        rt = "double" if re.match(r"\s*(long double|double|float)\b", ret) else \
             "int64_t" if "__int64" in ret.split("(")[0] else "void" if re.match(r"\s*void\s", ret) else "intptr_t"
        if conv == "stdcall" and va in RETAIL_ECX:   # reads its caller's ecx: thiscall, ecx first
            conv, n = "thiscall", n + 1
        attr = {"thiscall": "__attribute__((thiscall)) ", "stdcall": "__attribute__((stdcall)) ",
                "fastcall": "__attribute__((fastcall)) "}.get(conv, "")
        args = ", ".join(["intptr_t"] * n) or "void"
        out.append(f"typedef {rt} ({attr}*rt_{va:06x})({args});   /* 0x{va:x} {conv}: unported, retail's */")
        out.append(f"#define RC_{va:06x} ((rt_{va:06x})g_eng.retail(0x{va:x}u))")
    return "\n".join(out)


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
CLSTYPE = {}      # class-pointer locals / params: name -> class (their fields' real offsets and widths)
DECLNAMES = set() # the function's declared locals
FRAMEVARS = []    # stack scalars: (ebp offset, type, name), laid out in frame_ like the arrays
_CLS_SIZE = {}


def class_layout(cls):
    """the struct IDA really has for a recovered class: oss_ida_classes.py declares the fields at their offsets
    (char gaps between, the extent as the size) and IDA lays the declaration out with natural alignment — a
    misaligned field moves (and everything after it), the size rounds up.  Replayed here:
    (size, {nominal field offset: (real offset, width)}, {nominal gap offset: real offset})"""
    if not _CLS_SIZE:
        for line in open(os.path.join(IDA, EDITION[0], "classes_ida.tsv")):
            f = line.rstrip("\n").split("\t")
            if len(f) > 1 and f[0].startswith("cls_"):
                fl = [(int(o, 16), int(w)) for o, w in (x.split(":") for x in f[6].split(",") if x)] if len(f) > 6 and f[6] else []
                _CLS_SIZE[f[0]] = (int(f[1]), fl)
    if cls not in _CLS_SIZE:
        return 0, {}, {}
    extent, fl = _CLS_SIZE[cls]
    decl, cur = [], 0   # (nominal offset, size, alignment, kind)
    for o, w in sorted(fl):
        if o < cur:
            continue
        if o > cur:
            decl.append((cur, o - cur, 1, "gap"))
        decl.append((o, w, {1: 1, 2: 2, 4: 4, 8: 8, 10: 8}[w], "field"))
        cur = o + w
    if extent > cur:
        decl.append((cur, extent - cur, 1, "gap"))
    if not decl:
        decl = [(0, 4, 1, "gap")]
    pos, sal, fields, gaps = 0, 1, {}, {}
    for o, n, al, kind in decl:
        pos = (pos + al - 1) // al * al
        if kind == "field":
            fields[o] = (pos, n)
        else:
            gaps[o] = pos
        pos += n
        sal = max(sal, al)
    return (pos + sal - 1) // sal * sal, fields, gaps


def class_size(cls):
    return class_layout(cls)[0]


def gap_size(cls, nom):
    """a gap's byte count (the declared one: char arrays are not padded)"""
    extent, fl = _CLS_SIZE.get(cls, (0, []))
    nxt = [o for o, _ in fl if o > nom]
    return (min(nxt) if nxt else extent) - nom


CLS = ["cls_4022A0"]   # the default `this` class (--cls)


def accx(f, base, off):
    """the F32/F16/S16/F8 accessor at base + off: bytes, unless base is a typed pointer (its C arithmetic)"""
    if base.strip() in PTRVARS:
        return f"{f}({base.strip()} + ({off}), 0)"
    return f"{f}({base}, {off})"


EDITION = ["enold"]
RETAIL = set()    # unported callees called through g_eng.retail (PORT-DEBT(engine-call-slots))
RETAIL_ECX = set()  # ... IDA-__stdcall ones that read their caller's ecx
ECXPASS = set()   # module functions that take their caller's ecx (IDA __stdcall reading ecx): a leading ecx_ param
FASTLOCAL = set() # module functions IDA typed __fastcall that never read edx: thiscall (a1 = this), edx dropped
CUR = [None]      # the port name of the function being converted
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
    if "__fastcall" in head:   # edx never read: thiscall, a1 = this (the caller's ecx)
        hm = re.match(r"(.*?\()(.*)(\)\s*)$", head)
        ps = [x.strip() for x in hm.group(2).split(",")]
        bt = "\n".join(body)
        if len(ps) >= 2:
            n1, n2 = re.match(r".*?\b(\w+)$", ps[0]).group(1), re.match(r".*?\b(\w+)$", ps[1]).group(1)
            if not re.search(r"\b%s\b" % n2, bt):
                t1 = re.match(r"(.*?)\s*\b\w+$", ps[0]).group(1)   # its own type: an `int` a1 adds bytes
                ps = [t1 + " this"] + ps[2:]
                head = hm.group(1).replace("__fastcall", "__thiscall") + ", ".join(ps) + hm.group(3)
                body = [re.sub(r"\b%s\b" % n1, "this", l) for l in body]
                FASTLOCAL.add(CUR[0])
    hm = re.search(r"\((.*)\)\s*$", head)
    for p in (hm.group(1).split(",") if hm else []):
        pm = re.match(r"(.*?)\s*\b(\w+)$", p.strip())
        if pm and "*" in pm.group(1) and "*" in map_type(pm.group(1)):
            PTRVARS.add(pm.group(2))
    out, decls = [], []
    DECLNAMES.clear()
    for ln in body:
        m = re.match(r"^  ([A-Za-z_][\w\s\*]*?)\s*\b([A-Za-z_]\w*)(\[\d+\])?; // ", ln)
        if m:
            DECLNAMES.add(m.group(2))   # a local never takes a same-named data symbol (IDA names one `Block`)
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
            if cm:
                CLSTYPE[m.group(2)] = cm.group(1)
            if cm and class_size(cm.group(1)):
                CLSIDX[m.group(2)] = (cm.group(1), class_size(cm.group(1)))
            if fo:   # a stack local lives in frame_ at its ebp offset too: IDA splits arrays / structs into neighbour
                     # locals (`&v34` handed to a rect writer, then v35..v37 read), and slots it reuses alias as in retail
                FRAMEVARS.append((int(fo.group(1), 16), mt, m.group(2)))
                continue
            decls.append(f"    {mt} {m.group(2)}{m.group(3) or ''};")
            continue
        out.append(ln)
    DECLS.extend(decls)
    s = "\n".join(out)
    W = {1: "F8", 2: "F16", 4: "F32", 8: "F64", 10: "F64"}
    tm = re.search(r"\((cls_\w+) \*this\b", head)
    tsize, tfields, tgaps = class_layout(tm.group(1) if tm else CLS[0])
    if not tm:
        tsize = 1   # no class: `this + n` adds bytes
    if not tfields:
        tfields = {o: (o, w) for o, w in widths.items()}

    def fld(base, nom, fields, gaps):
        off, w = fields.get(nom, (nom, 4))
        return f"{W.get(w, 'F32')}({base}, 0x{off:x})"
    if tsize > 1:   # `this + n` (no cast) moves by whole objects, as in the asm
        s = re.sub(r"(?<![\w)*])this \+ ([^;]+);", lambda m: f"this + {tsize} * ({m.group(1)});", s)
    s = re.sub(r"this->field_([0-9A-F]+)", lambda m: fld(this, int(m.group(1), 16), tfields, tgaps), s)
    s = re.sub(r"this->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"GAP({this}, 0x{tgaps.get(int(m.group(1), 16), int(m.group(1), 16)):x}, {m.group(2)})", s)
    s = re.sub(r"\bsizeof\(this->gap([0-9A-F]+)\)", lambda m: str(gap_size(tm.group(1) if tm else CLS[0], int(m.group(1), 16))), s)
    s = re.sub(r"this->gap([0-9A-F]+)\b(?!\s*\[)", lambda m: f"((uint8_t *)({this}) + 0x{tgaps.get(int(m.group(1), 16), int(m.group(1), 16)):x})", s)
    def vgap(m):   # any class-pointer local's gap byte, at its class's real gap offset
        cl = CLSTYPE.get(m.group(1))
        off = class_layout(cl)[2].get(int(m.group(2), 16), int(m.group(2), 16)) if cl else int(m.group(2), 16)
        return f"GAP({m.group(1)}, 0x{off:x}, {m.group(3)})"
    s = re.sub(r"\b(?!this\b)([A-Za-z_]\w*)->gap([0-9A-F]+)\[([^\]]+)\]", vgap, s)
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
        s = re.sub(r"\*\(%s \*\)&(\w+)\[([^\]]+)\](?!\s*\.)" % t, r"%s(&\1[\2], 0)" % f, s)
    s = re.sub(r"\*\((?:cls_\w+|_DWORD|int|_UNKNOWN|void) \*\*\)&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
    s = re.sub(r"&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"(W + 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
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
        _, fs0, gs0 = class_layout(cls)
        el = lambda i: f"({v} + ({i}) * 0x{n:x})"
        s = re.sub(r"(&?)\b%s\[([^\]]+)\]\.gap([0-9A-F]+)\[([^\]]+)\]" % v,   # an element's gap byte
                   lambda m: f"{m.group(1)}GAP({el(m.group(2))}, 0x{gs0.get(int(m.group(3), 16), int(m.group(3), 16)):x}, {m.group(4)})", s)
        s = re.sub(r"\b%s\[([^\]]+)\]\.gap([0-9A-F]+)\b(?!\s*\[)" % v,
                   lambda m: f"((uint8_t *){el(m.group(1))} + 0x{gs0.get(int(m.group(2), 16), int(m.group(2), 16)):x})", s)
        s = re.sub(r"(?:\(char \*\))?&%s\[([^\]]+)\]" % v, lambda m: f"({v} + ({m.group(1)}) * 0x{n:x})", s)
        _, fs, gs = class_layout(cls)
        s = re.sub(r"\b%s\[([^\]]+)\]\.field_([0-9A-F]+)" % v,
                   lambda m: f"{ {1: 'F8', 2: 'F16', 4: 'F32', 8: 'F64'}.get(fs.get(int(m.group(2), 16), (0, 4))[1], 'F32')}"
                             f"({v} + ({m.group(1)}) * 0x{n:x}, 0x{fs.get(int(m.group(2), 16), (int(m.group(2), 16), 4))[0]:x})", s)
        if n in (1, 2, 4, 8):   # an element of a class this small: a scalar load
            s = re.sub(r"(?<![\w.&])%s\[([^\]]+)\](?!\s*\.)" % v,
                       lambda m: f"{ {1: 'F8', 2: 'F16', 4: 'F32', 8: 'F64'}[n]}({v}, ({m.group(1)}) * {n})", s)
    for v in INTVARS:   # an integer local used as a pointer by IDA's cast-free deref: a dword load
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b(?!\s*[\[(])|(?<=[=(,!])\*%s\b(?!\s*[\[(])" % (v, v), f"F32({v}, 0)", s)
    for v in CLSPTR:   # a class pointer kept as an integer: its unary deref is a dword load
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b|(?<=[=(,!])\*%s\b" % (v, v), f"F32({v}, 0)", s)
    def vfld(m):
        cl = CLSTYPE.get(m.group(1))
        if cl:
            _, fs, gs = class_layout(cl)
            return fld(m.group(1), int(m.group(2), 16), fs, gs)
        return f"F32({m.group(1)}, 0x{int(m.group(2), 16):x})"
    s = re.sub(r"\b(\w+)->field_([0-9A-F]+)", vfld, s)
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
                or re.match(r"^a\d+$", name) or re.search(r"\b%s\b" % re.escape(name), head) or name in DECLNAMES:
            continue
        s = re.sub(r"&%s\b" % re.escape(name), f"D(0x{va:x})", s)
        if name.startswith(("dword_", "word_", "byte_")):
            acc = {"dword_": "F32", "word_": "F16", "byte_": "F8"}[name.split("_")[0] + "_"]
            s = re.sub(r"(?<![\w.])%s\b(?!\s*\[)" % re.escape(name), f"{acc}(D(0x{va:x}), 0)", s)
        elif re.match(r"^(a[A-Z0-9]\w*|trailer|stru_\w+|unk_\w+|off_\w+|asc_\w+)$", name):
            s = re.sub(r"(?<![\w.&])%s\b(?!\s*[\[.])" % re.escape(name), f"D(0x{va:x})", s)
    s = re.sub(r"&(?:unk|byte|dword|word|asc|off|stru|o_cls_\w+?)_(?!8A6B60\b)([0-9A-F]{6})\b", lambda m: f"D(0x{int(m.group(1), 16):x})", s)
    for kind, f in (("dword", "F32"), ("word", "F16"), ("byte", "F8")):   # a global's value (not in data_xrefs)
        s = re.sub(r"(?<![\w&.])%s_([0-9A-F]{6})\b(?!\s*\[)" % kind, lambda m, f=f: f"{f}(D(0x{int(m.group(1), 16):x}), 0)", s)

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
        RETAIL.add(va)   # unported: a typed call of retail's own function (g_eng.retail: rhost = identity)
        return f"RC_{va:06x}("
    s = re.sub(r"\b(?:sub|[A-Za-z_]+?)_([0-9A-F]{6})\(", lambda m: call(m) if re.match(r"(sub|\w+)_[0-9A-F]{6}\(", m.group(0)) else m.group(0), s)
    s = re.sub(r"\(cls_\w+ \*\*+\)", "(intptr_t *)", s)   # a pointer to class pointers: its deref is a dword load
    s = re.sub(r"\(cls_\w+ \*\)", "", s)
    # a pointer cast under arithmetic scales it (`(_BYTE *)v + 2` = 2 bytes): kept, as the C type
    CT = {"_DWORD": "int32_t", "_WORD": "uint16_t", "_BYTE": "uint8_t", "int": "int32_t", "char": "uint8_t",
          "unsigned __int16": "uint16_t", "__int16": "int16_t", "unsigned int": "uint32_t", "void": "uint8_t"}
    s = re.sub(r"\((?:const )?(_DWORD|_WORD|_BYTE|int|char|unsigned __int16|__int16|unsigned int) (\*\*?)\)(?=(?:\w+|\((?:[^()]|\([^()]*\))*\)) [-+] )",
               lambda m: f"({CT[m.group(1)] if m.group(2) == '*' else 'int32_t'} *)", s)
    # every other IDA pointer cast is kept (_DWORD / _WORD / _BYTE are typedefs of the C widths): `**(_WORD **)x`
    # loads and pointer arithmetic then scale exactly as in the asm
    s = re.sub(r"\*&SYSB\((\d+)\)", r"SYS32(\1)", s)
    s = s.replace("operator new(", "(intptr_t)g_eng.op_new(").replace("operator delete(", "g_eng.op_delete((void *)")
    s = re.sub(r"(?<![\w.])exit\(", "g_eng.h5bf5db(", s)                    # the CRT exit 0x5bf5db (FLIRT `exit`)
    s = re.sub(r"\bstru_8A6B60\.zdm_ptr->name\b", "(SYS32(0x2884) + 0x2c)", s)  # the manager's track name (inline)
    s = re.sub(r"\bstru_8A6B60\.zdm_ptr\b", "SYS32(0x2884)", s)              # the system's music manager
    s = re.sub(r"&stru_8A9530\b(?!\.)", "((intptr_t)g_eng.log)", s)          # the error log object
    wsz, _, wgaps = class_layout("cls_40BE00")
    s = re.sub(r"(&?)(?:p_cls_40BE00|\bW)\[(\d+)\]\.gap([0-9A-F]+)\[([^\]]+)\]",
               lambda m: f"{m.group(1)}GAP(W, {int(m.group(2)) * wsz} + 0x{wgaps.get(int(m.group(3), 16), int(m.group(3), 16)):x}, {m.group(4)})", s)
    s = re.sub(r"&stru_8A6B60\b(?!\.)", "((intptr_t)g_eng.sys)", s)
    s = re.sub(r"&stru_8A6B60\._pad000\[([^\]]+)\]", r"((uint8_t *)g_eng.sys + (\1))", s)
    s = re.sub(r"\*\(_DWORD \*\)&W->gap([0-9A-F]+)\[(\d+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + {m.group(2)})", s)
    s = re.sub(r"W->field_([0-9A-F]+)", lambda m: f"F32(W, 0x{int(m.group(1), 16):x})", s)
    fa = r"(F32|F16)\(((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?), ((?:[^()]|\([^()]*\))+?)\)"
    for part, (acc, add) in (("LOWORD", ("F16", 0)), ("HIWORD", ("F16", 2)), ("LOBYTE", ("F8", 0)), ("BYTE1", ("F8", 1)),
                             ("BYTE2", ("F8", 2)), ("HIBYTE", ("F8", 3))):   # a part of a field: the narrower field
        s = re.sub(r"\b%s\(%s\)" % (part, fa), lambda m, acc=acc, add=add: f"{acc}({m.group(2)}, {m.group(3)}{f' + {add}' if add else ''})", s)
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
            elif ch in ";," and depth == 0:
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
    for a_, b_ in (("unsigned __int16", "uint16_t"), ("unsigned __int8", "uint8_t"), ("unsigned __int32", "uint32_t"),
                   ("__int16", "int16_t"), ("__int8", "int8_t"), ("__int32", "int32_t")):   # MSVC's, for any host
        s = re.sub(r"\b%s\b" % re.escape(a_), b_, s)
    for _, t, n, cnt in ARRAYS:   # a stack array lives in frame_ through a pointer: its sizeof is the array's
        s = re.sub(r"\bsizeof\(%s\)" % re.escape(n), f"({cnt} * sizeof({t}))", s)
    # retail's log 0x406440 inlined (OutputDebugStringA x 3 + the dated line appended to the log file)
    s = re.sub(r"OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\(OutputString\);\s*"
               r"(\w+) = fopen\(stru_8A9530\.path, Mode\);\s*if \( \3 != 0 \)\s*\{\s*GetLocalTime\(&SystemTime\);\s*"
               r"fprintf\(.*?\);\s*fclose\(\3\);\s*\}",
               r"g_eng.log_fail_406440(g_eng.log, (const char *)(\1), (const char *)(\2));", s, flags=re.S)
    s = re.sub(r"(\w+) = GetLastError\(\);.*?g_eng\.log_fail_406440\(\(const char \*\)(?:D\(0x8a9530\)|\(\(intptr_t\)g_eng\.log\)), (D\(0x[0-9a-f]+\)), D\(0x8a9b6c\)\);",
               r"g_eng.h426110(g_eng.log, (intptr_t)\2, 1, (intptr_t)D(0x8a9b6c));   /* the error box 0x426110, inlined */", s, flags=re.S)
    for _, t, n in FRAMEVARS:   # (last: the patterns above see IDA's names)
        s = re.sub(r"(?<![\w.>])%s\b" % re.escape(n), f"(*{n}__f)", s)
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
typedef int32_t _DWORD;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))
"""


def signature(head, name):
    """IDA header -> (C signature of the port function `name`, the param prologue)"""
    m = re.match(r"\s*(.*?)\s*(?:__thiscall|__stdcall|__fastcall|__cdecl)?\s*\bsub_[0-9A-F]+\((.*)\)\s*$", head)
    ret, params = m.group(1).replace("__thiscall", "").replace("__stdcall", "").strip(), m.group(2)
    ret = ("void" if ret == "void" else "double" if ret in ("long double", "double", "float")   # st0
           else "int64_t" if "__int64" in ret else "int32_t")                                    # edx:eax
    out, pro = ([], []) if name not in ECXPASS else (["intptr_t ecx_"], [])
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
    CLS[0] = a.cls
    for spec in a.va:
        va_s, _, pname = spec.partition(":")
        if pname:
            LOCAL[int(va_s, 16)] = pname
    protos_local, fnames = [], []
    ccs = ida_cc(a.edition)
    sizes = {}
    for line in open(os.path.join(IDA, a.edition, "functions_ida.tsv")):
        f = line.split("\t")
        if re.match(r"^[0-9a-f]{8}$", f[0]):
            sizes[int(f[0], 16)] = int(f[1])
    for va, pname in LOCAL.items():   # IDA-__stdcall module functions that take their caller's ecx
        if ccs.get(va, ("",))[0] == "stdcall" and ecx_in(a.edition, va, sizes.get(va, 0)):
            ECXPASS.add(pname)
    for spec in a.va:
        va_s, _, pname = spec.partition(":")
        va = int(va_s, 16)
        fnames.append(pname)
        CUR[0] = pname
        DECLS.clear(); ARRAYS.clear(); CLSPTR.clear(); INTVARS.clear(); PTRVARS.clear(); CLSIDX.clear(); CLSTYPE.clear()
        FRAMEVARS.clear()
        text = extract(a.edition, va)
        if pname in ECXPASS and "__stdcall" not in text.split("{")[0]:
            ECXPASS.discard(pname)   # the decompile's header is the authority (thiscall: its ecx is `this`)
        head, s = convert(text, names, widths, a.this)
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
        if ARRAYS or FRAMEVARS:
            top = max([o for o, *_ in ARRAYS] + [o for o, *_ in FRAMEVARS])
            out.append(f"    _Alignas(8) uint8_t frame_[0x{top:x} + 0x10];   /* the stack locals at their ebp offsets */")
            out.append("    memset(frame_, 0, sizeof frame_);")
            for o, t, n, cnt in sorted(ARRAYS, reverse=True):
                out.append(f"    {t} *{n} = ({t} *)(frame_ + 0x{top - o:x});   /* [{cnt}] at ebp-0x{o:x} */")
            for o, t, n in sorted(FRAMEVARS, reverse=True):
                out.append(f"    {t} *const {n}__f = ({t} *)(frame_ + 0x{top - o:x});   /* ebp-0x{o:x} */")
        out.append(s)
        out.append("}")
        funcs.append("\n".join(out))
        todo |= set(re.findall(r"RC_([0-9a-f]{6})\(", s))
    # the call sites IDA got wrong about ecx / edx (one pass over every call to a module function or a port one):
    # - a thiscall callee whose `this` IDA left out (its prototype refined after the caller was decompiled, or the
    #   caller passes its own ecx on): the caller's `this` (or its ecx_) is passed
    # - an IDA-__fastcall callee that is really thiscall + stack arguments (edx never read): IDA's second argument
    #   is the stale edx — dropped
    protos = prototypes()
    cc = ida_cc(a.edition)
    va_of = {n: v for v, n in names.items()}
    loc = {}
    for sig in protos_local:
        m = re.match(r"\w+ (\w+)\((.*)\);$", sig)
        loc[m.group(1)] = (0 if m.group(2) == "void" else m.group(2).count(",") + 1,
                           m.group(2).startswith(("wl_char *self", "intptr_t ecx_")), m.group(1) in FASTLOCAL)
    wnames = [n for n in USED if n in protos and n in va_of]
    pat = r"(?<![\w.])(?:(%s)|W_(%s)|RC_([0-9a-f]{6}))\(" % ("|".join(map(re.escape, loc)) or "(?!x)x", "|".join(map(re.escape, wnames)) or "(?!x)x")
    for k, f in enumerate(funcs):
        has_this = "intptr_t c = (intptr_t)self;" in f
        tex = "c" if has_this else ("ecx_" if fnames[k] in ECXPASS else "/*ECX: the caller's ecx*/ 0")
        out, pos = [], 0
        for m in re.finditer(pat, f):
            if m.start() < pos or re.match(r"\s*(int32_t|int64_t|double|void) $", f[max(0, m.start() - 8):m.start()]):
                continue
            args, j, depth, start = [], m.end(), 0, m.end()
            while j < len(f) and not (f[j] == ")" and depth == 0):
                if f[j] in "([":
                    depth += 1
                elif f[j] in ")]":
                    depth -= 1
                elif f[j] == "," and depth == 0:
                    args.append((start, j)); start = j + 1
                j += 1
            args.append((start, j))
            if len(args) == 1 and not f[args[0][0]:args[0][1]].strip():
                args = []
            if m.group(1):
                n, wants_this, fast = loc[m.group(1)]
                vcc = "fastcall" if fast else ("thiscall" if wants_this else "")
            elif m.group(3):   # a retail callee: its IDA arity counts `this` / ecx, edx as IDA's call does
                rv = int(m.group(3), 16)
                vcc, n = cc.get(rv, ("", 0))
                if vcc == "fastcall":
                    vcc = ""   # retail's own fastcall gets ecx, edx as IDA passes them
                if vcc == "stdcall" and ecx_in(a.edition, rv, sizes.get(rv, 0)):
                    RETAIL_ECX.add(rv)   # declared thiscall with ecx first: the caller's this goes in
                    vcc, n = "thiscall", n + 1
            else:
                wv = va_of[m.group(2)]
                n, vcc = len(protos[m.group(2)][1]), cc.get(wv, ("",))[0]
                if vcc == "stdcall" and len(args) == n - 1 and ecx_in(a.edition, wv, sizes.get(wv, 0)):
                    vcc = "thiscall"   # the port takes the ecx this IDA-__stdcall callee reads as its first parameter
            if vcc == "fastcall" and len(args) == n + 1 and len(args) >= 2:
                out.append(f[pos:args[0][1]] + ", /*edx*/")   # skip IDA's edx argument
                pos = args[1][1] + 1
            elif vcc == "thiscall" and len(args) == n - 1:
                out.append(f[pos:m.end()] + tex + (", " if args else ""))
                pos = m.end()
        funcs[k] = "".join(out) + f[pos:]
    print("\n".join(f'#include "{h}"' for h in sorted({PROTO_HDR[n] for n in USED if n in PROTO_HDR})))
    print(PROLOGUE)
    print(wrappers(USED, protos))
    print(retail_calls(a.edition))
    print("\n".join(protos_local))
    print()
    print("\n\n".join(funcs))
    s = " ".join(sorted(todo))
    print(f"/* unported callees: {' '.join('0x' + t for t in s.split()) or 'none'} */")


if __name__ == "__main__":
    main()
