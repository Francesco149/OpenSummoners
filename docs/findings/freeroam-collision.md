# Freeroam collision — the 442a70 tick geometry (movers + support), ckpt 175

Retires the walk-through-stairs divergence (USER studio note tick 2441, ex-PORT-DEBT
`char-collision-mover`).  Verification: the stairs-sweep re-drive (`runs/sync/
synth-stairs.jsonl`, hold R 2050-2900 / hold L 2950-3800 over `spam-confirm-nav`) —
wx bit-exact vs retail through the FULL right walk incl. the staircase climb + the
flush wall stop at wx 87600 (`state_diff.py` port-stairs|retail-stairs).

## The retail structure (all raw-disasm-verified; Ghidra arg numbering WRONG on both movers)

**Tick order inside `FUN_00442a70`** (the kinematic commit; this=entity, param_3=dest body):
1. **SUPPORT PROBE** (:104-118): `FUN_0054e5c0(body, delta=+1, pass=body[0x20], boxsel=param_5,
   out=&local, outactor=&body+0x284)` — a 1-unit downward probe; blocked ⇒ standing.
   `body+0x24 = (probe blocked)`. Out-Y committed ONLY for an actor support (`+0x284`!=0).
   Terrain type 3 (`ent[0x5653]`) ⇒ unsupported unconditionally.
2. State FSM (`body+0x38` switch): case 3 = jump (windup>4 → impulse `ent[0x5667]`,
   rise grav held/free `ent[0x5668/9]`); case 4 falling + supported ⇒ landed recovery.
3. **VERTICAL MOVER** (:917-945), gated `(!supported || vvel<0)`:
   `FUN_0054e5c0(body, vvel/100, …, out=&body.y)`; blocked ⇒ `vvel=0` (+ landed flag if
   falling). Unsupported grounded state (`local_10`) ⇒ `FUN_00426f50(body,4)` = the
   LEDGE WALK-OFF → FALL. Then gravity: `vvel += grav` capped at terminal (`ent[0x565e]`).
4. Pose/facing + the `0x445db0` velocity ramp.
5. **HORIZONTAL COMMIT** (:1091-1112): `FUN_0054db10(body, 445f50(vel/100), step_down,
   step_up=1, pass=body[0x20], boxsel=param_5, probe=0)`; blocked ⇒ `vel=0`.
   `step_down = (param_4==0) || (ent+0x200==0)` — real-body apply passes param_4=2
   (0x4791e1 = the 0x478ba0 AI call, param_5=1) + ent200 live-probed 0 ⇒ **step_down=1**.
   `0x445f50` = negate delta when facing==3 (retail vel is forward-positive; the port's
   world-signed vel yields the identical delta — truncation symmetric).

**`body+0x20` = the DROP-THROUGH timer**, NOT a box index (cmd[2]==9 = down+jump sets 8,
decrements :726-731): it is the `pass_slopes` short every mover/probe receives — live ⇒
pass THROUGH class-1 slope surfaces + probe unsupported (platform drop).  0 in all
captures → PORT-DEBT(char-drop-through).

