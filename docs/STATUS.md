# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
██░░░░░░░░░░░░░░░░░░  11.9% touched   (11.9% host-tested, 13.2% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   186 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **191** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1567 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (13.2% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 4–5 — porting the **in-game town backdrop** render path (the first
  in-game visible frame), building toward a trace that plays 1:1 pixel-perfect frame by
  frame on both sides. Milestone map: `ROADMAP.md`. Mechanical next chip:
  `port-frontier.md`.
- **Where we are (ckpt 64):** the **decode → grid → geometry → draw-list → present**
  chain is CLOSED, and the **camera/view object is now RE'd + the first-frame value
  live-probed** (ckpt 64 — the last non-wiring blocker, resolved). `map_decode` places
  tiles in the runtime render grid (`map_grid`), `map_render_walk` + `draw_pool` (the
  27-layer pool, `0x4917b0`/`0x586010`) accumulate the draw list, **`map_present`**
  (`0x48eac0` + projector `0x490b90`) walks the 27 layers → mode-3 backdrop → zdd blits.
  **Camera:** `view = *(room_state+0x104c)` = `operator_new(0x78)` (the 0x4017d0 ctor);
  the room-entry init (`586010:854-872` + `587d30`) is portable (`map_render_camera_init`),
  and the opening town's first frame is the live-verified constant `MAP_RENDER_CAM_TOWN_3F2`
  (`+0x60=128000`/`+0x5c=12800`, vp 64000×48000; window cols 39-60 / rows 3-18). All pure
  + host-tested: **821 pass / 0 fail / 6 skip**. Ledger **191/1490 touched / 186 tested**.
  Both GUI builds clean; the new modules are in the `src` wildcard but **not yet called by
  `main.c`**.
- **Next move:** **real town-backdrop pixels — only the wiring remains** (the camera is no
  longer a blocker). Wire `map_decode` + `map_render_walk` + `map_present(cam=MAP_RENDER_CAM_TOWN_3F2)`
  into `main.c`'s in-game `game_render`, with a real sprite resolver (`0x418470` /
  `&DAT_008a760c`), the EXE-NULL banks `0x570-0x572`, and the `0x586010` sim slice that
  populates the grid + builds the `view+0x54` layer table; then diff vs `runs/tas-ingame-1`
  anchored on `game_enter` (golden first town frame ~flip 1150). Deferred: present modes
  0/1/2 (actor draws) + the spawn-snap/intro-pan (PORT-DEBT `ingame-camera-snap`). Full
  writeup: `findings/in-game-intro.md` "The camera/view object".
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
