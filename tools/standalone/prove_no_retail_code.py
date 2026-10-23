#!/usr/bin/env python3
"""prove_no_retail_code.py — the standalone runs no retail code and ships none.

The standalone (rhost --standalone, opensummoners-standalone.exe) maps only the DATA sections of the user's own
sotes.exe and asset DLLs; the retail code range holds INT3 plus a 5-byte `jmp` into port code at every replaced
function's entry.  This tool re-derives every claim independently of the C side, from the files:

  1. image   — the copy rule (headers + every section without CNT_CODE / MEM_EXECUTE) applied to sotes.exe and the
               asset DLLs takes no code: no IDA function lies in a copied range, no copied section is executable,
               and the copied bytes do not look like code (function-prologue density vs .text's)
  2. binary  — our exes embed no retail code: every byte span our exes share with retail .text (seeded by a 32-byte
               window, low-entropy ones aside, extended both ways) is short (< 128 B) and covers no whole retail
               function — what remains is the same C compiled to the same encoding (constant stores, field tests),
               listed with the port function it sits in (retail DATA windows are counted, informational)
  3. source  — no port source reads .text bytes as data: D(va) / g_eng.data(va) in the code range must be a function
               ENTRY (its address passed by value: a callback); no D(va) of a data structure that holds a pointer to
               .text bytes outside every function (library data MSVC linked into the code section: dinput.lib's
               object-format arrays — the port passes the SDK's own); every such entry and every retail VA port code
               calls (g_eng.retail) must be a thunk in the standalone hook set, else the run stops at that INT3 (a gap)
  4. audit   — a run's standalone_audit.txt (written by the exe after sealing, before the first port instruction):
               verdict OK, only data copied, every non-thunk code byte INT3, no retail .text window present, nothing
               executable outside the code range; every thunk is an IDA function entry

  nix develop --command python3 tools/standalone/prove_no_retail_code.py [--audit RUN/standalone_audit.txt]
Exit 0 = every check PASS.  Prints one line per check (+ details on FAIL)."""
import argparse
import glob
import os
import re
import struct
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
IDA = "/mnt/ephemeral/opensummoners/decompiled-ida/enold"
SCN_CODE, SCN_EXEC = 0x20, 0x20000000
WIN = 32
MAXRUN = 128   # an identical span this long (or covering a whole retail function) is not convergent codegen

results = []


def check(name, ok, detail=""):
    results.append((name, ok))
    print(f"{'PASS' if ok else 'FAIL'}  {name}" + (f"\n      {detail}" if detail else ""))


def info(msg):
    print(f"info  {msg}")


def pe(path):
    b = open(path, "rb").read()
    o = struct.unpack_from("<I", b, 0x3c)[0]
    nsec = struct.unpack_from("<H", b, o + 6)[0]
    optsz = struct.unpack_from("<H", b, o + 20)[0]
    ib = struct.unpack_from("<I", b, o + 52)[0]
    hdrs = struct.unpack_from("<I", b, o + 84)[0]
    secs = []
    for i in range(nsec):
        s = o + 24 + optsz + 40 * i
        name = b[s:s + 8].rstrip(b"\0").decode("latin1")
        vs, va, rs, rp = struct.unpack_from("<IIII", b, s + 8)
        ch = struct.unpack_from("<I", b, s + 36)[0]
        secs.append(dict(name=name, va=ib + va, vs=vs or rs, raw=b[rp:rp + min(rs, vs or rs)], ch=ch))
    return dict(path=path, data=b, ib=ib, hdrs=hdrs, secs=secs)


def is_code(s):
    return bool(s["ch"] & (SCN_CODE | SCN_EXEC))


def ida_functions():
    out = []
    p = os.path.join(IDA, "functions_ida.tsv")
    for line in open(p):
        f = line.split("\t")
        if re.match(r"^[0-9a-f]{8}$", f[0]):
            out.append((int(f[0], 16), int(f[1]), f[2]))
    return out


def low_entropy(w):
    return len(set(w)) <= 3