**The movers** (`0x54db10` horizontal / `0x54e5c0` vertical): actor-vs-actor pre-scan
(world `0x8a9b50+0x278c` list; PORT-DEBT(mover-actor-scan)) → the tile sweep.
- Vertical tile half = `FUN_0054e990` (ported ckpt chip-2 `collision_move_vertical`).
- Horizontal tile half = **`FUN_0054ded0`** (ported `collision_sweep_horizontal`, verbatim):
  ≤100-unit steps; per step (a) **STEP-UP**: leading bottom corner blocked + the cell one
  sub-row up clear ⇒ raise the window 100 (`top-=100; yshift-=100; bot-=100`) — the STAIR
  CLIMB; (b) **STEP-DOWN floor hug** (`step_down!=0`): ground within 2 sub-rows below
  (some cell blocked at bot+200, or OOB) AND the row at bot+100 fully clear ⇒ drop 100 —
  the STAIR DESCENT; (c) the leading-edge COLUMN SCAN `[top,bot]`: class 10 blocks
  (class-1 slope surfaces do NOT block the column scan; only the corner tests resolve
  them); blocked ⇒ return 0 IMMEDIATELY — prior steps' per-step `*out_x` write-throughs
  STAND (partial movement commits, = retail's write-through to `&body.x`) and the
  accumulated yshift is DISCARDED. Clear ⇒ clamp x to `[0, DIM0_PX-width]` (leaving
  bounds also reports blocked). yshift commits to `*inout_y` on a fully-clear sweep only.
- The horizontal scan's Y extent = `[y+body[0x14], y+height-1]` (a top margin).
  **Arche's box (live-probed, `runs/arche-box` + `tools/flow/arche_box_fields.json`):
  w=2000 h=5600 margin=0**; box dims refresh from `ent[10]/[0xb]` each tick (:119-120).

**Slope profiles**: region-B `+0x8` = `.rdata` VA `0x5cc410` (bytes 32..1 descending) /
`0x5cc430` (1..32 ascending), h(sub-X); cell solid below the surface
(`0x20 - h < suby`).  The port reads the identical bytes off the user's installed
sotes.exe (`exe_data_bytes`, `main.c game_slope_resolve`; `.rdata` byte-equal
packed/unpacked — verified).  Retired PORT-DEBT(collision-slopes).

## The errands grid gap found by the sweep (the "walks through the house" half)

Left walk: retail stopped flush at wx 6400 (cell boundary col 2), port sailed to the
map-edge clamp 0.  Retail live-grid probe (`runs/arche-box/gridcells`, chain
`[[0x8a9b50]+0x1048]` region B): **col 1 = class 10 rows 12-18** — a wall column the
port grid lacked at rows 13-17.  Cause: the `0x1b972`/`0x1b977` `LAB_00589520`
shape-1/2 block + `0x1b97c` case 1 were deferred as PORT-DEBT(decode-occlusion-mark)
("culling marks") — **misread: they are invisible COLLISION WALLS** (region-B a=10,
d4=6 + region-D=1): shape 1 = a 1-col × 5-row column at (x, y..y+4), shape 2 = the same
at x+1 (`0x1b97c` shape 1 = 1×1).  The errands `113015` cell at (col 1, row 13, shape 1)
emits exactly rows 13-17.  Ported in `map_decode.c`; debt retired.

## Port wiring (`character.c` + `main.c`)

`character` gains `coll_grid/coll_slope/coll_slope_ctx` (NULL = the open-air flat
reduction, host tests only) + `supported`.  `character_step` restructured to the retail
tick order: jump trigger → support probe → vertical (windup | mover+gravity | ledge
walk-off → fall with `vvel=CHAR_JUMP_FALL_GRAV`) → ramp → horizontal commit
(step_down=1, step_up=1, pass=0; blocked ⇒ vel=0).  `main.c freeroam_begin` wires
`g_town.grid` + `game_slope_resolve`; `load_town_scene` re-points on room reload.
OSR state adds `fr_wy`/`fr_sup` (port) + `wy` (retail proxy) for the y-axis diff.

## Verification (state_diff, seed-pinned, tick axis)

- RIGHT walk 2052-2342: **0 divergences** — accel ramp, the staircase region, and the
  stop flush at wx 87600 (leading edge 89599 = col-28 wall edge − 1) all tick-exact.
  Both sides idle blocked 2343-2951 at the same wx.
- LEFT walk: port moves at 2952, retail at 2956 — the 4-tick STANDING TURN-AROUND state
  the port lacks (constant −960 wx thereafter) → PORT-DEBT(char-turn-state).  Net of
  the turn offset the ramp shape is identical; retail's left stop = the (now-ported)
  col-1 wall.
- wy climb verification: the rerun with `wy`/`fr_wy` (this ckpt).

## Open threads
- `mover-actor-scan` (actor pre-scan + platform actors + `body+0x284`).
- `char-drop-through` (`body+0x20` timer + cmd[2]==9).
- `char-turn-state` (the 4-tick reversal turn; `0x426f50(body,2)` case-2 sub-FSM).
- `char-reverse-decel` (−accel vs brake on direct reverse — indistinguishable so far).
- `ent+0x200` semantics (step_down gate input; 0 throughout the errands captures).
