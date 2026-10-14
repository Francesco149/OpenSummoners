# OpenSummoners methodology reset — rhost + IDA (overnight, 2026-10-14)

Prepared overnight by Claude (Opus 5.5) for the project owner. Mirrors openrecet's 2026-10-13
reset (`../openrecet/docs/audits/2026-10-13-methodology-reset.md`). Evidence:
`docs/findings/in-process-retail-host.md`, `tools/rhost/README.md`, quirks #123–#128.
**§7 = what needs you.**

## TL;DR

1. **rhost works for SotES.** Retail `sotes.exe` runs *inside our own process* on the Windows box,
   deterministically, ~830–1000 fps single (title → new game → prologue → Tilelia → house →
   errands freeroam in ~5 s). No Frida, no pins, no lockstep injection, no wall clock.
2. **Determinism gate PASS** on every scenario (5 EN-old + EN-SE boot): 3 independent runs →
   identical state hash over all retail .data + heap. Record ≡ replay (a human recording replays to
   the same hash).
3. **Both editions:** EN-old (port target) and the **updated EN Steam SE** (buildid 24635888,
   2026-08-10 — newer than the July table) run under rhost; SE VAs come from an automatic IDA
   crosswalk.
4. **Port-vs-retail on identical state works.** Shadow: port `rand` 22,966 calls / 0 divergences,
   `input_poll_consume` 19,511 / 0. **Hybrid:** with both port functions *replacing* retail's, all 5
   scenarios end byte-identical to pure retail.
5. **Coverage map:** the intro executes 729 game functions (48.7% of game code bytes); only 27% of
   those bytes are port-touched — the port reproduces the intro mostly through stand-ins (cast
   tables etc.), not retail's scene code.
6. **IDA 9.4 export for all 3 builds** (EN-old 1824 fns, EN-SE 2027, JP-SE 2028; 0 decompile
   failures), plus a repo knowledge pack and **C++ class recovery** (595 classes / 1160 typed
   methods on EN-old) — which also restores `this` arguments Hex-Rays otherwise drops.
7. **Editions recommendation:** keep porting **EN-old**; treat SE as a measured delta layer later
   (rhost + crosswalk already make it cheap to start). §3.
8. **Coexists with openrecet:** separate runner task/queue/dirs; verified by running 3 openrecet
   rhost runs and our 5 at the same moment — both sides' hashes unchanged.

## 1. What was built

| piece | where | what |
|---|---|---|
| rhost | `tools/rhost/` | in-process retail host: fixed-address mapping, flip-locked virtual clock, fake SetTimer/WaitMessage pump, filtered WndProc, launcher emulation, fixed heap arena, fake kernel+GDI handles, external calls on a separate system stack, fake DInput7 keyboard, virtual BGM clock + silent DSound, file overlay, seed pin, state dumps/hashes, shots, coverage, shadow/replace hooks, write watchpoints, register traps, record mode |
| driver | `tools/rhost/rhost.py` | `setup boot run det corpus(--check/--bless) verify coverage record diff` — builds, stages, queues to the Windows runner, fetches results |
| Windows runner | `tools/rhost/win/` | `oss-rhost-runner` scheduled task (interactive session), `C:\oss-rhost\{bin,game,runs,queue}` |
| scenarios | `tests/scenarios/rh-*` | DIK-level traces + `expected.json` (frame/tick/rng/state_sem) |
| IDA export | `tools/ida/` | headless idat over ssh: pseudocode, disasm, call/data xrefs, strings, fingerprints, classes (outputs: `/mnt/ephemeral/opensummoners/decompiled-ida/<edition>/`, never committed) |
| crosswalk | `tools/ida/crosswalk.py` | EN-old ↔ EN-SE ↔ JP-SE function + global matching |
| knowledge pack | `tools/ida/gen_knowledge.py` | port provenance names, retail_fields names, thiscall protos, singletons, retail-layout structs → IDA |
| class recovery | `tools/ida/oss_ida_classes.py` | C++ classes without RTTI/vtables (sotes has none) |

## 2. Measurements

- **Speed:** boot→title ~1000 fps; full intro 4320 flips at 830 fps (system stack on; 1037 raw);
  3 parallel ≈ 340 each. Corpus (5 scenarios) ≈ 10–20 s wall incl. staging.
