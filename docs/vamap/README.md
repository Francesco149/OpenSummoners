# vamap — cross-edition VA map for the SotES engine family

A **BinDiff-style structural function matcher** that maps every function's VA across the
retail SotES builds, so RE done against one edition transfers to the others. Tuned for this
**near-identical, same-compiler (MSVC ~2012) family** — where the editions differ only by
tiny code shifts + a handful of localizer edits — and it **scans for similar (relocated /
slightly-edited) functions**, scoring how much each match changed.

## Why this and not BinDiff
BinDiff is the industry tool, but it needs the **BinExport** Ghidra plugin + the `bindiff`
binary, and **neither is packaged in our pinned nixpkgs** (checked: only `vbindiff`/`bsdiff`).
`radiff2`/`rz-diff` are available but weaker on MSVC C++. Since all editions are already
imported+analyzed in `ghidra/projects/opensummoners`, a Ghidra-driven matcher is both
**simpler and more precise here**: for a same-compiler pair the code is byte-identical modulo
relocations, so a relocation-invariant fingerprint match hits **99.6%** — better than generic
BinDiff, which trades exactness for cross-compiler robustness we don't need. It also plugs
into our annotation workflow (names carry across; `docs/decompiled/` VAs bridge in). If you
ever want BinDiff too, export each program with BinExport and diff — this map is the baseline.

## Editions
| tag | binary | engine |
|-----|--------|--------|
| `jpse`  | `vendor/unpacked/editions/sotes-ense-jp.exe`  | EN-SE bundled JP build (`sotes.exe`) — the voiced reference |
| `ense`  | `vendor/unpacked/editions/sotes-ense-en.exe`  | EN-SE English build (`sotes_en.exe`) — the ennse_voice patch target |
| `enold` | `vendor/unpacked/sotes.unpacked.exe`          | EN 2012 (Carpe Fulgur) — the **port's decompile target** (`docs/decompiled/`) |
| `jpdisc`| *(not yet — lzsotes/SPL-packed; see below)*   | JP retail CD (Lizsoft 2008-09) |

## Regenerate
```
nix develop --command bash tools/vamap/gen_vamap.sh
```
Exports fingerprints (`tools/ghidra-scripts/ExportFuncFingerprints.java`), matches every pair
(`match.py`), 3-way joins (`combine.py`). Outputs (gitignored — derived from the user's
binaries): `docs/vamap/{jpse,ense,enold}.jsonl`, `docs/vamap/<pair>.csv`, `docs/vamap/vamap.csv`.

## How the match works
Each function gets a relocation-invariant fingerprint: `mh` (mnemonic-only hash), `sh`
(mnemonics + operand-class + scalars, **ADDRESS operands masked** so relocations don't
perturb it but stable struct offsets/constants stay), referenced string literals, callee VAs,
and a **bottom-32 KMV sketch of 3-gram instruction shingles** for Jaccard similarity. The
matcher claims still-unclaimed pairs in confidence order:
1. **anchor** — a fingerprint unique on *both* sides + equal (`sh` → string-set → `mh`).
2. **gap-order / gap-bracket** — between two anchors the leftovers keep VA order; pair them
   positionally. A *lone* anchor-bracketed pair is matched even if it hash-differs — this
   recovers **localizer-EDITED** functions (they differ too much to hash but sit alone
   between confident anchors; e.g. the audio-bank loader where the voice load was removed).
3. **callgraph** — aligned call sites propagate callee identity.
4. **sim** — remaining leftovers paired by bottom-k Jaccard, mutual-best, ≥0.55.

## Reading the output (`vamap.csv`)
`enold_va, ense_va, jpse_va, name, jp~en_method, jp~en_sim, en~old_method, en~old_sim, 3way`
- **method** = confidence: `anchor-*` high, `gap-*`/`sim` structural, `UNIQUE-*` = no partner
  (localizer add/remove or a genuine miss).
- **sim** (0..1) = how identical the two bodies are. `sim<1` ⇒ a real edit; note the bottom-32
  sketch is coarse for huge functions (a one-line cut in a 13 k-instruction fn can still read
  1.0 — cross-check `size_delta`). Low sim on a correct match just means "heavily changed".
- **3way** = `ok`/`DIFFER` cross-check of jpse→enold direct vs via ense (DIFFER ⇒ review).

Match rates: **jpse↔ense 99.6%** (7/8 unique — the localizer's true adds/removes),
ense↔enold 79.7%, jpse↔enold 77.9% (lower: EN-old is a separate, older lineage). The
**UNIQUE lists are a feature** — they pinpoint exactly what the localizer changed.

## Voice-subsystem anchor table (this session's RE; see findings/ense-voice-combat-init.md)
| role | jpse | ense | enold |
|------|------|------|-------|
| bank_load (LoadLibraryA wrapper) | 0x5d6880 | 0x5d8b10 | 0x5b0890 |
| bank INIT (clears cluster) | 0x57f180 | 0x580ec0 | 0x562210 |
| bank LOADER (voice load cut in EN) | 0x5c94f0 | 0x5cb880 | 0x5a4770 |
| boot / main-loop (voice-MGR gate) | 0x57fe50 | 0x581ba0 | — |
| boot CALL site (ennse_voice hook) | — | **0x58113e** | — |
| actor builder (COMBAT-voice bake) | 0x423890 | 0x423850 | 0x4289f0 |
| dialogue trigger | 0x435050 | 0x435000 | 0x439690 |
| voice PLAY | 0x437db0 | 0x437dc0 | 0x43c1b0 |

## Adding the JP disc exe (`jpdisc`)
Not yet mapped: the JP retail-CD `sotes.exe` is **lzsotes/SPL-packed** (NOT SteamStub — Steamless
won't touch it), so it needs the Lizsoft/SPL unpacker first. Once unpacked and imported into
the Ghidra project, add `[jpdisc]=<name>.exe` to the `PROG` map in `gen_vamap.sh` and extend
`combine.py`; the matcher handles it unchanged. Expected to match `jpse` at very high rate
(same JP engine generation, possibly one revision older).
