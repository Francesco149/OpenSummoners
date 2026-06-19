# Butterfly vertical flutter — the case-3 "jump" FSM (THEME 2, note #2 residual)

> **Status: PHYSICS RE'd + PORTED + bit-exact (ckpt 143); the flap TRIGGER is a
> captured PORT-DEBT stand-in.** The butterfly's vertical bob is the shared case-3
> airborne sub-FSM (the SAME physics as Arche's jump, `character.c`) with the
> butterfly's own constants, read straight off the install. The port reproduces
> the retail butterfly dst-Y **tick-for-tick** over the settled town (Y-axis
> bit-exact; 0 X regression). The only residual is the pre-existing horizontal
> position lag of the entering butterfly (`butterfly-bounds-writer`, unchanged).

## The gap
The 4 town butterflies (EFFECT `0xe29a`, res `0x3fa`) glide/flap up and down
~18px as they patrol. The chip-1 port (`butterfly.c`) did only the horizontal
patrol; worldY held flat (`butterfly-flutter` debt). This chip ports the vertical.

## The mechanism (RE'd, verified bit-exact off `runs/butterfly-flutter`)
The butterfly's apply pass (`0x485fc0` -> `0x442a70`) runs the **case-3 airborne
sub-FSM** (`body+0x38 == 3`) — the same FSM `character.c` ports for Arche's jump.
Per tick:
1. **Windup** (sub-state 0, `0x442a70:834-841`): a 4-tick charge while the body
   keeps gliding; on the 5th tick (`cnt > 4`) it snaps `vvel` to the flap impulse
   and advances to sub-state 1. The windup `goto`s past the grav-select, so the
   impulse tick uses the **pre-switch default grav** (the fall accel).
2. **Integrate** (`0x442a70:922`, the `0x54e5c0` vertical mover): `worldY += vvel/100`
   using the CURRENT vvel (so the impulse tick moves by impulse/100).
3. **Ramp** (`0x442a70:941`): `vvel = min(vvel + grav, cap)`, where the grav is
   the **rise** accel while `vvel < 0` (held vs free by `cmd_2`) else the **fall**.

### The constants (off the install `0x41f200` case `0xe29a`, lines 2175-2181)
`FUN_00427d30(0xffff8300, 1000, 4000, ...)` and `FUN_00427c30(1, 16000, 2000, ...)`
set the actor's physics fields — the port `#define`s mirror them exactly:

| field | actor off (idx) | value | meaning |
|-------|-----------------|-------|---------|
| impulse | `+0x1599c` (0x5667) | **-32000** | the flap up-kick (`body+0x18 = this`) |
| fall cap | `+0x15950` (0x5654) | **16000** | terminal (down) velocity |
| fall accel | `+0x15954` (0x5655) | **2000** | default / fall grav |
| rise grav (held) | `+0x159a0` (0x5668) | **1000** | while `cmd_2==8` (`param_1[2]`) |
| rise grav (free) | `+0x159a4` (0x5669) | **4000** | once `cmd_2` released |
| mover mode | `+0x1594c` (0x5653) | **1** | `0x427c30` p1 — selects the tile-scan AI branch |

So a flap = snap to -32000, +2000 on the impulse tick (= -30000), then +1000/tick
while the flap is held and rising, +4000/tick once released (cutting the rise
short — variable-height, exactly like Arche's jump), then +2000/tick falling,
capped at +16000. `worldY += vvel/100` integrates the bob.

## The TRIGGER — the shared terrain-aware mover (PORT-DEBT, captured)
What makes the AI *issue* a flap (`body+0x38 -> 3` via `0x467bb0:201`, gated on the
command-block `cmd_2 == 7`) is the autonomous wander move-command `0x43f880`. For
the butterfly (mode 1, open air) it **scans the collision grid DOWNWARD for the
floor** (`0x43f880:230-238`: `local_fc = floor_tile_row*0xc80 - body_height - worldY`)
and flaps to hold altitude above it — the jump is gated `0x43f880:311` on
`in_ECX[0x5659]=8000 <= local_fc` (don't flap while the floor is far). So the
butterfly glides down and flaps up to stay ~8000 units above the terrain it scans;
the irregular flap cadence (16-38 ticks) emerges from that tile-scan + the
every-other-tick AI gate + the command-priority dispatch (`0x4412d0`).

That is the **same shared terrain-aware character mover the freeroam arc needs**.
Until it lands, the per-tick flap trigger — the `(body+0x38==3, cmd_2==8)` control,
2 bits/butterfly/tick — is **captured** from a seed-pinned retail field-capture
(`tools/extract/butterfly_flap_ctrl.py` -> `src/butterfly_flap_ctrl.h`). Because
the port boots `OSS_RNG_DEFAULT_SEED` and the harness pins the same seed, the
spawn + flap cadence are deterministic and identical to `retail.osr`.
**`PORT-DEBT(butterfly-flutter-trigger)`** — retire when the terrain mover lands.

## Verification
- **Offline** (`runs/butterfly-flutter`, the captured AI fields): driving the
  ported physics from the `(state3, cmd2_held)` control reproduces the captured
  `vvel` with **0 mismatches / 1824 ticks x 4 butterflies**.
- **Host test** `test_butterfly_flutter`: the lane-0 `vvel`+`worldY` match the
  captured reference bit-exact at ticks 1/8/17/18/21/25 (the impulse, the held/
  free rise, the fall ramp); an unlaned butterfly just glides to the cap.
- **Port `.osr` vs `retail.osr`** (`draw_probe --res 0x3fa`, ticks 80-360): the
  butterfly dst-**Y matches retail tick-for-tick** (per-butterfly Y-mismatch = 1,
  a crossing/entry artifact). My change touched **0** dst-X positions; the X lag
  vs retail is byte-identical before/after (289==289) = the pre-existing
  `butterfly-bounds-writer` debt, NOT this chip.

## The port (`src/butterfly.{c,h}`)
Added vertical state (`world_y`/`vvel`/`flap_sub`/`flap_cnt`/`life_tick`/`ctrl_lane`)
+ the case-3 physics in `butterfly_step` (after the horizontal apply), driven by
`BUTTERFLY_FLAP_CTRL` indexed by `life_tick` (matched to a lane by spawn worldX).
`main.c` mirrors `b->world_y` into the EFFECT render-state each tick (like worldX).

Cross-refs: `docs/findings/butterfly-direction-sprite.md` (the sprite half),
`docs/port-debt.md` (`butterfly-flutter-trigger`, `butterfly-bounds-writer`),
`docs/findings/engine-quirks.md` (#93 the butterfly id), `src/character.c` (the
same case-3 jump FSM for Arche), `tools/extract/butterfly_flap_ctrl.py`.
