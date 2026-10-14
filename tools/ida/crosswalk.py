#!/usr/bin/env python3
"""crosswalk.py — match functions between two SotES editions from the IDA fingerprints.

  python3 tools/ida/crosswalk.py enold ense-en [--out DIR]

Inputs: <IDA_DIR>/<ed>/fingerprints.jsonl (tools/ida/oss_ida_export.py). Output (derived from
the user's binaries → never committed): <out>/crosswalk_<a>_<b>.tsv
  a_va  b_va  method  same  a_size  b_size  a_name  b_name
method: bytes (masked instruction bytes identical, unique) / mnem (mnemonic sequence identical,
unique) / graph (unique candidate among callees/callers of matched pairs) / order (aligned by
linker order between matched anchors, similar shape) ; same=1 when the masked bytes are equal.
Prints a summary: matched %, identical %, changed, only-in-a, only-in-b (by count + bytes).
"""
import argparse
import collections
import json
import os

IDA_DIR = os.environ.get("OSS_IDA_LOCAL", "/mnt/ephemeral/opensummoners/decompiled-ida")


def load(ed):
    fs = [json.loads(l) for l in open(os.path.join(IDA_DIR, ed, "fingerprints.jsonl"))]
    return {f["va"]: f for f in fs}


def shape_sim(x, y):
    """0..1 similarity of two fingerprints (size, blocks, edges, strings, consts, callee count)"""
    def r(a, b):
        return 1.0 if a == b else min(a, b) / max(a, b) if max(a, b) else 1.0
    s = r(x["size"], y["size"]) * 2 + r(x["nb"], y["nb"]) + r(x["ne"], y["ne"]) + r(x["ninsn"], y["ninsn"]) * 2
    s += r(len(x["callees"]), len(y["callees"])) + r(x["nglob"], y["nglob"])
    sx, sy = set(x["strs"]), set(y["strs"])
    s += (len(sx & sy) / len(sx | sy)) * 2 if (sx or sy) else 2
    cx, cy = set(x["consts"]), set(y["consts"])
    s += (len(cx & cy) / len(cx | cy)) if (cx or cy) else 1
    return s / 11.0


def crosswalk(A, B):
    m = {}          # a_va -> (b_va, method)
    rb = {}         # b_va -> a_va

    def add(a, b, how):
        if a in m or b in rb:
            return False
        m[a] = (b, how); rb[b] = a
        return True

    for key, how in (("bhash", "bytes"), ("mhash", "mnem")):
        ia, ib = collections.defaultdict(list), collections.defaultdict(list)
        for f in A.values():
            if f["va"] not in m:
                ia[f[key]].append(f["va"])
        for f in B.values():
            if f["va"] not in rb:
                ib[f[key]].append(f["va"])
        for k, va in ia.items():
            vb = ib.get(k, [])
            if len(va) == 1 and len(vb) == 1:
                add(va[0], vb[0], how)
            elif len(va) == len(vb) and len(va) > 1 and key == "bhash":
                # identical duplicates (thunks, tiny stubs): pair them in address order
                for a, b in zip(sorted(va), sorted(vb)):
                    add(a, b, how)
    # callers index
    callers_a, callers_b = collections.defaultdict(set), collections.defaultdict(set)
    for f in A.values():
        for c in f["callees"]:
            callers_a[c].add(f["va"])
    for f in B.values():
        for c in f["callees"]:
            callers_b[c].add(f["va"])
    changed = True
    while changed:
        changed = False
        for a, (b, _) in list(m.items()):
            for na, nb in ((A[a]["callees"], B[b]["callees"]), (callers_a[a], callers_b[b])):
                ua = [x for x in na if x not in m and x in A]
                ub = [x for x in nb if x not in rb and x in B]
                if not ua or not ub:
                    continue
                for x in ua:
                    best = sorted(((shape_sim(A[x], B[y]), y) for y in ub), reverse=True)
                    if best and best[0][0] >= 0.75 and (len(best) == 1 or best[0][0] - best[1][0] >= 0.1):
                        if add(x, best[0][1], "graph"):
                            changed = True
                            ub.remove(best[0][1])
    # linker order: between consecutive matched anchors, align the unmatched runs in order
    sa = sorted(A); sb = sorted(B)
    pos_b = {v: i for i, v in enumerate(sb)}
    anchors = sorted((a, m[a][0]) for a in m)
    prev_ia, prev_ib = -1, -1
    ia_of = {v: i for i, v in enumerate(sa)}
    for a, b in anchors + [(None, None)]:
        ia = ia_of[a] if a is not None else len(sa)
        ib = pos_b[b] if b is not None else len(sb)
        if ia > prev_ia and ib > prev_ib:
            run_a = [v for v in sa[prev_ia + 1:ia] if v not in m]
            run_b = [v for v in sb[prev_ib + 1:ib] if v not in rb]
            j = 0
            for x in run_a:
                for k in range(j, len(run_b)):
                    if shape_sim(A[x], B[run_b[k]]) >= 0.7:
                        add(x, run_b[k], "order"); j = k + 1
                        break
        if a is not None:
            prev_ia, prev_ib = ia, ib
    return m