- **Determinism:** det PASS everywhere; volatile slots (system-DLL/COM pointers + HWND) calibrated
  per edition (EN-old 3464, EN-SE 192), masked by address only.
- **Root causes found + fixed** (each a retail fact, quirks #123–#127): GDI handle numbering in the
  heap; `FindResourceA(NULL)` map data (the exe's 59 MB .rsrc); the WM_TIMER/WaitMessage pump
  contract; retail **reads uninitialized stack** (palette byte 3 at 0x586010/0x417c40, move-command
  pad at 0x43f880) — system-DLL garbage reached the heap; BGM loops by polling DirectShow positions.
- **Coexistence:** openrecet ×3 + ours ×5 simultaneously → openrecet 3/3 identical, ours 5/5 =
  expected.json.
- **Oracle:** shadow rand 22,966/0, input_poll 19,511/0; replace both → 5/5 end states = pure.
- **Coverage:** 861/1824 fns hit; game 729/1550 = 48.7% of bytes; 513 hit fns (630 KB) unported.

## 3. Editions — both at once, or base first?

Facts (`crosswalk.py`, quirk #128): SE is the same engine **recompiled with changes** — 94% of
EN-old functions have an SE counterpart, but only 16% of EN-old bytes are identical/operand-only;
55% sit in functions with ≤5% instruction delta, 18% moderate, 2.5% major; 312 SE-only functions
(246 KB: voice manager, new title animation, …); some struct offsets moved (pump flag +0x1c→+0x20);
asset DLLs differ (`sotesd.dll`, plus `sotesd_en`/`sotesx_*` in SE). Of the 234 functions the port
touches, 162 are identical/operand-only in SE — but 79% of the ported *bytes* changed minor/moderate.

**Recommendation: keep EN-old as the parity target; add SE later as a delta layer.**
- Doing both now roughly doubles verification for every large function, while the port is at ~15%
  of bytes and the methodology itself is being reset.
- The 180 checkpoints of EN-old parity work stay valid.
- The delta is now *mechanical* to measure: for each ported function, run its shadow hook against
  SE's crosswalked counterpart under `rhost --edition ense-en`; pass ⇒ edition-agnostic; fail ⇒ an
  edition branch. Start once the port-side input/cadence work (§5) lands.
- Counter-argument (your call): SE is what Steam currently sells and what the mod loader targets; if
  the drop-in is meant primarily for SE owners, switching the target early is cheaper than late.

## 4. IDA vs Ghidra here + C++ analysis

- IDA 9.4 headless: 13–16 s auto-analysis, ~5 min full export per build, **0 decompile failures**
  (Ghidra timed out on the 46 KB `FUN_005a4770`; IDA decompiles it). FLIRT names the MSVC CRT
  (274 lib functions on EN-old). openrecet measured Ghidra garbling x87 math where IDA is exact.
- sotes is MSVC6 C++ built `/GR-` with **no virtual methods** (0 vtables) — RTTI/vtable tools
  (OOAnalyzer, ClassInformer) find nothing. `oss_ida_classes.py` instead unions `__thiscall`
  methods that pass their `this` on, binds global/heap instances, and turns every `this+off` access
  into per-class structs applied as typed `this`: **595 classes / 1160 methods** on EN-old (one
  over-merged 259-method "world" class is a known heuristic limit). Effects: consistent
  `this->field_XX` across a class, typed locals, and `this` arguments restored at call sites where
  plain Hex-Rays drops ECX (e.g. `sub_412DB0(0,0)` → `sub_412DB0(*v3,0,0)`).
- The knowledge pack applies the repo's names/types (0 decl errors): e.g. `input_dev *this`,
  `this->dev_obj`, `this->acquired` in the DInput wrapper. Names keep the VA suffix.

## 5. Proposed workflow (per function — openrecet's loop, adapted)

1. **Read:** `decompiled-ida/enold/all_ida.c` (+ `all_ida.asm`, `functions_ida.tsv` callers/callees,
   `data_xrefs.tsv`, `classes_ida.tsv`).
2. **Write** the C port with retail semantics (retail layout where the port already mirrors it).
3. **Verify vs retail on identical state:** add a wrapper + row in `tools/rhost/rh_hooks.c`; run
   `rhost.py verify` (shadow, per call) and `corpus --check --hook replace:NAME` (hybrid end state).
4. **Commit with an evidence line:** `rhost shadow: N calls / K scenarios, 0 divergences;
   replace: corpus --check PASS`.

First port ↔ retail probe (`rhost.py portshots rh-newgame-prologue --at …`; port at real-time 60 fps,
its title pacing is wall-clock): **f100, f640, f900 pixel-identical**; f700 9 px, f800 273 px (menu
quill phase); f300/f450 (logo + subtitle fade-in) and f1100/f1300 (prologue gem glow) differ — the
port runs AHEAD of deterministic retail through the title intro. Phase pillar, not logic: exactly what
the port-side determinism item below closes.

Phase-0 items for the port side (not done tonight):
- **DIK-level input path** in the port (the producer `0x46a880`: DIK → ring ids + held axis, ring
  timestamps = event time) so the port consumes the same `rh-*` traces.
- **Cadence decision** (§7 item 1), then frame-number alignment port ↔ rhost and a
  `rhost.py portshots` (retail | port | diff at the same flips).
- `.osr` capture from inside rhost (load `capture_proxy`'s engine hooks in-process) so Trace Studio
  diffs deterministic retail streams.
- Re-verify `docs/parity-ledger.md` "confirmed 1:1" claims under rhost (openrecet found several
  earlier claims did not hold).
- Transparent shadow hook (exact end-state equality in shadow mode too).

## 6. Risks / caveats

- Only EN-old boot→errands and SE boot are exercised; dungeons/battles/saves untested under rhost.
- The 16 ms/flip clock is a *model*: real retail cadence is wall-clock dependent (quirk #75).
- More uninitialized-read sites will surface as more port code runs in hybrid (census recipe in the
  rhost README); each is neutralized with a documented patch.
- Class recovery is heuristic (over-merging via shared helpers; some spurious instance bindings).
- `C:\oss-rhost\game\*` are copies of your install (never redistributed; rhost never writes them).

## 7. What needs you (priority order)

**Decisions**
1. **Deterministic cadence for parity:** rhost = 1 update per presented frame (16 ms/flip).
   The port's freeroam replay pins `now = sim_tick × 33 ms` ("2 flips per tick" from real-time
   captures). Pick one model for port↔retail; I recommend rhost's 1:1 (simplest, deterministic,
   already the port's present loop).
2. **Editions:** confirm "EN-old first, SE as a delta layer later" (§3) or switch the target.
3. **Retire the Frida retail drive?** (`frida_capture.py --lockstep`, pins) — rhost supersedes it
   for retail-side truth; keep Trace Studio for visuals. CLAUDE.md now points at rhost as the
   verification foundation; the old sections are kept, marked legacy.

**Quick checks (≤5 min each)**
4. See rhost yourself — the intro scenario in a visible window at 60 fps (~75 s; drop `--extra`
   for the ~5 s turbo replay):
   `python3 tools/rhost/rhost.py run rh-intro-errands --show --extra=--realtime`
5. Record a scenario by playing (window appears on the desktop; close it to stop; Enter = confirm):
   `python3 tools/rhost/rhost.py record rh-my-play` → then `rhost.py det rh-my-play`.
   Keyboard only (no gamepad recording yet). A save-based start: `--save <user dir or .tgz>`.
6. SE intro looks right? `python3 tools/rhost/rhost.py boot --edition ense-en --frames 1500 --shot-every 100`
   (PNGs in the run dir).

## Appendix — where things are

| item | location |
|---|---|
| rhost + README | `tools/rhost/` |
| findings | `docs/findings/in-process-retail-host.md`, quirks #123–#128 |
| run results | `/mnt/ephemeral/opensummoners/rhost-runs/<batch>/<run>/` |
| IDA exports | `/mnt/ephemeral/opensummoners/decompiled-ida/{enold,ense-en,ense-jp}/`, plain baseline in `plain/` |
| crosswalks | `/mnt/ephemeral/opensummoners/decompiled-ida/plain/crosswalk_*.tsv` |
| IDA DBs (Windows) | `C:\oss-ida\<edition>\*.i64`; re-export: `python3 tools/ida/ida.py export [--kb] [--fresh]` |
| SE unpacked exes | `/mnt/ephemeral/opensummoners/editions/` |
| Windows rhost | `C:\oss-rhost\`, task `oss-rhost-runner` |
