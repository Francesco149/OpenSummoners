# Freeroam U/D-POSE commands (crouch / slide / up-defensive) — ckpt 153

**Status: the COMMAND layer is PORTED + host-tested (1051 pass, +6); the APPLY
physics is the next chip (needs a live const capture).**  USER directive (ckpt 152):
"port + verify ALL freeroam MOVEMENT TYPES" — dash ✓ (ckpt 150), now the U/D moves:
**crouch** (DOWN in place), **slide** (DOWN during a dash), **up-defensive** (UP held
stops you faster / enters doors).  All three are driven by the AI command `cmd[3]`
(@ entity `+0x14860`) → consumed by the apply integrator `0x442a70` (case 0x75).

## The command block (`0x478ba0`, RE-confirmed)

`0x478ba0` snapshots `entity+0x14854` into `local_608[0..7]`, zeroes it, re-derives each:
- `cmd[0]` `+0x14854` — WALK/DASH L/R (1/2 walk, 5/6 dash) — ported (`character_step` +
  `character_resolve_run`).
- `cmd[2]` `+0x1485c` — JUMP (7 ring-execute / 8 held / 9 down-jump) — ported.
- **`cmd[3]` `+0x14860` — the U/D POSE (10 DOWN / 0xb UP)** — THIS chip's command half.
- `cmd[4]` `+0x14864` — ATTACK (0xe/0xf) — not ported.

### cmd[3] resolution (478ba0:248-259) — clean, no var-reuse traps
```
DOWN (:248-253): r = FUN_00479960(now,10,800,1, 3, ...)        // ring id 3 (DOWN), [10,800]ms
  if (down_held(+0x118) && (local_608[3]==10 || -1<r || now-B[+0x144]>0xf0))  +0x14860 = 10;
UP   (:254-259): r = FUN_00479960(now,10,800,1, 1, ...)        // ring id 1 (UP),   [10,800]ms
  if (up_held(+0x114)   && (local_608[3]==0xb || -1<r || now-B[+0x140]>0xf0))  +0x14860 = 0xb;
```
- Held-direction AND one of: **self-sustain** (prev cmd[3]); **a fresh ring press** aged
  in [10,800]ms (the primary trigger; the 10ms floor = a 1-frame input buffer); **held
  >240ms** (`array_B` press-ts at +0x140/+0x144).
- UP runs SECOND, writes the same slot ⇒ UP overrides DOWN if both held.
- `FUN_00479960(now,lo,hi,flag,id,…,used,consume)` = scan ring `+0xc` (64) for id==X,
  flag==1, age∈[lo,hi]; `used`=NULL ⇒ return first match index (no mark), `consume`=0.

### the RING-ID bug fixed
`input.h` had `INPUT_RING_DIR_DOWN 1 / UP 3` — **backwards**.  Four sources agree on
**UP=1, DOWN=3, LEFT=2, RIGHT=4**: the producer's fixed arrows (0xc8/0xd0/0xcb/0xcd →
1/3/2/4), the title-menu nav (id 1=up/3=down), and 478ba0 itself (UP block scans id 1,
DOWN block id 3).  Never bit because only the L/R dash (2/4, correct) consumed these.
The comment above the macros even said UP=1/DOWN=3 — the `#define`s contradicted it.

## The port (command layer)
- **`input.{c,h}`** — fixed the ring-id macros + `input_ring_find_recent(m,now,dir,lo,hi)`
  (FUN_00479960 w/ NULL used-map: first pressed `dir` aged in [lo,hi], else -1).
- **`character.{c,h}`** — `character_resolve_pose(c,m,now,axis)` mirrors :248-259: snapshot
  `cmd_pose`, reset, DOWN-block then UP-block (UP wins).  New `int16_t cmd_pose` field
  (0/10/0xb self-sustain).  Constants `CHAR_POSE_{DOWN=10,UP=0xb,WINDOW_LO=10,WINDOW_HI=800}`.
- **`main.c` `freeroam_step`** — resolves the pose each tick off `g_game_drive.input` +
  `GetTickCount()`; emits `fr_pose`/`fr_lr`/`fr_wx`/`fr_vel` as OSR_STATE (binary-verify
  the command layer).  `character_step` does NOT consume `cmd_pose` yet (apply = next chip).
