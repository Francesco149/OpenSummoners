#!/usr/bin/env python3
"""hook_abi.py — every rhost hook's wrapper pops what retail's function pops.

A REPLACE hook patches retail's call sites to the wrapper, so the wrapper's `ret N` must equal retail's `retn N`
(else the caller's stack is off by the difference: 0x556e50, an IDA `__fastcall(ecx, edx, 3 stack args)` hooked as a
5-argument stdcall, crashed the first time the water shrine reached it, 2026-10-30).  Reads the hook table H[] out of
the built rhost.exe (name / retail VA / wrapper), disassembles each wrapper to its rets, and compares with the rets
inside retail's function in the IDA export (all_ida.asm + functions_ida.tsv).  Checks the code as built, whatever
macro or hand wrapper produced it.  Also every engine-call slot named by a VA (src/engine_calls.h): the bytes its C
type pops vs retail's retn (port code calls retail, or the replacing wrapper, through it).

  nix develop --command python3 tools/rhost/hook_abi.py [--exe build/rhost/rhost.exe] [--edition enold] [-v]
Exit 1 when a wrapper and retail disagree.
"""
import argparse, os, re, struct, subprocess, sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
IDA = "/mnt/ephemeral/opensummoners/decompiled-ida"
HOOK_T = 4 * 9 + 256       # hook_t: name, va, wrapper, mode, every, calls, compared, diverge, skipped, first[256]


def tool(name):
    return "i686-w64-mingw32-" + name


def sections(exe):
    out = subprocess.run([tool("objdump"), "-h", exe], capture_output=True, text=True, check=True).stdout
    secs = []
    for m in re.finditer(r"^\s*\d+\s+(\S+)\s+([0-9a-f]+)\s+([0-9a-f]+)\s+[0-9a-f]+\s+([0-9a-f]+)", out, re.M):
        secs.append((m.group(1), int(m.group(3), 16), int(m.group(2), 16), int(m.group(4), 16)))
    return secs


def reader(exe):
    data, secs = open(exe, "rb").read(), sections(exe)

    def rd(va, n):
        for name, vma, size, off in secs:
            if vma <= va < vma + size and off:
                return data[off + va - vma: off + va - vma + n]
        return None
    return rd


def symbols(exe):
    out = subprocess.run([tool("nm"), "-n", exe], capture_output=True, text=True, check=True).stdout
    syms = []
    for l in out.splitlines():
        p = l.split()
        if len(p) == 3 and p[1] in "tT":
            syms.append((int(p[0], 16), p[2]))
    return syms


def hook_table(exe, rd):
    out = subprocess.run([tool("nm"), exe], capture_output=True, text=True, check=True).stdout
    h = next(int(l.split()[0], 16) for l in out.splitlines() if l.split()[-1] == "_H")
    hooks = []
    while True:
        b = rd(h + len(hooks) * HOOK_T, 12)
        if not b:
            break
        np, va, wr = struct.unpack("<III", b)
        s = rd(np, 128) if np else None
        nm = s.split(b"\0")[0] if s else b""
        if not re.fullmatch(rb"[\w.@:+-]+", nm):   # past H[H_N]'s end
            break
        hooks.append((nm.decode(), va, wr))
    return hooks


def wrapper_rets(exe, syms, wr):
    nxt = next((a for a, _ in syms if a > wr), wr + 0x4000)
    out = subprocess.run([tool("objdump"), "-d", "--no-show-raw-insn", f"--start-address={wr:#x}",
                          f"--stop-address={nxt:#x}", exe], capture_output=True, text=True).stdout
    rets = set()
    for m in re.finditer(r"^\s*[0-9a-f]+:\s+ret[lw]?\s*(\$0x[0-9a-f]+)?\s*$", out, re.M):
        rets.add(int(m.group(1)[1:], 16) if m.group(1) else 0)
    return rets


