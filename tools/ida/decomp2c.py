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
import json
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(os.path.dirname(HERE))
IDA = os.environ.get("IDA_DIR", "/mnt/ephemeral/opensummoners/decompiled-ida")


OVERRIDES = {0x4766a0: "view_box_visible", 0x490b90: "view_project"}   # ported functions whose header line is not a `FUN_ name` table
NOBIND = {0x5b8fc0, 0x43d1d0}   # the Flip and the camera easer: always through RC_ (rhost's stubs step the frame count /
                                # virtual clock there; a direct call to the ported Flip ran a loading loop off the clock)


def rhost_modules():
    """the src/ modules rhost links (tools/rhost/build.sh's list): the retail-layout port.  A port function outside
    them (the older port-side renderer / zdd / bitmap code) is not callable from retail-layout code"""
    t = open(os.path.join(REPO, "tools", "rhost", "build.sh")).read()
    m = re.search(r"^for f in ([\w ]+); do", t, re.M)
    return set(m.group(1).split()) if m else None


def port_names():
    names = dict(OVERRIDES)
    protos = prototypes()
    mods = rhost_modules()
    for fn in sorted(os.listdir(os.path.join(REPO, "src"))):
        if not fn.endswith(".h") or (mods is not None and fn[:-2] not in mods):
            continue
        for m in re.finditer(r"FUN_00([0-9a-f]{6})\s+([A-Za-z_]\w*)", open(os.path.join(REPO, "src", fn), errors="replace").read()):
            if m.group(2) in protos:   # a real port function (prose like "FUN_0044e680 call (...)" is not)
                names.setdefault(int(m.group(1), 16), m.group(2))
    # two port functions may claim a VA (an older port-side twin: glyph_cell_layout on menu_ctrl vs grid_cell_text on
    # the retail layout, both 0x40fa00): rhost's hook table names the verified retail-layout one
    hp = os.path.join(REPO, "tools", "rhost", "rh_hooks.c")
    if os.path.exists(hp):
        for m in re.finditer(r'X\(\w+, "(\w+)", 0x([0-9a-f]+)u\)', open(hp, errors="replace").read()):
            va, n = int(m.group(2), 16), m.group(1)
            if va in names and names[va] != n and n in protos:
                names[va] = n
    for va in NOBIND:
        names.pop(va, None)
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
        if (op == "or" and re.match(r"\s*ecx,\s*0FFFFFFFFh\s*$", args)) or (op == "and" and re.match(r"\s*ecx,\s*0\s*$", args)):
            return None   # a constant write (`or ecx, -1`: the repne scasb strlen idiom), not a read
        if reg(src) or re.search(r"\[[^\]]*\becx\b", dst) or (op not in ("mov", "lea", "pop", "movzx", "movsx", "xor") and reg(dst)) \
                or (op == "xor" and reg(dst) and not re.match(r"\s*ecx,\s*ecx", args)):
            return "read"
        if reg(dst):
            return None
    return None


_SIZES = {}
FMAPS = []   # per module function: (va, ARRAYS, FRAMEVARS, FRAMECLS) — the stack locals by ebp offset


def asm_calls(edition, caller, callee):
    """the line indices of `call <callee>` in the caller's asm, in address order"""
    if edition not in _ASM:
        ecx_in(edition, caller, 0)   # (builds the asm index)
    lines, idx = _ASM[edition]
    i = idx.get(caller)
    if i is None:
        return [], lines
    out = []
    if edition not in _SIZES:
        _SIZES[edition] = {int(x.split("\t")[0], 16): int(x.split("\t")[1]) for x in open(os.path.join(IDA, edition, "functions_ida.tsv"))
                           if re.match(r"^[0-9a-f]{8}\t\d+", x)}
    end = caller + _SIZES[edition].get(caller, 0)
    while i < len(lines):
        m = re.match(r"^([0-9a-f]{8})\s+(\w+)\s*(.*)$", lines[i])
        if m and int(m.group(1), 16) >= end:
            break
        if m and m.group(2) == "call":
            cm = re.match(r"\s*(?:sub_([0-9A-F]+)|[A-Za-z_]\w*?_([0-9A-F]{6}))\b", m.group(3))
            if cm and int(cm.group(1) or cm.group(2), 16) == callee:
                out.append(i)
        i += 1
    return out, lines


def asm_named_offset_only(edition, va, name):
    """a plain-named global (`String`, a runtime-filled buffer) this function's asm only takes the address of"""
    if va is None:
        return False
    if edition not in _ASM:
        ecx_in(edition, va, 0)
    if edition not in _SIZES:
        _SIZES[edition] = {int(x.split("\t")[0], 16): int(x.split("\t")[1]) for x in open(os.path.join(IDA, edition, "functions_ida.tsv"))
                           if re.match(r"^[0-9a-f]{8}\t\d+", x)}
    lines, idx = _ASM[edition]
    i, end = idx.get(va), va + _SIZES[edition].get(va, 0)
    seen = addr = 0
    while i is not None and i < len(lines):
        m = re.match(r"^([0-9a-f]{8})\s+\w+\s*(.*)$", lines[i]); i += 1
        if not m:
            continue
        if int(m.group(1), 16) >= end:
            break
        for mm in re.finditer(r"(\boffset\s+)?\b%s\b" % re.escape(name), m.group(2).split(";")[0]):
            if mm.group(1):
                addr += 1
            else:
                seen += 1
    return addr > 0 and seen == 0


def asm_offset_only(edition, va, names):
    """the globals among names this function's asm only ever takes the address of (`push offset byte_8A47F8`): a C
    array IDA prints bare as a pointer argument (0x5b6650's message strings) — its bare name is its address"""
    if va is None or not names:
        return set()
    if edition not in _ASM:
        ecx_in(edition, va, 0)   # (builds the asm index)
    if edition not in _SIZES:
        _SIZES[edition] = {int(x.split("\t")[0], 16): int(x.split("\t")[1]) for x in open(os.path.join(IDA, edition, "functions_ida.tsv"))
                           if re.match(r"^[0-9a-f]{8}\t\d+", x)}
    lines, idx = _ASM[edition]
    i, end = idx.get(va), va + _SIZES[edition].get(va, 0)
    seen, addr = set(), set()
    while i is not None and i < len(lines):
        m = re.match(r"^([0-9a-f]{8})\s+\w+\s*(.*)$", lines[i]); i += 1
        if not m:
            continue
        if int(m.group(1), 16) >= end:
            break
        ops = m.group(2).split(";")[0]
        for n in names:   # (by the VA: the asm may label it differently — `offset unk_628670` for C's dword_628670)
            for mm in re.finditer(r"(\boffset\s+)?\b(?:dword|word|byte|unk|off|stru|asc)_%s\b" % n.split("_")[-1], ops):
                (addr if mm.group(1) else seen).add(n)
    return addr - seen


def frame_expr(fmap, off, addr):
    """the C expression for the stack slot at ebp-off (its address when addr, else its dword value), or None"""
    _, arrays, fvars, fcls = fmap
    for o, t, n, cnt in arrays:
        esz = {"uint8_t": 1, "char": 1, "int8_t": 1, "uint16_t": 2, "int16_t": 2}.get(t, 4)
        if o >= off > o - cnt * esz:
            if addr:
                return f"(intptr_t){n}" if o == off else f"((intptr_t)(uint8_t *){n} + 0x{o - off:x})"
            return f"F32({n}, 0x{o - off:x})"
    for o, t, n in fvars:
        size = class_size(fcls[n]) if n in fcls else 4
        if o >= off > o - max(size or 4, 4):
            if addr:
                return f"((intptr_t){n}__f + 0x{o - off:x})" if o != off else f"(intptr_t){n}__f"
            return f"(intptr_t)(*{n}__f)" if o == off else f"F32({n}__f, 0x{o - off:x})"
    return None


def asm_ecx(edition, fmap, callee, occ, c_count):
    """what the caller loads into ecx before its occ-th call of callee (when its asm has as many calls as the C):
    a frame local's address / value, the system object, a global — else None"""
    calls, lines = asm_calls(edition, fmap[0], callee)
    if len(calls) != c_count or occ >= len(calls):
        return None
    targets = set()
    for ln in lines[calls[0] - 400 if calls[0] > 400 else 0:calls[-1]]:
        jm = re.match(r"^[0-9a-f]{8}\s+j\w+\s+(?:short\s+)?loc_([0-9A-F]+)", ln)
        if jm:
            targets.add(int(jm.group(1), 16))
    i = calls[occ] - 1
    for _ in range(24):
        m = re.match(r"^([0-9a-f]{8})\s+(\w+)\s*(.*?)(?:;.*)?$", lines[i])
        if not m:
            return None
        va, op, args = int(m.group(1), 16), m.group(2), m.group(3).strip()
        if op == "call" or op.startswith("j") or op in ("retn", "ret"):
            return None
        if re.match(r"(ecx|cx|cl|ch)\b", args) and op not in ("push", "cmp", "test"):
            fm = re.match(r"ecx,\s*\[esp\+[0-9A-F]+h\+var_([0-9A-F]+)(?:\.\w+)?(?:\+([0-9A-F]+)h?)?\]$", args)
            if fm and op in ("lea", "mov"):
                off = int(fm.group(1), 16) - (int(fm.group(2), 16) if fm.group(2) else 0)
                return frame_expr(fmap, off, op == "lea")
            if op == "mov" and re.match(r"ecx,\s*offset stru_8A6B60$", args):
                return "(intptr_t)g_eng.sys"
            om = re.match(r"ecx,\s*offset \w+?_([0-9A-F]{6})$", args)
            if op == "mov" and om:
                return f"D(0x{int(om.group(1), 16):x})"
            return None
        if va in targets:   # another path joins here: its ecx is not this block's
            return None
        i -= 1
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


IAT = {"enold": (0x5cc000, 0x5cc284)}   # the import address table (PE data directory 12)
API_USED = {}   # Win32 import -> argument count (from its first call site)
ZERO_LOCALS = [False]   # --zero-locals: register locals start at 0 (IDA's LOWORD(v) = x on an unset v)
TYPED = [False]  # --typed: the knowledge pack's structs (src/ headers: paint_ctx, bitmap_session ...) stay C types
KBH = set()      # ... and the headers that define the ones used


def kb_globals():
    """va -> (name, C type) of the knowledge pack's typed globals (gen_knowledge.py GLOBALS)"""
    if not hasattr(kb_globals, "c"):
        kb_globals.c = {}
        p = os.path.join(IDA, "knowledge.%s.json" % EDITION[0])
        if os.path.exists(p):
            d = json.load(open(p))
            for va, t in d.get("gtypes", {}).items():
                n = d.get("names", {}).get(va)
                if n:
                    kb_globals.c[int(va, 16)] = (n, t)
    return kb_globals.c


def kb_types():
    """typedef'd struct name -> the src/ header defining it (the knowledge pack's types come from these headers)"""
    if not hasattr(kb_types, "c"):
        kb_types.c = {}
        for fn in sorted(os.listdir(os.path.join(REPO, "src"))):
            if fn.endswith(".h") and fn not in ("wnd_proc.h",):   # (its shells clash with input.h: paint_ctx stays a word)
                for m in re.finditer(r"^\}\s*(\w+)\s*;", open(os.path.join(REPO, "src", fn), errors="replace").read(), re.M):
                    kb_types.c.setdefault(m.group(1), fn)
    return kb_types.c
API_VARARG = {"wsprintfA"}