- Tests: `character_resolve_pose_{down_up,window,up_overrides_down}` (3) +
  `input_ring_find_recent_{hit,window,rejects}` (3).

## The APPLY physics — CAPTURED + PORTED (ckpt 153, bit-exact)

`0x442a70` case 0x75 (the locomotion case `character.c` ports) reads `cmd[3]` (= `param_1[3]`)
and drives the body FSM (`body+0x38`, set by `FUN_00426f50(body,state)`).  Pinned by the
per-tick body capture (`runs/pose-demo/cap-body`, the 3 phases — `bstate`/`hvel` per tick):
- **DOWN (cmd[3]==10) → state 2 (CROUCH)** for the player (`[0x5675]`=1 gate).  Sub-states
  0→1→2 drive the crouch anim; the VELOCITY law is the brake below.  A **SLIDE is just a
  crouch entered with momentum** (state 2 from a dash/walk) — same state, the hvel just
  starts higher.  (The decompile's state-6 + the `[0x5656/57]`=64000/4000 consts feed a
  NON-player path `local_4`≠0, not the player's down — a decode trap avoided by the capture.)
- **UP (cmd[3]==0xb) → state 5 (DEFENSIVE)** (`[0x5684]`=1 gate).
- **The law (states 2 & 5 set `bVar16=false`, :959 ⇒ skip the accel ramp):** the velocity
  brakes toward 0 at the WALK brake **−800/tick**, EVEN WHILE A DIRECTION IS STILL HELD.
  Capture proof: UP-pose from a 24000 walk (cmd0=2 right STILL commanded) → hvel
  24000,23200,22400,…,800,0 (−800/tick) = "UP stops you faster"; crouch-from-walk identical;
  crouch-from-rest holds at 0.  Bit-exact, no hidden faster rate.
- The UP DOOR/LEDGE enter (478ba0:260-285) runs the apply in PROBE mode + `FUN_00440f40`
  hit-test → also sets cmd[3]=0xb.  Collision-coupled ⇒ PORT-DEBT(char-up-door-probe).

**The port (`character.c` `character_step`):** when `c->cmd_pose != 0` (and grounded) skip
the accel ramp and `ramp_toward(vel, 0, CHAR_WALK_BRAKE)` — overriding the held-direction
accel.  Host-tested bit-exact (`character_pose_brakes`: 24000→0 while right held; the run
cap 48000→0 for the slide; crouch-from-rest 0).  No new const needed (the −800 brake is the
known `[0x565e]`).  RESIDUAL: the crouch/up ANIM cels (the visible pose sprite) =
PORT-DEBT(char-pose-anim) — this chip is the MOVEMENT (the body stops/slides), not yet the
crouch sprite.

### ckpt-117 lesson — confirmed
The case-0x75 ramp's decompile reuses stack slots (the apparent state-6 slide path / the
`[0x5656/57]` consts were a trap).  RE'd the structure (states 2/5, accel-disable) but PINNED
the law by the per-tick capture — which showed DOWN→state 2 (not 6) for the player and the
uniform −800 brake.  Never a line-by-line port.

### the move-tuning constants — CAPTURED (ckpt 153, off Arche's idle errands entity)
Read live off Arche's entity (`pose_consts_fields.json`, `runs/pose-demo/cap-consts2`, the
errands freeroam frame 4554 — wx=19200, idle):

| `in_ECX[idx]` | value | role |
|---|---|---|
| `[0x565b]`/`[0x565c]`/`[0x565e]` | 24000 / 1600 / -800 | walk cap / accel / brake (sanity ✓) |
| **`[0x5675]`** | **1** | CROUCH gate (state-2 entry, 442a70:776) — Arche can crouch |
| **`[0x5684]`** | **1** | UP-pose gate (state-5 entry, :795) — Arche can up-pose |
| **`[0x5656]`/`[0x5657]`** | **64000 / 4000** | SLIDE consts (case-6 local_20 / param_3, :908-909) |
| `[0x5663]`/`[0x5666]` | 1 / 1 | run / dash gates (active) |
| `[0x566a]`/`[0x566b]`/`[0x566c]` | 48000 / 1600 / 3200 | = run-cap / walk-accel / run-accel (alt target) |
| `[0x5676]` | 2400 | crouch state-2 transition value (:825) |
| `[0x5659]`/`[0x5689]`/`[0x5204]`/`[0x5205]` | 0 / 0 / 0 / 0 | terrain / pose / buff gates off (flat ground) |

