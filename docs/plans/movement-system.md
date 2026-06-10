# Plan — the ENTITY MOVEMENT SYSTEM (Phase 4)

> Opened ckpt 108 (2026-06-10). The keystone arc: the in-game movement / physics /
> collision FSM. Unblocks **PORT-DEBT(butterfly-wander)** (the last visible town-intro
> residual — @1177 box 305px + @1627), the **controllable-character milestone**
> (Arche → walk/run/jump), and the **freeroam** scene (the USER directive: advance past
> the dialogue into the house). USER-chosen direction over the smaller intro-polish chips.
>
> Multi-session. Each chip ends at a `/clear` breakpoint. Read this → `FRONT.md` →
> `HANDOFF.md` to resume. Ground-truth EVERYTHING (the system is too tangled to port by
> reading alone — the decompile already hid one write, see §Bounds).

## Why this is one arc, not a chip

The butterfly's drift is NOT a self-contained particle — it goes through the **full entity
movement machinery**, the same code that will drive Arche. So "make the butterflies move"
== "port the movement system." The butterfly is just its **simplest exerciser** (no input,
open air → collision probes mostly return clear), so it is the right **first** target.

## Architecture (mapped ckpt 108 — addresses are stable, names rename)

Per-tick flow, top to bottom:

| layer | fn | size | role | port status |
|---|---|---|---|---|
| master band walk | `0x46cd70` | 1031 | walks the per-tick bands. **TWO passes over the EFFECT band (`+0x1160`, 32 slots): pass 1 = the AI `0x47b990` (writes the cmd block), pass 2 = the APPLY `0x485fc0` (integrates).** Both run for every active EFFECT actor. | **PORT MIRRORS** (`game_actor_update`, band order proven ckpt 98/99) — but the port mirrors only the AI consume-stub, NOT the apply pass yet |
| actor AI / update | `0x47b990` | 7461 | per-tick per-actor (18 town actors: townsfolk + the 4 butterflies + `0xe2a5` — NOT butterflies-only, corrects a chip-108 note). Big `switch(this+0x1d4)`. **GATED to work-ticks** (`+0x14232`, every-other). butterfly = the `0xe29a` case → picks heading → `0x43f880`; jumps PAST the `0x1422c` state machine to the tail (`0x484c10` at :826 is NOT on the butterfly path) | **PARTIAL** — `butterfly_step` consumes the gate/flit/heading RNG draws ONLY; the heading-flip + `0x43f880` unported |
| movement intent | `0x43f880` | 5491 | open-air butterfly: `0x15998==0` ⇒ skips the tile-grid block ⇒ the action list resolves to a simple move; writes the 8-int **command block** to `this+0x14854`. Runs only on **work ticks** (gated) | unported |
| swept collision probe | `0x4412d0` | 993 | allocs a temp swept-path (`operator_new 0xa500`), steps it via `0x442a70`, tests vs the entity list (`DAT_008a9b50+0x278c`) + the tile grid (`0x440e40`). Returns blocked/clear. **Does NOT move the actor.** | unported (chip 2) |
| **apply pass** | **`0x485fc0`** | 4593 | **the 2nd EFFECT-band pass (`0x46cd70:71`). Reads the cmd block + calls the integrator `0x442a70(this+0x14854, body, body, 0, 0)` on the REAL body (`485fc0:348`, gated `local_2c==0`). Runs EVERY tick (NOT gated) — capture-confirmed: position moves on both gate phases.** | unported (chip 1) |
| path stepper / integrator | `0x442a70` | 12026 | shadow-copies src→dst body (`:49-100`) then integrates **in place** when called `(cmd, body, body)`. Core: `vel(+0x18) += 2000` toward target (clamped `>-20000`), position step via the mover `FUN_0054e5c0(body, vel/100, …)`; multiple axis/direction cases = the 12 KB. Shared with probing | unported (chip 1 reduced / chip 2 full) |
| ledge/dir probe | `0x47dbb0`, `0x441ae0` | ? | directional collision-grid probes (clear/blocked in a direction) | unported |
| tile collision grid | `DAT_008a9b50+0x1048` → `+0x2c1030` (row widths) / `+0x2c1040` (tile flags) | — | indexed by `worldX/0xc80`, `worldY/0xc80`. **`0xc80` = 3200 = one tile = the movement quantum** | unported |