def imports(edition):
    """the exe's imports by name -> IAT slot VA (data_xrefs.tsv's names inside the IAT)"""
    lo, hi = IAT[edition]
    out = {}
    for line in open(os.path.join(IDA, edition, "data_xrefs.tsv")):
        f = line.rstrip("\n").split("\t")
        if len(f) > 1 and re.match(r"^[0-9a-f]{8}$", f[0]) and lo <= int(f[0], 16) < hi:
            n = f[1].replace("__imp_", "")
            if re.match(r"^[A-Za-z_]\w*$", n) and not re.match(r"(unk|dword|off|byte|word)_", n):
                out[n] = int(f[0], 16)
    return out


def call_args(f, j):
    """the top-level argument strings of the call whose '(' ends at j; returns (args, index of its ')')"""
    args, depth, start = [], 0, j
    while j < len(f):
        ch = f[j]
        if f.startswith("/*", j):   # a comment: its quotes / parens are not syntax
            e = f.find("*/", j + 2)
            j = (e + 2) if e >= 0 else len(f)
            continue
        if ch in "\"'":   # a string / char literal: commas and parens inside are not syntax
            q, j = ch, j + 1
            while j < len(f) and f[j] != q:
                j += 2 if f[j] == "\\" else 1
        elif ch in "([":
            depth += 1
        elif ch in ")]":
            if depth == 0:
                break
            depth -= 1
        elif ch == "," and depth == 0:
            args.append(f[start:j]); start = j + 1
        j += 1
    if f[start:j].strip():
        args.append(f[start:j])
    return args, j


_EXE = {}
EXE_PATHS = {"enold": "vendor/unpacked/sotes.unpacked.exe", "ense-en": "vendor/unpacked/sotes_en.unpacked.exe"}


def exe_read(va, n, edition=None):
    """n bytes of the retail image at va (the PE's section table), or b"" """
    ed = edition or EDITION[0]
    if ed not in _EXE:
        import struct
        try:
            d = open(os.path.join(REPO, EXE_PATHS.get(ed, "")), "rb").read()
        except OSError:
            d = b""
        secs = []
        if d[:2] == b"MZ":
            pe = struct.unpack_from("<I", d, 0x3c)[0]
            ns, osz = struct.unpack_from("<H", d, pe + 6)[0], struct.unpack_from("<H", d, pe + 20)[0]
            base = struct.unpack_from("<I", d, pe + 24 + 28)[0]
            for i in range(ns):
                o = pe + 24 + osz + 40 * i
                vs, rva, rs, ro = struct.unpack_from("<IIII", d, o + 8)
                secs.append((base + rva, max(vs, rs), ro, rs))
        _EXE[ed] = (d, secs)
    d, secs = _EXE[ed]
    for sva, sz, ro, rs in secs:
        if sva <= va < sva + sz:
            off = va - sva
            return d[ro + off:ro + min(off + n, rs)] if off < rs else b"\0" * n
    return b""


_FREFS = {}


def func_data_refs(edition, fva):
    """the data VAs the function at fva references (data_xrefs.tsv)"""
    if edition not in _FREFS:
        m = {}
        for line in open(os.path.join(IDA, edition, "data_xrefs.tsv")):
            f = line.rstrip("\n").split("\t")
            if len(f) > 2 and re.match(r"^[0-9a-f]{8}$", f[0]) and re.match(r"^[0-9a-f]{8}$", f[2]):
                m.setdefault(int(f[2], 16), set()).add(int(f[0], 16))
        _FREFS[edition] = m
    return _FREFS[edition].get(fva, set())


def is_cstr(va):
    """the image holds a non-empty NUL-terminated text string at va (ASCII / SJIS bytes)"""
    b = exe_read(va, 512)
    z = b.find(b"\0")
    return z > 0 and all(c >= 0x20 or c in (9, 10, 13) for c in b[:z])


_KBN = []


def kb_names():
    if not _KBN:
        _KBN.append({n for n, _ in kb_globals().values()})
    return _KBN[0]


# CRT calls with an effect outside the program (the clock, files, the environment): retail's own CRT (FLIRT `_x`), so
# rhost's virtual clock / file overlay stay under them — the host CRT would bypass both
CRT_REDIRECT = ("time", "localtime", "gmtime", "mktime", "strftime", "clock", "fopen", "fclose", "fprintf", "fread",
                "fwrite", "fseek", "ftell", "fgets", "fputs", "fflush", "remove", "rename", "getenv", "_findfirst",
                "_findnext", "_findclose", "_mkdir", "_access", "fscanf", "fgetc", "ungetc", "setlocale", "atexit",
                "_setmbcp", "_onexit", "srand")
CRT_USED = {}


def crt_calls(f, edition):
    flirt = {}
    for line in open(os.path.join(IDA, edition, "functions_ida.tsv")):
        x = line.split("\t")
        if len(x) > 3 and x[3] == "1":
            flirt.setdefault(x[2], int(x[0], 16))
    def rep(m):
        n = m.group(1)
        va = flirt.get("_" + n.lstrip("_")) or flirt.get(n)
        if n in CRT_REDIRECT and va:
            CRT_USED[n] = va
            return f"CRT_{n}("
        return m.group(0)
    return re.sub(r"(?<![\w.>])([A-Za-z_]\w*)\(", rep, f)


def crt_decls():
    return "\n".join(f"#define CRT_{n} ((intptr_t (*)())g_eng.retail(0x{va:x}u))   /* retail's CRT {n} (cdecl) */"
                      for n, va in sorted(CRT_USED.items()))


def strip_seh(body):
    """MSVC's SEH / C++ EH frame (the TEB's ExceptionList link, the handler stub, the __alloca_probe result): only the
    unwinder reads them — dropped"""
    out = []
    for ln in body:
        if re.match(r"^\s*ExceptionList = NtCurrentTeb\(\)->NtTib\.ExceptionList;\s*$", ln) or \
                re.match(r"^\s*NtCurrentTeb\(\)->NtTib\.ExceptionList = .*;\s*$", ln) or \
                re.match(r"^\s*[\w\[\]\.]+ = &loc_[0-9A-F]+;\s*$", ln) or \
                re.match(r"^\s*[\w\[\]\.]+ = ExceptionList;\s*$", ln):
            continue
        am = re.match(r"^\s*(\w+) = alloca\(\d+\);\s*$", ln)
        if am and sum(len(re.findall(r"\b%s\b" % am.group(1), x)) for x in body) == 2:   # (its decl + this line)
            continue
        out.append(ln)
    return out


def api_calls(f, imps):
    """a Win32 call -> a typed call through retail's own import slot (g_eng.api: rhost's GDI / clock overrides
    stay in the path); every argument passed as a word, as retail pushes it"""
    out, pos = [], 0
    for m in re.finditer(r"(?<![\w.>])(?:::)?([A-Za-z_]\w*)\(", f):
        n = m.group(1)
        if n not in imps or m.start() < pos:
            continue
        args, j = call_args(f, m.end())
        API_USED.setdefault(n, len(args))
        out.append(f[pos:m.start()] + f"API_{n}(" + ", ".join(f"(intptr_t)({api_calls(x.strip(), imps)})" for x in args) + ")")
        pos = j + 1
    out.append(f[pos:])
    return "".join(out)


def word_args(f):
    """--typed: every argument of an RC_ / W_ call passed as a word (a typed struct pointer then passes as retail
    pushes it)"""
    out, pos = [], 0
    for m in re.finditer(r"(?<![\w.])((?:RC_[0-9a-f]{6}|W_\w+)\()", f):
        if m.start() < pos:
            continue
        args, j = call_args(f, m.end())
        out.append(f[pos:m.start()] + m.group(1) + ", ".join(f"(intptr_t)({word_args(x.strip())})" for x in args) + ")")
        pos = j + 1
    out.append(f[pos:])
    return "".join(out)


# Win32 calls returning an UNSIGNED word (DWORD / UINT / ATOM / COLORREF / SIZE_T): retail compares their results
# unsigned (0x5b1030's `last - GetTickCount() < 5` is a `jnb`) — a signed return turned a wrapped difference true
API_UNSIGNED = {"GetTickCount", "timeGetTime", "GetLastError", "GetModuleFileNameA", "GetFileVersionInfoSizeA",
                "FormatMessageA", "GetTempPathA", "GetSystemDirectoryA", "GetWindowsDirectoryA", "GetCurrentDirectoryA",
                "GetFileSize", "SetFilePointer", "GetFileAttributesA", "WaitForSingleObject", "GetSystemPaletteEntries",
                "GetPaletteEntries", "RegisterClassA", "RegisterClassExA", "GetVersion", "GetCurrentProcessId",
                "GetCurrentThreadId", "SizeofResource", "GetPrivateProfileIntA", "GetPrivateProfileStringA", "GetPixel",
                "SetTextColor", "SetBkColor", "GetTextColor", "GetBkColor", "timeBeginPeriod", "timeEndPeriod",
                "ResumeThread", "SuspendThread", "GetEnvironmentVariableA", "MsgWaitForMultipleObjects", "SleepEx",
                "GetQueueStatus", "SetTimer", "GetDoubleClickTime", "GetCaretBlinkTime", "MapVirtualKeyA",
                "GetKeyboardType", "GetLogicalDrives", "WaitForMultipleObjects", "GetFullPathNameA",
                "GetShortPathNameA", "GetLongPathNameA", "SetErrorMode", "GetSysColor", "RealizePalette",
                "SetPaletteEntries", "GetNearestColor", "SetTextAlign", "GetTextAlign", "SetDIBitsToDevice",
                "timeGetDevCaps", "joyGetNumDevs", "joyGetPosEx", "waveOutGetNumDevs", "mmioDescend", "mmioClose",
                "mmioAscend"}


def api_decls():
    out = []
    for n, k in sorted(API_USED.items()):
        rt = "uint32_t" if n in API_UNSIGNED else "intptr_t"
        if n in API_VARARG:
            out.append(f"typedef {rt} (*api_{n}_t)(intptr_t, ...);   /* {n}: cdecl, variadic */")
        else:
            out.append(f"typedef {rt} (__attribute__((stdcall)) *api_{n}_t)({', '.join(['intptr_t'] * k) or 'void'});")
        out.append(f'#define API_{n} ((api_{n}_t)g_eng.api("{n}"))')
    return "\n".join(out)


def retail_calls(edition):
    """the typed pointers to retail's own (unported) callees: RC_xxxxxx(...) = ((rt_xxxxxx)g_eng.retail(va))(...)"""
    cc = ida_cc(edition)
    pro = _CC[edition][1]
    out = []
    for va in sorted(RETAIL):
        conv, n = cc.get(va, ("cdecl", 0))
        ret = pro.get(va, "")
        rt = "double" if re.match(r"\s*(long double|double|float)\b", ret) else \
             "int64_t" if "__int64" in ret.split("(")[0] else "intptr_t"   # (a `void` one too: a caller may read eax)
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
        if len(f) > 1 and re.match(r"^[0-9a-f]{8}$", f[0]) and re.match(r"^(?:[A-Za-z_]\w{2,}|a[A-Z0-9]\w*)$", f[1]):   # (aA: a string)
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
    m = re.search(r"\n([^\n;]*\b\w*?_%X(?:@<\w+>)?\([^;{]*?\)\n\{.*?\n\})" % va, src, re.S)   # sub_X or a KB name_X
    if not m:   # a FLIRT name without the VA (unknown_libname_2): the export's `===== name @ 0x00VA` header
        m = re.search(r"\n/\* ===== \S+ @ 0x%08x [^\n]*\n(?://[^\n]*\n)*([^\n;]*\([^;{]*?\)\n\{.*?\n\})" % va, src, re.S)
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
FRAMECLS = {}     # class objects held by value on the stack: name -> class
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
CURVA = [None]    # ... and its VA
DECLS = []
ARRAYS = []
CLSPTR = []


