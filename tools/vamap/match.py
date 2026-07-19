#!/usr/bin/env python3
"""Cross-edition VA mapper for the SotES engine family.

Consumes the per-edition fingerprint JSONL from
tools/ghidra-scripts/ExportFuncFingerprints.java and emits a 1:1 function
correspondence between two editions — a BinDiff-style structural match tuned for
this near-identical, same-compiler (MSVC ~2012) family where the editions differ
only by tiny code shifts + a handful of localizer edits.

Match pipeline (each phase only ever claims a still-unclaimed pair):
  1. anchors   — fingerprints that are unique on BOTH sides and equal:
                 fine hash `sh` (mnemonics+operand-class+scalars, addr-masked),
                 then string-set, then coarse mnemonic hash `mh`.
  2. gap-fill  — between two consecutive anchors the leftover functions sit in the
                 SAME VA order on both sides (compiler preserves order); pair them
                 positionally when the counts match, else by best (mh,size).
  3. callgraph — propagate along matched calls[]: aligned call sites map callees.
Anything left unmatched is reported as unique-to-edition (localizer add/remove).

Usage:
  match.py A.jsonl B.jsonl --out out.csv [--tagA jp --tagB en]
  match.py --combine  (see gen_vamap.sh — builds the multi-edition table)
"""
import argparse, csv, json, sys
from collections import defaultdict


def load(path):
    funcs = {}
    order = []
    with open(path) as f:
        for line in f:
            d = json.loads(line)
            d["ival"] = int(d["va"], 16)
            funcs[d["va"]] = d
            order.append(d["va"])
    order.sort(key=lambda v: funcs[v]["ival"])
    return funcs, order


def uniq_index(funcs, keyfn, skip):
    idx = defaultdict(list)
    for va, d in funcs.items():
        if va in skip:
            continue
        k = keyfn(d)
        if k is not None:
            idx[k].append(va)
    return idx