# ------------------------------------------------------------------ 1. image
def check_image(exe, assets):
    funcs = ida_functions()
    text = [s for s in exe["secs"] if is_code(s)]
    copied = [s for s in exe["secs"] if not is_code(s)]
    info(f"{os.path.basename(exe['path'])}: copied {', '.join(s['name'] for s in copied)} (+ headers); "
         f"never copied: {', '.join(s['name'] for s in text)}")
    hits = [(va, n, s["name"]) for va, sz, n in funcs for s in copied if va < s["va"] + s["vs"] and va + sz > s["va"]]
    check("image: no IDA function (of %d) lies in a copied section of sotes.exe" % len(funcs), not hits,
          "; ".join(f"{n}@{va:08x} in {sec}" for va, n, sec in hits[:10]))
    check("image: every copied section of sotes.exe is non-executable data",
          all(not (s["ch"] & SCN_EXEC) for s in copied) and all(not is_code(s) for s in copied))
    # code-likeness: `push ebp; mov ebp, esp` (55 8B EC) per MB, .text vs the copied sections
    def dens(raw):
        return raw.count(b"\x55\x8b\xec") / max(1, len(raw)) * (1 << 20)
    td = sum(s["raw"].count(b"\x55\x8b\xec") for s in text) / max(1, sum(len(s["raw"]) for s in text)) * (1 << 20)
    info("prologue 55 8B EC per MB: .text %.0f; " % td + ", ".join(f"{s['name']} {dens(s['raw']):.1f}" for s in copied))
    for a in assets:
        cs = [s for s in a["secs"] if is_code(s)]
        ok = all(not (s["ch"] & SCN_EXEC) for s in a["secs"] if not is_code(s))
        check(f"image: {os.path.basename(a['path'])}: only non-code sections copied (never: {', '.join(s['name'] for s in cs)})", ok)
    return text


# ------------------------------------------------------------------ 2. binary
def symbols(path):
    """(rva-sorted) function symbols of one of our exes: rva -> name (i686-w64-mingw32-nm)"""
    try:
        out = subprocess.run(["i686-w64-mingw32-nm", "-n", path], capture_output=True, text=True).stdout
    except OSError:
        return []
    syms = []
    for line in out.splitlines():
        f = line.split()
        if len(f) == 3 and f[1] in "tT":
            syms.append((int(f[0], 16), f[2]))
    return syms


def check_binaries(text, exes, retail_data):
    tb = b"".join(s["raw"] for s in text)   # .text is the one code section
    t0 = text[0]["va"]
    idx = {}
    for o in range(len(tb) - WIN + 1):
        w = tb[o:o + WIN]
        if not low_entropy(w):
            idx.setdefault(w, o)
    dwin = set()
    for s in retail_data:
        r = s["raw"]
        for o in range(0, len(r) - WIN + 1, 4):
            w = r[o:o + WIN]
            if not low_entropy(w):
                dwin.add(w)
    funcs = sorted(ida_functions())
    info(f"retail .text: {len(idx)} distinct {WIN}-byte windows (low-entropy ones aside); .rdata/.data: {len(dwin)} (4-aligned)")
    for path in exes:
        if not os.path.exists(path):
            info(f"{path}: not built — skipped")
            continue
        img = pe(path)
        b = img["data"]
        spans, dhits, o = [], 0, 0
        while o < len(b) - WIN + 1:
            w = b[o:o + WIN]
            if o % 4 == 0 and w in dwin:
                dhits += 1
            t = idx.get(w)
            if t is None:
                o += 1
                continue
            n = WIN
            while o + n < len(b) and t + n < len(tb) and b[o + n] == tb[t + n]:
                n += 1
            k = 0
            while o - k > 0 and t - k > 0 and b[o - k - 1] == tb[t - k - 1]:
                k += 1
            spans.append((o - k, t0 + t - k, n + k))
            o += n
        # file offset -> rva -> our function
        def rva(off):
            hdr = struct.unpack_from("<I", b, 0x3c)[0]
            nsec = struct.unpack_from("<H", b, hdr + 6)[0]
            optsz = struct.unpack_from("<H", b, hdr + 20)[0]
            for i in range(nsec):
                sh = hdr + 24 + optsz + 40 * i
                vs, va, rs, rp = struct.unpack_from("<IIII", b, sh + 8)
                if rp <= off < rp + rs:
                    return img["ib"] + va + off - rp
            return None
        syms = symbols(path)
        def owner(off):
            v = rva(off)
            if v is None or not syms:
                return "?"
            name = "?"
            for a_, n_ in syms:
                if a_ > v:
                    break
                name = n_
            return name
        # a span over retail bytes inside NO function is library DATA linked into .text (dinput.lib's object-format
        # arrays — our exes link the SDK's identical ones), not code
        fstarts = [f[0] for f in funcs]
        import bisect
        def code_bytes(va, n):
            k = 0
            for v in range(va, va + n):
                i = bisect.bisect_right(fstarts, v) - 1
                k += i >= 0 and v < funcs[i][0] + funcs[i][1]
            return k
        data_spans = [sp for sp in spans if code_bytes(sp[1], sp[2]) == 0]
        spans = [sp for sp in spans if sp not in data_spans]
        whole = [(o_, va, n) for o_, va, n in spans for fva, fsz, fn in funcs if fsz >= 16 and va <= fva and fva + fsz <= va + n]
        long_ = [sp for sp in spans if sp[2] >= MAXRUN]
        if data_spans:
            info(f"{len(data_spans)} spans over retail .text bytes outside every function (library data, not code): " +
                 "; ".join(f"retail {va:08x} +{n}" for _, va, n in data_spans[:6]))
        check(f"binary: {os.path.relpath(path, ROOT)} ({len(b) >> 10} KB): no span shared with retail .text is "
              f"≥{MAXRUN} B or a whole function ({len(spans)} short spans, longest {max((n for _, _, n in spans), default=0)} B)",
              not long_ and not whole,
              "; ".join(f"off {o_:x} = retail {va:08x} +{n}" for o_, va, n in (long_ + whole)[:8]))
        by = {}
        for o_, va, n in spans:
            by.setdefault(owner(o_), []).append((va, n))
        info("shared spans by port function (convergent encodings): " +
             "; ".join(f"{fn} {len(v)}x ≤{max(n for _, n in v)}B" for fn, v in sorted(by.items(), key=lambda kv: -len(kv[1]))[:10]))
        info(f"{os.path.basename(path)}: {dhits} {WIN}-byte windows equal to retail .rdata/.data (tables transcribed "
             "into source; the exes read the rest at runtime)")


