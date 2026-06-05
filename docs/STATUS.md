# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
██░░░░░░░░░░░░░░░░░░  12.1% touched   (12.1% host-tested, 13.3% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   188 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **193** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1565 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (13.3% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path (the first
  in-game visible frame), building toward a trace that plays 1:1 pixel-perfect frame by
  frame on both sides. Milestone map: `ROADMAP.md`. Mechanical next chip:
  `port-frontier.md`.
- **Where we are (ckpt 66): PARALLAX FAR-PLANE landed.** On top of the ckpt-65 wired
  backdrop (decode → grid → walk → present, `town_render.{c,h}` driven by `main.c`), the port
  now also draws the **sky + mountain parallax background** behind the tiles — where it
  previously rendered black. Live-verified in-game (port `game_enter@1116`, DATA 1022 town):
  frame 1200 shows the blue sky band (layer A bank `0x55`) + the mountains (layers C/B banks
  `0x58`/`0x59`) under the rooftops/walls/ivy. RE: producer `FUN_00490cd0` (free-roam, called
  first in the per-frame driver `0x48c150`) + its twin `0x499100`/`FUN_00499560`
  (establishing-shot path) read a 3-layer descriptor from the **grid front-header** written by
  the `0x587e00` prologue's `switch(room[0x44])` — town (area `0xd2`) → case 4. Ported pure +
  host-tested (`src/parallax.{c,h}`, `parallax_select`+`parallax_render`), wired via
  `town_render_parallax` (drawn before the tilemap, `0x490cd0` order) with a `zdd_object_blt_onto`
  (`0x5b9a40`) sink. **836 pass / 0 fail / 6 skip** (+9). Ledger **193/1490 touched / 188 tested**
  (+2: `0x490cd0`, `0x499560`). Both GUI builds clean.
- **NOT `differ_px==0` yet — named residuals** (NOT logic bugs): **foreground trees** +
  **dialogue/caption overlay** (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`, now narrowed —
  the parallax slice is retired); the **NPC actors** (present modes 0/1/2, PORT-DEBT
  `present-actor-modes`); retail's **zoomed-out intro establishing shot** at the hold (flip
  1150 vista zooms to 1:1 by ~1800; PORT-DEBT `ingame-establishing-zoom`); the per-sprite
  palette tint (`render-palette-tint` — the parallax uses the plain frame getter `0x418470`,
  not the palette-aware `0x417c40`). Backdrop TILE + parallax layers are asset+scale-correct.
- **Verification note (harness):** the saved retail input-trace **no longer navigates** under
  `--seed-pin --lockstep --no-turbo` — retail sits on the title (no scene anchors fire). The
  in-game retail re-drive is currently broken (title-menu input-injection black box); ckpt 66
  verified the parallax against the **existing golden** `runs/tas-ingame-1` (asset+scale, the
  ckpt-65 method) + the two-witness static RE. A new `--parallax-probe` is in place to live-
  confirm the descriptor once nav is restored. **This is the standing blocker for live in-game
  retail ground truth** (it doesn't block the port).
- **Next move:** the **actor renderers** (`0x491ae0` et al. → present modes 0/1/2 — the NPCs:
  Arche + co) then the **dialogue overlay** (`0x5a00c0` glyph pipeline). Also pin the
  **establishing-shot/zoom** relationship (so port + golden share a camera) to unlock a flip-
  anchored full-frame diff vs `runs/tas-ingame-1`. Full writeup: `findings/in-game-intro.md`
  "The PARALLAX far-plane".
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` now carries `seq` +
  `CALL_TRACE_BEGIN/FIELD/END`; the Frida agent reads same-named retail fields per
  `tools/flow/retail_fields.json`; `tools/flow_diff.py` names the first `[chain]`/`[data]`
  divergence (+ `--field-timeline`). First probe: `rng` (`DAT_008a4f94`) at the Flip
  (`0x5b8fc0`, the shared per-frame VA) — it already proved the title-sparkle RNG is
  **data-1:1** (both sides converge to `0x404a0a8f`), per-flip skew = the R3 title-pace
  (phase) pillar, not logic. Ckpt 64 added a `src:"chain"` field (global root + pointer
  hops, e.g. `*(*(0x8a9b50)+0x104c)+off`) and used it to live-probe the in-game camera
  (the `cam_*` fields). Remaining Phase B: **B1** unified `scenario-test.py`, **B3**
  DDraw blit-command trace + `render_diff.py`.
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.

> Hand-edited in `docs/FRONT.md`, injected here verbatim so it can't drift.

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `../CLAUDE.md` — the dense auto-loaded entry point (conventions, parity model, paths).
- `FRONT.md` — the hand-edited current front (source of the block above).
- `HANDOFF.md` — rolling current-checkpoint detail (module layout + open threads).
- `parity-model.md` — the multi-pillar model; `parity-ledger.md` — confirmed-1:1 guard.
- `port-debt.md` — synthetic/MVP shortcuts owed back.
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — how to work on this repo (read at session start).
- `PLAN.md` — goal, constraints, phased roadmap.