def data_crosswalk(ea, eb, m):
    """globals: vote by the matched functions that reference them (same access kind)"""
    def refs(ed):
        r = collections.defaultdict(collections.Counter)   # global -> Counter((func, kind))
        for l in open(os.path.join(IDA_DIR, ed, "data_xrefs.tsv")).readlines()[1:]:
            t, _, f, _, k = l.rstrip("\n").split("\t")
            r[int(t, 16)][(int(f, 16), k)] += 1
        return r
    ra, rb = refs(ea), refs(eb)
    by_func_b = collections.defaultdict(lambda: collections.defaultdict(collections.Counter))   # func -> kind -> Counter(global)
    for g, c in rb.items():
        for (f, k), n in c.items():
            by_func_b[f][k][g] += n
    out = {}
    for g, c in ra.items():
        votes = collections.Counter()
        for (f, k), n in c.items():
            if f in m:
                fb = m[f][0]
                cand = by_func_b.get(fb, {}).get(k, {})
                for gb, nb in cand.items():
                    if nb == n:
                        votes[gb] += 2
                    else:
                        votes[gb] += 1
        if votes:
            (best, v), *rest = votes.most_common(2) + [(None, 0)]
            if v >= 2 and (not rest or rest[0][1] < v):
                out[g] = (best, v, sum(c.values()))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("a"); ap.add_argument("b")
    ap.add_argument("--out", default=IDA_DIR)
    a = ap.parse_args()
    A, B = load(a.a), load(a.b)
    m = crosswalk(A, B)
    out = os.path.join(a.out, f"crosswalk_{a.a}_{a.b}.tsv")
    same = 0; same_bytes = 0
    by = collections.Counter()
    with open(out, "w") as f:
        f.write("a_va\tb_va\tmethod\tsame\ta_size\tb_size\ta_name\tb_name\n")
        for va in sorted(m):
            b, how = m[va]
            s = int(A[va]["bhash"] == B[b]["bhash"])
            same += s; same_bytes += A[va]["size"] if s else 0
            by[how] += 1
            f.write(f"{va:08x}\t{b:08x}\t{how}\t{s}\t{A[va]['size']}\t{B[b]['size']}\t{A[va]['name']}\t{B[b]['name']}\n")
    ta = sum(f["size"] for f in A.values()); tb = sum(f["size"] for f in B.values())
    ma = sum(A[v]["size"] for v in m); mb = sum(B[m[v][0]]["size"] for v in m)
    game_a = [v for v in A if not A[v]["lib"]]
    print(f"{a.a}: {len(A)} fns {ta} B   {a.b}: {len(B)} fns {tb} B")
    print(f"matched {len(m)} = {100 * len(m) / len(A):.1f}% of {a.a} fns ({100 * ma / ta:.1f}% bytes), {100 * mb / tb:.1f}% of {a.b} bytes; by method {dict(by)}")
    print(f"identical (masked bytes) {same} fns = {100 * same_bytes / ta:.1f}% of {a.a} bytes; changed {len(m) - same}")
    print(f"game fns (non-FLIRT) in {a.a}: {len(game_a)}, matched {sum(1 for v in game_a if v in m)}, identical {sum(1 for v in game_a if v in m and A[v]['bhash'] == B[m[v][0]]['bhash'])}")
    ob = [v for v in B if v not in {x[0] for x in m.values()}]
    print(f"only in {a.a}: {len(A) - len(m)} fns {ta - ma} B; only in {a.b}: {len(ob)} fns {sum(B[v]['size'] for v in ob)} B")
    print(f"→ {out}")
    dm = data_crosswalk(a.a, a.b, m)
    dout = os.path.join(a.out, f"crosswalk_data_{a.a}_{a.b}.tsv")
    with open(dout, "w") as f:
        f.write("a_va\tb_va\tvotes\tnrefs\n")
        for g in sorted(dm):
            f.write(f"{g:08x}\t{dm[g][0]:08x}\t{dm[g][1]}\t{dm[g][2]}\n")
    print(f"globals: {len(dm)} mapped → {dout}")


if __name__ == "__main__":
    main()
