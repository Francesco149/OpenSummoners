# Freeroam standing TURN-AROUND — the reversal pivot (char-turn-state, ckpt 177)

Ports the STANDING TURN the port lacked (the ckpt-175 collision residual, ex-PORT-DEBT
`char-turn-state`).  A from-rest REVERSAL (facing right, command left) no longer SNAPS
facing + walks — it plays retail's 8-tick pivot.

## Ground truth — retail-stairs res 0x570 (bank 0x8b), the LEFT reversal @ 2950

`draw_probe.py retail-stairs.osr --res 0x570 2946 2962` (cel# reveals facing via the +152
left-mirror: <152 right, >=152 left):

| ticks | cel | facing | worldX | phase |
|---|---|---|---|---|
| ..2950 | fr 1 | right idle | still | idle |
| **2951-2954** | **fr 6** | right | **still (hvel 0)** | turn WINDUP (4t) |
| **2955-2958** | **fr 159** (=7+152) | **left** | walk ramp from 0 | turn flipped + walk |
| 2959+ | fr 160 | left walk | accel | walk |

So the turn = the SAME turn poses fr 6/7 the house cutscene turn uses (`ARCHE_HOUSE_TURN_CLIP`
plays their mirrors 158->7): a 4-tick STATIONARY windup on fr 6 (facing HELD), then FLIP
facing + begin the walk ramp (fr 7 -> +152 fr 159 lingers 4 more render ticks), then the
walk clip (fr 160).  The walk onset is delayed exactly **4 ticks = the -960 wx** the old
instant flip caused (240 dwx/tick × 4).  The old port SNAPPED: fr 160 (walk) from the latch,
no turn cels — moving ~4 ticks early.

## The RE (why 4 ticks; not curve-fit)

The turn is `0x442a70`'s STATE-1 FSM (`:1011-1090`): a from-rest reversal (`iVar9`
command-vs-facing switch, `local_14` set at `hvel==0`) requests a turn ACTION via
`FUN_0040a540(body,2,200)` when the tunables gate it (Arche `in_ECX[0x5661]=1`
`[0x5662]=0`, entity-init `0x8b/8c/8d` @ all.c:23196-98; the gate `((0x5661==0)||(param_2==0
&& 0x5662==0))` = INSTANT flip iff `param_2==0`, else the action).  `body+0x48=2` then routes
the top switch (`0x442a70:191`) to DEFAULT (movement frozen) while the ANIM form-FSM
`0x45e830` plays the turn clip + flips facing `+0x2c` at its keyframe (the same `+0x54`
sub-state that flips facing on the BACK-attack, `45e830:363-365`).  The 8-tick / dur-4 clip
length is the un-ported `0x45e830` turn form — MEASURED off the draw stream (fr 6 held 4t,
fr 159 held 4t), the same measured-clip stand-in as `ARCHE_WALK_CLIP`
(`char-walk-anim-distance`).  NB `0x45e830` does NOT play `body+0x48==2` in the decompile
(switch `uVar8-10` -> default) — the walk-turn form routing is the un-RE'd remainder; the
observable pivot (cels + timing + hold) is reproduced exactly regardless.

## The port (character.c + actor_spawn.c + main.c)

- `character.c` `character_step`: the from-rest reversal branch starts `turn_ctr` (1..8).
  `turn_ctr <= CHAR_TURN_HOLD(4)` = WINDUP (vel braked to 0, facing UNCHANGED, `turn_frame=0`
  = fr 6); at `CHAR_TURN_HOLD+1` FLIP facing + `walk_accel` (the extracted held-dir ramp,
  shared with the normal walk); `turn_frame=1` (fr 7) through `CHAR_TURN_TOTAL(8)`; then
  `turn_ctr=0` -> the walk clip resumes.  `character_turn_frame()` exposes the render cel.
- `actor_spawn.c` `ARCHE_TURN_CLIP` = base 0, delta {6,7}: frame 0 = fr 6 (windup), frame 1 =
  fr 7 (-> +152 fr 159 left / +0 fr 7 right).  BOTH reversal directions emerge from the
  renderer's +152 facing mirror — R->L renders 6->159, L->R renders 158->7 (== the house turn).
- `main.c` `freeroam_step`: while `character_turn_frame() >= 0` the clip is `ARCHE_TURN_CLIP`,
  its FRAME forced from that (not the clip timer) so the sim flip + the cel stay locked.

## Verification

- **host `test_character_turn_around`**: the SIM law tick-exact — warmup, 4-tick windup
  (dwx 0, facing held, turn_frame 0), flip (dwx -16, facing left, turn_frame 1), the -16k
  ramp to the -240 cap, turn_frame 1 linger then -1.  1088/1088 pass.
- **draw_probe `port-stairs2.osr`**: the port now renders fr 6 x4 -> fr 159 x4 -> fr 160 —
  retail's exact turn structure (was fr 160 snap).
- **state_diff `port-stairs2 | retail-stairs`**: RIGHT walk 2050-2350 MATCH tol=0 / 301 ticks
  (no regression); LEFT walk RAMP SHAPE bit-exact (port relwx 0,-32,-80,-144.. ≡ retail),
  the -960 gap GONE.

## Residual — a constant 1-tick reversal-onset phase (−240, NOT the turn)

The port's pivot starts 1 tick EARLY (fr 6 from 2950 vs retail 2951 -> walk-start 2955 vs
2956 -> a constant −240 relwx).  This is NOT the turn windup (bumping `CHAR_TURN_HOLD` would
delay the walk but over-hold fr 6 = a cel mismatch): it is the reversal LATCH phase — the
port detects the fresh LEFT press 1 tick before retail's warmup gate (`char-input-autorepeat`,
the wall-clock 11ms gate the constant `DELAY=2` approximates), within retail's ±1-2 tick
coalescing slop (the RIGHT walk from the hand-off is 0-div, so it is reversal-specific).
The turn STRUCTURE is bit-exact; the 1-tick onset folds into `char-input-autorepeat`.
