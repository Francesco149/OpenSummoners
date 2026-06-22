# Freeroam sword ATTACK (chip 2) — RE + port

> Sibling of `freeroam-sword-system.md` (the plan + the chip-1 draw/sheathe) and
> `freeroam-pose-commands.md` (the U/D-pose + slide).  Ground truth: the USER's clean
> real-play recording `sword2.osr` + its input trace `sword2-input.jsonl` (the
> OSS_INPUT_RECORD held-trace).  Probe: `tools/trace_studio2/sword_cels.py` +
> `tools/trace_studio2/attack_probe.py` (flip→tick + input interleave).

## The command (RE'd off 0x478ba0 — the char-AI builder)
X = `DIK_X` (scancode 0x2d) → `axis_held[5]` (the +0x128 attack LEVEL) + ring 0x24
(confirm).  The attack is the cmd[4] arm:
- **478ba0:296-299** `if (entity+0x158a4 + 0x128 != 0) && (200 < now − +0x154)` →
  `FUN_00479f90` + cmd[4] = **0xe** (the X-held auto-attack, a 200 ms refractory off
  `+0x154` = the last fire).
- **478ba0:301-303** `if (param_1+0x66 != 0 [SWORD OUT] && +0x68 == 0 [not mid-swing]
  && id 9 in 400 ms)` → cmd[4] = **0xf** (the discrete attack).
- **478ba0:459-460** `if (+0x66 != 0 || cmd[4]==0xe)` → cmd[4] = **0xf** (normalize).

So **X (the +0x128 held level) → cmd[4]=0xe → 0xf** (the swing).  The consumer
0x442a70 sets `+0x68` (mid-swing) so a new swing can't start until this one finishes
(`+0x66` is the charge/drain meter — gameplay, deferred).

## The cels (RE'd off sword2.osr res 0x571, tick-aligned to the input trace)
All on bank **0x8c = res 0x571** (sword-OUT, blade baked in); LEFT = **+192**
(ARCHE_SWORD_OUT_FLIP).  RIGHT-facing cels:

| attack | trigger (facing R) | cels | dur | movement | end state |
|--------|--------------------|------|-----|----------|-----------|
| **NEUTRAL** | X (no dir) | **104→109** | 6 each (36t) | STATIONARY | idle |
| FORWARD | dir+X | **120→126** | ~6 (42t) | LUNGE +54px fwd | idle (moved) |
| DOWN | DOWN+X | **112→115** | 8,6,5,7 | lunge +24px | crouch |
| BACK | opp-dir+X | **144→148** | 4,4,7,7,5 | lunge −32px | TURNS around (→opp idle) |
| UP | UP+X | **separate sheet** | ~36t | ? | up-pose |