### The command block `this+0x14854` (8 ints / 32 bytes)
The **resolved per-tick move**: `{action_type, dx, dy, sub, …}`. Written by `0x479e40`
("set move command"), zeroed by `0x411530`, the chosen one copied out at the end of both
`0x47b990` (→`+0x14854`) and `0x43f880` (→`this+0x5215`==`+0x14854`).
**RESOLVED (ckpt 109): the apply is the band walk's SECOND EFFECT-band pass `0x485fc0`**
(`0x46cd70:71`), which reads `+0x14854` and calls the integrator
`FUN_00442a70(this+0x14854, body, body, 0, 0)` on the **real** body (`485fc0:348`). It is
a SEPARATE pass over the same 32-slot band, AFTER all AI ran, and runs EVERY tick (not
gated) — capture-confirmed (`runs/butterfly-fsm`: position moves on both gate phases). So
the butterfly's per-tick motion is: AI `0x47b990` decides + writes the cmd block on
**work ticks only** (gated `+0x14232`), then `0x485fc0`→`0x442a70` integrates **every tick**
(the integrator carries the accel/velocity state, so the butterfly keeps gliding on the
gated-out ticks). The `0x484c10`/state-machine hypotheses are RULED OUT (the `0xe29a` case
jumps past them to the tail).

## Entity struct (`this` = ~90 KB per actor; INFERRED, confirm in chip 1)

- `this+0x1d4` — actor code (`0xe29a` butterfly, `0xc35a` Arche, `0xc3dc`… townsfolk).
- `this+0x40` — → **physics body** sub-struct (also passed as `0x47b990` param_2 / `0x43f880` param_3):
  - `body+4` worldX · `+8` worldY · `+0xc` width · `+0x10` height · `+0x18` velocity ·
    `+0x2c` facing (1/3) · `+0x38` state · `+0x284` ? (link/target)
- Movement/AI fields on `this`:
  - `+0x14244` heading (1/3) · `+0x14248` heading-flip cooldown · `+0x14232` every-other-tick
    gate · `+0x14234` move duration · `+0x14236` flit-pick timer · `+0x1422c` **state selector**
    (the `switch` in `0x47b990`: 2→`0x482ac0`/`0x482880`, 3→`0x482380`, 5→`0x483b50`,
    6→`0x481ac0`, default→`0x485xxx`) · `+0x14854` command block.
  - `+0xc890` wander range (recomputed each tick `(rand*0xc80)>>15 + 0x640` = [1600,4800];
    spawn-seeded by `0x428780`) · `+0xc894`=8000 · `+0xc898`=0 (wander params) · `+0xc874`
    move-freq (spawn-captured, ckpt 98) · `+0x15950..0x15960` wander min/max (`0x427c30`).
  - **`+0x14264` / `+0x14268` — the L/R patrol bounds the butterfly moves toward.**

### Bounds `+0x14264`/`+0x14268` — RESOLVED (ckpt 109, the chip-1 lynchpin)
The `0xe29a` case passes `*(this+0x14264 | 0x14268)` as the move **target** to `0x43f880`;
heading 1 → `+0x14264` (move right), heading 3/0 → `+0x14268` (move left). A full static
grep finds no write (set via a computed pointer static analysis can't see) — but the dense
capture (`runs/butterfly-fsm`) pins both **values + formula**: each butterfly has exactly
ONE `(b1,b3)` pair, **dead constant** across all 286 ticks, and across all 4 butterflies:
```
b1 (+0x14264) = spawn_wx + 11200      b3 (+0x14268) = spawn_wx - 8000
  (≡ center spawn_wx+1600 ± 9600=3·0xc80; note 8000=+0xc894, 11200=8000+0xc80)
spawn_wx ∈ {99200, 105600, 176400, 181200} = the 4 map worldX (= wx@tick0)
```
**Chip 1: just set `b1/b3` at register-time from the spawn worldX** (the port already has
it from the map spawn). Tag `PORT-DEBT(butterfly-bounds-writer)`: the +11200/−8000 are
exact for the town but their derivation (the spawn-time writer, likely off `0x428780`'s
`+0xc890/+0xc894`) is un-RE'd — `mem_watch this+0x14264` at spawn to close it (deferred;
the values are bit-exact for chip 1, the writer matters for generality in chip 2/3).

## Ground truth — chip 0 DONE (ckpt 109, `runs/butterfly-fsm`)

The dense per-tick capture is **HAD** (`runs/butterfly-fsm/`, seed-pinned + lockstep,
`game_enter@1434`, sim-ticks 0..285 × 4 butterflies; spec `tools/flow/butterfly_capture_fields.json`,
analysis `runs/butterfly-fsm/analyze.py`). `0x47b990` field-spec per tick: worldX/Y, heading,
facing, bounds, cooldown/gate/flit/movedur, wander-range, cmd block, vel, flap. **The bit-exact
target for chip 1.** Findings (the per-tick motion model, all capture-verified):

- **Two clocks.** AI decision (`0x47b990` `0xe29a`) runs on **work ticks** (gate `+0x14232`,
  every-other); the APPLY (`0x485fc0`→`0x442a70`) integrates **every tick**. The butterfly
  glides between decisions because the integrator carries velocity.