WIN_INT = {"HDC", "HGDIOBJ", "HWND", "HBITMAP", "HFONT", "HBRUSH", "HPEN", "HANDLE", "HMODULE", "HINSTANCE", "HRGN",
           "HPALETTE", "HLOCAL", "HGLOBAL", "HMMIO", "HRSRC", "LPVOID", "PVOID", "BOOL", "LONG", "INT",
           "LPARAM", "LRESULT", "HRESULT"}   # word-sized Win32 types (signed)
WIN_UINT = {"UINT", "DWORD", "COLORREF", "WPARAM", "ULONG", "SIZE_T"}   # ... unsigned
WIN_STRUCT = {"POINT", "RECT", "SIZE", "BITMAPINFOHEADER", "BITMAPINFO", "RGBQUAD", "PALETTEENTRY", "LOGFONTA",
              "TEXTMETRICA", "BITMAP", "MSG", "PAINTSTRUCT", "SYSTEMTIME", "MMCKINFO", "WAVEFORMATEX", "WNDCLASSA",
              "OSVERSIONINFOA", "LOGBRUSH", "LOGFONTA", "struct tm", "WNDCLASSEXA", "FILE"}   # kept as <windows.h>'s own
WORD_SHELLS = ("paint_ctx", "input_dev", "input_mgr", "zdm", "log_singleton")   # wnd_proc.h's shells: passed through as words
SHELL_SIZE = {"input_dev": 12, "paint_ctx": 0x170, "log_singleton": 0x504}   # ... IDA's sizes for them (knowledge pack decls)
SHELL_ARRAYS = {"log_singleton": {"path": 0x404}}   # ... their array members (an address, not a load)
SHELL_FIELDS = {"paint_ctx": {"zdd_device": 0x2c, "blit_x": 0x138, "blit_y": 0x13c, "blit_w": 0x140, "blit_h": 0x144,
                              "state": 0x164, "back_ctx": 0x16c},
                "input_dev": {"dev_obj": 0x4, "acquired": 0x8},
                "input_mgr": {"zdm_ptr": 0x2884}}   # (wnd_proc.h's pinned offsets; 0x58ffa0's `input_mgr *this` is the system object)
WIN_ALIAS = {"struct tagRECT": "RECT", "tagRECT": "RECT", "struct tagPOINT": "POINT", "tagPOINT": "POINT",
             "struct _SYSTEMTIME": "SYSTEMTIME", "_SYSTEMTIME": "SYSTEMTIME",
             "struct _MMCKINFO": "MMCKINFO", "_MMCKINFO": "MMCKINFO", "LPMMCKINFO": "MMCKINFO *",
             "struct tagMSG": "MSG", "tagMSG": "MSG", "LPMSG": "MSG *", "struct tagWNDCLASSA": "WNDCLASSA",
             "tagWNDCLASSA": "WNDCLASSA", "struct _iobuf": "FILE", "_iobuf": "FILE", "struct tagWNDCLASSEXA": "WNDCLASSEXA", "tagWNDCLASSEXA": "WNDCLASSEXA", "struct tagLOGBRUSH": "LOGBRUSH", "tagLOGBRUSH": "LOGBRUSH", "struct tagLOGFONTA": "LOGFONTA", "struct _OSVERSIONINFOA": "OSVERSIONINFOA", "LPOSVERSIONINFOA": "OSVERSIONINFOA *",
             "LPDIRECTSOUND": "void *", "LPDIRECTSOUNDBUFFER": "void *", "LPDIRECTINPUT7A": "void *",
             "LPDIRECTDRAW7": "void *", "LPDIRECTDRAWSURFACE7": "void *",
             "struct tagSIZE": "SIZE",
             "LPCSTR": "char *", "LPSTR": "char *", "LPBITMAPINFO": "BITMAPINFO *", "LPPOINT": "POINT *",
             "LPRECT": "RECT *", "WORD": "uint16_t", "BYTE": "uint8_t", "LPBYTE": "uint8_t *", "HPSTR": "char *"}
TYPEMAP = [("_BOOL1", "uint8_t"), ("_BOOL2", "uint16_t"), ("_BOOL4", "int32_t"), ("unsigned __int16", "uint16_t"), ("__int16", "int16_t"), ("unsigned int", "uint32_t"),
           ("unsigned __int8", "uint8_t"), ("_DWORD", "int32_t"), ("_WORD", "uint16_t"), ("_BYTE", "uint8_t"),
           ("__int64", "int64_t"), ("unsigned __int64", "uint64_t"), ("long double", "double"), ("char", "char"), ("unsigned char", "uint8_t"), ("bool", "int"), ("int", "int32_t")]


def map_type(t):
    t = re.sub(r"\b(const|volatile)\s+", "", t.strip()).strip()   # qualifiers: the pointer stays a pointer
    t = re.sub(r"\bCHAR\b", "char", t)
    if t.replace("*", "").strip() in WIN_ALIAS:
        st = t.count("*"); a = WIN_ALIAS[t.replace("*", "").strip()]
        return a + (" " + "*" * st if st else "")
    if t.replace("*", "").strip() == "struct tm":   # the CRT's (localtime): the same 9 ints in both CRTs
        st = t.count("*")
        return "struct tm" + (" " + "*" * st if st else "")
    if t.startswith("cls_") or t.startswith("struct ") or t.startswith("_UNKNOWN"):
        n = t.count("*")
        if n >= 2:   # a pointer to class pointers: a table of 4-byte slots
            return "intptr_t " + "*" * (n - 1)
        return "intptr_t"   # class pointers: integers (a class-sized index then fails to compile: fix by hand)
    stars = t.count("*")
    base = t.replace("*", "").strip()
    if base in WIN_ALIAS:
        a = WIN_ALIAS[base]
        stars += a.count("*"); base = a.replace("*", "").strip()
    if base in WIN_STRUCT:
        return base + (" " + "*" * stars if stars else "")
    if base in WORD_SHELLS:
        return "intptr_t" + (" " + "*" * (stars - 1) if stars > 1 else "")
    if TYPED[0] and base in kb_types():
        KBH.add(kb_types()[base])
        return base + (" " + "*" * stars if stars else "")
    if base in WIN_INT:
        return "intptr_t" + (" " + "*" * stars if stars else "")
    if base in WIN_UINT:
        return "uint32_t" + (" " + "*" * stars if stars else "")
    for a, b in TYPEMAP:
        if base == a:
            base = b
            break
    else:
        if base not in ("void", "int8_t", "int16_t", "int32_t", "uint8_t", "uint16_t", "uint32_t", "int64_t", "uint64_t",
                        "double", "float", "long double"):
            return "intptr_t"   # Windows / CRT types IDA guessed (FILE *, LPCSTR, HWND ...): integers
    if stars == 0 and base == "int32_t":   # (an `unsigned int` stays unsigned: its >> is retail's shr, its < unsigned)
        return "intptr_t"
    return base + (" " + "*" * stars if stars else "")


RESERVED = ("c", "self", "W", "D", "rnd")   # the converter's own names: an IDA local by one of them is renamed


INDEXED = set()   # the dword_ / word_ / byte_ tables a function indexes (an array: its bare name is its address)
CLS2 = {}         # the `cls_X **` locals -> cls_X


NAMED_DWORDS = {"hInstance": 0x8a6b38}   # named word globals IDA types by their API use
GLOBAL_CLS = {"g_app_state": (0x8a9b64, "cls_5624C0")}   # KB-named class-pointer globals (the export has no type for them)


def global_cls(text):
    """`G[k].field_X` / `G->field_X` of a class-pointer global: the field's load at its real offset; bare `G`: the pointer"""
    for g, (va, cls) in GLOBAL_CLS.items():
        if not re.search(r"\b%s\b" % g, text):
            continue
        size, fs, _ = class_layout(cls)
        W_ = {1: "_BYTE", 2: "_WORD", 4: "_DWORD", 8: "__int64"}
        def fld(k, nom):
            off, w = fs.get(nom, (nom, 4))
            return f"*({W_.get(w, '_DWORD')} *)((char *)(*(int *)&dword_{va:X}) + {k * size + off})"
        text = re.sub(r"\b%s\[(\d+)\]\.field_([0-9A-F]+)\b" % g, lambda m: fld(int(m.group(1)), int(m.group(2), 16)), text)
        text = re.sub(r"\b%s->field_([0-9A-F]+)\b" % g, lambda m: fld(0, int(m.group(1), 16)), text)
        text = re.sub(r"\b%s\b" % g, f"(*(int *)&dword_{va:X})", text)
    return text


def scale_this_sum(s, pat, n):
    """`this + <expr>` -> `this + n * (<expr>)`, <expr> running to the enclosing argument's end (a top-level `,` / `;`
    or the closing paren) — the pointer arithmetic of a sized `this`"""
    out, pos = [], 0
    for m in re.finditer(pat, s):
        if m.start() < pos:
            continue
        j, depth = m.end(), 0
        while j < len(s):
            ch = s[j]
            if ch in "([":
                depth += 1
            elif ch in ")]":
                if depth == 0:
                    break
                depth -= 1
            elif ch in ",;?:" and depth == 0:
                break
            elif ch == "=" and depth == 0 and s[j + 1:j + 2] != "=" and s[j - 1:j] not in "=!<>":
                break
            j += 1
        e = s[m.end():j].rstrip()
        if not e:
            continue
        out.append(s[pos:m.end()] + f"{n} * ({e})")
        pos = m.end() + len(e)
    return "".join(out) + s[pos:]


def unused_win_locals(f):
    """A <windows.h>-typed local nothing uses any more (the inlined log's FILE * / SYSTEMTIME once it collapses to
    the log call): a plain word, so the module needs no <windows.h> for it (the host test suite builds natively)."""
    out = []
    for ln in f.split("\n"):
        m = re.match(r"^(\s+)(%s)(\s*\*?\s*(?:const\s+)?)(\w+)\b(.*)$" % "|".join(re.escape(w) for w in WIN_STRUCT), ln)
        if m and len(re.findall(r"\b%s\b" % re.escape(m.group(4)), f)) == 1:
            ln = m.group(1) + "intptr_t" + m.group(3) + m.group(4) + m.group(5).replace("(%s *)" % m.group(2), "(intptr_t *)")
        out.append(ln)
    return "\n".join(out)


def int_literals(text):
    """`(int)"text"`: IDA typed the parameter a word — retail passes the ADDRESS of its data, and what follows the
    text belongs to it (0x40dbc0's book record: the title, then the pages from +0x28).  Retail's own bytes, not a
    host copy of the title."""
    def lit(m):
        raw = m.group(1).encode("latin1").decode("unicode_escape").encode("latin1")
        hits = [va for va in func_data_refs(EDITION[0], CURVA[0]) if exe_read(va, len(raw) + 1) == raw + b"\0"]
        return f"D(0x{hits[0]:x})" if len(hits) == 1 else m.group(0)
    return re.sub(r'\(int\)"((?:[^"\\]|\\.)*)"', lit, text)


