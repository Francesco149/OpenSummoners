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
- **Where we are (ckpt 73): the actor-band residual is PINNED to the RNG pillar — and the
  shared LCG stream is NON-DETERMINISTIC run-to-run EVEN UNDER `--seed-pin`.** Ran the ckpt-72
  directed live check: drove retail **twice** (`--seed-pin --lockstep --no-turbo`, same
  in-game trace), snapshotting the LCG state `DAT_008a4f94` at the per-sim-tick actor-update
  boundary (`0x46cd70`, new `rng` field). **Result: `rng` matches 0/8643 in-game
  sim-ticks** — the stream is at a different phase every tick despite the pinned seed and the
  deterministic sim-tick index. **Smoking gun:** at `prologue_enter` BOTH runs are on the
  IDENTICAL flip 946 yet rng differs (`0x84654e6f` vs `0xa79a2d6e`) → at the same flip the
  engine has drawn a *different number* of LCG values. Mechanism: a consumer draws per-PRESENT
  and the presents-per-sim-tick count is non-deterministic (quirk #75), so the stream phase
  desyncs and never re-converges. Since `0x54f980`'s behaviour cases draw this exact LCG
  (`FUN_005bf505`, ~40 sites: idle waits `+0x5c`, the idle→wander branch pick, move offsets →
  `0x450ef0`), the actors pick different waits/dirs/positions run-to-run = the #75 ~6.7k-px
  band. **CONCLUSION:** an RNG-reading subsystem needs its OWN **RNG anchor** (snapshot/restore
  `DAT_008a4f94` at the game_enter sim-tick, both sides) — the camera's `g_sim_tick` anchor is
  insufficient for it (works only because the camera reads no RNG). Parity bar for the actor
  band = "data-1:1 given a matched RNG state" (retail-vs-retail isn't observed-1:1 here).
  (The `a0_clip/a0_frame` fields matched 8643/8643 but TRIVIALLY — main-band slot 0 was inert
  the whole run; the `rng` divergence is the real result.) Tool: `tools/rng_tick_diff.py`.
  Engine-quirk #77; `in-game-intro.md`. **DIRECTION (user): defer ALL RNG-order parity**
  until every in-scene RNG consumer is RE'd, then match consumption order (rng+`rngcalls`
  both sides — the flow trace now carries `rngcalls`, the unified consumption signal,
  openrecet-style; commit `4c587c0`). **Next chips = implement the scene's VISUAL elements**
  (LETTERBOX #74 → `0x5a00c0` banner/tree → NPC render/spawn); RNG behaviour parity comes after.
- **Prior (ckpt 72): the ACTOR ANIMATION cycle RE'd + the frame-stepper ported — rides the
  sim-tick clock, no separate counter.** The per-tick UPDATE pass (`0x439690:1108`→`0x46cd70`
  once/tick→`0x54f980` per actor) runs one byte-identical inline stepper on the render-state
  anim fields (`+0x6c` clip/`+0x70` timer/`+0x72` frame/`+0x74` done): `timer++`; at `>=clip.dur`
  → `frame++`,`timer=0`; at `>=clip.count` → loop or one-shot hold. Clip = a fixed **0x154-B
  32-frame** descriptor, (re)set on STATE CHANGE (`0x40afe0`/`0x41e600`). **PORTED (host-tested
  bit-exact, 854 pass): `src/anim_clip.{c,h}`.** The stepper reads no GetTickCount/Flip/RNG → it
  is deterministic under the camera's `g_sim_tick` anchor; ckpt 73 then proved the leftover band
  diff is the RNG-driven BEHAVIOUR, not the stepper. Engine-quirk #76.
- **Prior (ckpt 71): TIMESTEP DETERMINISM established — the SIM-TICK is the only
  valid frame-of-reference; the "house off by 3px" was FLIP-misalignment, not a bug.**
  The in-game sim is a wall-clock GetTickCount frame-limiter (`FUN_00439690:776-859`): one
  logical sim tick per outer iteration (easer `0x43d1d0` once at `:1123`) but a VARIABLE
  number of Flips per tick → **the Flip index is non-deterministic run-to-run** (two identical
  retail runs disagree on 47-86% of flips by ≤3px; `--lockstep-epsilon-ms 0` is worse, so it's
  intrinsic, not the epsilon). The **sim-tick index** (easer-call count) is bit-identical.
  The user's whole-foreground 3px trail (background Δ0) is the signature of flip-misalignment
  — the 0.5×/0.25× parallax hides the same camera offset the 1× foreground exposes; the tile
  math is provably identical at equal `cam_x60`. **FIX (committed):** the agent counts easer
  calls (`g_sim_tick`), tags every captured frame (`frame_<flip>_t<simtick>.png` + manifest)
  + call-trace event, and RESETS the counter at the `game_enter` scene-load anchor (synchronize
  at every non-deterministic load) → cross-run deterministic (99 ticks, 0 cam-mismatches; pan
  starts at tick 92 both runs). `tools/sim_tick_diff.py` matches two run-dirs by sim_tick/cam
  (dx=0) vs flip (the 3px trail). Engine-quirk #75; `in-game-intro.md` "Timestep determinism".
  **DECISION (user):** anchor each subsystem for determinism rather than a global timestep
  hack (fallback if it gets unwieldy). The camera is synced (sim-tick); the actor anim cycle is
  now RE'd + ported (ckpt 72 above — it rides the same sim-tick clock, no new pin needed). The
  intra-tick-identical observation is now explained: the stepper reads no flip/clock/RNG.
  Standing rule: never diff on the Flip index — anchor on the sim tick. NB `--turbo` is NOT faster in-game (Frida/LAN overhead dominates, not Sleep) and
  breaks the no-turbo-timed input traces; use `--no-turbo --lockstep` until traces are re-timed.
- **Prior (ckpt 70): the intro-PAN camera is WIRED LIVE — the town now pans.**
  `main.c game_render` steps a live `camera_view` each frame (`camera_follow_step` =
  `FUN_0043d1d0`, with the `CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror) and projects the
  backdrop through its *current* scroll instead of the static const. `enter_game`
  spawn-snaps it (`camera_apply_snap` → cur=tgt=128000/12800); a hold timer fires the
  scripted pan (`camera_apply_pan` → tgt=12800/12800, speed 300) at hold-end. The two
  target-setters are bit-exact ports of `0x439690`'s SNAP/PAN command arms (`:599-664`),
  host-tested (clamp to `[0, map-vp]`; snap-jumps-cur / pan-keeps-cur). **Visually confirmed
  on the feed:** hold (cam x=128000) → mid-pan → settled (cam x=12800, town left edge).
  **848 pass / 0 fail / 6 skip** (+2). Also added `MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800).
- **CADENCE + TRIGGER measured → the pan is now TRAJECTORY-1:1 (ckpt 70b).** A retail
  field-spec trace (`--seed-pin --lockstep --no-turbo`, easer `0x43d1d0` + Flip hooked,
  contiguous Flip whitelist) pinned both stand-ins to ground truth: the easer fires **once
  per 2 Flips** (the sim runs at half the Flip rate; `cam_x60` is a STEP function, −300/2flips
  at cruise) and the pan command fires at **`game_enter + 184` Flips** (Flip 1616 HOLD, 1617
  PAN). `game_camera_step` now gates the sim to every 2nd frame (`hold & 1`), trigger at
  `GAME_CAMERA_HOLD_FRAMES=184`. **The port now passes through the IDENTICAL `cam_x60`
  sequence as retail** (128000,127990,127970,127940,…,cruise −300/2flips — verified by
  diffing the captured `0x43d1d0` mirror). **RESIDUAL (PORT-DEBT `ingame-camera-pan`):** a
  ~2-3 Flip startup-jitter PHASE offset (retail's sim accumulator is wall-clock-paced — a
  4-Flip plateau at 1618-1621 a clean 2:1 step can't reproduce; ≤1 step ≈ 3px, transient,
  zero at hold+settled) + the cutscene-script TRIGGER source — both downstream of the
  in-game sim / `0x5a00c0` port.
- **Methodology (reinforced ckpt 69):** "annotate" = the **flow-trace field spec**
  (`retail_fields.json` named functions+fields + port `CALL_TRACE_BEGIN` mirrors) — a CORE
  step of finishing any RE/port; thiscall/struct tagging is a SEPARATE static-readability
  lane. Never an ad-hoc symbol-rename. (CLAUDE.md "Annotate as you RE".)
- **Prior (ckpt 68): 24bpp parallax LUT grade LANDED — sky colour USER-CONFIRMED.**
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
  the intro **pan** is wired + cadence/trigger-matched (ckpt 70b) — it passes through retail's
  exact `cam_x60` sequence; residual is a ~2-3 Flip startup-jitter PHASE (PORT-DEBT
  `ingame-camera-pan`), zero at the hold + settled ends (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800).
- **PAN BACKDROP DIFF DONE — verified pixel-1:1 (ckpt 70b).** Captured fresh retail pan frames
  (`--no-turbo --seed-pin --lockstep`) + their `cam_x60`, matched port frames by `cam_x60` (port
  Flips 1304/1344/1384/1422/1462 ↔ retail 1617/1660/1700/1740/1780, shared cam 127990/125690/
  120050/114350/108350), and diffed: the **backdrop is Δ0** (shift-search peaks sharply at
  `dx=dy=0`; pan-start `x=80` column all Δ0). The remaining diff is the **named missing layers
  ONLY** — exactly the signal we wanted. NEW retail ground-truth (quirk #74): the establishing
  shot is **LETTERBOXED** (solid-black bars rows 0-63 + 416-479, a 640×352 cinematic window; the
  "dark top" the user saw, with a matching bottom bar). Parity-ledger entry #7.
- **Next move (the named layers, simplest first):** (a) the **cinematic LETTERBOX** (quirk #74 —
  draw black over rows 0-63/416-479 during the establishing shot; a big, simple slice of the diff,
  likely a `0x5a00c0` overlay); (b) the **"Town of Tonkiness" banner** + **foreground tree/veg**
  (`0x5a00c0` scripted-scene overlay player — also where the pan TRIGGER source lives, closing
  `ingame-nontile-layers` + the trigger half of `ingame-camera-pan`); (c) the **actor renderers**
  (present modes 0/1/2, need the entity/spawn system first). Writeups: `findings/in-game-intro.md`
  "The pan CADENCE + TRIGGER measured" + the diff verification; quirk #74.
- **Tooling front:** **Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` carries `seq` + `CALL_TRACE_BEGIN/FIELD/END`;
  the Frida agent reads same-named retail fields per `tools/flow/retail_fields.json` (now incl.
  the `cam_*` camera chain + the ckpt-67 `tint`/`lutgate1/2`/`lut*` colour-grade probes +
  the ckpt-69 `camera_follow_step` producer entry);
  `tools/flow_diff.py` names the first `[chain]`/`[data]` divergence. Remaining Phase B: **B1**
  unified `scenario-test.py`, **B3** DDraw blit-command trace + `render_diff.py`.
  **`mem_watch.py` (ckpt 69):** now resolves **chain heap addresses**
  (`--watch-chain ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]`) + a **`--hw` hardware
  watchpoint** (frida-17 per-thread DR) — the fitting tool for a hot heap field (found the
  camera easer through its heap fn-pointer dispatch in one run).
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
- `AGENT-WORKFLOW.md` — when to spawn subagents vs stay in the main loop.
- `PLAN.md` — goal, constraints, phased roadmap.