- **Glide.** `vel(+0x18)` ramps ±2000/tick to a ±16000 cap; the worldX step ramps to ±100/tick
  (decelerates to ~0 then reverses on a heading flip). The integrator state, not the gated AI,
  drives the smooth motion. (Exact step law = `0x442a70`'s open-air path — port + fit to dwx.)
- **Heading vs facing.** `+0x14244` heading = the INTENT (which bound to chase, flips in the
  gated AI); `body+0x2c` facing = the actual travel direction (the integrator flips it when the
  velocity sign reverses — it LAGS the heading by the decel/reverse window, ~5 ticks).
- **Heading flip.** On a work tick, if cooldown `+0x14248`==0 AND (`|worldX-bound| < 0xc81`
  OR a `0x47dbb0` collision OR a 10% RNG roll) → flip heading, set cooldown 0x3c. Cooldown
  decrements ~1.5/tick (−1 every tick at `:405` + −1 more on work ticks at `:773`) ⇒ ~40-tick
  min between flips. Many observed flips are the 10% roll (fired well before the bound).
- **cel_fr.** The within-clip flap `render-state+0x72` ∈ {0,1,2} is **heading-INDEPENDENT**
  (the looping 3-frame flap, already `BUTTERFLY_CLIP` in the port). OPEN for chip-1 validation:
  whether the RESOLVED emit `cel_fr` base tracks direction (the ckpt-107 emit showed {0,4,8,…};
  could be the 4 colour variants, not heading) — correlate the EMIT-side `cel_fr` (`0x492670`)
  with heading at matched ticks before assuming a direction→base map.

## Chips (each ends at a `/clear`)

0. **Per-tick butterfly-state field-spec capture — DONE (ckpt 109).** Resolved BOTH plan open
   items as bycatch: the apply (`0x485fc0`→`0x442a70`, every tick) and the bounds formula
   (`spawn_wx + 11200 / − 8000`, constant). `mem_watch` the bounds writer is DEFERRED (values
   are exact; writer derivation → chip 2/3).
1. **Butterfly open-air motion** — port the `0xe29a` heading FSM (`0x47b990`: gate/flit timer
   already done → MOVE the 2 RNG draws/tick into the real FSM unchanged + add the heading flip)
   + set bounds at register-time (`b1=spawn_wx+11200`, `b3=spawn_wx-8000`) + the **reduced**
   `0x43f880` path (open air ⇒ action list = move-toward-bound) + the per-tick apply (the
   `0x442a70` open-air integrator: `vel+=2000` clamp ±16000, `worldX += vel/100`-style step —
   **fit to the captured dwx until tick-exact**). Validate **field-exact** vs `runs/butterfly-fsm`
   on the sim-tick axis (worldX/Y/heading/facing per tick). Closes **butterfly-wander** (motion).
   Tag the open-air reduction `PORT-DEBT(...)` so it doesn't fork the shared `0x43f880` — chip 2
   generalises it. **RNG-stream risk:** keep the draw count/order identical or the ckpt-99
   settled-town stream regresses.
2. **Tile collision** — port the grid (`0x2c1030`/`0x2c1040`) + `0x4412d0`/`0x440e40` swept
   probe + `0x441ae0`/`0x47dbb0` directional probes. Ground actors stop clipping terrain.
   Prereq for Arche.
3. **Controllable Arche** — the party-leader band `0x4997b0` (see
   `plans/party-character-system.md`) + DirectInput → the `0xc35a` case in `0x47b990` →
   walk/run/jump physics. The milestone.
4. **Freeroam** — finish the cutscene (dialogue chip 4, `plans/dialogue-cutscene.md`) →
   hand control to Arche → a NEW trace-studio session (the USER's "house freeroam" directive).

## Validation (per chip)
Seed-pinned + lockstep field-spec capture of the entity's per-tick fields (a
`retail_fields.json` field, **not** a bespoke probe), diffed port↔retail on the **sim-tick
axis** to field-exact / `differ_px==0`. `flow_diff` names the first diverging call;
`render_diff` names the wrong blit (`cel_fr`/dst). Annotate each newly-RE'd fn in
`retail_fields.json` + the port `CALL_TRACE_BEGIN` mirror as you go.

## Risks
- **The apply step is unlocated** — chip-1 step 1. If integration is entangled with
  collision, chip 1 may need a thin slice of chip 2.
- **RNG alignment** — `butterfly_step` already consumes the `0xe29a` draws (ckpt 98). The
  FSM port must consume the **exact same draws in the same order** or the settled-town
  stream (bit-exact to ckpt 99) regresses. The heading FSM's 2 RNG draws/tick
  (`+0xc890` range, the 10% flag) are ALREADY in the live stream → the port must move them
  from `butterfly_step` into the real FSM **without changing the count/order**.
- **Shared code** — `0x43f880`/`0x47b990` also serve ground chars + Arche. Structure the
  butterfly port as a documented open-air **reduction** of the real fns, not a parallel
  fork, so chips 2–3 extend rather than rewrite.