def convert(text, names, widths, this):
    text = int_literals(global_cls(text))
    CLS2.clear()
    INDEXED.clear()
    INDEXED.update(re.findall(r"\b((?:dword|word|byte)_[0-9A-F]{6})\s*\[", text))
    INDEXED.update(asm_offset_only(EDITION[0], CURVA[0], set(re.findall(r"\b(?:dword|word|byte)_[0-9A-F]{6}\b", text))))
    lines = text.split("\n")
    k = lines.index("{")
    head, body = " ".join(l.strip() for l in lines[:k]), lines[k + 1:-1]
    # `if ( p ) ctor(p); else v = nullptr;`: the constructor returns `this` in eax and retail stores it (IDA, with a
    # void prototype, dropped the assignment: 0x56aea0's v6)
    bt = "\n".join(body)
    bt = re.sub(r"if \( (\w+) != nullptr \)\r?\n(\s+)(\w+)\(\1\);\r?\n(\s+)else\r?\n(\s+)(\w+) = nullptr;",
                lambda m: f"if ( {m.group(1)} != nullptr )\n{m.group(2)}{{ {m.group(3)}({m.group(1)}); {m.group(6)} = {m.group(1)}; }}   /* (a ctor returns its object) */\n"
                          f"{m.group(4)}else\n{m.group(5)}{m.group(6)} = nullptr;", bt)
    body = strip_seh(bt.split("\n"))
    for rn in RESERVED:   # IDA names TextOutA's length local `c`: it would alias `this` (0x496ec0 read this->+0x3d0 off it)
        if any(re.match(r"^  [A-Za-z_][\w\s\*]*?\b%s(\[\d+\])?; // " % rn, l) for l in body) or re.search(r"[(,]\s*[^,()]*\b%s\)" % rn, head):
            body = [re.sub(r"\b%s\b" % rn, rn + "_l", l) for l in body]
            head = re.sub(r"\b%s\b(?=\s*[,)])" % rn, rn + "_l", head)
    if "__fastcall" in head:   # edx never read: thiscall, a1 = this (the caller's ecx)
        hm = re.match(r"(.*?\()(.*)(\)\s*)$", head)
        ps = [x.strip() for x in hm.group(2).split(",")]
        bt = "\n".join(body)
        if len(ps) >= 2:
            n1, n2 = re.match(r".*?\b(\w+)$", ps[0]).group(1), re.match(r".*?\b(\w+)$", ps[1]).group(1)
            if not re.search(r"\b%s\b" % n2, bt):
                t1 = re.match(r"(.*?)\s*\b\w+$", ps[0]).group(1)   # its own type: an `int` a1 adds bytes
                ps = [(t1.rstrip() + "this") if t1.rstrip().endswith("*") else (t1 + " this")] + ps[2:]   # (`cls_X *this`:
                # the class layout keys on that spelling)
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
        if m and m.group(1).strip().startswith(("cls_", "_UNKNOWN", "struct ")) and m.group(1).count("*") == 1:
            CLSPTR.append(m.group(2))   # (a `cls_X **` local is a real C pointer: `(*v57)[1]` stays C)
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
            cm2 = re.match(r"(cls_\w+) \*\*$", m.group(1).strip())
            if cm2:
                CLS2[m.group(2)] = cm2.group(1)   # a `cls_X **` local: `(*v)[k].gapN` (0x4ee690)
            if cm and class_size(cm.group(1)):
                CLSIDX[m.group(2)] = (cm.group(1), class_size(cm.group(1)))
            if fo and re.match(r"^cls_\w+$", m.group(1).strip()):
                FRAMECLS[m.group(2)] = m.group(1).strip()
            if fo:   # a stack local lives in frame_ at its ebp offset too: IDA splits arrays / structs into neighbour
                     # locals (`&v34` handed to a rect writer, then v35..v37 read), and slots it reuses alias as in retail
                FRAMEVARS.append((int(fo.group(1), 16), mt, m.group(2)))
                continue
            init = " = 0" if ZERO_LOCALS[0] and not m.group(3) and mt not in WIN_STRUCT else ""   # (retail: a register's leftover; here: a fixed 0, never the port's stack garbage)
            decls.append(f"    {mt} {m.group(2)}{m.group(3) or ''}{init};")
            continue
        out.append(ln)
    DECLS.extend(decls)
    s = "\n".join(out)
    for n, cl in FRAMECLS.items():   # a class object held by value on the stack: its fields at their real offsets
        _, fsv, gsv = class_layout(cl)
        W2 = {1: "F8", 2: "F16", 4: "F32", 8: "F64"}
        s = re.sub(r"(?<![\w.>])%s\.field_([0-9A-F]+)" % re.escape(n),
                   lambda m: f"{W2.get(fsv.get(int(m.group(1), 16), (0, 4))[1], 'F32')}({n}__f, 0x{fsv.get(int(m.group(1), 16), (int(m.group(1), 16), 4))[0]:x})", s)
        s = re.sub(r"(?<![\w.>])%s\.gap([0-9A-F]+)\[([^\]]+)\]" % re.escape(n),
                   lambda m: f"GAP({n}__f, 0x{gsv.get(int(m.group(1), 16), int(m.group(1), 16)):x}, {m.group(2)})", s)
        s = re.sub(r"(?<![\w.>])%s\.gap([0-9A-F]+)\b" % re.escape(n),
                   lambda m: f"((uint8_t *)({n}__f) + 0x{gsv.get(int(m.group(1), 16), int(m.group(1), 16)):x})", s)
    # a global struct's members, by offset (its _padNNN shells are named by offset; the log singleton's path: +0x404)
    s = re.sub(r"\bstru_(?!8A6B60\b)([0-9A-F]{6})\._pad([0-9a-f]{3})\b", lambda m: f"((uint8_t *)D(0x{int(m.group(1), 16):x}) + 0x{m.group(2)})", s)
    s = re.sub(r"(?<!fopen\()\bstru_8A9530\.path\b", "((char *)D(0x8a9530) + 0x404)", s)
    s = re.sub(r"\bstru_([0-9A-F]{6})\.path\b", lambda m: f"((char *)D(0x{int(m.group(1), 16):x}) + 0x404)", s)   # (another log singleton: 0x8a6620)   # (the inlined log's fopen: collapsed below)
    s = s.replace("Concurrency::details::_ReaderWriterLock::_ReaderWriterLock(", "sub_5A4760(")   # (FLIRT's name for a 16-byte retail ctor)
    s = re.sub(r"\bunknown_libname_2\(", "sub_5B6E70(", s)   # (FLIRT's name for the engine's bitmap session ctor)
    s = re.sub(r"\((cls_\w+)\)(?=[\w(])", "", s)   # IDA's by-value class cast of an expression
    W = {1: "F8", 2: "F16", 4: "F32", 8: "F64", 10: "F64"}
    tm = re.search(r"\(\s*(cls_\w+) \*this\b", head)   # (a long header breaks after the paren: `( cls_X *this`)
    tsize, tfields, tgaps = class_layout(tm.group(1) if tm else CLS[0])
    if not tm:
        tsize = 1   # no class: `this + n` adds bytes
    tp = None if tm else re.search(r"\(\s*([A-Za-z_][\w ]*?) (\*+)this\b", head)
    this_expr = this
    if tp and tp.group(1) != "void" and not tp.group(1).startswith(("struct", "_UNKNOWN")):
        mt = map_type(tp.group(1) + " " + tp.group(2)[1:])   # `HDC *this` / `_DWORD *this`: `this + n` scales by the element
        if mt in ("intptr_t", "int32_t", "uint32_t", "uint16_t", "int16_t", "uint8_t", "char") or mt.endswith("*") or mt in WIN_STRUCT \
                or (TYPED[0] and mt in kb_types()):
            this_expr = f"(({mt} *){this})"
    if not tfields:
        tfields = {o: (o, w) for o, w in widths.items()}

    def fld(base, nom, fields, gaps):
        off, w = fields.get(nom, (nom, 4))
        return f"{W.get(w, 'F32')}({base}, 0x{off:x})"
    if tsize > 1:   # `this + n` (no cast) moves by whole objects, as in the asm
        s = scale_this_sum(s, r"(?<![\w)*])this \+ ", tsize)   # (to the argument's end: 0x5bb180's `this + 1, 0)`
        # keeps its 0; 0x484bc0's `this + 1536 * a2 + 12 * a3` scales whole)
        if tsize in (1, 2, 4, 8):   # `*(this + n)` of a word-sized class: the whole object by value (0x42b5f0's `*(this + 7) = ...`)
            s = re.sub(r"\*\(this \+ %d \* \(([^()]+)\)\)" % tsize,
                       lambda m: f"{ {1: 'F8', 2: 'F16', 4: 'F32', 8: 'F64'}[tsize]}(this, {tsize} * ({m.group(1)}))", s)
    s = re.sub(r"this->field_([0-9A-F]+)", lambda m: fld(this, int(m.group(1), 16), tfields, tgaps), s)
    s = re.sub(r"this->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"GAP({this}, 0x{tgaps.get(int(m.group(1), 16), int(m.group(1), 16)):x}, {m.group(2)})", s)
    for v2, c2 in CLS2.items():   # `(*v)[k].gapN` of a `cls_X **` local: element k of the class it points at
        csz, cfs, cgaps = class_layout(c2)
        s = re.sub(r"\(\*%s\)->field_([0-9A-F]+)\b" % v2,   # ... and `(*v)->field_X` (0x562ea0's `(*v33)->field_44`)
                   lambda m, v=v2, cfs=cfs: f"{ {1: 'F8', 2: 'F16', 4: 'F32', 8: 'F64'}.get(cfs.get(int(m.group(1), 16), (0, 4))[1], 'F32')}"
                                            f"(*{v}, 0x{cfs.get(int(m.group(1), 16), (int(m.group(1), 16), 4))[0]:x})", s)
        if csz:
            s = re.sub(r"\(\*%s\)\[(\d+)\]\.gap([0-9A-F]+)\b(?!\s*\[)" % v2,
                       lambda m, v=v2, csz=csz, cgaps=cgaps: f"((uint8_t *)(*{v}) + 0x{int(m.group(1)) * csz + cgaps.get(int(m.group(2), 16), int(m.group(2), 16)):x})", s)
    for cm in re.finditer(r"(cls_\w+) \*(\w+)\b", head):   # a class-pointer PARAMETER's `p[k].gapN[i]` (0x585400's `a1[1].gap0`)
        csz, _, cgaps = class_layout(cm.group(1))
        if csz and cm.group(2) != "this":
            s = re.sub(r"\b%s\[(\d+)\]\.gap([0-9A-F]+)\[([^\]]+)\]" % cm.group(2),
                       lambda m, v=cm.group(2), csz=csz, cgaps=cgaps: f"GAP({v}, 0x{int(m.group(1)) * csz + cgaps.get(int(m.group(2), 16), int(m.group(2), 16)):x}, {m.group(3)})", s)
            s = re.sub(r"\b%s\[(\d+)\]\.gap([0-9A-F]+)\b(?!\s*\[)" % cm.group(2),
                       lambda m, v=cm.group(2), csz=csz, cgaps=cgaps: f"((uint8_t *)({v}) + 0x{int(m.group(1)) * csz + cgaps.get(int(m.group(2), 16), int(m.group(2), 16)):x})", s)
    if tm and tsize:   # `this[k].gapN[i]`: past the object by whole objects (0x456d30's `this[1].gap0[37820]`)
        s = re.sub(r"\bthis\[(\d+)\]\.gap([0-9A-F]+)\[([^\]]+)\]",
                   lambda m: f"GAP({this}, 0x{int(m.group(1)) * tsize + tgaps.get(int(m.group(2), 16), int(m.group(2), 16)):x}, {m.group(3)})", s)
    s = re.sub(r"\bsizeof\(this->gap([0-9A-F]+)\)", lambda m: str(gap_size(tm.group(1) if tm else CLS[0], int(m.group(1), 16))), s)
    s = re.sub(r"this->gap([0-9A-F]+)\b(?!\s*\[)", lambda m: f"((uint8_t *)({this}) + 0x{tgaps.get(int(m.group(1), 16), int(m.group(1), 16)):x})", s)
    def vgap(m):   # any class-pointer local's gap byte, at its class's real gap offset
        cl = CLSTYPE.get(m.group(1))
        off = class_layout(cl)[2].get(int(m.group(2), 16), int(m.group(2), 16)) if cl else int(m.group(2), 16)
        return f"GAP({m.group(1)}, 0x{off:x}, {m.group(3)})"
    s = re.sub(r"\b(?!this\b)([A-Za-z_]\w*)->gap([0-9A-F]+)\[([^\]]+)\]", vgap, s)

    def vgap0(m):   # a class-pointer local's bare gap (its address)
        cl = CLSTYPE.get(m.group(1))
        off = class_layout(cl)[2].get(int(m.group(2), 16), int(m.group(2), 16)) if cl else int(m.group(2), 16)
        return f"((uint8_t *)({m.group(1)}) + 0x{off:x})"
    s = re.sub(r"\b(?!this\b|p_cls_)([A-Za-z_]\w*)->gap([0-9A-F]+)\b(?!\s*\[)", vgap0, s)
    for sh, flds in SHELL_ARRAYS.items():   # a word shell's array member: its address
        vs = set(re.findall(r"\b%s \*(\w+)\b" % sh, head + "\n" + text))
        for v in vs:
            s = re.sub(r"\b%s->(%s)\b" % (re.escape(v), "|".join(flds)), lambda m, v=v, flds=flds: f"((char *){v} + 0x{flds[m.group(1)]:x})", s)
    for sh, flds in SHELL_FIELDS.items():   # a word shell's member: a load at its pinned offset
        vs = set(re.findall(r"\b%s \*(\w+)\b" % sh, head + "\n" + text))
        for v in vs:
            s = re.sub(r"\b%s->(%s)\b" % (re.escape(v), "|".join(flds)), lambda m, v=v: f"F32({v}, 0x{flds[m.group(1)]:x})", s)
    if tp and tp.group(1) in SHELL_SIZE and len(tp.group(2)) == 1:   # a word shell's `this + n`: n of IDA's struct
        s = re.sub(r"(?<![\w)*])\bthis \+ (\d+)\b", lambda m: f"this + 0x{SHELL_SIZE[tp.group(1)] * int(m.group(1)):x}", s)   # (0x5ba4c0's +0x18)
    if this_expr != this:   # a non-class `T *this`: its derefs / indexing scale by T; a plain use stays the word
        s = re.sub(r"\bthis->", this_expr + "->", s)   # (a KB struct's member, --typed)
        s = re.sub(r"\bthis\[", this_expr + "[", s)
        s = re.sub(r"\*\(this \+ ", "*(" + this_expr + " + ", s)
        s = re.sub(r"\*this\b", "(*" + this_expr + ")", s)
        esz = {"uint8_t": 1, "char": 1, "int8_t": 1, "uint16_t": 2, "int16_t": 2}.get(this_expr[2:].split(" *")[0], 4)
        if esz > 1 and not (tp and tp.group(1) in SHELL_SIZE and len(tp.group(2)) == 1):   # ... and `this + n` as a value (`v4 = this + 2604` = +10416 bytes for a _DWORD *this)
            s = scale_this_sum(s, r"(?<![\w)*])\bthis \+ ", esz)
    s = re.sub(r"\bthis\b", this, s)
    s = s.replace("nullptr", "0")
    s = re.sub(r"\*\*\(\((?:_DWORD|int|cls_\w+) \*\*\)(\w+) \+ (\d+)\)", r"F32(F32(\1, 4 * \2), 0)", s)   # `**((_DWORD **)v + n)`
    s = re.sub(r"\*\(\(_DWORD \*\)(\w+) \+ (\d+)\)", r"F32(\1, 4 * \2)", s)
    s = re.sub(r"\*\(\(_WORD \*\)(\w+) \+ (\d+)\)", r"F16(\1, 2 * \2)", s)
    # `*(&dword_X + i)`: element i of a global table (IDA's pointer arithmetic: by the element size)
    for kind, f, n in (("dword", "F32", 4), ("word", "F16", 2), ("byte", "F8", 1)):
        s = re.sub(r"\*\(&%s_([0-9A-F]{6}) \+ ([^()]+?)\)" % kind, lambda m, f=f, n=n: f"{f}(D(0x{int(m.group(1), 16):x}), {n} * ({m.group(2)}))", s)
    inner = r"((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?)"
    s = re.sub(r"\*\*\(_DWORD \*\*\)&stru_8A6B60\._pad000\[800\]", "CFG()", s)
    s = re.sub(r"\*\*\((?:_DWORD|int|cls_\w+|_UNKNOWN) \*\*\)&stru_8A6B60\._pad000\[((?:[^\[\]]|\[[^\[\]]*\])+)\]", r"F32(SYS32(\1), 0)", s)
    for t, f in (("_WORD", "uint16_t"), ("unsigned __int16", "uint16_t"), ("__int16", "int16_t"), ("_BYTE", "uint8_t"), ("char", "char")):
        s = re.sub(r"\*\*\(%s \*\*\)&stru_8A6B60\._pad000\[((?:[^\[\]]|\[[^\[\]]*\])+)\]" % t, r"(*(%s *)SYS32(\1))" % f, s)   # a table of narrow pointers
    s = re.sub(r"\*\((?:cls_\w+ \*+|_DWORD \*|int \*|_UNKNOWN \*\*|unsigned __int16 \*\*|__int16 \*\*|_WORD \*\*|char \*\*|_BYTE \*\*|_DWORD \*\*)\)&stru_8A6B60\._pad000\[((?:[^\[\]]|\[[^\[\]]*\])+)\]", r"SYS32(\1)", s)
    s = re.sub(r"\*\(_WORD \*\)&stru_8A6B60\._pad000\[((?:[^\[\]]|\[[^\[\]]*\])+)\]", r"F16(g_eng.sys, \1)", s)
    for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("_BYTE", "F8")):
        s = re.sub(r"\*\(%s \*\)&W->gap([0-9A-F]+)\[([^\]]+)\]" % t, lambda m: f"{f}(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
        s = re.sub(r"\*\(%s \*\)&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]" % t, lambda m: f"{f}(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
        s = re.sub(r"\*\(%s \*\)&(\w+)\[([^\]]+)\](?!\s*\.)" % t, r"%s(&\1[\2], 0)" % f, s)
    s = re.sub(r"\*\((?:cls_\w+|_DWORD|int|_UNKNOWN|void) \*\*\)&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
    s = re.sub(r"&p_cls_40BE00->gap([0-9A-F]+)\[([^\]]+)\]", lambda m: f"(W + 0x{int(m.group(1), 16):x} + ({m.group(2)}))", s)
    for t, f in (("_BYTE", "F8"), ("_WORD", "F16"), ("_DWORD", "F32"), ("char", "F8"), ("__int16", "S16")):
        s = re.sub(r"\*\*\(%s \*\*\)\(%s \+ (\d+)\)" % (t, inner), lambda m, f=f: f"{f}({accx("F32", m.group(1), m.group(2))}, 0)", s)
    s = re.sub(r"\*\(\(_DWORD \*\)(\w+) \+ ([^()]+?)\)", r"F32(\1, 4 * (\2))", s)
    s = re.sub(r"(?<!\*)\*\(\((?:_DWORD|cls_\w+|int|_WORD|_UNKNOWN) \*\*\)(\w+) \+ ([^()]+?)\)", r"F32(\1, 4 * (\2))", s)
    s = re.sub(r"(?<!\*)\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int|_WORD|_BYTE|char|__int16|unsigned __int16) \*\*+\)\(%s \+ ([^()]+?)\)" % inner, lambda m: accx("F32", m.group(1), m.group(2)) if m.group(1).count("[") == m.group(1).count("]") else m.group(0), s)
    for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("unsigned __int16", "F16"), ("_BYTE", "F8")):
        s = re.sub(r"\*\(%s \*\)(\w+)->gap([0-9A-F]+)\b(?!\s*\[)" % t,
                   lambda m, f=f: f"{f}({'W' if m.group(1) == 'p_cls_40BE00' else m.group(1)}, 0x{int(m.group(2), 16):x})", s)
    # `*(T **)v->field_N` is `*(v->field_N)` (-> binds tighter than the cast): the field load stays for vfld below
    s = re.sub(r"(?<!\*)\*\((?:cls_\w+|_UNKNOWN|void|_DWORD|int) \*\*+\)(\w+->field_[0-9A-F]+)\b", r"F32(\1, 0)", s)
    s = re.sub(r"(?<!\*)\*\((?:cls_\w+|_UNKNOWN|void) \*\*+\)(\w+)\b(?!\s*[\[(]|->)", r"F32(\1, 0)", s)
    for _ in range(4):
        for t, f in (("_DWORD", "F32"), ("int", "F32"), ("_WORD", "F16"), ("__int16", "S16"), ("unsigned __int16", "F16"),
                     ("_BYTE", "F8"), ("char", "F8"), ("unsigned __int8", "F8"), ("unsigned int", "FU32"), ("_UNKNOWN \\*", "F32")):
            s = re.sub(r"\*\(%s \*\)\(%s \+ (\d+)\)" % (t, inner), lambda m, f=f: accx(f, m.group(1), m.group(2)), s)
            s = re.sub(r"\*\(%s \*\)(\w+(?:->\w+)+)\b(?!\s*[\[(])" % t, r"%s(\1, 0)" % f, s)   # (the member's deref)
            s = re.sub(r"\*\(%s \*\)(\w+)\b(?!\s*[\[(]|\s*->)" % t, r"%s(\1, 0)" % f, s)
            s = re.sub(r"\*\(%s \*\)(\(%s\)(?:->\w+)+)\b(?!\s*[\[(])" % (t, inner), r"%s(\1, 0)" % f, s)   # (a member's deref)
            s = re.sub(r"\*\(%s \*\)\(%s\)(?!\s*->)" % (t, inner), r"%s((\1), 0)" % f, s)
        s = re.sub(r"(?<!\*)\*\((?:_UNKNOWN|void|cls_\w+|_DWORD|int|_WORD|_BYTE|char|__int16|unsigned __int16) \*\*\)\(%s\)" % inner, r"F32((\1), 0)", s)
    for v, (cls, n) in CLSIDX.items():   # a class pointer's `v + k` as a value moves by whole objects (0x5642e0's v9 + 1)
        if n > 1:
            s = re.sub(r"(?<=[=,(] )%s \+ (\d+)(?=[;,)])" % re.escape(v), lambda m: f"{v} + {n} * {m.group(1)}", s)
            # ... and so does its ++ / -- (0x56c430's dword walk: `++v3` is +4)
            s = re.sub(r"(?m)^(\s*)(?:\+\+%s|%s\+\+);" % (re.escape(v), re.escape(v)), lambda m: f"{m.group(1)}{v} += {n};", s)
            s = re.sub(r"(?m)^(\s*)(?:--%s|%s--);" % (re.escape(v), re.escape(v)), lambda m: f"{m.group(1)}{v} -= {n};", s)
            # ... anywhere else too (a for header's `++i`: 0x479a10 walked its 4-byte slots a byte at a time)
            s = re.sub(r"(?<![\w+])\+\+%s\b(?!\s*(?:->|\[|\.))" % re.escape(v), f"({v} += {n})", s)   # (not `--v->field`: the field)
            s = re.sub(r"(?<![\w-])--%s\b(?!\s*(?:->|\[|\.))" % re.escape(v), f"({v} -= {n})", s)
            s = re.sub(r"\b%s\+\+(?![\w+])" % re.escape(v), f"(({v} += {n}) - {n})", s)
            s = re.sub(r"\b%s--(?![\w-])" % re.escape(v), f"(({v} -= {n}) + {n})", s)
    for v, (cls, n) in CLSIDX.items():   # a class pointer kept as an integer, indexed: by the class size
        _, fs0, gs0 = class_layout(cls)
        el = lambda i: f"({v} + ({i}) * 0x{n:x})"
        s = re.sub(r"(&?)\b%s\[([^\]]+)\]\.gap([0-9A-F]+)\[([^\]]+)\]" % v,   # an element's gap byte
                   lambda m: f"{m.group(1)}GAP({el(m.group(2))}, 0x{gs0.get(int(m.group(3), 16), int(m.group(3), 16)):x}, {m.group(4)})", s)
        s = re.sub(r"\b%s\[([^\]]+)\]\.gap([0-9A-F]+)\b(?!\s*\[)" % v,
                   lambda m: f"((uint8_t *){el(m.group(1))} + 0x{gs0.get(int(m.group(2), 16), int(m.group(2), 16)):x})", s)
        s = re.sub(r"&%s\[([^\]]+)\]\.field_([0-9A-F]+)" % v,   # an element's field address
                   lambda m: f"((uint8_t *){el(m.group(1))} + 0x{fs0.get(int(m.group(2), 16), (int(m.group(2), 16), 4))[0]:x})", s)
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
    for v in CLSPTR:   # a class pointer kept as an integer: its unary deref is a dword load (but `*v->field_X` derefs
        # the FIELD: `->` binds tighter — 0x5b8ea0's `*(_DWORD *)a1->field_8`)
        s = re.sub(r"(?<=[=(,!&|<>?:]\s)\*%s\b(?!\s*->|\+\+|--)|(?<=[=(,!)])\*%s\b(?!\s*->|\+\+|--)" % (v, v), f"F32({v}, 0)", s)   # (`*v9++`: the
        # pointer advances — a `cls **` local is an intptr_t *: kept as C)
        s = re.sub(r"(?m)^(\s*)\*%s\b(?=\s*=[^=])" % v, lambda m: f"{m.group(1)}F32({v}, 0)", s)   # `*v3 = 0;` (a store)
    def vfld(m):
        cl = CLSTYPE.get(m.group(1))
        if cl:
            _, fs, gs = class_layout(cl)
            return fld(m.group(1), int(m.group(2), 16), fs, gs)
        return f"F32({m.group(1)}, 0x{int(m.group(2), 16):x})"
    s = re.sub(r"\b(\w+)->field_([0-9A-F]+)", vfld, s)
    s = re.sub(r"\(?(F32\((?:[^()]|\([^()]*\))*\))\)?->field_([0-9A-F]+)", lambda m: f"F32({m.group(1)}, 0x{int(m.group(2), 16):x})", s)   # (*pp)->field
    s = re.sub(r"\*\*\(_DWORD \*\*\)&stru_8A6B60\._pad000\[800\]", "CFG()", s)
    s = re.sub(r"(?<!\*)\*\((?:_DWORD|int|cls_\w+) \*\*\)\(%s \+ (\d+)\)" % r"((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?)", r"F32(\1, \2)", s)
    s = re.sub(r"\(int\)&loc_([0-9A-F]{6})\b|(?<![\w.])&loc_([0-9A-F]{6})\b", lambda m: f"0x{int(m.group(1) or m.group(2), 16):x}", s)   # an
    # immediate IDA took for a code address (0x5749b0's colour 0x40c040)
    s = re.sub(r"\b(\w+)\[0\]\.gap([0-9A-F]+)\[(\d+)\]", lambda m: f"F8({m.group(1)}, 0x{int(m.group(2), 16) + int(m.group(3)):x})", s)   # an
    # element 0 of a class-typed stack array: its byte at +N + i (0x5a4770's `v88[0].gap0[0] = 0`)
    s = s.replace("`eh vector destructor iterator'(", "eh_vec_dtor(").replace("(void (__thiscall *)(void *))", "(void *)")   # MSVC's delete[]
    s = s.replace("`eh vector constructor iterator'(", "eh_vec_ctor(")   # ... and new[]
    s = s.replace("rand_draw_5BF505()", "rnd()")
    s = re.sub(r"\bp_cls_40BE00 = ", "*(intptr_t *)g_eng.world = ", s)   # the world pointer itself (0x5a4320 clears it)
    s = s.replace("p_cls_40BE00", "W")
    s = re.sub(r"\*\(_DWORD \*\)&stru_8A6B60\._pad000\[(\d+)\]", r"SYS32(\1)", s)
    s = re.sub(r"stru_8A6B60\._pad000\[(\d+)\]", r"SYSB(\1)", s)
    s = re.sub(r"\bdbl_([0-9A-F]{6})\b", lambda m: f"(*(const double *)D(0x{int(m.group(1), 16):x}))", s)
    s = re.sub(r"\bflt_([0-9A-F]{6})\b", lambda m: f"(*(const float *)D(0x{int(m.group(1), 16):x}))", s)
    for kind, f in (("dword", "int32_t"), ("word", "uint16_t"), ("byte", "uint8_t")):
        s = re.sub(r"\b%s_([0-9A-F]{6})\[" % kind, lambda m: f"(({f} *)D(0x{int(m.group(1), 16):x}))[", s)
    for kind, ct in (("dword", "int32_t"), ("word", "uint16_t"), ("byte", "uint8_t")):   # an array into a pointer local:
        s = re.sub(r"\b(\w+) = %s_([0-9A-F]{6});" % kind,                               # its address (0x560a40's v12)
                   lambda m, ct=ct: f"{m.group(1)} = ({ct} *)D(0x{int(m.group(2), 16):x});" if m.group(1) in PTRVARS else m.group(0), s)
    for name, va in NAMED_DWORDS.items():   # named word globals (`::hInstance` where a parameter shadows it)
        if name in DECLNAMES:
            continue
        s = re.sub(r"::%s\b" % name, f"(*(intptr_t *)D(0x{va:x}))", s)
        if not re.search(r"\b%s\b(?=\s*[,)])|\b%s\b" % (name, name), head):
            s = re.sub(r"(?<![\w.>:])%s\b" % name, f"(*(intptr_t *)D(0x{va:x}))", s)
    for name, va in SYMS.items():
        if not re.search(r"\b%s\b" % re.escape(name), s) or name.startswith(("sub_", "p_cls_")) or name in ("Buffer", "stru_8A6B60") \
                or re.search(r"\b%s\b" % re.escape(name), head) or name in DECLNAMES:   # (a1..: a parameter only when in the head)
            continue
        if re.match(r"^a[A-Z0-9]", name):   # a string's element address: &aText[i]
            s = re.sub(r"&%s\[((?:[^\[\]]|\[[^\[\]]*\])+)\]" % re.escape(name), lambda m: f"((char *)D(0x{va:x}) + ({m.group(1)}))", s)
        s = re.sub(r"&%s\b" % re.escape(name), f"D(0x{va:x})", s)
        if name.startswith(("dword_", "word_", "byte_")) and name in INDEXED:
            # IDA indexes it somewhere in this function: an ARRAY — its bare name is its address (0x41bf60 passes
            # dword_6940C8 to 0x41c150: `push offset`), not its first element
            ct = {"dword_": "int32_t", "word_": "uint16_t", "byte_": "uint8_t"}[name.split("_")[0] + "_"]
            s = re.sub(r"(?<![\w.])%s\b(?!\s*\[)" % re.escape(name), f"(({ct} *)D(0x{va:x}))", s)
        elif name.startswith(("dword_", "word_", "byte_")):
            acc = {"dword_": "F32", "word_": "F16", "byte_": "F8"}[name.split("_")[0] + "_"]
            s = re.sub(r"(?<![\w.])%s\b(?!\s*\[)" % re.escape(name), f"{acc}(D(0x{va:x}), 0)", s)
        elif re.match(r"^[A-Za-z]\w*$", name) and not re.match(r"^(dword|word|byte|qword|flt|dbl|off|unk|stru|asc|o_cls)_|^a[A-Z0-9]", name) \
                and (is_cstr(va) or asm_named_offset_only(EDITION[0], CURVA[0], name)) and name not in kb_names():
            # a global IDA named after the API parameter it feeds (SubBlock "\\", Mode "a", OutputString "\n", msg):
            # its bytes are a C string — the name is the string's address
            s = re.sub(r"(?<![\w.&:])%s\b(?!\s*[\[.(]|\s*->)" % re.escape(name), f"((char *)D(0x{va:x}))", s)
        elif name == "trailer":   # IDA's `char trailer` (the empty string's byte): `&trailer` is its address (above),
            # bare `trailer` its byte (0x5b0890's `Filename[0] = trailer`: `mov al, trailer`)
            s = re.sub(r"(?<![\w.&])trailer\b(?!\s*[\[.])", f"F8(D(0x{va:x}), 0)", s)
        elif re.match(r"^(a[A-Z0-9]\w*|trailer|stru_\w+|unk_\w+|off_\w+|asc_\w+)$", name):
            if TYPED[0] and re.match(r"^a[A-Z0-9]", name):   # a string: indexable (--typed passes words anyway)
                s = re.sub(r"(?<![\w.&])%s\b(?!\s*[.])" % re.escape(name), f"((char *)D(0x{va:x}))", s)
            s = re.sub(r"(?<![\w.&])%s\b(?!\s*[\[.])" % re.escape(name), f"D(0x{va:x})", s)
    for va, (name, ct) in kb_globals().items():   # the knowledge pack's named globals (g_gamma_lut ...), typed
        if not re.search(r"\b%s\b" % name, s):
            continue
        am = re.match(r"(.+?)\s*\[(\d+)\]$", ct)
        if am:   # an array: its name is its address
            s = re.sub(r"(?<![\w.])&?%s\b" % name, f"(({map_type(am.group(1))} *)D(0x{va:x}))", s)
        else:
            mt = map_type(ct)
            s = re.sub(r"&%s\b" % name, f"(({mt} *)D(0x{va:x}))", s)
            s = re.sub(r"(?<![\w.])%s\b" % name, f"(*({mt} *)D(0x{va:x}))", s)
    s = re.sub(r"&(?:unk|byte|dword|word|asc|off|stru|o_cls_\w+?)_(?!8A6B60\b)([0-9A-F]{6})\b", lambda m: f"D(0x{int(m.group(1), 16):x})", s)
    s = re.sub(r"(?<![\w&])o_cls_\w+?_([0-9A-F]{6})\b", lambda m: f"D(0x{int(m.group(1), 16):x})", s)   # a vtable (an array: its address)
    for kind, f in (("dword", "F32"), ("word", "F16"), ("byte", "F8")):   # a global's value (not in data_xrefs) — or, an
        # array the asm only takes the address of, its address (0x5a4770's `dword_628670` is `push offset unk_628670`)
        ct = {"dword": "int32_t", "word": "uint16_t", "byte": "uint8_t"}[kind]
        s = re.sub(r"(?<![\w&.])%s_([0-9A-F]{6})\b(?!\s*\[)" % kind,
                   lambda m, f=f, ct=ct, kind=kind: f"(({ct} *)D(0x{int(m.group(1), 16):x}))" if f"{kind}_{m.group(1)}" in INDEXED
                   else f"{f}(D(0x{int(m.group(1), 16):x}), 0)", s)

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
    s = re.sub(r"\b(?:sub|[A-Za-z_]\w*?)_([0-9A-F]{6})\(", lambda m: call(m) if re.match(r"(sub|\w+)_[0-9A-F]{6}\(", m.group(0)) else m.group(0), s)
    s = re.sub(r"\((?:cls_\w+|_UNKNOWN) \*(\*+)\)", lambda m: f"(intptr_t {m.group(1)})", s)   # cls * = an int   # a pointer to class pointers: its deref is a dword load
    s = re.sub(r"\(cls_\w+ \*\)", "", s)
    # a pointer cast under arithmetic scales it (`(_BYTE *)v + 2` = 2 bytes): kept, as the C type
    CT = {"_DWORD": "int32_t", "_WORD": "uint16_t", "_BYTE": "uint8_t", "int": "int32_t", "char": "uint8_t",
          "unsigned __int16": "uint16_t", "__int16": "int16_t", "unsigned int": "uint32_t", "void": "uint8_t"}
    s = re.sub(r"(?<!\*)\((?:const )?(_DWORD|_WORD|_BYTE|int|char|unsigned __int16|__int16|unsigned int) (\*\*?)\)(?=(?:\w+|\((?:[^()]|\([^()]*\))*\)) [-+] )",
               lambda m: f"({CT[m.group(1)]} *)" if m.group(2) == '*' else "(int32_t **)", s)   # (`**((_DWORD **)this + v2 + 3)`
               # derefs twice: 0x566300)
    # every other IDA pointer cast is kept (_DWORD / _WORD / _BYTE are typedefs of the C widths): `**(_WORD **)x`
    # loads and pointer arithmetic then scale exactly as in the asm
    s = re.sub(r"\*&SYSB\((\d+)\)", r"SYS32(\1)", s)
    s = re.sub(r"(?:->|\.)unused\b", "", s)
    s = re.sub(r"\b(\w+)->_pad([0-9a-f]{3})\[", r"((uint8_t *)(\1) + 0x\2)[", s)   # a word shell's padding (named by its offset)
    s = re.sub(r"\b(\w+)->_pad([0-9a-f]{3})\b(?!\s*\[)", r"((uint8_t *)(\1) + 0x\2)", s)   # ... its address
    s = re.sub(r"\((?:uint8_t|char) \*\)(\w+) - \((?:uint8_t|char) \*\)(\w+)", r"((uint8_t *)\1 - (uint8_t *)\2)", s)   # byte pointers' distance   # a Win32 handle's dummy member (HDC__ { int unused; }): the handle itself
    s = re.sub(r"\((?:const )?(%s)( ?\**)\)" % "|".join(sorted(WIN_INT | WIN_UINT | set(WIN_ALIAS), key=len, reverse=True)),
               lambda m: f"({map_type(m.group(1) + m.group(2))})", s)   # Win32 casts: (HDC)x, (LPCSTR)s, (HGDIOBJ ***)p
    s = re.sub(r"\((?:const )?(%s) (\*+)\)" % "|".join(WORD_SHELLS), lambda m: f"(intptr_t{' ' + m.group(2)[1:] if len(m.group(2)) > 1 else ''})", s)
    if TYPED[0]:
        for t in set(re.findall(r"\b([a-z]\w*)\s*\*", s)) & set(kb_types()):
            KBH.add(kb_types()[t])
    s = s.replace("operator new(", "(intptr_t)g_eng.op_new(").replace("operator delete(", "g_eng.op_delete((void *)")
    s = re.sub(r"(?<![\w.])exit\(", "g_eng.h5bf5db(", s)                    # the CRT exit 0x5bf5db (FLIRT `exit`)
    s = re.sub(r"\bstru_8A6B60\.zdm_ptr->name\b", "(SYS32(0x2884) + 0x2c)", s)  # the manager's track name (inline)
    s = re.sub(r"\bstru_8A6B60\.zdm_ptr\b", "SYS32(0x2884)", s)              # the system's music manager
    s = re.sub(r"&stru_8A9530\b(?!\.)", "((intptr_t)g_eng.log)", s)          # the error log object
    s = re.sub(r"\bstru_8A9530\.path\b", "((char *)D(0x8a9530) + 0x404)", s)    # ... its log-file path (log_singleton)
    wsz, _, wgaps = class_layout("cls_40BE00")
    s = re.sub(r"(&?)(?:p_cls_40BE00|\bW)\[(\d+)\]\.gap([0-9A-F]+)\[([^\]]+)\]",
               lambda m: f"{m.group(1)}GAP(W, {int(m.group(2)) * wsz} + 0x{wgaps.get(int(m.group(3), 16), int(m.group(3), 16)):x}, {m.group(4)})", s)
    s = re.sub(r"&stru_8A6B60\b(?!\.)", "((intptr_t)g_eng.sys)", s)
    s = re.sub(r"&stru_8A6B60\._pad000\[((?:[^\[\]]|\[[^\[\]]*\])+)\]", r"((uint8_t *)g_eng.sys + (\1))", s)
    s = re.sub(r"\*\(_DWORD \*\)&W->gap([0-9A-F]+)\[(\d+)\]", lambda m: f"F32(W, 0x{int(m.group(1), 16):x} + {m.group(2)})", s)
    s = re.sub(r"W->field_([0-9A-F]+)", lambda m: f"F32(W, 0x{int(m.group(1), 16):x})", s)
    fa = r"(F32|F16)\(((?:[^()]|\((?:[^()]|\([^()]*\))*\))+?), ((?:[^()]|\([^()]*\))+?)\)"
    for part, (acc, add) in (("LOWORD", ("F16", 0)), ("HIWORD", ("F16", 2)), ("LOBYTE", ("F8", 0)), ("BYTE1", ("F8", 1)),
                             ("BYTE2", ("F8", 2)), ("HIBYTE", ("F8", 3))):   # a part of a field: the narrower field
        s = re.sub(r"\b%s\(%s\)" % (part, fa), lambda m, acc=acc, add=add: f"{acc}({m.group(2)}, {m.group(3)}{f' + {add}' if add else ''})", s)
    s = re.sub(r"(\+\+|--)HIDWORD\((\w+)\)", lambda m: f"(({m.group(2)}) {'+' if m.group(1) == '++' else '-'}= 0x100000000ll)", s)
    s = re.sub(r"(\+\+|--)LOWORD\(((?:[^()]|\([^()]*\))+)\)", lambda m: f"SETLO({m.group(2)}, (uint16_t)({m.group(2)}) {'+' if m.group(1) == '++' else '-'} 1)", s)
    for part, (acc, add) in (("LOWORD", ("F16", 0)), ("HIWORD", ("F16", 2)), ("LOBYTE", ("F8", 0)), ("BYTE1", ("F8", 1)),
                             ("BYTE2", ("F8", 2)), ("HIBYTE", ("F8", 3))):   # a part of a word global: the narrower store
        s = re.sub(r"\b%s\(\(\*\((?:intptr_t|int32_t|uint32_t|int) \*\)D\((0x[0-9a-f]+)\)\)\)" % part,
                   lambda m, acc=acc, add=add: f"{acc}(D({m.group(1)}), {add})", s)
    SETS = {"LOWORD": "SETLO", "HIWORD": "SETHI", "LOBYTE": "SETLB", "BYTE1": "SETB1", "LODWORD": "SETLODW", "HIDWORD": "SETHIDW"}
    out, pos = [], 0
    for m in re.finditer(r"\b(LOWORD|HIWORD|LOBYTE|BYTE1|LODWORD|HIDWORD)\((\w+|\w+\[[^\[\]]+\])\) = ", s):
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
    # retail's log 0x406440 inlined (OutputDebugStringA x 3 + the dated line appended to the log file; its string
    # globals OutputString / Mode are already addresses here)
    s = re.sub(r"OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\(([^;]+)\);\s*OutputDebugStringA\((?:(?:::)?OutputString|\(\(char \*\)D\(0x[0-9a-f]+\)\))\);\s*"
               r"(\w+) = fopen\((?:stru_8A9530\.path|\(\(char \*\)D\(0x8a9530\) \+ 0x404\)), (?:Mode|\(\(char \*\)D\(0x[0-9a-f]+\)\))\);\s*if \( \3 != 0 \)\s*\{\s*GetLocalTime\(&SystemTime\);\s*"
               r"fprintf\(.*?\);\s*fclose\(\3\);\s*\}",
               r"g_eng.log_fail_406440(g_eng.log, (const char *)(\1), (const char *)(\2));", s, flags=re.S)
    s = re.sub(r"(\w+) = GetLastError\(\);.*?g_eng\.log_fail_406440\((?:\(const char \*\)(?:D\(0x8a9530\)|\(\(intptr_t\)g_eng\.log\)), (D\(0x[0-9a-f]+\)), D\(0x8a9b6c\)"
               r"|g_eng\.log, \(const char \*\)\((D\(0x[0-9a-f]+\))\), \(const char \*\)\(D\(0x8a9b6c\)\))\);",
               lambda m: f"g_eng.h426110(g_eng.log, (intptr_t){m.group(2) or m.group(3)}, 1, (intptr_t)D(0x8a9b6c));   /* the error box 0x426110, inlined */", s, flags=re.S)
    s = re.sub(r"\((cls_\w+)\)(?=-?\d)", "", s)   # IDA's by-value class cast of a constant
    s = re.sub(r"&::([A-Za-z_]\w*)\b", lambda m: f"D(0x{SYMS[m.group(1)]:x})" if m.group(1) in SYMS else m.group(0), s)   # (a global a local / parameter shadows)
    s = re.sub(r"(?<![\w:])::([A-Za-z_]\w*)\b", lambda m: (f"((char *)D(0x{SYMS[m.group(1)]:x}))" if TYPED[0] else f"D(0x{SYMS[m.group(1)]:x})")
               if m.group(1) in SYMS else m.group(1), s)   # `::X`: the global X (a local may share its name)
    for _, t, n in FRAMEVARS:   # (last: the patterns above see IDA's names)
        s = re.sub(r"(?<![\w.>])%s\b" % re.escape(n), f"(*{n}__f)", s)
    return head, s


PROLOGUE = """#pragma GCC optimize ("no-strict-aliasing")
#pragma GCC diagnostic ignored "-Wint-conversion"
#pragma GCC diagnostic ignored "-Wincompatible-pointer-types"
#define CFG() ((intptr_t)**(uint8_t ***)((uint8_t *)g_eng.sys + 0x320))
#define SYS32(o) F32(g_eng.sys, (o))
#define SYSB(o) F8(g_eng.sys, (o))   /* the system object's byte at +o (an lvalue: &SYSB(o) is its address) */
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
typedef uint8_t _UNKNOWN;   /* IDA's untyped byte (casts: `(_UNKNOWN *)p`) */
typedef uint8_t _BOOL1;
typedef uint16_t _BOOL2;
typedef int32_t _BOOL4;
typedef uint16_t _WORD;
typedef uint8_t _BYTE;
typedef uint64_t _QWORD;
#define GAP(p, o, i) F8(p, (o) + (i))   /* IDA's gapN[i]: the byte at +N + i */
#define qmemcpy memcpy   /* IDA's inline memcpy */
static inline void *memset32(void *d, uint32_t v, size_t n) { uint32_t *p = d; while (n--) *p++ = v; return d; }   /* IDA's rep stosd */
static inline void eh_vec_ctor(intptr_t p, uint32_t size, int32_t n, void *ctor, void *dtor)   /* MSVC's `eh vector */
{                                                                                             /* constructor iterator' */
    typedef void (__attribute__((thiscall)) *c_t)(void *);
    (void)dtor;
    for (int32_t i = 0; i < n; i++) ((c_t)ctor)((void *)(p + (intptr_t)size * i));
}
static inline void eh_vec_dtor(intptr_t p, uint32_t size, int32_t n, void *dtor)   /* MSVC's `eh vector destructor iterator': */
{                                                                                  /* the elements' dtors, last first */
    typedef void (__attribute__((thiscall)) *d_t)(void *);
    p += (intptr_t)size * n;
    while (--n >= 0) { p -= size; ((d_t)dtor)((void *)p); }
}
typedef char CHAR;
#define SETLO(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff0000u) | (uint16_t)(v)))
#define SETHI(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffu) | ((uint32_t)(uint16_t)(v) << 16)))
#define SETLB(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffffff00u) | (uint8_t)(v)))
#define SETB1(x, v) ((x) = (intptr_t)(((uint32_t)(x) & 0xffff00ffu) | ((uint32_t)(uint8_t)(v) << 8)))
#define SETLODW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffff00000000ull) | (uint32_t)(v)))
#define SETHIDW(x, v) ((x) = (int64_t)(((uint64_t)(x) & 0xffffffffull) | ((uint64_t)(uint32_t)(v) << 32)))
#define F32(p, o) (*(int32_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define FU32(p, o) (*(uint32_t *)((uint8_t *)(intptr_t)(p) + (o)))   /* an `unsigned int` load: >> / < / / stay unsigned */
#define F16(p, o) (*(uint16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define S16(p, o) (*(int16_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F8(p, o)  (*(uint8_t *)((uint8_t *)(intptr_t)(p) + (o)))
#define F64(p, o) (*(int64_t *)((uint8_t *)(intptr_t)(p) + (o)))
"""


def signature(head, name):
    """IDA header -> (C signature of the port function `name`, the param prologue)"""
    m = re.match(r"\s*(.*?)\s*(?:__thiscall|__stdcall|__fastcall|__cdecl|__userpurge|__usercall)?\s*\b(?:\w*?_[0-9A-F]+|[A-Za-z_]\w*@\d+|DialogFunc|WinMain|(?:\w+::)+~?\w+)(?:@<\w+>)?\((.*)\)\s*$", head)   # (a FLIRT C++ name: 0x5a4760's `Concurrency::...::_ReaderWriterLock`)
    ret, params = m.group(1).replace("__thiscall", "").replace("__stdcall", "").strip(), re.sub(r"@<\w+>", "", m.group(2))
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
    ap.add_argument("--zero-locals", action="store_true", help="register locals start at 0 (no stack garbage: a deterministic port)")
    ap.add_argument("--typed", action="store_true", help="keep the knowledge pack's struct types (src/ headers) + word-cast call arguments")
    a = ap.parse_args()
    TYPED[0] = a.typed
    ZERO_LOCALS[0] = a.zero_locals
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
        FRAMECLS.clear()
        FRAMEVARS.clear()
        text = extract(a.edition, va)
        CURVA[0] = va
        if pname in ECXPASS and "__stdcall" not in text.split("{")[0]:
            ECXPASS.discard(pname)   # the decompile's header is the authority (thiscall: its ecx is `this`)
        head, s = convert(text, names, widths, a.this)
        FMAPS.append((va, list(ARRAYS), list(FRAMEVARS), dict(FRAMECLS)))
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
        m = re.match(r"[\w\s*]*?\b(\w+)\((.*)\);$", sig)   # (a pointer return type: `char * f(...)`)
        if m is None: sys.exit(f"decomp2c: unparsed signature {sig!r}")
        loc[m.group(1)] = (0 if m.group(2) == "void" else m.group(2).count(",") + 1,
                           m.group(2).startswith(("wl_char *self", "intptr_t ecx_")), m.group(1) in FASTLOCAL)
    wnames = [n for n in USED if n in protos and n in va_of]
    pat = r"(?<![\w.])(?:(%s)|W_(%s)|RC_([0-9a-f]{6}))\(" % ("|".join(map(re.escape, loc)) or "(?!x)x", "|".join(map(re.escape, wnames)) or "(?!x)x")
    lva = {n: v for v, n in LOCAL.items()}
    for k, f in enumerate(funcs):
        has_this = "intptr_t c = (intptr_t)self;" in f
        tex = "c" if has_this else ("ecx_" if fnames[k] in ECXPASS else "/*ECX: the caller's ecx*/ 0")
        out, pos = [], 0
        occ, ccount = {}, {}
        for m in re.finditer(pat, f):   # every call of each callee in this function (the asm's order to match)
            if not re.match(r"\s*(int32_t|int64_t|double|void) $", f[max(0, m.start() - 8):m.start()]):
                ccount[m.group(0)] = ccount.get(m.group(0), 0) + 1
        for m in re.finditer(pat, f):
            if re.match(r"\s*(int32_t|int64_t|double|void) $", f[max(0, m.start() - 8):m.start()]):
                continue
            o = occ.get(m.group(0), 0)
            occ[m.group(0)] = o + 1
            if m.start() < pos:
                continue
            cva = lva.get(m.group(1)) if m.group(1) else (int(m.group(3), 16) if m.group(3) else va_of.get(m.group(2)))
            ex = asm_ecx(a.edition, FMAPS[k], cva, o, ccount[m.group(0)]) if cva else None
            ctex = ex or tex   # the caller's ecx as its asm loads it; else the guess (its this / ecx_)
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
                kst = cc.get(wv, ("", 0))[1]
                if vcc == "stdcall" and len(args) == kst + 2 and n in (kst, kst + 1) and len(args) >= 2:
                    # the decompiler's view of a k-argument stdcall as __fastcall(ecx, edx, ...): ecx / edx are
                    # pass-through registers (0x41e360 char_unload): the port takes the stack arguments (+ ecx
                    # first when it reads it)
                    if n == kst:
                        out.append(f[pos:m.end()] + "/*ecx, edx*/ ")
                        pos = args[2][0]
                    else:
                        out.append(f[pos:args[0][1]] + ", /*edx*/")
                        pos = args[1][1] + 1
                    continue
                if vcc == "stdcall" and len(args) == n - 1 and ecx_in(a.edition, wv, sizes.get(wv, 0)):
                    vcc = "thiscall"   # the port takes the ecx this IDA-__stdcall callee reads as its first parameter
            if vcc == "fastcall" and len(args) == n + 1 and len(args) >= 2:
                if len(args) == 2:   # the edx one is the last: keep the closing paren
                    out.append(f[pos:args[0][1]] + " /*edx*/")
                    pos = args[1][1]
                else:
                    out.append(f[pos:args[0][1]] + ", /*edx*/")   # skip IDA's edx argument
                    pos = args[1][1] + 1
            elif vcc == "thiscall" and len(args) == n - 1:
                out.append(f[pos:m.end()] + ctex + (", " if args else ""))
                pos = m.end()
        funcs[k] = "".join(out) + f[pos:]
    imps = imports(a.edition)
    funcs = [crt_calls(api_calls(f, imps), a.edition) for f in funcs]
    # a function passed by ADDRESS (an EnumDevices callback 0x5ba200, a window procedure): retail calls it with
    # IDA's convention — a port function gets a shim with that convention; anything else stays retail's own address
    shims = {}
    rets = {}
    for sig in protos_local:
        sm = re.match(r"([\w\s*]*?)\b(\w+)\((.*)\);$", sig)
        if sm:
            rets[sm.group(2)] = sm.group(1).strip()

    def cbref(m):
        v = int(m.group(1), 16)
        if v not in sizes:
            return m.group(0)
        pn = LOCAL.get(v) or names.get(v)
        if not pn:
            RETAIL.add(v)
            return f"((intptr_t)g_eng.retail(0x{v:x}u))"
        # a ported function stored by address (a window procedure, an atexit / EnumDevices callback): retail's own
        # address — game memory never holds a port pointer; rhost patches that function's entry to its REPLACE wrapper
        return f"((intptr_t)D(0x{v:x}) /* {pn} by address */)"
    def fp_params(f):   # a vtable call's function-pointer type: its class / word-shell parameters are words
        out, pos = [], 0
        for m in re.finditer(r"\((?:__stdcall|__thiscall|__cdecl|__fastcall) \*+\)\(", f):
            if m.start() < pos:
                continue
            j, depth = m.end(), 0
            while j < len(f) and not (f[j] == ")" and depth == 0):
                depth += {"(": 1, ")": -1}.get(f[j], 0)
                j += 1
            out.append(f[pos:m.end()] + re.sub(r"\b(?:cls_\w+|%s) \*+" % "|".join(WORD_SHELLS), "void *", f[m.end():j]))
            pos = j
        return "".join(out) + f[pos:]
    funcs = [fp_params(f) for f in funcs]
    fnames_ida = set()
    for line in open(os.path.join(IDA, a.edition, "functions_ida.tsv")):
        fl = line.split("\t")
        if len(fl) > 2 and re.match(r"^[0-9a-f]{8}$", fl[0]) and re.match(r"^\w+_[0-9A-F]{6}$", fl[2]):
            fnames_ida.add(fl[2])
    def cbref_(m):
        return cbref(m) if m.group(0) in fnames_ida and not re.match(r"(cls|dword|word|byte|unk|off|stru|asc|flt|dbl)_", m.group(0)) else m.group(0)
    BARE_FN = {"DialogFunc": 0x4013c0}   # engine functions IDA names without their VA (the launcher's dialog procedure)
    funcs = [re.sub(r"(?<![\w.&])(%s)\b(?!\s*\()" % "|".join(BARE_FN), lambda m: cbref(re.match(r"(.*)", "%06X" % BARE_FN[m.group(1)])), f)
             for f in funcs]
    funcs = ["\n".join(l if l.startswith("/* 0x") else re.sub(r"(?<![\w.&])[A-Za-z]\w*?_([0-9A-F]{6})\b(?![\s@]*[(\[@])", cbref_, l)
                       for l in f.split("\n")) for f in funcs]
    if TYPED[0]:
        funcs = [word_args(f) for f in funcs]
    funcs = [unused_win_locals(f) for f in funcs]
    if any(re.search(r"\b(%s)\b" % "|".join(WIN_STRUCT), f) for f in funcs):   # a module that keeps a <windows.h> struct
        print("#include <windows.h>")
        print("#include <stdio.h>")
        print("#include <time.h>")
    print("\n".join(f'#include "{h}"' for h in sorted(KBH)))
    print("\n".join(f'#include "{h}"' for h in sorted({PROTO_HDR[n] for n in USED if n in PROTO_HDR})))
    print(PROLOGUE)
    print(wrappers(USED, protos))
    print(retail_calls(a.edition))
    if API_USED:
        print(api_decls())
    if CRT_USED:
        print(crt_decls())
    print("\n".join(protos_local))
    if shims:
        print("\n".join(shims.values()))
    print()
    print("\n\n".join(funcs))
    s = " ".join(sorted(todo))
    print(f"/* unported callees: {' '.join('0x' + t for t in s.split()) or 'none'} */")


if __name__ == "__main__":
    main()