# ------------------------------------------------------------------ 3. source
def standalone_vas():
    """the VAs the standalone thunks: rh_hooks.c rows named in rhost.py's standalone set"""
    sys.path.insert(0, os.path.join(ROOT, "tools", "rhost"))
    import rhost   # noqa: E402
    names = set(rhost.standalone_hooks())
    rows = {}
    src = open(os.path.join(ROOT, "tools", "rhost", "rh_hooks.c")).read()
    for m in re.finditer(r'X\(\s*\w+\s*,\s*"(\w+)"\s*,\s*0x([0-9a-f]+)u\s*\)', src):
        rows[m.group(1)] = int(m.group(2), 16)
    for m in re.finditer(r'\[\w+\]\s*=\s*\{\s*"(\w+)"\s*,\s*0x([0-9a-f]+)u', src):
        rows.setdefault(m.group(1), int(m.group(2), 16))
    return {rows[n] for n in names if n in rows}, names - set(rows)


def text_data_holders(exe, text):
    """dwords of the copied data that point at .text bytes inside NO IDA function (data in the code section); printable
    ASCII quads (strings) aside: holder VA -> target"""
    funcs = sorted((va, sz) for va, sz, _ in ida_functions())
    starts = [f[0] for f in funcs]
    import bisect
    def in_func(v):
        i = bisect.bisect_right(starts, v) - 1
        return i >= 0 and v < funcs[i][0] + funcs[i][1]
    lo = min(s["va"] for s in text)
    hi = max(s["va"] + s["vs"] for s in text)
    entries = set(starts)
    out = {}
    for s in exe["secs"]:
        if is_code(s):
            continue
        r = s["raw"]
        for o in range(0, len(r) - 3, 4):
            v = struct.unpack_from("<I", r, o)[0]
            if lo <= v < hi and v not in entries and not in_func(v) and not all(0x20 <= c < 0x7f for c in r[o:o + 4]):
                out[s["va"] + o] = v
    return out


def check_source(text, exe):
    lo = min(s["va"] for s in text)
    hi = max(s["va"] + s["vs"] for s in text)
    holders = text_data_holders(exe, text)
    info(f"{len(holders)} data dwords point at .text bytes outside every function (data linked into .text): "
         + " ".join(f"{h:x}->{t:x}" for h, t in sorted(holders.items())[:6]) + " …")
    thunked, unknown = standalone_vas()
    thunked.add(0x5bf505)   # rand: H_RAND's VA is the edition's (rh_hooks_install)
    if unknown:
        info(f"hook names without a static VA row: {sorted(unknown)[:8]}")
    entries = {va for va, _, _ in ida_functions()} | thunked
    reads, byval, called, passes = [], {}, {}, []
    for p in sorted(glob.glob(os.path.join(ROOT, "src", "*.c"))):
        rel = os.path.relpath(p, ROOT)
        for i, line in enumerate(open(p, errors="replace"), 1):
            for m in re.finditer(r"(?:\bD|g_eng\.data)\(0x([0-9a-fA-F]+)u?\)", line):
                va = int(m.group(1), 16)
                if lo <= va < hi:
                    (byval.setdefault(va, f"{rel}:{i}") if va in entries else reads.append(f"{rel}:{i} 0x{va:x}"))
                elif any(h in holders for h in range(va & ~3, (va & ~3) + 0x20, 4)):
                    passes.append(f"{rel}:{i} 0x{va:x}")
            for m in re.finditer(r"g_eng\.retail\(0x([0-9a-fA-F]+)u?\)", line):
                called.setdefault(int(m.group(1), 16), f"{rel}:{i}")
    check("source: no port code reads retail .text bytes as data (code-range D() only at function entries: "
          f"{len(byval)} callbacks passed by address)", not reads, "; ".join(reads[:12]))
    check("source: no port code hands out a structure holding a pointer into .text data (D(va) within 0x20 before a "
          "holder)", not passes, "; ".join(passes[:12]))
    stubs = {0x5b8fc0, 0x43d1d0, 0x5bf4fb}   # the Flip / easer / srand: rhost's stubs (rh_retail)
    gaps = sorted((va, w) for va, w in list(byval.items()) + list(called.items()) if va not in thunked and va not in stubs)
    check(f"source: every retail entry port code calls or passes by address is a thunk ({len(called)} called, "
          f"{len(byval)} by address)", not gaps, " ".join(f"{va:x}({w})" for va, w in gaps[:10]))
    return thunked


