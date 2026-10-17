#!/usr/bin/env python3
"""decomp2c_drift.py — which committed decomp2c functions would come out DIFFERENT from today's decomp2c.

    python3 tools/ida/decomp2c_drift.py [src/x.c ...] [--flags "--typed --zero-locals"] [--show]

A decomp2c module carries `/* 0xVA: <IDA head> */` before each function; this regenerates every module's functions
with the current tool (one decomp2c run per module, the same VA:name list, so cross-calls bind the same way) and
compares each function body with the committed one (whitespace-normalized).  A difference is either a hand finish
(the module header names those) or a decomp2c fix the module predates — a latent bug to re-check (the chapter
script 0x4d64f0's table address, found 2026-10-17)."""
import argparse
import difflib
import os
import re
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def funcs_of(text):
    """[(va, name, body)] in file order: a `/* 0xVA: ... */` comment, the signature line, the body to the closing `}`"""
    out = []
    for m in re.finditer(r"^/\* 0x([0-9a-f]{6}): [^\n]*?\*/\n[\w\s\*]*?\b(\w+)\([^\n]*\)\n\{\n", text, re.M):
        start, depth, i = m.start(), 0, m.end() - 2
        while i < len(text):
            if text[i] == "{":
                depth += 1
            elif text[i] == "}":
                depth -= 1
                if depth == 0:
                    break
            i += 1
        out.append((m.group(1), m.group(2), text[start:i + 1]))
    return out


def norm(s):
    return [l.strip() for l in s.split("\n") if l.strip()]


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("files", nargs="*")
    ap.add_argument("--flags", help="decomp2c flags (default: the combination that reproduces the module best)")
    ap.add_argument("--show", action="store_true", help="print the differing lines")
    a = ap.parse_args()
    files = a.files or sorted(os.path.join("src", f) for f in os.listdir(os.path.join(REPO, "src"))
                              if f.endswith(".c") and "A FIRST PASS through tools/ida/decomp2c.py" in open(os.path.join(REPO, "src", f), errors="replace").read(2000))
    total = 0
    for f in files:
        text = open(os.path.join(REPO, f), errors="replace").read()
        fs = funcs_of(text)
        if not fs:
            continue
        best = None
        for flags in ([a.flags.split()] if a.flags else [["--typed", "--zero-locals"], ["--zero-locals"], ["--typed"], []]):
            cmd = [sys.executable, os.path.join(REPO, "tools", "ida", "decomp2c.py")] + flags + [f"{va}:{n}" for va, n, _ in fs]
            r = subprocess.run(cmd, capture_output=True, text=True)
            if r.returncode:
                continue
            regen = {n: b for _, n, b in funcs_of(r.stdout)}
            bad = []
            for va, n, b in fs:
                g = regen.get(n)
                if g is None:
                    bad.append((va, n, ["(not regenerated)"]))
                elif norm(g) != norm(b):
                    d = [l for l in difflib.unified_diff(norm(b), norm(g), lineterm="", n=0) if l[:1] in "+-" and l[:3] not in ("+++", "---")]
                    bad.append((va, n, d))
            if best is None or sum(len(d) for *_, d in bad) < sum(len(d) for *_, d in best[1]):
                best = (flags, bad)
        if best is None:
            print(f"{f}: decomp2c failed")
            continue
        flags, bad = best
        total += len(bad)
        print(f"{f} [{' '.join(flags) or 'no flags'}]: {len(fs)} functions, {len(bad)} differ" + (": " + " ".join(f"0x{va}" for va, _, _ in bad) if bad else ""))
        if a.show:
            for va, n, d in bad:
                print(f"  --- 0x{va} {n} ({len(d)} lines)")
                for l in d[:40]:
                    print("    " + l[:200])
    return 1 if total else 0


if __name__ == "__main__":
    sys.exit(main())
