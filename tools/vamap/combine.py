#!/usr/bin/env python3
"""Join the pairwise edition maps into one multi-edition VA table.

Hub = EN-SE-EN (`ense`): it bridges to EN-SE-JP (`jpse`, ~97%) and to EN-old
(`enold`, ~69%, our decompile target).  Emits one row per distinct function with
whatever VAs are known in each edition, the best symbol name, the match method +
similarity per edge, and a 3-way cross-check.  Functions with no ense partner
(unique to jpse or enold) are appended so nothing is silently dropped.

Usage:
  combine.py --jpse-ense jpse-ense.csv --ense-enold ense-enold.csv \
             --jpse-enold jpse-enold.csv --out vamap.csv
"""
import argparse, csv


def load_pair(path):
    """{a_va:(b_va,method,sim)}, {b_va:(a_va,method,sim)}, {va:name} for matched rows."""
    fwd, rev, names = {}, {}, {}
    with open(path) as f:
        r = csv.reader(f)
        next(r)
        for row in r:
            a, b, name, method, sim = row[0], row[1], row[2], row[3], row[4]
            if a and b and not method.startswith("UNIQUE"):
                fwd[a] = (b, method, sim)
                rev[b] = (a, method, sim)
            if a:
                names.setdefault(a, name)
            elif b:
                names.setdefault(b, name)
    return fwd, rev, names


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--jpse-ense", required=True)
    ap.add_argument("--ense-enold", required=True)
    ap.add_argument("--jpse-enold", required=True)
    ap.add_argument("--out", required=True)
    a = ap.parse_args()

    je_fwd, je_rev, je_names = load_pair(a.jpse_ense)      # jpse -> ense
    ee_fwd, ee_rev, ee_names = load_pair(a.ense_enold)     # ense -> enold
    je2_fwd, _, _            = load_pair(a.jpse_enold)     # jpse -> enold (cross-check)

    all_ense = set(je_rev) | set(ee_fwd) | set(ee_names)
    rows = []
    seen_jpse, seen_enold = set(), set()

    for ense in sorted(x for x in all_ense if x):
        jp = je_rev.get(ense)
        eo = ee_fwd.get(ense)
        jpse = jp[0] if jp else ""
        enold = eo[0] if eo else ""
        name = ee_names.get(ense) or je_names.get(jpse) or ""
        agree = ""
        if jpse and enold:
            agree = "ok" if je2_fwd.get(jpse, ("",))[0] == enold else "DIFFER"
        rows.append([enold, ense, jpse, name,
                     (jp[1] if jp else ""), (jp[2] if jp else ""),
                     (eo[1] if eo else ""), (eo[2] if eo else ""), agree])
        if jpse: seen_jpse.add(jpse)
        if enold: seen_enold.add(enold)

    for jpse, nm in je_names.items():
        if jpse and jpse not in seen_jpse and jpse not in je_fwd:
            rows.append(["", "", jpse, nm, "UNIQUE-jpse", "", "", "", ""])
    for enold in ee_names:
        if enold and enold not in seen_enold and enold not in ee_rev:
            rows.append([enold, "", "", ee_names[enold], "", "", "UNIQUE-enold", "", ""])

    rows.sort(key=lambda r: int(r[1], 16) if r[1] else (int(r[0], 16) if r[0] else 1 << 40))
    with open(a.out, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["enold_va", "ense_va", "jpse_va", "name",
                    "jp~en_method", "jp~en_sim", "en~old_method", "en~old_sim", "3way"])
        w.writerows(rows)

    n3 = sum(1 for r in rows if r[0] and r[1] and r[2])
    differ = sum(1 for r in rows if r[8] == "DIFFER")
    print(f"[combine] {len(rows)} rows; {n3} present in all three editions; "
          f"{differ} 3-way disagreements")


if __name__ == "__main__":
    main()