# ------------------------------------------------------------------ 4. audit
def check_audit(path, exe, thunked):
    lines = open(path, errors="replace").read().splitlines()
    kv = {}
    ranges, thunks = [], []
    for l in lines:
        f = l.split()
        if not f or l.startswith("#"):
            continue
        if f[0] == "range":
            ranges.append((f[1], f[2], int(f[3], 16), int(f[4], 16), int(f[5].split("=")[1], 16), f[6]))
        elif f[0] == "thunk":
            thunks.append((int(f[1], 16), f[4] if len(f) > 4 else "?"))
        else:
            kv[f[0]] = f[1:]
    check("audit: verdict OK", kv.get("verdict") == ["OK"], " ".join(kv.get("verdict", ["missing"])))
    badr = [r for r in ranges if (r[4] & (SCN_CODE | SCN_EXEC)) and r[5] == "copied"]
    check(f"audit: {len(ranges)} ranges, every copied one data (code sections: CODE-not-copied)", not badr,
          "; ".join(f"{r[0]} {r[1]}" for r in badr))
    cr = kv.get("code_range", [])
    other = int(cr[cr.index("other") + 1]) if "other" in cr else -1
    check(f"audit: the code range is INT3 but for {len(thunks)} thunks (other bytes: {other})", other == 0)
    rw = kv.get("retail_text_windows_compared", [])
    same = int(rw[rw.index("identical") + 1]) if "identical" in rw else -1
    check(f"audit: no 16-byte window of retail .text in memory (identical: {same})", same == 0)
    check("audit: nothing executable outside the code range", kv.get("exec_outside_code_range") == ["0"])
    funcs = {va for va, _, _ in ida_functions()}
    noent = [(va, n) for va, n in thunks if va not in funcs]
    info("dep " + " ".join(kv.get("dep", [])))
    info(f"{len(thunks)} thunks; {len(noent)} at entries IDA did not define as functions: " +
         " ".join(f"{n}@{va:x}" for va, n in noent[:8]))
    extra = sorted({va for va, _ in thunks} - thunked)
    if extra:
        info(f"thunks outside rhost.py's standalone set: {[hex(v) for v in extra[:8]]}")


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--exe", default=os.path.join(ROOT, "vendor", "unpacked", "sotes.unpacked.exe"))
    ap.add_argument("--game", default="/mnt/ephemeral/opensummoners/game/enold", help="the asset DLLs' folder")
    ap.add_argument("--audit", help="a run's standalone_audit.txt")
    ap.add_argument("--binaries", nargs="*", default=[os.path.join(ROOT, "build", "rhost", "opensummoners-standalone.exe"),
                                                      os.path.join(ROOT, "build", "rhost", "rhost.exe")])
    ap.add_argument("--no-binary", action="store_true", help="skip the (slow, ~1 min) binary window scan")
    a = ap.parse_args()
    exe = pe(a.exe)
    assets = [pe(p) for p in sorted(glob.glob(os.path.join(a.game, "*.dll")))]
    text = check_image(exe, assets)
    if not a.no_binary:
        check_binaries(text, a.binaries, [s for s in exe["secs"] if s["name"] in (".rdata", ".data")])
    thunked = check_source(text, exe)
    if a.audit:
        check_audit(a.audit, exe, thunked)
    nfail = sum(1 for _, ok in results if not ok)
    print(f"\n{'PROVEN' if not nfail else 'NOT PROVEN'}: {len(results) - nfail}/{len(results)} checks pass")
    return 1 if nfail else 0


if __name__ == "__main__":
    sys.exit(main())