def match(A, ordA, B, ordB):
    mA, mB = {}, {}          # va -> (other_va, method)

    def take(a, b, method):
        if a in mA or b in mB:
            return False
        mA[a] = (b, method)
        mB[b] = a
        return True

    # ---- phase 1: anchors on globally-unique, equal fingerprints ----
    for method, keyfn in (
        ("sh",     lambda d: d["sh"] if d["nins"] >= 3 else None),
        ("strset", lambda d: tuple(d["strs"]) if d["strs"] else None),
        ("mh",     lambda d: d["mh"] if d["nins"] >= 4 else None),
    ):
        ia = uniq_index(A, keyfn, mA)
        ib = uniq_index(B, keyfn, mB)
        for k, va in ia.items():
            vb = ib.get(k)
            if len(va) == 1 and vb and len(vb) == 1:
                take(va[0], vb[0], "anchor-" + method)

    # ---- phase 2: order-preserving gap fill between consecutive anchors ----
    # anchors in A-order and their B partners; the spans between them line up.
    anchorsA = [va for va in ordA if va in mA]
    # sentinel-bracket the whole range
    def gapfill():
        filled = 0
        prev_a = prev_b = None
        bpos = {vb: i for i, vb in enumerate(ordB)}
        apos = {va: i for i, va in enumerate(ordA)}
        bounds = [(-1, -1)] + [(apos[a], bpos[mA[a][0]]) for a in anchorsA] + [(len(ordA), len(ordB))]
        for (a0, b0), (a1, b1) in zip(bounds, bounds[1:]):
            gapA = [ordA[i] for i in range(a0 + 1, a1) if ordA[i] not in mA]
            gapB = [ordB[i] for i in range(b0 + 1, b1) if ordB[i] not in mB]
            if not gapA or not gapB:
                continue
            if len(gapA) == len(gapB):
                lone = len(gapA) == 1        # exactly one unmatched fn each side, anchor-bracketed
                for a, b in zip(gapA, gapB):
                    # accept if plausibly the same fn: same structure, close size, close
                    # instruction count (~5%), or a lone anchor-bracketed pair.  The last
                    # recovers localizer-EDITED functions (differ too much to hash-match but
                    # sit alone between two confident anchors) — e.g. the audio-bank loader.
                    nins_ok = A[a]["nins"] and abs(A[a]["nins"] - B[b]["nins"]) <= max(3, A[a]["nins"] // 20)
                    if A[a]["mh"] == B[b]["mh"] or A[a]["sh"] == B[b]["sh"] or \
                       abs(A[a]["size"] - B[b]["size"]) <= 2 or nins_ok or lone:
                        method = "gap-bracket" if (lone and A[a]["mh"] != B[b]["mh"]) else "gap-order"
                        if take(a, b, method):
                            filled += 1
            else:
                # unequal: greedily pair by identical mh within the gap, in order
                bi = 0
                for a in gapA:
                    for j in range(bi, len(gapB)):
                        b = gapB[j]
                        if b in mB:
                            continue
                        if A[a]["mh"] == B[b]["mh"] and (A[a]["nins"] >= 4 or A[a]["strs"] == B[b]["strs"]):
                            if take(a, b, "gap-mh"):
                                bi = j + 1
                                filled += 1
                            break
        return filled

    gapfill()

    # ---- phase 3: call-graph propagation to fixpoint ----
    changed = True
    while changed:
        changed = False
        for a, (b, _) in list(mA.items()):
            ca, cb = A[a]["calls"], B[b]["calls"]
            if len(ca) != len(cb) or not ca:
                continue
            for x, y in zip(ca, cb):
                if x in A and y in B and x not in mA and y not in mB:
                    if A[x]["mh"] == B[y]["mh"] or A[x]["sh"] == B[y]["sh"]:
                        if take(x, y, "callgraph"):
                            changed = True

    # ---- phase 4: similarity — pair leftovers by bottom-k Jaccard, mutual-best ----
    # recovers heavily-relocated / moderately-EDITED functions that no exact hash caught.
    ua = [va for va in ordA if va not in mA and len(A[va]["sk"]) >= 4]
    ub = [vb for vb in ordB if vb not in mB and len(B[vb]["sk"]) >= 4]
    sA = {va: set(A[va]["sk"]) for va in ua}
    sB = {vb: set(B[vb]["sk"]) for vb in ub}

    def jac(a, b):
        u = sorted(sA[a] | sB[b])[:32]
        return sum(1 for x in u if x in sA[a] and x in sB[b]) / len(u) if u else 0.0

    def size_ok(a, b):
        na, nb = A[a]["nsh"] or 1, B[b]["nsh"] or 1
        return 0.4 <= na / nb <= 2.5
    bestB = {a: max(((jac(a, b), b) for b in ub if size_ok(a, b)), default=(0.0, None)) for a in ua}
    bestA = {b: max(((jac(a, b), a) for a in ua if size_ok(a, b)), default=(0.0, None)) for b in ub}
    for a in ua:
        s, b = bestB[a]
        if b and s >= 0.55 and bestA.get(b, (0, None))[1] == a:   # mutual best + decent sim
            take(a, b, "sim")

    # one more gap pass now that callgraph + similarity added anchors
    anchorsA = [va for va in ordA if va in mA]
    gapfill()
    return mA, mB


def jac_raw(ska, skb):
    """bottom-k Jaccard similarity (0..1) between two sketches, for reporting."""
    a, b = set(ska), set(skb)
    if not a or not b:
        return 1.0 if a == b else 0.0
    u = sorted(a | b)[:32]
    return round(sum(1 for x in u if x in a and x in b) / len(u), 3)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("A"); ap.add_argument("B")
    ap.add_argument("--tagA", default="A"); ap.add_argument("--tagB", default="B")
    ap.add_argument("--out", required=True)
    args = ap.parse_args()

    A, ordA = load(args.A)
    B, ordB = load(args.B)
    mA, mB = match(A, ordA, B, ordB)

    with open(args.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow([f"{args.tagA}_va", f"{args.tagB}_va", "name", "method", "sim",
                    f"{args.tagA}_size", f"{args.tagB}_size", "size_delta"])
        for va in ordA:
            if va in mA:
                vb, method = mA[va]
                nm = A[va]["name"]
                if nm.startswith("FUN_") and not B[vb]["name"].startswith("FUN_"):
                    nm = B[vb]["name"]
                w.writerow([va, vb, nm, method, jac_raw(A[va]["sk"], B[vb]["sk"]),
                            A[va]["size"], B[vb]["size"], B[vb]["size"] - A[va]["size"]])
        for va in ordA:
            if va not in mA:
                w.writerow([va, "", A[va]["name"], "UNIQUE-" + args.tagA, "",
                            A[va]["size"], "", ""])
        for vb in ordB:
            if vb not in mB:
                w.writerow(["", vb, B[vb]["name"], "UNIQUE-" + args.tagB, "",
                            "", B[vb]["size"], ""])

    methods = defaultdict(int)
    for _, (_, m) in mA.items():
        methods[m] += 1
    print(f"[{args.tagA}->{args.tagB}] matched {len(mA)}/{len(A)} "
          f"({100*len(mA)/len(A):.1f}%); {len(A)-len(mA)} unique-{args.tagA}, "
          f"{len(B)-len(mB)} unique-{args.tagB}")
    for m, n in sorted(methods.items(), key=lambda kv: -kv[1]):
        print(f"    {m:16} {n}")


if __name__ == "__main__":
    main()