Both pose gates are 1 (Arche crouches + up-poses).  **The HOST self-starts** —
`frida_capture.py ensure_frida_server` auto-spawns frida-server via UAC-elevated
Start-Process (set `OPENSUMMONERS_FRIDA_SERVER_EXE` to the devtools path); the earlier
"host down, blocked" read was wrong (it just needed a capture to bring it up).

### per-tick body (the brake LAW) — capturing now
`pose_body_fields.json` (lean 7 fields) + `runs/pose-demo/pose-town-{nav,held}.jsonl` (the
3 phases: crouch-from-rest / up-from-walk / dash-then-slide) → `body+0x28` (hvel) +
`body+0x38` (bstate) per tick → pins whether crouch/up/slide brake at -800 (the var-reuse
decompile's apparent rate) or a faster "stops-you-faster" rate.  Then port + verify.

## Binary verification (port-side — the command works in the real exe)
Beyond the 6 host tests, drove the PORT into the errands freeroam (`runs/pose-demo/`,
the full-errands nav + DOWN/UP ring + held axis), `--osr-state`, read `fr_pose` per flip:
- **DOWN held + DOWN ring (id 3) → `fr_pose=10`** (flips 4821-5010 / ticks 1853-1947 —
  exactly the held-"down" window).
- **UP held + UP ring (id 1) → `fr_pose=0xb`** (flips 5127-5320 / ticks 2006-2102 — the
  held-"up" window); DOWN precedes UP, no crossing.
- The ring-id fix is CONFIRMED live: injecting ids 3/1 produced DOWN/UP (not swapped).
- `fr_wx` holds at the spawn (19200) the whole time — the pose resolves but does NOT yet
  move Arche, since `character_step` doesn't consume `cmd_pose` (the apply physics chip).
So the input→ring→command chain (`freeroam_step` → `character_resolve_pose` → the OSR emit)
is proven in the actual binary, not just the unit.

## Capture prep (ready for host-up)
The physics chip's capture is staged so it runs the moment the Frida host is back:
- **`tools/flow/pose_consts_fields.json`** — the field-spec: the new move-tuning consts
  (`[0x5656/57/59]`, `[0x5663]`, `[0x5666]`, `[0x566a/b/c]`, `[0x5675/76]`, `[0x5684]`,
  `[0x5689]`, `[0x5204/05]`) + the known ones (sanity) + the per-tick body
  (`hvel`=body+0x28, `bstate`=body+0x38, `wx`, `vvel`) + `cmd0`/`cmd3` (confirm the pose
  engaged).  Same leader chain as `jump_consts_fields.json` (the proven capconsts chain).
- **`runs/pose-demo/pose-nav.jsonl`** (DOWN ring id 3 + UP ring id 1 after freeroam) +
  **`pose-held.jsonl`** (hold "down" then "up").  Starting timing; tune the tick↔frame
  overlap against the first capture (use `--no-turbo --lockstep` so GetTickCount advances
  deterministically — the pose's [10,800]ms window needs real-clock advance, unlike the
  dash's [0,800]).
- **Recipe:** `frida_capture.py --no-turbo --lockstep --seed-pin --input-trace
  runs/pose-demo/pose-nav.jsonl --held-trace runs/pose-demo/pose-held.jsonl --field-spec
  tools/flow/pose_consts_fields.json --field-spec-only --call-trace --call-trace-frames
  <freeroam frames> --run-dir runs/pose-demo/cap` (inside `nix develop`).  Then read
  `hvel`/`bstate` per tick while DOWN/UP held → pin the apply states 2/5/6 → port → verify.

## PORT-DEBT
- `char-pose-holdtime` — the 240ms `array_B` held-time arm (the producer doesn't fill
  `+0x140/+0x144`); the ring [10,800] + self-sustain cover a continuous hold, so it's a
  redundant backstop here.
- `char-up-door-probe` — the UP door/ledge enter (478ba0:260-285), collision-coupled.
- `char-pose-physics` — the apply states 2/5/6 velocity effect (crouch/slide/up-stop) +
  the crouch/slide ANIM cels; pending the live const capture.
