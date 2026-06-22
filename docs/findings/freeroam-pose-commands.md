# Freeroam U/D-POSE commands (crouch / slide / up-defensive) — ckpt 153 / 153b

**Status: COMMAND + APPLY physics + the visible POSE SPRITE are all PORTED + bit-exact.**
ckpt 153 = command + physics (below).  **ckpt 153b = the crouch/up ANIM cels (the
"char-pose-anim" debt) — RE'd off retail + ported + verified (the "## The POSE ANIM"
section).**  USER directive (ckpt 152):
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

## The SLIDE (dash-then-down) — PROVEN state-2 CROUCH from 48000 (ckpt 154)

The ckpt-153 dismissal of state-6 ("non-player path") was UNTESTED FROM A DASH — cap-body
only reached DOWN from a WALK (cmd0=2, 24000; its `ids:[4,4]` came AFTER the held-right so
the double-tap never fired — the #3395 trap).  Captured a REAL dash-then-down
(`runs/pose-demo/cap-slide3`: the proven inject `ids:[4,4]`+held right → **cmd0=6, hvel→48000**,
THEN DOWN ring+held, RIGHT still held): the apply puts Arche in **state 2 (CROUCH)**, hvel
brakes **48000→0 at −800/tick** (per-sim-tick: 45/47 deltas −800, the 2 outliers = missing
samples; tick 1581 hvel 48000 → tick 1582 cmd3=10 bstate=2 hvel 47200 = −800), cmd0 stays 6
through the whole brake.  The discriminators (added to `pose_consts_fields.json`):
- **`[0x5653]` (entity+0x1594c) = 0** = the FLAT-GROUND terrain/collision mode ⇒ `local_4`
  stays 0 (442a70:736-740 sets it only for `[0x5653]`∈[1,3]) ⇒ the state-6 branch
  (442a70:780-783) is UNREACHABLE ⇒ DOWN → state 2 always.
- **`param_2` (`*(body+0x24)`) = 1** (≠0) ⇒ even with `local_4` set, 442a70:781 `param_2==0`
  would still gate out state 6.

So on flat ground a SLIDE IS a CROUCH (state 2) entered with the 48000 dash momentum,
bleeding at −800/tick over ~60 ticks (a ~119px glide) = "hold to keep sliding".  **The port
already does exactly this** (`character.c:78-89` brakes ANY pose to 0 at −800; the host
`character_pose_brakes` 48000→0 case == cap-slide3 tick-for-tick).  No code change — the chip
was a VERIFICATION that resolved the open question (state 2, not 6, from a dash).

**State 6 (the REAL slope-slide) — UNREACHED, `PORT-DEBT(char-slope-slide)`.**  442a70 case 6
(907-914 + 975-1036): on terrain `[0x5653]`∈[1,3] with `param_2==0`, DOWN enters state 6 which
(a) does NOT skip the accel ramp (`bVar16` stays true at :959) → for cmd0=6 facing-aligned,
`iVar5`=+`[0x566c/b]` toward cap `[0x566a]`=48000 → MAINTAINS the dash speed (true momentum
slide, not a brake); (b) case-6 adds `vvel += [0x5657]`=4000 cap `[0x5656]`=64000 (the
slope-fall term); (c) exits to idle after 8 ticks (`body+0x3c`).  Needs a SLOPE scene to
capture + port — every freeroam scene reached so far (errands/town) is flat (`[0x5653]`=0).

### Verified off port-slide.osr (ckpt 154)
Drove the port into a dash-then-down (`runs/pose-demo/port-slide-{nav,held}.jsonl`,
`C:\oss-osr\port-slide.osr`).  `draw_probe --res 0x570`: dash cels 16→20 (dst-x +5px/tick =
the 48000 cap), then on DOWN → CROUCH **enter cel 31** (4t) → **hold 32** while dst-x
decelerates +5,+4,+3,+2,+1,+0 (the −800/tick brake) over ~60 ticks (dx 275→412 = **119px
glide**) → on release **exit 31** (5t) → idle.  The slide renders the ckpt-153b-verified
state-2 crouch cels gliding on the bit-exact brake = the cap-slide3 physics, drawcall-faithful.

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

## The POSE ANIM (the crouch/up SPRITE) — RE'd + PORTED (ckpt 153b, char-pose-anim)

The MOVEMENT (ckpt 153) was bit-exact but Arche rendered her idle/walk cel while
posing ("slides around in one pose").  RE'd the cels off the retail DRAW STREAM —
the established `draw_probe.py --res 0x570` pattern (bank 0x8b = Arche), which
needed a retail `.osr` of the errands freeroam with DOWN/UP HELD.

**The capture tool (the gap):** the trace-studio proxy only injected the discrete
RING (one-frame edges); a pose needs a HELD direction.  Added **held-axis
injection** to the proxy (`tools/capture_proxy/engine_input.h`, committed 2f16a72):
hook the leaf key-state query `FUN_005ba520(scancode)` and write 0x80 into
`device+0x18+scancode` for held scancodes → the producer `FUN_0046a880` fills
`mgr+0x114..` naturally (quirk #41).  `OSS_HELD_TRACE` = `{"frame":N,"keys":[…]}`,
3rd arg to `run_proxy.sh`.  Capture recipe + a footgun in **## The retail capture**.

**Ground truth (`retail-pose.osr`, res 0x570, Arche at the errands spawn 162,336):**
a 3-phase FSM keyed on the body sub-state (`body+0x3a`) — a TRANSITION cel on enter
AND exit, holding a steady cel between:

| pose | cmd_pose | body state | enter (4t) | hold | exit (5t) → idle |
|------|----------|-----------|-----------|------|------------------|
| CROUCH / slide | 10 | 2 | cel **31** (37×53) | cel **32** (42×40, the low crouch) | cel **31** → 0 |
| UP-defensive   | 0xb | 5 | cel **34** (33×60) | cel **35** (37×57) | cel **34** → 0 |

(Cross-checked vs cap-body's `bstate`: crouch engages at cmd3=10/main-state 2,
sub 0→1; the cel TRANSITION 31→32 tracks the sub-state.)

**The port (`actor_spawn.{c,h}` + `main.c`):** `arche_pose_clip(st, cmd_pose, …)` —
a pure, host-tested FSM (`test_arche_pose_clip`).  While a pose is engaged it
returns the enter→hold one-shot (`ARCHE_CROUCH_CLIP` {31,32} dur 4 / `ARCHE_UP_CLIP`
{34,35}); on release it plays the exit transition cel (`ARCHE_*_EXIT_CLIP`) for
`ARCHE_POSE_EXIT_TICKS`=5, then falls through to `arche_freeroam_clip` (walk/idle).
`freeroam_step` calls it once per sim-tick off `g_freeroam_char.cmd_pose` (the pose
wins over walk/run).  Quirk #114 extended.  **Verified:** a port `.osr` (drive into
the errands freeroam + held DOWN/UP) renders cels 31/32/34/35 at res 0x570 ==
retail (`## The POSE ANIM verification`).  Retires `PORT-DEBT(char-pose-anim)`.

**LEFT-facing — DONE too (via the +152 mirror).**  See "## The freeroam mirror + the walk/idle cels".

## The freeroam mirror is +152, and the right walk/idle cels were wrong (ckpt 153b)

Captured the full freeroam walk/idle set (`retail-walk.osr`: held RIGHT→idle→LEFT→idle;
`retail-poseL.osr`: left pose).  Two findings overturned the ckpt-144 stand-ins:

1. **The LEFT-facing mirror is a UNIFORM `+152`, not `+4`.**  Every freeroam animation's
   left cels = its right cels **+ 152**: idle 0-2 → **152-154**, walk 8-15 → **160-167**,
   run 16-21 → 168-173, crouch 31/32 → **183/184**, up 34/35 → **186/187**.  So bank 0x8b
   IS engine-mirrored (the renderer's facing==3 flip adds `flip_table[0x8b]`) — the port
   just had the wrong offset (`ARCHE_FREEROAM_FLIP` 4 → **152**).  One value fixes the
   left-facing walk, idle, run, AND pose at once — no per-animation left clips.
2. **The RIGHT walk + idle cels were WRONG.**  Retail's freeroam right WALK is cels
   **8-15** (8-cel cycle, dur 6) and IDLE is **0,1,2** (dur 14) — not the ckpt-144 stand-ins
   (walk 0-3 / idle 0-1).  The ckpt-144 "+4 walk verified 1:1" didn't scrutinise the cels.

**The fix (unified):** `ARCHE_FREEROAM_FLIP` = 152; `ARCHE_WALK_CLIP` = cels 8-15 (dur 6);
`ARCHE_FREEROAM_IDLE_CLIP` = cels 0-2 (dur 14).  The pose reverted to the RIGHT clips
(31/32, 34/35) rendered at the CHARACTER facing — the +152 flip produces the left cels
(183/184, 186/187), so the dedicated-left-clip workaround (the earlier ckpt-153b approach)
was removed.  **VERIFIED off `port-walkidle.osr`** (held RIGHT→idle→LEFT→idle→crouch→up):
right walk 8-15, right idle 0,1,2; left walk 160-167, left idle 152-154 (showed 152,153);
left crouch 183/184, left up 186/187 — all == retail via the single +152 flip.  This
RESOLVES `PORT-DEBT(char-freeroam-left-cels)` for walk/idle/pose.

RESIDUAL: the left RUN (16-21 → 168-173) + left JUMP via the +152 flip are consistent with
the pattern but not directly captured (verify when a dash-left / jump-left is exercised).

## PORT-DEBT
- `char-pose-holdtime` — the 240ms `array_B` held-time arm (the producer doesn't fill
  `+0x140/+0x144`); the ring [10,800] + self-sustain cover a continuous hold, so it's a
  redundant backstop here.
- `char-up-door-probe` — the UP door/ledge enter (478ba0:260-285), collision-coupled.
- `char-slope-slide` — the REAL state-6 momentum slide (442a70 case 6: maintain dash speed
  + slope-fall `vvel+=4000` cap 64000, exit after 8t).  Gated on terrain `[0x5653]`∈[1,3] +
  `param_2==0` — UNREACHED in any flat freeroam (`[0x5653]`=0; cap-slide3).  Capture + port
  when a SLOPE scene is reached.  On flat ground DOWN-from-dash is state-2 crouch (done).
- ~~`char-slide-from-dash`~~ — RETIRED ckpt 154 (PROVEN state-2 crouch / −800 brake from the
  48000 dash, cap-slide3; the port + host test already bit-exact; see "## The SLIDE").
- ~~`char-pose-physics`~~ — RETIRED ckpt 153 (the apply states 2/5 brake law, bit-exact).
- ~~`char-pose-anim`~~ — RETIRED ckpt 153b (the crouch/up SPRITE, BOTH facings: right
  31/32/34/35 + left 183/184/186/187 via the +152 flip; see "## The POSE ANIM" + "## The
  freeroam mirror" above).
- ~~`char-freeroam-left-cels`~~ — RETIRED ckpt 153b (folded into the +152 mirror fix): the
  right walk/idle cels were corrected (8-15 / 0-2) and `ARCHE_FREEROAM_FLIP` 4→152, so the
  left walk (160-167) + idle (152-154) now render correctly via the renderer flip — verified
  off `port-walkidle.osr`.  (Left run/jump via the same +152 flip = the small residual above.)

## The SWORD — DRAW + sword-out state (ckpt 156, chip 1; quirk #115)
USER ground truth = the real-play recording `sword-realplay.osr` (FRESH new game, res 0x570).
Z (the unsheathe key) TOGGLES the sword; X attacks when out (chip 2).  Ported the DRAW + the
sword-out STATE; the ATTACK (combo/directional) + sword-out walk need the clean injected
capture (chip 2).
- **Input:** Z = ring **9** (`INPUT_RING_SWORD`; the live keybind dump, ckpt 155 — was
  unwired).  `input_live.c` KEYMAP `{DIK_Z,-1,9}`; `character_resolve_sword`
  (`input_poll_consume` id 9, consume-on-read = one toggle/press) flips `character.sword_out`.
  Wired in `freeroam_step` (ring is NULL while the opening dialogue locks input → Z dead until
  control hands off, = retail's post-tutorial enable point).
- **DRAW cels (RE'd off the recording, ticks 2152-2199):** res 0x570 **96→97→98→99→100→101→
  102→103** then the sword-out idle resumes.  Per-cel hold 96 ~7t / 97-100 = **3t** (the fast
  swing) / 101 = 9t / 102 = 9t / 103 = 12t = 3·{2,1,1,1,1,3,3,4} ⇒ a uniform dur-3 clip with
  the slow cels repeated (`ARCHE_SWORD_DRAW_CLIP`, 16 frames, oneshot ~48t — the ARCHE_RUN_CLIP
  "repeat a held cel" idiom).  cel WIDTHS 41/35/54/67/41/33/32/31 (the sword swinging out).
- **sword-out IDLE/WALK = the BASE cels** (the recording's sword-out idle is the RNG fidget over
  cels 0-3 @14t = the base idle; no distinct sword-out idle cels).  So `arche_sword_clip`
  delegates to `arche_pose_clip` (walk/idle/pose) when no draw/sheathe transient is active.
- **SHEATHE = reverse draw (103→96)** = `PORT-DEBT(sword-sheathe-cels)` stand-in (the USER never
  sheathed in the recording; real cels from the chip-2 capture).
- **Gate:** retail needs `weapon+0xd4=2` (errands quest case 8, `4dc510:1167`); the quest is
  unported ⇒ `PORT-DEBT(sword-quest-gate)`: Z toggles freely in the errands freeroam (the
  same post-tutorial point retail enables it).
- **VERIFIED** off `port-sword.osr` vs `sword-realplay.osr` (`draw_probe --res 0x570`): the draw
  plays 96→103, every cel's dst W×H **byte-identical** to the recording, per-cel durations EXACT
  for 97-102 (3,3,3,3,9,9), ±2t only at the entry/exit boundary frames (the FSM-timer/stepper
  pre-advance; within the non-lockstep recording's noise; total 48t vs 49t); the toggle sheathes
  103→96.  `arche_sword_clip`/`character_resolve_sword` host-tested (`test_arche_sword_clip`,
  `test_character_resolve_sword`).  Plan: `plans/freeroam-sword-system.md`.
- RESIDUAL (chip 2): the ATTACK (X, neutral 120-127 + 128-132 + the 3-combo/directionals), the
  attack TRAIL vfx, the sword-out WALK/crouch/jump cels, and the real SHEATHE cels — all need the
  clean Frida force-quest-state (`weapon+0xd4=2`) injected capture (the recording is a single
  held-X session → can't disambiguate the attack structure).
