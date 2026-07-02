# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
███░░░░░░░░░░░░░░░░░  14.4% touched   (14.4% host-tested, 14.9% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   222 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **227** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1531 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (14.9% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase — the freeroam sim**, built bit-exact vs errands re-drives. The prior arcs
  (town intro → arrival/house/errands cutscene; the HUD panel ckpt 167–174) render 1:1.
- **Landed ckpt 175 — COLLISION:** the movers `0x54ded0`/`0x54db10` tile halves ported
  (`collision.c`) + `character_step` restructured to the retail `0x442a70` tick order
  (support probe → vertical mover/ledge-fall → ramp → worldX commit); slope ramps
  read live off the user's exe; the `LAB_00589520` "occlusion marks" = invisible
  collision WALLS (map_decode fix). **Arche stops/climbs/descends the errands stairs
  tick-exact** (dwx==dwy==0 through the whole climb; both walls flush) — retires the
  studio-note-2441 divergence + debts `char-collision-mover`/`collision-slopes`/
  `decode-occlusion-mark`. The 4-tick standing TURN-AROUND (`char-turn-state`) is
  now PORTED (ckpt 177 below). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Landed ckpt 176 — dialogue mark t1197 FIXED + char-turn RE corrected.** (a1) The
  "port skips dialogue early" mark is NOT an advance-gate bug — the advance PRESSES
  already match retail (port DLGT + dialogue_timeline off the stairs pair).  It was a
  box RENDER linger: retail keeps the arrival L9 box up through tick 1200, the port
  cleared it at 1192 → `ARRIVAL_EXIT_BOX_HOLD=8` (== the house-exit box-hold pattern),
  drawcall-verified L9 adv 1192→1200 == retail.  `findings/dialogue-advance-early.md`.
  (c) char-turn RE CORRECTED: the ckpt-175 "case-2" pointer was WRONG (that's the
  DOWN/crouch, already ported); the real reversal turn is the STATE-1 horizontal FSM
  `0x442a70:1011-1090`.  See `port-debt.md`.
- **Landed ckpt 177 — `char-turn-state` PORTED (retires the collision residual).**
  The from-rest REVERSAL now plays retail's 8-tick pivot: `CHAR_TURN_HOLD=4`
  STATIONARY windup ticks (facing HELD, the fr-6 turn cel) → flip facing + walk ramp
  (fr-7 → +152 fr-159 lingers 4 render ticks) → walk (fr 160).  `character.c`
  `turn_ctr`/`turn_frame` + `ARCHE_TURN_CLIP` {6,7} (both dirs via the +152 mirror,
  == the house turn 158→7).  GROUND TRUTH retail-stairs res 0x570 (fr 6×4 → 159×4 →
  160); `draw_probe port-stairs2` == retail, `state_diff` RIGHT 0-div/301t (no regress)
  + LEFT ramp-shape bit-exact (the −960 gap GONE).  RESIDUAL: a constant 1-tick
  reversal-ONSET phase (−240) — the port latches the reverse press 1t before retail's
  warmup gate (→ `char-input-autorepeat`, not the turn), within retail's ±1-2t
  coalescing slop.  RE: `findings/freeroam-turn-around.md`.  **USER: verify the pivot
  animation — studio shortcut = `port-stairs2 | retail-stairs` @ tick ~2950.**
- **Landed ckpt 178 — ALL-MAPS SWEEP: the 587e00 tile dispatch is COMPLETE.**
  `tools/extract/map_sweep.py` (all 376 map resources vs the port's own sources):
  tile-id cell coverage 5.0% → **100.0%** (87/87 ids; every family transcribed
  emit-for-emit; quirks #119-#121).  The trailing `FUN_0058cb30` placeholder pass
  is PORTED (map codes 90010/90011 → region-E link anchors — NOT spawns).
  STRUCTURE def table complete (0xf295→0x77; 0xeead→the runtime bank 0x88/89/8a
  per param_4 5/6/8, RE'd off the 587e00 prologue).  res_explorer: PLACEHOLDER
  band named + region-E inspect; CHARACTER label kept (the ENGINE's own band
  name — holds props/emitters too, answers the USER naming question); param_4
  cfg 1-8.  Maps 1057/1047 render fully (feed); town regression clean.  1095
  host tests.  Still stubbed: EFFECT/CHARACTER sprite tables beyond the town
  captures, the decode-prologue header/palette installs, the region-E spawn
  CONSUMER (next link for (b) below).
- **Landed ckpt 179 — the arrival→house dialogue DRIFT FIXED (`ARRIVAL_EXIT_WAIT` 10→20).**
  The chain played −11t early because the arrival-exit WAIT was curve-fit to 10 (a bogus
  "2× anomaly") when it's the script's `0x14=20` mapped **1:1**.  The scene_fade PERFORMER
  is bit-exact (settle rates match) — the "settle-rate" hypothesis is DISPROVEN; the old
  "room-load black-hold" read was a CONFOUND of the −11t global shift.  The "2×" was a
  measurement error: ckpt-137 timed "first CENTER alpha" but retail's cover grows TOP-DOWN
  (sweep, not center-out), so the center lags the arm.  GROUND TRUTH off `retail-stairs.osr`
  (res-1110 box + 64×4 fade coverage; retail fades capture as res=0 subtract-blend, no
  alpha split): house L0 box open −11t→**−1t**, **house box close now tick-EXACT (1650)**,
  errands −20t→−6t.  `findings/dialogue-advance-early.md` "Component 2/3 RESOLVED"; quirk
  #122; 1095 host pass.  **USER: verify the transition — studio `port-waitfix | retail-stairs`
  @ ~1200-1350 (the arrival→house cover/reveal); feed `waitfix_cmp.png`.**
- **Landed ckpt 180 — the errands t2278 mark DIAGNOSED + the kitchen CABINET FIXED.**  The
  "missing POT" is a `port-waitfix` STALE-TRACE artifact: the pot (res1074) renders BIT-EXACT
  in the walk-aligned `port-stairs2` vs retail-stairs @t2278 (proof: port-stairs2==retail
  bit-for-bit — pot 376/376, wall-shelf 108/108, Arche 270,296).  The ckpt-179 waitfix
  desynced port-waitfix's tick-keyed freeroam walk (+176px camera lag, 4 props measured) — so
  **the freeroam studio pair is `port-stairs2 | retail-stairs`, NOT port-waitfix** (waitfix is
  valid only for the dialogue window ≲tick 2000).  The real gap = the RIGHT-side/upstairs props
  ERRANDS_CAST systematically missed (captured from the static tick-2200 LEFT/centre view): the USER
  flagged the kitchen CABINET (res1023 fr4 + hutch fr2) AND the POT (res1026 fr58 @228,208, "right of
  Mom's head").  FIXED as **6** ERRANDS_CAST entries (cabinet/hutch/pot + 3 upstairs props, DATA-1025
  CHARACTER codes 0x112d1/0x112da/0x11279/0x112d3/0x1124c; banks via slot+13; world = map ×100) —
  VERIFIED at the clamp t2500: all 6 render at retail's EXACT pos+dims (feed `pot_crop_cmp.png`).
  **ROOM PROPS NOW VERIFIED COMPLETE** — a map↔retail cross-ref confirms ERRANDS_CAST covers ALL 20 of
  the map's visible CHARACTER objects; a port↔retail draw diff at t1800 AND the t2500 clamp shows NO
  missing prop either side.  Also PORTED (ckpt 180b): the **pot STEAMS** (POT_CLIP 57→60 dur6) + the
  **clock pendulum SWINGS** (CLOCK_CLIP 43↔45 dur25), both == retail.  Remaining (NOT props): the HUD
  (res1900 = the bottom-left HUD strip, NOT a room object; + the blank portrait blocker + strips), Mom's
  pose (res1127 fr0 vs fr2), the clock/pot anim start-phase seed.  The runtime map-driven spawn (retiring
  the ERRANDS_CAST capture) is now pure cleanup, blocked on the char-band z-order (no draw_pool Y-sort);
  the code→bank table + variant model are proven + ready.  `errands-render-gaps.md` §6.  **USER: studio
  `port-cabinet | retail-stairs`, scrub the CLAMP (~t2400+) — the pot should steam, the clock swing.**
- **Next move (the errands −6t residual + the 2nd USER mark):**
  (a2') **errands entry −6t (OPEN, smaller)** — the house dialogue is now tick-exact, so
  the residual is entirely the house→errands transition (house-close 1650 → errands-open
  port 1693/retail 1699).  `HOUSE_EXIT` has no preceding WAIT, so it's the errands ENTRY
  reveal arming ~6t early (main.c arms on chain-complete, no errands room-load latency →
  `PORT-DEBT(cutscene-errands-entry-latency)`).  Measure the house→errands cover/reveal
  envelope off retail-stairs before adjusting — do NOT curve-fit.
  (b) **house props (mark t2278) — DIAGNOSED + the cabinet FIXED (ckpt 180).**  The POT
  (res1074) is NOT missing: it renders BIT-EXACT in the walk-aligned `port-stairs2` vs
  retail-stairs @t2278 — the "missing" was a `port-waitfix` STALE-TRACE artifact (the
  ckpt-179 waitfix desynced the tick-keyed `synth-stairs` held-walk → +176px camera phase
  lag).  The real gap = the KITCHEN CABINET (res1023 fr4 @280,96 + fr2 upstairs hutch),
  errands-map (DATA 1025) CHARACTER code 0x112d1 map layer[18]/[31] — FIXED as 2 ERRANDS_CAST
  entries (world 70400,25600 / 70400,6400; verified 172px offset == map).  (region-E is
  WAYPOINTS, not props — the old "prop spawn consumer" framing was wrong.)  **⇒ freeroam
  studio pair = `port-stairs2 | retail-stairs`, NOT port-waitfix (stale past ~tick 2000).**
  `findings/errands-render-gaps.md` §6.  Still open at t2278: Mom's pose clip.
  (c) `char-turn-state` — **DONE ckpt 177** (`findings/freeroam-turn-around.md`).
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`, `char-input-autorepeat` (the residual
  1-tick reversal onset); HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).

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
