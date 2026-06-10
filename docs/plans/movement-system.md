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
| swept collision probe | `0x4412d0` | 993 | allocs a temp swept-path (`operator_new 0xa500`), steps it via `0x442a70`, tests vs the **two ENTITY lists** (`DAT_008a9b50+0x278c`/`+0x2788`) via `0x440e40`. **CORRECTION (ckpt 111): `0x440e40` is ENTITY-vs-entity (`in_ECX+0x40` actor list, stride 0x294), NOT the tile grid.** Returns blocked/clear; does NOT move the actor. | unported — chip 3 (entity collision) |
| **apply pass** | **`0x485fc0`** | 4593 | **the 2nd EFFECT-band pass (`0x46cd70:71`). Reads the cmd block + calls the integrator `0x442a70(this+0x14854, body, body, 0, 0)` on the REAL body (`485fc0:348`, gated `local_2c==0`). Runs EVERY tick (NOT gated) — capture-confirmed: position moves on both gate phases.** | unported (chip 1) |
| path stepper / integrator | `0x442a70` | 12026 | shadow-copies src→dst body (`:49-100`) then integrates **in place** when called `(cmd, body, body)`. Core: `vel(+0x18) += 2000` toward target (clamped `>-20000`), position step via the mover `FUN_0054e5c0(body, vel/100, …)`; multiple axis/direction cases = the 12 KB. Shared with probing | unported (chip 1 reduced / chip 2 full) |
| ledge/dir probe | `0x47dbb0`, `0x441ae0` | 655/749 | directional tile-grid probes (clear/blocked in a direction) — read region **D** (`+0x2c1040` flag, `==1` wall) + region **C** (`+0x195030` slope-type) + actor state (`in_ECX` fields gate the ledge scan). **ACTOR-entangled** | unported — chip 3 (the AI caller is live there) |
| vertical tile-mover | `0x54e990` | 861 | **PORTED ckpt 111 → `collision.c` `collision_move_vertical`.** The gravity/ground/ceiling clamp: pure over (grid, body box, delta); `in_ECX` is the GRID, not the actor. Sweeps world-Y in ≤100 steps, scans the X-extent vs region-B class (10=wall, 1=slope), clamps. Slopes via callback (PORT-DEBT `collision-slopes`) | **DONE (chip 2)** — host-tested ×6 |
| tile collision grid | `DAT_008a9b50+0x1048` → region **B** `+0x140030` (class+slope) · region **C** `+0x195030` (slope-type) · region **D** `+0x2c1040` (flag) | — | indexed `idx = (worldX/0xc80)*0x80 + worldY/0xc80`. **`0xc80`=3200=one tile.** **ALREADY BUILT (corrects "unported"): `map_decode.c` (the 0x587e00 town arms) deposits all three regions on the proven-1:1 render path; read accessors PORTED ckpt 111 (`map_grid_obj_*`/`map_grid_flag`)** | **BUILT + read accessors DONE (chip 2)** |

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
1. **Butterfly open-air motion — DONE (ckpt 110).** Ported the `0xe29a` heading FSM (the 2 RNG
   draws/tick MOVED into the real flip logic, count/order unchanged; flip toward the far bound
   when `cd==0` AND `|wx−bound|<0xc81`/10%-roll; `0x47dbb0` collision omitted = open-air clear),
   bounds at register (`b1=spawn_wx+11200`, `b3=spawn_wx−8000`), and the reduced apply (the
   open-air HORIZONTAL integrator: `world_x += hvel` then `hvel` ramps ±10/tick→±100, **step
   before ramp** = the capture's form, run EVERY tick). `main.c` apply-wires it into the rendered
   actors. **Validated field-exact** vs `runs/butterfly-fsm` on the sim-tick axis
   (`compare.py`): HEADING 0 mismatches (every flip tick, all 4 → LCG byte-aligned thru tick 269),
   facing ≤1/286, worldX exact between reversals; residual = a ≤170-unit ≈ ≤2px BOUNDED transient
   at turn-arounds. **Deferred** (`PORT-DEBT`, retire with the full integrator chip 2/3):
   `butterfly-flutter` (the VERTICAL flutter sawtooth `body+0x18` + the `cmd_2` flap sub-FSM →
   worldY bob + the flap/reversal coupling = the ≤2px residual), `butterfly-bounds-writer`,
   per-instance frame_base multicolor. The `vel`-based motion model in §"Ground truth" was the
   VERTICAL axis (`0x442a70`'s gravity/flutter block writes `body+8` via `0x54e5c0(body,vel/100)`)
   — the horizontal patrol is the separate clean ±10/tick→±100 law fit above. **RNG preserved**
   (host `butterfly_pertick` unchanged; the exact flip ticks prove it).
2. **Tile collision — RE-SCOPED + core DONE (ckpt 111).** The big surprise: the collision
   GRID is **already built** — `map_decode.c` (the `0x587e00` town arms) deposits region B
   (class+slope) / C (slope-type) / D (flag) per cell on the proven-1:1 render path, so
   "port the grid" was moot. Chip 2 is the **read side**:
   - **DONE:** the grid read accessors (`map_grid_obj_*`/`map_grid_flag`) + the VERTICAL
     tile-mover `0x54e990` (`collision.c` `collision_move_vertical`, flat collision, slopes
     via callback = PORT-DEBT `collision-slopes`), host-tested ×6. Pure over (grid, body,
     delta) — no live actor needed.
   - **DEFERRED → chip 3** (they need a live grounded actor to validate, and the probes are
     actor-entangled): the directional AI probes `0x441ae0`/`0x47dbb0`; the integrator
     generalization (wiring the mover into `0x442a70`/`0x485fc0` so a grounded actor stops
     clipping — the butterfly apply stays the field-exact open-air reduction). `0x4412d0`/
     `0x440e40` are ENTITY-vs-entity, also chip 3.
   So "ground actors stop clipping terrain" lands WITH Arche (chip 3), where the mover gets
   its first live caller.
3. **Controllable Arche** — the milestone. **GROUND-TRUTH STARTED (ckpt 112, USER chose
   "ground-truth freeroam first"; engine-quirk #101).** Driving retail past the town-arrival
   cutscene (Z-spam the ring) **REACHES the inn FREEROAM** (control transfers at the "PLAYER!"
   prompt, flip 4500/sim-tick 1556 — `runs/freeroam-gt`), so the state IS reachable. Three
   findings RESHAPE the port plan:
   - **The leader `room_state+0x200c` is PERSISTENT (Arche since new-game), not cutscene-set;**
     the transfer flips a per-actor controllable flag (`entity+0x200=1` via `0x41e070`/`0x4c6830`).
     `0x4997b0` (leader render) is real, but the leader is already Arche.
   - **Arche's freeroam mover is NOT `0x47b990`** (it fired only for the townsfolk 0xc3dc/0xc440).
     The `0xc35a` case in `0x47b990` is the CUTSCENE-actor behaviour, NOT the freeroam mover.
   - **Freeroam movement reads the HELD-AXIS array `input-mgr+0x114` (quirk #41), not the event
     ring** — the harness's `--input-trace` ring injection drives menus + dialogue-advance but NOT
     held walking (injected dir ids leave Arche idle).
   - **RESOLVED (ckpt 113 harness + ckpt 114 mover):** (a) the held-axis injection harness landed
     (`src/held_trace.{c,h}` + the agent leaf-hook `0x5ba520`; Arche WALKS, ckpt 113). (b) The
     mover is PINNED by call-tracing `0x442a70` over the walk (`runs/mover-caller`, quirk #101 final
     bullet): freeroam movement is **TWO layers, both shared with the actor system** —
     **AI `FUN_00478ba0`** (the RNG-free character update, townsfolk-shared, counterpart of the
     butterfly's `0x47b990`) reads the held axis at `*(entity+0x158a4)+0x114/118/11c/120` (U/D/L/R)
     and builds the command block **`entity+0x14854`** (LEFT→`[0]`=1/5, RIGHT→2/6, DOWN→`[3]`=10,
     UP→`[3]`=0xb; walk/run via speed-mode `entity+0x158a0`); **APPLY `FUN_00485fc0+0x96e`→
     `FUN_00442a70(cmd,body,body,0,0)`** integrates it in-place on the real body — the SAME apply
     pass the butterflies use (`0x46cd70:71`).  vel `body+0x18`=0 ⇒ direct position write; the
     per-tick step accelerates +16/tick → ~+240 cap.  So chip 3's port = the `0x478ba0` character
     AI (input→command) + the FULL `0x442a70` integrator (the port has only the butterfly open-air
     reduction).  Supersedes the `0x405e80`/`0x406210`/`0x40c380` candidate guesses.
   - **NEXT (in order):** (c) RE the run/jump scancodes (the `0x8a6e80` keybind defaults) →
     capture walk/run/jump per-tick (the body spec `tools/flow/freeroam_arche_fields.json` reads her
     independent of the mover) → bit-exact target; trace the CALLER of `0x478ba0`/`0x485fc0` for
     `0xc35a` (band slot vs party path `0x4997b0`); (d) PORT the `0x478ba0` AI + the full `0x442a70`
     integrator + wire the chip-2 collision mover/probes (their first LIVE caller) → validate
     "Arche walks + stops at terrain" field-exact.
4. **Freeroam** — REACHED in retail (ckpt 112). Remaining for the PORT side: finish the cutscene
   (dialogue chip 4, `plans/dialogue-cutscene.md`) → the control hand-off → a NEW trace-studio
   session (the USER's "house freeroam" directive). The port reaches the dialogue but not yet the
   hand-off; the retail ground truth is now HAD.

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
