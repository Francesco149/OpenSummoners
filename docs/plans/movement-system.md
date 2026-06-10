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
| master band walk | `0x46cd70` | 1031 | walks the per-tick bands, calls the actor update once per actor; **no separate apply call** — motion happens inside the AI path | **PORT MIRRORS** (`game_actor_update`, band order proven ckpt 98/99) |
| actor AI / update | `0x47b990` | 7461 | per-tick per-actor. Big `switch(this+0x1d4)` on actor-code. Decides a move, runs a state machine (`switch(this+0x1422c)`) | **PARTIAL** — `butterfly_step` (ckpt 98) consumes the gate/flit-timer/heading RNG draws ONLY; the motion + state machine are unported |
| movement intent | `0x43f880` | 5491 | builds an ordered action list (axes `local_b8[0..4]`∈{0,1,2,5,6}; action-types `local_80`∈{1..6}), collision-tests each, writes the chosen 8-int **command block** to `this+0x14854`, returns a result code | unported |
| swept collision probe | `0x4412d0` | 993 | allocs a temp swept-path (`operator_new 0xa500`), steps it via `0x442a70`, tests vs the entity list (`DAT_008a9b50+0x278c`) + the tile grid (`0x440e40`). Returns blocked/clear. **Does NOT move the actor.** | unported |
| path stepper / integrator | `0x442a70` | ? | advances one physics sub-step. Used for probing (temp buffer) AND (**TBD**) the real apply | unported |
| ledge/dir probe | `0x47dbb0`, `0x441ae0` | ? | directional collision-grid probes (clear/blocked in a direction) | unported |
| tile collision grid | `DAT_008a9b50+0x1048` → `+0x2c1030` (row widths) / `+0x2c1040` (tile flags) | — | indexed by `worldX/0xc80`, `worldY/0xc80`. **`0xc80` = 3200 = one tile = the movement quantum** | unported |

### The command block `this+0x14854` (8 ints / 32 bytes)
The **resolved per-tick move**: `{action_type, dx, dy, sub, …}`. Written by `0x479e40`
("set move command"), zeroed by `0x411530`, the chosen one copied out at the end of both
`0x47b990` (→`+0x14854`) and `0x43f880` (→`this+0x5215`==`+0x14854`).
**OPEN (chip 1):** which step READS `+0x14854` and integrates body position (the "apply").
No obvious separate apply in `0x46cd70`, so it is inside the AI path — most likely
`0x442a70` invoked on the **real** body (not the temp probe buffer) by one of the state
handlers, or by `0x484c10` (called in `0x47b990:826` before the state switch). **Pin this
empirically in chip 1** — do not try to read every helper.

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

### Bounds `+0x14264`/`+0x14268` — the one decompile LIE (chip-1 lynchpin)
The `0xe29a` case passes `iVar2 = *(this+0x14264 | 0x14268)` as the **target** to
`0x43f880`. A full static grep of every decompiled fn finds **no write** to either offset,
yet:
- the `0xe29a` logic proves the butterfly moves toward that target (`0x43f880` with target=0
  ⇒ always moves toward worldX 0 ⇒ always-left), and
- the ground truth shows **bidirectional** drift (butterfly B +4410, C/D −700..−900 over 53
  ticks) ⇒ the bounds are **non-zero, per-instance**.

So they ARE set, via a path static analysis can't see (computed pointer / mangled
decompile). **Chip 1 step:** `mem_watch` `this+0x14264` across butterfly spawn + first
ticks (live) to recover the writer + the value (likely `spawn_worldX ± wander_range`).
This is the canonical "decompile ambiguous → hook the real value" case.

## Ground truth

**Have** — `runs/butterfly-emit` (settled town, seed-pinned lockstep): 2 sparse snapshots
(sim_ticks 294, 347), 4 butterflies, `wx/wy/cel_fr`:
- drift ~25–80 u/tick **with direction changes**; left pair wanders ~102k–108k (flowerbeds),
  right pair ~173k–188k (fountain).
- `cel_fr` = **direction_base** (multiples of 4: {0,4,8,12,16,24,28} seen) + flap(0..2). So
  ≥8 dir/colour variants, and the base **follows the movement direction** — NOT a fixed
  spawn frame_base. ⇒ the "multicolor" half of @1627 is also FSM-driven (the rendered
  direction-frame tracks heading), so motion + colour land together.

**Need** (chip-1 step 0) — a **dense per-tick** capture: a `retail_fields.json` field-spec
on the butterfly actor reading per sim-tick: `body+4/+8` (worldX/Y), `+0x14244` heading,
`+0x1422c` state, `+0x14234`/`+0x14248` timers, `+0x14264`/`+0x14268` bounds, `cel_fr`,
seed-pinned + lockstep over the reveal→settle window. This is the bit-exact target.

## Chips (each ends at a `/clear`)

0. **Per-tick butterfly-state field-spec capture** — the validation target (above). Also
   `mem_watch` the bounds writer. (Small; do first in the next session.)
1. **Butterfly open-air motion** — port the `0xe29a` heading FSM (`0x47b990`: gate/flit
   timer already done → add the heading flip + the `0x43f880` call) + the **reduced**
   `0x43f880` path (open air ⇒ probes clear ⇒ integrate toward target at the flit velocity)
   + the apply (`0x442a70` on the real body, pinned in step 0) + the direction→`cel_fr` map.
   Validate **bit-exact** vs the capture (sim-tick axis). Closes **butterfly-wander**
   (motion + multicolor). Tag the open-air reduction `PORT-DEBT(...)` so it doesn't fork the
   shared `0x43f880` — chip 2 generalises it.
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