def retail_rets(edition):
    size = {}
    for l in open(os.path.join(IDA, edition, "functions_ida.tsv")):
        p = l.split("\t")
        if re.match(r"^[0-9a-f]{8}$", p[0]):
            size[int(p[0], 16)] = int(p[1])
    starts = sorted(size)
    rets = {}
    import bisect
    for l in open(os.path.join(IDA, edition, "all_ida.asm"), errors="replace"):
        m = re.match(r"^([0-9a-f]{8})\s+retn?\b\s*([0-9A-F]+h?)?", l)
        if not m:
            continue
        a = int(m.group(1), 16)
        i = bisect.bisect_right(starts, a) - 1
        if i < 0 or a >= starts[i] + size[starts[i]]:
            continue
        n = m.group(2)
        rets.setdefault(starts[i], set()).add(int(n.rstrip("h"), 16) if n else 0)
    return rets


def slot_pops():
    """the engine-call slots (src/engine_calls.h) named by a retail VA → (field, the bytes its C type pops).  Port code
    calls retail (or, under REPLACE, the hook wrapper) through them: the type must pop what retail pops too."""
    src = open(os.path.join(ROOT, "src", "engine_calls.h")).read()
    src = re.sub(r"/\*.*?\*/", "", src, flags=re.S)
    src = re.sub(r"\s+", " ", src)
    out = {}
    for m in re.finditer(r"\beng_tc(\d+) (\w+);", src):
        out[m.group(2)] = 4 * int(m.group(1))
    for m in re.finditer(r"\((ENG_STDCALL|ENG_THISCALL)? ?\*(\w+)\)\(([^()]*)\);", src):
        cc, name, ps = m.group(1), m.group(2), m.group(3).strip()
        n = 0 if ps in ("", "void") else ps.count(",") + 1
        if "..." in ps or cc is None:
            out[name] = 0
        else:
            out[name] = 4 * (n - 1 if cc == "ENG_THISCALL" else n)
    res = {}
    for name, pop in out.items():
        v = re.search(r"(?:^h|_)([0-9a-f]{6})$", name)
        if v:
            res[name] = (int(v.group(1), 16), pop)
    return res


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.path.join(ROOT, "build", "rhost", "rhost.exe"))
    ap.add_argument("--edition", default="enold")
    ap.add_argument("-v", action="store_true", help="also list the hooks retail's export shows no ret for")
    a = ap.parse_args()
    rd = reader(a.exe)
    syms = symbols(a.exe)
    hooks = hook_table(a.exe, rd)
    rr = retail_rets(a.edition)
    bad, unknown, ok = [], [], 0
    for name, va, wr in hooks:
        if not va or not wr:
            continue
        w = wrapper_rets(a.exe, syms, wr)
        r = rr.get(va)
        if not r or not w:
            unknown.append((name, va, sorted(w), sorted(r or ())))
            continue
        if w != r:
            bad.append((name, va, sorted(w), sorted(r)))
        else:
            ok += 1
    for name, va, w, r in bad:
        print(f"MISMATCH {name:28s} {va:#08x}: wrapper ret {w} vs retail retn {r}")
    if a.v:
        for name, va, w, r in unknown:
            print(f"unchecked {name:28s} {va:#08x}: wrapper ret {w} retail retn {r}")
    print(f"hook_abi: {len(hooks)} hooks, {ok} agree, {len(bad)} mismatch, {len(unknown)} unchecked "
          f"(no ret in retail's export range / the wrapper)")
    sbad, sok, sun = 0, 0, []
    for name, (va, pop) in sorted(slot_pops().items(), key=lambda x: x[1][0]):
        r = rr.get(va)
        if not r:
            sun.append(name)
        elif r != {pop}:
            print(f"MISMATCH slot g_eng.{name:20s} {va:#08x}: its C type pops {pop} vs retail retn {sorted(r)}")
            sbad += 1
        else:
            sok += 1
    print(f"hook_abi: {sok + sbad + len(sun)} engine slots, {sok} agree, {sbad} mismatch, {len(sun)} unchecked"
          + (f" ({', '.join(sun)})" if a.v and sun else ""))
    return 1 if bad or sbad else 0


if __name__ == "__main__":
    sys.exit(main())