NEUTRAL dst W×H (sword2.osr ticks 3485-3520): 104=43×62, 105=36×74, 106/107=**64×52**
(the swing reaches forward, dst x→294 from the cel's own anchor; off_x stays 0), 108=
44×62, 109=31×68.  **No animation-level combo** — a spammed/held X plays 104-109
back-to-back (the "3-combo" the USER mentioned is gameplay damage scaling, not distinct
cels; every spammed swing is 104-109).  LEFT = +192 (296-301; DOWN left = 304-307).
**UP+X draws on a separate multi-sprite sheet** (not 0x570/0x571 — ~18 sprites inserted
mid-thrust raising the HUD seq, ticks 3880-3915) — chip 2c.

## The port — chip 2a (NEUTRAL only)
`character.attacking / attack_timer / attack_kind / attack_last_ms` +
`character_resolve_attack` (`character.{c,h}`): X held (axis 5) + sword_out + grounded
+ the 200 ms refractory START a swing; a swing in progress advances one tick/call and
clears `attacking` at CHAR_ATTACK_NEUTRAL_TICKS (36) — the +0x68 mid-swing lock, so a
held X re-swings each completion.  `character_step` brakes vel→0 while `attacking`
(the NEUTRAL swing is stationary — the same lock the pose uses).  `arche_sword_clip`
gains `attacking`/`attack_kind`: while attacking it returns `ARCHE_ATTACK_NEUTRAL_CLIP`
(104-109 dur-6 oneshot), winning over pose/walk/idle but NOT the draw/sheathe transient
(finish drawing first).  `freeroam_step` calls `character_resolve_attack` each sim-tick
and passes the state to the clip.  The bank swap already selects res 0x571 (the swing
requires sword_out), and the +192 flip mirrors the LEFT cels (296-301).

**PORT-DEBT(sword-attack-gameplay):** the +0x66 charge meter / combo damage scaling +
the HITBOX are gameplay, deferred — chip 2a is the ANIM + the movement lock.
**chip 2b** = the directional swings (fwd/down/back) + their lunge displacement (RE the
cmd[4] body state in 0x442a70 that drives world_x) + the held-dir variant select.
**chip 2c** = the UP attack (the separate sheet) + the attack TRAIL vfx.

## Verification (port .osr vs sword2.osr, `sword_cels.py`)
Drove the port into the errands freeroam, drew the sword (nav ring 9 @tick 2030), then
tapped X (held-trace scancode 45 = axis 5).  `port-attack-r.osr` (no walk, facing RIGHT)
— all 3 X taps fire clean neutral swings rendering res 0x571 **104→109**, every cel's dst
W×H **byte-identical to sword2.osr's neutral attack**:

| cel | retail W×H | port W×H | dur (port) |
|----:|-----------|----------|-----------|
| 104 | 43×62 | 43×62 ✓ | 5t* |
| 105 | 36×74 | 36×74 ✓ | 6t |
| 106 | 64×52 | 64×52 ✓ | 6t |
| 107 | 64×52 | 64×52 ✓ | 6t |
| 108 | 44×62 | 44×62 ✓ | 6t |
| 109 | 31×68 | 31×68 ✓ | 7t* |

(\*the entry/exit boundary frames carry the ±1t FSM pre-advance noise chip-1 also had;
the swing core is dur-6.)  Stationary throughout (dst x=162 = idle resumes there).  The
first capture (`port-attack.osr`, faced LEFT after a left-walk) rendered the +192 mirror
**296-301**, W×H byte-identical to 104-109 — confirming the left mirror too.  Host:
`test_character_resolve_attack` + `test_character_attack_locks_movement` + the
`arche_sword_clip` neutral-swing case.

## chip 2b — the DIRECTIONAL swings (FORWARD / DOWN / BACK) + the lunge
**The variant is picked at trigger from the held direction vs facing** (the RE'd
structure: the sword-out form 0x41f200 registers a *distinct* action+variant template
per swing — `FUN_004287d0(0x27d9,var,…)` / `(0x27da,var,…)` `:1181-1201` — each with its
own scripted displacement const).  `character_resolve_attack` now reads the held axis at
the trigger: **DOWN** beats a held **L/R** (FORWARD if toward facing, BACK if away) beats
no-direction **NEUTRAL**; UP+X is the separate-sheet up-thrust (0x283f, chip 2c — falls
through to NEUTRAL until then).  Per-kind duration via `character_attack_ticks()` (the
clip's total cels·durs, kept 1:1 with the movement lock).

**The movement mechanism (RE'd, `FUN_00447ed0` @0x447ed0 → the collision mover 0x54db10):**
the swing moves world_x **directly** by a per-substate displacement, sign-flipped on
`facing==3` (toward facing) — *not* a velocity.  So:
- **NEUTRAL / DOWN** — stationary (`D=0`; DOWN returns to crouch while DOWN held).
- **FORWARD** — a forward LUNGE: `character_step` keeps the vel-lock (brake→0) and adds a
  direct world_x step toward facing, even-distributing the captured net
  `CHAR_ATTACK_FORWARD_LUNGE = 5400` (+54px) across the swing with an exact-integer running
  sum.  (The magnitude is captured ground truth — the per-substate profile of the 0x27da
  forward template is un-traced through the unreliable 0x45e830 combo path =
  `PORT-DEBT(sword-attack-gameplay)`.)
- **BACK** — net 0, but **turns her around**: at swing completion `character_resolve_attack`
  flips facing 1↔3 + negates vel — the literal `0x45e830:363-365` turn-around (the `+0x54==4`
  branch flips `+0x2c` at sub-state 4, sets `+0x28 = -+0x28`).  The swing renders the
  pre-flip facing's cels (144-148), so the post-swing idle lands on the opposite +192 bank.

**Cels (sword2.osr res 0x571, `attack_probe.py`):** FORWARD 120→126 (dur-6, 42t) / DOWN
112→115 (durs [8,6,5,7], 26t) / BACK 144→148 (durs [4,4,7,7,5], 27t); LEFT = +192
(`ARCHE_SWORD_OUT_FLIP`, confirmed off the facing-left DOWN at fr 304-307).  Clips
`ARCHE_ATTACK_FORWARD/DOWN/BACK_CLIP` in `actor_spawn.c` (per-cel durs faked via repeated
`frame_delta`, frame_dur 1, ≤32 frames).

### Verification (port-attack-dir.osr vs sword2.osr, `sword_cels.py`)
Drove the port: nav draws the sword @tick 2030, the held-trace does down+X / right+X /
left+X (scancodes 208/205/203 + 45).  All three fire clean swings, every cel's dst **W×H
byte-identical** to sword2.osr, durations match within the ±1t entry/exit FSM noise:

| kind | cels | key W×H (port == rec) | movement (verified) |
|------|------|------------------------|---------------------|
| FORWARD | 120→126 | 29×61/46×62/36×59/**80×49**/**80×49**/43×58/30×67 | idle dst 162→**216 = +54px** (lunge), camera follows |
| DOWN | 112→115 | 54×59/**80×47**/**80×47**/52×59 | stationary at dst 162, returns to facing-right idle |
| BACK | 144→148 | 37×59/44×57/**78×54**/**78×54**/37×58 | stationary, then idle flips to **192-194 (LEFT)** — turned around |

Host: `test_character_attack_directional` (variant select for all 6 face/dir combos +
per-kind durations + the BACK facing-flip + the exact ±LUNGE over the swing + DOWN
stationary).  1058 host pass.

## chip 2c — the UP (overhead) attack: BODY done bit-exact; TRAIL = scoped debt
**UP+X = an overhead THRUST on a THIRD body bank.** Ground truth = sword2.osr (the
first up-attack is RIGHT-facing, ticks 3880-3915; `up_attack_probe.py 9440 9760`,
`sword572_cels.py`).  Two distinct draw layers:

### The BODY — res 0x572 (bank 0x8d) — PORTED + BIT-EXACT
Arche RE-installs from bank 0x8c (res 0x571) to **bank 0x8d (res 0x572)** for the
swing — res 0x571 VANISHES, exactly as the Z draw swaps 0x8b->0x8c.  The form install
registers slot 2 -> bank 0x8d at `41f200:901` (`0x426db0(2,0x8d,...)`); the up action
is **0x283f**, registered in the same 0xc35b sword-out form (`41f200:1193` via
`0x427eb0`, `:1202` via `0x427430`).  Cels **0->5, durs [4,4,4,8,8,8] = 36t** (encoded
dur-4 x `{0,1,2,3,3,4,4,5,5}` to fit `ANIM_CLIP_MAX_FRAMES` 32), then back to the
res 0x571 sword-out idle.  STATIONARY (the vel-lock, like NEUTRAL).
- **Per-frame draw OFFSET** (the renderer's case-0x1872d off_x/off_y, NOT the cel's
  sheet origin — ox/oy/ow/oh are byte-identical port<->retail): off_y = **-16** uniform
  (a RAISE for the overhead pose; the up-attack renders at screen y=320 vs the 336
  idle/walk/pose baseline), off_x = **+16** (cels 0-2,5) / **+26** (cels 3-4 lean
  forward).  RE'd off the dst deltas vs the tick-adjacent stationary up-pose at
  world_screen (270,336), off_x 0.  Port: `ARCHE_ATTACK_UP_CLIP` (actor_spawn.c) +
  `character_resolve_attack` picks `CHAR_ATTACK_UP` (UP beats DOWN beats L/R beats
  NEUTRAL) + `freeroam_step` swaps bank 0x8d while `attack_kind==UP`.
- **VERIFIED** off `port-attack-up.osr` vs sword2.osr (`sword572_cels.py`): cels 0-5
  dst W×H byte-identical (38x54/39x52/61x61/36x96/36x96/60x63), durs [3,4,4,8,8,9] (±1t
  entry/exit FSM boundary, total 36t), y=320 IDENTICAL both sides, the +10 x on cels
  3-4 reproduced; res 0x571 vanishes during the thrust.  Commit (chip 2c-1).
- RESIDUAL: bank-0x8d LEFT mirror = +192 hypothesis (set in flip table), unverified —
  needs a left-facing up-attack capture (the first sword2.osr up-attack is right-facing).

### The TRAIL — res 0x40b sparkles — PORTED + verified BIT-EXACT (ckpt 163, `sword_trail.{c,h}`)
A trail of shrinking sparkles follows the sword tip.  Ground truth (`up_attack_probe`):
res **0x40b** (a 32x32 sprite, bank 0x1ad, registered `57ca40:2748`
`0x5748c0(...,0x40b,0x20,0x20,...)`), **frames 24->31** each smaller (24=22x22, 25=20x20,
26=18x18, 27=14x14, 28=12x12, 29=10x10, 30=8x8, 31=6x6), **additive** blend (mode 0xb).
**BEHAVIOR FULLY CHARACTERIZED + EXTRACTED (ckpt 162, the user-chosen "resolve the trail"
pass)** — `tools/trace_studio2/trail_extract.py` dumps every sparkle (frame + position
RELATIVE to the stationary world-screen anchor) per tick.  The emitter:
- **2 sparkles/tick** emitted at the sword-tip for **swing-ticks 9-17** (9 ticks, 18 total),
  then NO more; they age out.  (Up-attack body starts tick 3880; first sparkle 3889.)
- Each sparkle **ages frame 24->31 at +1 / 2 ticks**, holds 31, **expires at lifetime ~16t**;
  position ~static (a tiny ~+1px/tick drift) — a STATIC after-image arc.
- The **sword-tip arc** the pairs spawn on (world-rel to the stationary anchor (270,336),
  RE'd off `trail_extract`): a clean overhead sweep —
  T0 (+36,+53)(+42,+52) / T1 (+48,+50)(+53,+48) / T2 (+58,+45)(+63,+42) / T3 (+68,+37)(+71,+33)
  / T4 (+74,+28)(+77,+22) / T5 (+78,+16)(+78,+10) / T6 (+78,+4)(+76,-2) / T7 (+73,-7)(+70,-12)
  / T8 (+65,-16)(+60,-18).  Additive blend via **ramp_b** (`DAT_008a9308`, the SAME the sky
  particle uses — `particle.c` `sky_fade_idx`; `55d140` sets the trail's `+0x24=ramp_b[...]`).
- **Mechanism (the +0x13e0 DEVICE particle band):** spawned by the 0x283f handler `45e830`
  case 2 via `FUN_00557370(this, 0x186f2, ...)` (pool @ `DAT_008a9b50+0x13e0`), geometry
  `FUN_004505c0`, bank/frame stamp `0x426d70`, lifetime `+0x260` / fade-ramp `+0x25c=0xc`,
  step `FUN_0046e510`, render `FUN_00493480`; res 0x40b config in `55d140` (a 0x186bX case,
  bank 0x1ad).  (The exact decompile path 0xc35b->res-0x40b is partly traced — the captured
  tip-arc is the faithful stand-in for the un-ported retail emitter, like `butterfly-flap-ctrl`.)
- **PORTED (ckpt 163, `sword_trail.{c,h}`):** a captured-emitter pool mirroring `particle.c`.
  `sword_trail_emit` spawns 2 sparkles/sim-tick on `SWORD_TRAIL_TIP_ARC` (the captured fr24 dst
  offsets, anchored to Arche's stationary up-pose) for swing-ticks 9-17, co-located with Arche's
  world pos (she is stationary through the thrust); `sword_trail_step` ages the 24->31 dur-2
  8-frame one-shot (the cel's own metric_0c origin centres each shrinking frame, so the +1px/tick
  drift emerges); `sword_trail_render` emits MODE-1 additive nodes.  Wired into `main.c` around
  `freeroam_step` — STEP-before-emit (the +0x13e0 PARTICLE band steps before the CHARACTER-band
  emitter, quirk #95, so a fresh sparkle renders unstepped at fr24) — and rendered after the body.
  - **res 0x40b was ALREADY registered** (sprite slot **407 -> bank 0x1a4**, `game_sprites` 32x32
    type 2, added in `edbaf19`): the "not registered" BLOCKER was a 3-digit-hex grep miss
    (`0x40b` vs the table's `0x040b`) — NO registration work was needed.
  - **The blend is a CONSTANT additive `ramp_A[19]`** (LUT md5 727d856f), NOT the per-age ramp the
    sky particle uses (sword2.osr shows every sparkle at one descriptor regardless of age; the
    visual fade is the shrinking frame alone).  Retail's decompile sets `ramp_b[18]` (`DAT_008a9308`);
    that IS `ramp_a[19]` because retail's ramp tables are one contiguous 0x50-byte block (quirk #117).
    The port's `g_ramp_a`/`g_ramp_b` are separate arrays, so the trail emits the ramp_A index 19.
    Pinned by a boot-time dump of the port's ramp LUTs vs the recording's 727d856f.
- **VERIFIED off `port-trail.osr` vs `sword2.osr`** (`/tmp/final_verify.py`): the fr24 tip-arc
  RELATIVE to the up-attack body is **byte-identical** (port tick 2151 `(+20,+69)(+26,+68)` == rec
  tick 3889; the +10 cel-lean at tick 2154 == rec tick 3892, etc.), frames 24->31 present, blend
  LUT **727d856f matches retail**.  Feed: the port|recording montage (the pink additive sparkle arc
  sweeps the overhead sword on both sides).  Host: `test_sword_trail.c` (5 cases: emit window,
  spawn config, left mirror, aging/expire, render).  Capture recipe: `runs/sword-trail/trail-nav.jsonl`
  (the deterministic errands nav + dense errands-dialogue confirms + a tick-2080 Z draw) +
  `trail-held.jsonl` (UP+X held wide).
- **STILL captured (`PORT-DEBT(sword-attack-trail)` retained as a geometry stand-in):** the tip-arc
  table + the 2/tick cadence replace the un-ported emitter geometry (`FUN_004505c0` + the `45e830`
  case-0x283f emit gate) — exactly the butterfly-flutter-trigger pattern (ships bit-exact, only the
  autonomous geometry is captured).  Retire by porting `0x4505c0` so the arc EMERGES from the swing.

### The slide-attack body 48/49 = `PORT-DEBT(char-slope-slide)` (already scoped, ckpt 154)
The prone slide cels (res 0x571 **48<->49**, 68x30) are the state-6 momentum slide, which
needs terrain `[0x5653]` in [1,3] (a SLOPE) — UNREACHED on the flat errands floor (a
flat-ground dash+DOWN is a state-2 crouch gliding on cels 31/32, already bit-exact, ckpt
154).  So 48/49 is the existing `PORT-DEBT(char-slope-slide)`, not new chip-2c work.
