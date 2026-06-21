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

## The APPLY physics (RE'd structure; the NEXT chip)

`0x442a70` case 0x75 (the locomotion case `character.c` already ports) reads `cmd[3]`
(= `param_1[3]`) and drives the body FSM (`body+0x38`, set by `FUN_00426f50(body,state)`):
- **DOWN (`param_1[3]==10`, :775-785)** → state **2** (CROUCH, if `piVar13`+`[0x5675]`) or
  state **6** (SLIDE, if `param_2==0`).  State 6 (:907-914) uses distinct accel/cap
  `[0x5657]/[0x5656]`, runs 8 ticks → back to idle.
- **UP (`param_1[3]==0xb`, :795)** → state **5** (DEFENSIVE), gated `piVar14`+`[0x5684]`.
- **states 2 & 5 set `bVar16=false` (:959)** ⇒ SKIP the accel ramp (:974 `if(!bVar16) goto
  LAB_00444b4b`) ⇒ the body can only BRAKE toward 0 = the "UP stops you faster" (a held
  dash would otherwise stay at cap) + the crouch's stop.  The ramp primitive is
  `FUN_00445db0(vel, accel, brake_over_cap, cap)` (separate accel vs over-cap brake).
- The UP DOOR/LEDGE enter (478ba0:260-285) runs the apply in PROBE mode (cmd=0xb) +
  `FUN_00440f40` hit-test → also sets cmd[3]=0xb.  Collision-coupled ⇒ PORT-DEBT.

### why the physics needs a CAPTURE (ckpt-117 lesson)
The case-0x75 horizontal ramp's Ghidra decompile REUSES stack slots for vertical+horizontal
terms and has control-flow artifacts (e.g. `local_8 = local_2c = 3` can't be the switch's
real selector).  RE the STRUCTURE (states 2/5/6, the accel-disable), but PIN the per-tick
velocity + the gate/tuning constants by a LIVE capture — never a line-by-line port.

### the constants the next chip must capture (off Arche's entity, `in_ECX[idx]`)
Known (capconsts, ckpt 117): `[0x565b]`=24000 cap, `[0x565c]`=1600 accel, `[0x565e]`=-800
brake, `[0x5667]`=-80000 impulse, `[0x5668]`=2000 / `[0x5669]`=8000 rise-gravs.
NEEDED (outside that band — extend the field-spec): `[0x5656]`/`[0x5657]` (slide cap/accel),
`[0x5675]` (crouch gate), `[0x5684]` (up gate), `[0x5689]`, `[0x5666]`, `[0x566a/b/c]`,
`[0x5663]` (run gate), `[0x5676]` (state-2 transition), `[0x5204]`/`[0x5205]` (buff flags),
`[0x5659]`.  Then drive retail freeroam (errands), inject DOWN/UP ring + held axis (the
dash-window harness), capture per-tick `body+0x28` (vel) + `body+0x38` (state) for
crouch/slide/up-pose; port the apply states 2/5/6; verify `differ`/tick-equal.

**BLOCKER:** the Frida host (`cutestation.soy:27042`) is DOWN — the capture needs it up.

## PORT-DEBT
- `char-pose-holdtime` — the 240ms `array_B` held-time arm (the producer doesn't fill
  `+0x140/+0x144`); the ring [10,800] + self-sustain cover a continuous hold, so it's a
  redundant backstop here.
- `char-up-door-probe` — the UP door/ledge enter (478ba0:260-285), collision-coupled.
- `char-pose-physics` — the apply states 2/5/6 velocity effect (crouch/slide/up-stop) +
  the crouch/slide ANIM cels; pending the live const capture.
