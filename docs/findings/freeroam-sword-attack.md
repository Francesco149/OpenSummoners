# Freeroam sword ATTACK (chip 2) — RE + port

> Sibling of `freeroam-sword-system.md` (the plan + the chip-1 draw/sheathe) and
> `freeroam-pose-commands.md` (the U/D-pose + slide).  Ground truth: the USER's clean
> real-play recording `sword2.osr` + its input trace `sword2-input.jsonl` (the
> OSS_INPUT_RECORD held-trace).  Probe: `tools/trace_studio2/sword_cels.py` +
> `/tmp/attack_probe.py` (flip→tick + input interleave).

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
