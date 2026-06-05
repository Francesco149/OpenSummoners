# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
██░░░░░░░░░░░░░░░░░░  12.3% touched   (12.3% host-tested, 13.5% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   192 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **197** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1561 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (13.5% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path toward a trace
  that plays 1:1 pixel-perfect frame by frame on both sides. Milestone map: `ROADMAP.md`.
  Mechanical next chip: `port-frontier.md`.
- **Where we are (ckpt 68): 24bpp parallax LUT grade LANDED — sky colour USER-CONFIRMED.**
  Found retail grades the 24bpp sky/mountain banks (`0x55`/`0x58`/`0x59`) at **DECODE**, not via
  the palette path (`0x417c40` early-exits to the plain getter when a bank has no palette): its
  **flag-3 branch** (the 24bpp case) stamps the slot's brightness descriptor (`f_08=1`, scales
  1000 = tint case 0, `f_18`=tone-curve LUT) before the getter, and the lazy `ar_sprite_decode`
  runs `ar_sheet_decode_pixels` (already ported, quirk #46). The port's parallax sink used the
  plain getter so never stamped it → sky decoded raw/too-bright. **Fix:** `game_arm_parallax_grade`
  in `main.c` replicates the stamp in `game_parallax_blit`. Verified: raw sky `(66,150,255)`→LUT
  →565 = `(33,125,239)`; **blue `239` matches retail's main sky band exactly**, and the user
  confirmed the grade looks correct on the feed. (The old finding's raw `(132,186,255)`/retail
  `(103,165,231)` numbers were wrong — actual raw is `(66,150,255)`.) **OPEN (deferred):** a
  "dark top gradient" the user sees in the establishing-scene frame (but not in settled gameplay)
  — likely a **per-scene CINEMATIC effect tied to the establishing shot**, to be confirmed by
  probing ground truth alongside the intro PAN RE.
- **Prior (ckpt 67):** backdrop TILES `differ_px==0` via the 8bpp palette grade (`color_grade.{c,h}`);
  the "establishing shot" proven a leftward **PAN not a zoom** (only `+0x60` moves; dx=0, same
  scale; `MAP_RENDER_CAM_TOWN_3F2`). **840 pass / 0 fail / 6 skip.** Ledger **194/1490 touched /
  189 tested**. Both GUI builds clean.
- **NOT a full `differ_px==0` frame yet — named residuals** (NOT logic bugs): the **NPC actors**
  (present modes 0/1/2, blocked on the entity/spawn system — PORT-DEBT `present-actor-modes`); the
  **foreground tree** + **"Town of Tonkiness" banner** (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`);
  the intro **pan** itself (`ingame-camera-snap`) — until it lands, port (gameplay cam) and retail
  (establishing-pan cam) sample different sky rows, so a true row-for-row sky diff isn't possible yet.
- **Next move:** port the **intro PAN** (`ingame-camera-snap` — animate `+0x60`) so port+retail
  share a camera and a flip-anchored full-frame backdrop diff becomes meaningful; then the
  **foreground tree/banner** (`0x5a00c0`) and the **actor renderers** (need the entity/spawn system
  first). Full writeup: `findings/in-game-intro.md` "The in-game COLOR-GRADE LUT".
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` carries `seq` + `CALL_TRACE_BEGIN/FIELD/END`;
  the Frida agent reads same-named retail fields per `tools/flow/retail_fields.json` (now incl.
  the `cam_*` camera chain + the ckpt-67 `tint`/`lutgate1/2`/`lut*` colour-grade probes);
  `tools/flow_diff.py` names the first `[chain]`/`[data]` divergence. Remaining Phase B: **B1**
  unified `scenario-test.py`, **B3** DDraw blit-command trace + `render_diff.py`.
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
