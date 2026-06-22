# OpenSummoners — status at a glance

> **DERIVED FILE** — regenerate with `python3 tools/gen_port_ledger.py`.
> Headline numbers only; narrative lives in `PROGRESS.md`, "right now" in
> `HANDOFF.md`, durable RE in `findings/`, full map in `port-ledger.md`.

## Port coverage (engine-proper functions of `sotes.exe`)

```
███░░░░░░░░░░░░░░░░░  13.3% touched   (13.3% host-tested, 14.3% of code bytes)
```

| status      | count | what it means                                          |
|-------------|------:|--------------------------------------------------------|
| tested      |   206 | ported + module covered by the host unit suite       |
| ported      |     5 | reimplemented in src/, no host test for that module  |
| **touched** | **211** | tested + ported (FUN_ provenance ref in src/)    |
| unported    |  1547 | exists in engine, never referenced from src/         |

**Denominator note (read this before judging the %):** the headline % is over
**engine-proper** functions — the **1490** below
`0x5bdab0`. The other **268** non-thunk functions are the
statically-linked MSVC CRT tail (operator_new, _malloc, _strcmp, __ftol, the
`entry` startup, RtlUnwind, …); OpenSummoners is a **drop-in** that *links*
those like retail rather than porting them (PLAN.md §2-3), so counting them
would bury real progress. Full table is **1758** non-thunk
functions (of 1768 incl. thunks).

Code-byte coverage (14.3% of engine-proper bytes) is the truer progress
signal: the engine has a long tail of tiny leaf helpers, so function count
understates how much actual instruction volume is ported.

## Current front

- **Phase:** Phase 4 — the town intro renders ~1:1; the cutscene chains arrival→house→errands and all three
  ROOM BACKDROPS render (ckpt 130).  **USER directive (ckpt 131): close EVERY rendering gap up to the
  errands, frame-by-frame 1:1 via trace-studio v2, BEFORE the freeroam.**  Gaps closed: ckpt 131 errands
  floor tileset / house+errands props / the dialogue-portrait +13 offset; ckpt 132 the dialogue BOX POSITION
  (faithful `0x49c640`); ckpt 133 the dialogue TYPEWRITER-SKIP (the desync blocker); **ckpt 134 the MATCHED-
  CADENCE nav + the dialogue cadence TICK-1:1 — THEME 1 of the punch-list DONE** (the arrival dialogue L0-L7
  now tracks retail tick-for-tick, 314/323 ticks bit-equal); **ckpt 135 the dialogue PORTRAIT FADE-IN
  (USER ckpt-134 follow-up) — drawcall+LUT-exact, tick 661 now pixel-identical** (the cross-fade arms one
  tick after scale==1000 so idx 0 holds 2 ticks; quirk #108); **ckpt 136 the dialogue PORTRAIT FADE-OUT
  dissolve — drawcall+LUT-exact, ALL 3 arrival speaker changes tick-1:1** (the closing bust dissolves via
  the reverse ramp idx 18→2 then GONE; the advance presses 2t early + the new box's re-pop delays 2t so the
  box-frame stays 28/28, quirk #108); **ckpt 137 the arrival→house TRANSITION CHOREOGRAPHY (THEME 3) — the
  beat model: the L7→L8 camera-pan + run-off gap + the room-transition fades now track retail TICK-1:1
  through the house dialogue** (L8 1150 / L9 1190 / house L1-L3 all == retail; quirk #109).
  **ckpt 138 the TRANSITION FADE punch-list (1)-(3) DONE + TICK-1:1 — center-out var-0 stand-in + the box
  renders OVER the cover + house L0 +8t→0** (the +8t was the cover-arm TIMING, not a hard wipe — retail ages
  the fade cells too; the exit wait recalibrated 20→10t off the draw stream; quirk #109 extended).
  **ckpt 139 the BUTTERFLY DIRECTION SPRITE (THEME 2 notes #0+#2) — DONE + DRAWCALL-1:1** (the butterfly
  `frame_base` = the per-instance map VARIANT +0x18 = base direction 0/4/8/12, + the facing-mirror
  `DAT_008a8440[0x146]=16` corrected from 4; `cel = frame_base + 16·(facing==3) + flap`, live-read-verified
  2452/2452, draw_probe 273/280 ticks frame-identical; quirk #110).
  **ckpt 140 the cast "ARCHE RUNNING" run-off render (THEME 2 note #5/#3) — DONE + frame-sequence bit-exact**
  (on the L7→L8 beat Arche plays her RUN→DECEL→arrival-IDLE clips and runs to the house via the REAL ported
  run physics; cels 16-21/8-11/152-154 RE'd off retail.osr; quirk —; `findings/arche-runoff-render.md`).
  **ckpt 141 the run-off CAMERA-PAN OVERLAP restructure — DONE + camera/run BIT-EXACT** (the run-off now fires
  CONCURRENT with "Cool!" — on its beat completion ~tick 972, not its confirm; the static caravan + Arche's
  screen-x/run cels match retail TICK-FOR-TICK, framing offset 0px (was ~40); quirk #111).
  **ckpt 142 the NPC COLOUR variant (THEME 2 #1) — DONE + bit-exact** (the townswoman `0xc440` colour
  variant = the map field `+0x2c`=1; her variant bank is a CLONE of the base 8bpp sheet + a palette-INDEX
  remap `DAT_006748d0` — ported the decode consumer + fixed the info/clone pass-order bug that wiped the
  remap pointer; she renders blonde/pink, crop tick 274 differ_px 1387→0; quirk —; `findings/npc-colour-variant.md`).
  **ckpt 143 the BUTTERFLY VERTICAL FLUTTER (THEME 2 #2 residual) — PHYSICS RE'd + PORTED + bit-exact** (the bob
  is the shared case-3 "jump" FSM = Arche's jump physics, with the butterfly's install constants impulse −32000 /
  cap 16000 / gravs 2000·1000·4000 / windup 4; the flap TRIGGER is the terrain-aware mover `0x43f880` — flap to
  hold ~8000 above the floor it scans — captured as `PORT-DEBT(butterfly-flutter-trigger)` pending the freeroam
  mover; port dst-Y matches retail tick-for-tick, 0 X regression; quirk #112; `findings/butterfly-flutter.md`).
  **ckpt 144 the HOUSE CAST + the FREEROAM HAND-OFF + the ERRANDS tile-frames — the house/errands arc DONE**
  (USER: "the errands scene and the scene right before it — map 1:1, arche's movement, arche+mom+dad missing
  in the house"): (1) the HOUSE renders Arche+Mom+Dad bit-exact in position (a per-room cast pool, captured
  positions == the cutscene speaker coords); (2) the FREEROAM HAND-OFF — controllable Arche walks in the
  errands on live input via the bit-exact `character_step` (walk/idle clips + facing mirror); (3) the ERRANDS
  backdrop is now bit-exact — the auto-footprint wall/floor tile FRAME = the cell's `arg_0c` (not the constant
  scene_frame); every tile bank's per-frame count == retail.
  (4) the ERRANDS CAST — Father + Mother + the 10 shop props/NPCs (res 1027) all bit-exact (`ERRANDS_CAST`).
  (5) freeroam JUMP wired (axis_held[4], bit-exact arc).
  **ckpt 145 the house→errands TRANSITION FADE — USER studio notes #6/#7 (the missing transition): the house
  now EXITs with a fade-TO-black COVER (edges-in var 1, RE'd off 0x4d7d80:1203 + the retail.osr full-frame iris)
  and the errands ENTERs with a fade-FROM-black REVEAL (center-out var 0, main.c on chain-complete).  Both sides
  reach full black aligned (tick ~1699, differ 59k→1379); render matches retail.  Committed `22047fb`, 1028 host
  pass.  RESIDUAL: a ~13t cover-START phase offset = the HOUSE dialogue cadence is not yet tick-1:1 (the arrival
  is, ckpt 134) — a phase-pillar follow-up.**
  **ckpt 153: the res=0 UI SPRITE BANK is RESOLVED + the dialogue ADVANCE INDICATOR + the inline `@@` KEY-CAP
  ICONS are DONE + pixel-verified — `PORT-DEBT(dialogue-arrow-art)` RETIRED (`findings/res0-ui-banks.md`).**
  Frida-pinned the UI banks (thischain on `0x410560`/`0x411940` + `--res-probe`): god+`0xb8c` = **PE res
  `0x455` (sotesd.dll)** = port slot 43 (the BOOK advance icon, frames 20-23); the inline key-caps = **PE res
  `0x6fa` (sotesd.dll)** = port slot 55 (←=f3/→=f1/X=f9, 279/279-px matched).  **Every UI/HUD/dialogue bank
  is a sotesd.dll DATA resource (res=0 was a capture-side ID gap, NOT a special module — NO legal blocker;
  the ckpt-149 "res 1000 sotesp" guess was WRONG, quirk #92 retracted).**  Ported: the BOOK advance indicator
  (`render_dialogue_box`, box+(368,92), gated `dialogue_awaiting_advance`) == retail book @(400,284); the `@@`
  parser (`dialogue_keycap_token` + `dialogue_expand_text` count icons as 3 cells in the wrap;
  `dialogue_body_row_text` blits slot-55 frames) == retail icons @(336,210)/(378,210)/(224,266) — the whole
  errands tutorial line is pixel-identical (`icons_compare.png`).  1045 host pass.
  **NEXT (do FRESH after a /clear): port + verify ALL freeroam MOVEMENT TYPES** (USER directive ckpt 152:
  double-tap dash ✓, up-to-stop-faster, slide/crouch, the full combo set — engine-quirks #~3311 + the input
  findings).  **DONE (ckpt 153): the U/D-POSE move (CROUCH / SLIDE / UP-stops-you-faster) — COMMAND +
  PHYSICS, bit-exact.**  (a) COMMAND: `character_resolve_pose` (the `cmd[3]` of `0x478ba0:248-259`: DOWN→10
  crouch/slide, UP→0xb defensive, off a held axis + ring [10,800]ms find + self-sustain) +
  `input_ring_find_recent` + the **ring-id fix** (input.h had UP/DOWN backwards; UP=1/DOWN=3).  (b) PHYSICS:
  `character_step` consumes `cmd_pose` — a posed+grounded body skips the accel ramp and brakes vel toward 0
  at the WALK brake (-800/tick) EVEN WHILE the dir is held (apply states 2/5, `0x442a70:959`).  GROUND-TRUTHED
  off the live retail entity — **the Frida host SELF-STARTS** (`frida_capture.py ensure_frida_server` auto-spawns
  it via UAC-elevated Start-Process; set `OPENSUMMONERS_FRIDA_SERVER_EXE`; the earlier "host down/blocked" read
  was WRONG): consts (gates [0x5675]/[0x5684]=1, brake [0x565e]=-800) + per-tick body (`runs/pose-demo/cap-body`:
  DOWN→state 2, UP→state 5, both -800/tick to 0; the state-6/[0x5656/57] slide was a stack-reuse decode trap →
  the player's down is state 2).  VERIFIED bit-exact: host `character_pose_brakes` + a port `.osr`
  (`port-walkup.osr` fr_vel 23200→0 at -800 while right held == retail hvel tick-for-tick, sim-tick axis).
  1052 host pass.  **ALSO (USER ckpt-153 feedback): control is now LOCKED during the errands opening
  dialogue** — the ckpt-152 "concurrent control" hand-off was WRONG (it RE-inferred "control is live, the
  player just didn't move in the recording"; the owner says she's NOT controllable until the lines end).
  `freeroam_step` drives a zeroed axis while `g_errands_dlg_pending || cutscene_active`, so Arche holds
  idle at the spawn through the 3 lines, then hands off (verified off a port .osr: locked tick 1803-1897
  while csln 0→1→2, unlocks at completion).
  **ckpt 153b: the crouch/up SPRITE (char-pose-anim) — DONE + bit-exact (the USER "slides around in one
  pose" fix).**  Added HELD-AXIS injection to the trace-studio PROXY (`engine_input.h`: hook the leaf key
  query `0x5ba520`, write 0x80 into `device+0x18+sc` for held scancodes → the producer fills `mgr+0x114..`;
  the ring injection was edge-only) so retail can be captured crouch/up-HELD in the errands freeroam.  RE'd
  the cels off `retail-pose.osr` (res 0x570 = bank 0x8b, `draw_probe.py --res 0x570`): a 3-phase FSM, a
  TRANSITION cel on enter AND exit holding a steady cel between — CROUCH enter/exit cel 31 (4t)/hold 32,
  UP enter/exit 34 (4t)/hold 35.  Ported `arche_pose_clip` (`actor_spawn.{c,h}`, host-tested) keyed on
  `cmd_pose`; `freeroam_step` calls it once/sim-tick (the pose wins over walk/run).  VERIFIED off a port
  `.osr` vs `retail-pose.osr`: res 0x570 renders 31/32/34/35 at (162,336) TICK-FOR-TICK == retail (enter
  4t, exit 5t bit-exact).  **LEFT-facing DONE too** (USER ckpt-153b directive): captured a left-facing pose
  (`retail-poseL.osr`) — bank 0x8b is NOT engine-mirrored, it has DEDICATED left cels (right+152: left crouch
  183/184, up 186/187), so `arche_pose_clip` selects the left clip by the CHARACTER facing + `freeroam_step`
  renders facing=1 (no +4 flip).  port-poseL.osr renders 183/184/186/187 == retail.  `PORT-DEBT(char-pose-anim)`
  RETIRED (both facings); quirk #114 extended; 1053 host pass.  **SURFACED (new debt `char-freeroam-left-cels`):
  the WALK/IDLE left cels are also dedicated (left walk 159-165, idle 152-154) — the port still mirrors them via
  +4 (wrong); the pose half is fixed, the walk/idle half is open (needs a sustained-left-walk capture).**
  Remaining moveset: the dash-then-down SLIDE (= same crouch state, just entered with momentum),
  sword(Z)/attack(X), the door-enter (= char-up-door-probe, collision-coupled).
  `findings/freeroam-pose-commands.md`.  **THEN the freeroam HUD** (fully SCOPED, `findings/freeroam-hud.md`; its banks are also
  sotesd.dll DATA = loadable — the layout + party-data is the work).  The errands opening DIALOGUE is DONE
  (ckpt 152).  RESIDUAL on the icons: only ←/→/X are mapped — other `@@` codes (↑/↓/Z/C) are unknown-skipped
  until a line uses them (then verify the slot-55 frame + add to the token map).
  Plus two SCOPED gaps from this pass: (A) Arche's house TURN (USER notes #3-5) — **DONE ckpt 146, TICK-ALIGNED
  ckpt 151**: the emote `0x401e60(Arche,1)` = actor cmd-2 "turn to face dir 1", cels 158(4t)→7(4t)→idle 0/1/2
  after house L5; ckpt 146 ported it fire-and-forget (left it ~7t late); **ckpt 151 re-ported it as the BLOCKING
  beat retail uses** (`CS_BEAT_ACTOR_TURN` = L6's lead, `in_ECX[8]=4` actor-wait) — now 158@1579/7@1583/0@1587
  == retail (the ~7t lag GONE; the house dialogue was already tick-1:1, so the lag was the missing block, not a
  cadence-phase debt).  (B) 3 missing errands FURNITURE
  FURNITURE objects (USER notes #8/#9/#18-21: counter, bookshelf frame, clock) — these are CHARACTER-band
  codes 0x112d1/d2/cf/d9 (70000-range) → bank 0x16f/0x16b (res 1023/1026), frame_base=variant, that the port's
  suppressed-for-non-town character band never rendered.  FIXED ckpt 145 (`01dc162`): captured into ERRANDS_CAST,
  counter/bookshelf/clock now render (differ 13236→11233).  RESIDUAL: finer bookshelf shelf-props + the fireplace
  FIRE (animated) + a subtle wall tint.  (I first wrongly dismissed these as the reveal phase; the USER corrected
  with post-reveal notes — they were genuinely missing.)**
  **ckpt 146 the house Arche TURN (scoped gap A) — DONE + drawcall-faithful (`cfc6a96`, 1030 host pass):** the
  script emote `0x401e60(Arche,1)` at `0x4d7d80:1170` (after house L5) = actor cmd-2 "turn to face dir 1"
  (`0x43e5b0` case 2); off retail.osr res 0x570 (static at screen 354,336) Arche runs cels 158(4t)→7(4t)→the
  base-0 standing idle 0/1/2, turning from the arrival-listening idle (152-155) to face her father.  Ported as a
  fire-and-forget `CS_ACT_ACTOR_TURN` (cutscene_room.turn_after_line=5) on the room-cast Arche (HOUSE_CAST[0]);
  verified off port-turn.osr — cels/durations/position match retail EXACTLY (158@1586 / 7@1590 / 0@1594).
  RESIDUAL: ~7t absolute lag = the house-cadence phase debt (auto-aligns when that lands; the turn is keyed to
  the advance).  `findings/arche-house-turn.md`.
  **ckpt 146 ALSO the errands SHELF/BOOKSHELF props Z-ORDER fix (`ead9c49`, 1031 host pass):** the USER's
  "bookshelf/shelf missing props" were NOT a missing spawn — the port EMITS them (structure band res=1026/1027,
  exact retail frames/pos) but the ckpt-145 ERRANDS_CAST furniture stand-in (bookshelf frame + shelf units) at
  the CAST LAYER 13 drew OVER the layer-8 props, occluding them (draw pool walks layers low→high = back→front;
  retail draws frame/units seq 257/261 THEN props 268+).  Fix: `room_cast_member.layer` per-member; background
  furniture → layer 7.  Both shelves recon pixel-match retail.  LESSON: a "missing" element may be emitted-but-
  OCCLUDED — check the draw seq (z).  `findings/errands-render-gaps.md` §4 (+ the fire/HUD/wall-tint RE).
  **ckpt 147 the errands FIREPLACE FIRE (USER osr_notes #3) — DONE + PIXEL-EXACT (`e320105`+`ad405b1`, 1032
  host pass; autonomous, USER away):** the port showed a black recess, retail a roaring fire.  Ported as an
  additive-alpha `ERRANDS_CAST` member — res 1034 (bank 0x1a3 = the ar_pool POOL index for group3 slot 406),
  frames 0-5 LOOPING at a uniform dur-6 (the clip's single +0x44, read off the CLEAN non-coalesced retail
  ticks), additive `ramp_a[14]` (its blend descriptor extracted from retail.osr is BYTE-IDENTICAL to the
  port's g_pd_boot_group_a[14] — one full match of 20), dst (329,178) 48x39.  Then FOUND+FIXED the sheet's
  decode residual: the port's global 8bpp colour-grade was OVER-darkening the fire (retail grades only the
  0x417c40-getter tiles/sky, not plain-getter EFFECT sheets) — excluding slot 406 (`FIRE_BANK_SLOT`) makes the
  fire sheet dhash == retail bit-for-bit and the fire-rect recon **differ_px==0**.  +host test `errands_fire`.
  ALSO RE'd (deferred): the wall-tint (#4) = res 1897/1898 errands floor clones decode differently — but the
  over-grade was RULED OUT (excluding the clone slots was a no-op) → a deeper per-room floor decode, entry
  point logged; the door-indicator (#5) + HUD (#7-9) = the res=0 freeroam UI subsystem (best with USER); the
  idle-fidget (#6) = the deferred RNG behaviour subsystem 0x54f980.  `findings/errands-render-gaps.md` §1+§3.
  **ckpt 148 the errands WALL-TINT (USER osr_notes #4) — DONE + PIXEL-EXACT (`05e8742`, 1034 host pass;
  autonomous, USER away):** the errands floor rendered WARM BROWN vs retail's cooler greenish-gray.  RE'd the
  code (not curve-fit): the floor banks 0x187/0x188 (clones of the town floor res 0x769/0x76a PIXELS) carry an
  info-entry **+8 = &DAT_00675500** whose first u16 = **0x186** → **FUN_00417bc0** swaps their palette to slot
  0x186 = **res 0x76b** (a 32×32 palette-holder with the cooler errands-floor colours).  So the SAME floor
  pixels render warm in the town (own palette) and grey in the errands (res 0x76b's) — a per-room PALETTE SWAP,
  the cross-slot sub-case of the same +8 field as the NPC colour variant (which is the within-palette sub-case,
  first u16 == 0; the port only modelled THAT, identity for the floor → no effect → wrong colour).  Ported
  `ar_apply_slot_palette_swap` (the cross-slot half of FUN_00417bc0): at decode, before the grade, overwrite the
  named palette entries with the source slot's RAW +0x34 palette (matching retail 0x490f30's per-bank order
  embedded→swap→grade→install).  **VERIFIED:** floor sheets res 1897/1898 all `differ_px==0` (dhash byte-
  identical); the USER wall crop tick 1726 recon `differ_px==0`; town 58/58 + errands 73/73 shared sheets match
  (no regression).  +2 host tests.  `findings/errands-render-gaps.md` §3.
  Studio: `plans/trace-studio-v2.md`; freeroam arc: `plans/controllable-arche-faithful.md`; milestones: `ROADMAP.md`.
  - Movement-system progress: butterflies ✓ → tile collision read-side ✓ → controllable Arche
    WALK/JUMP/DASH/windup bit-exact ✓ → MVP live-wire REMOVED ✓ → FAITHFUL live keyboard input ✓ →
    town-arrival DIALOGUE ADVANCE ✓ → CONTROL-PATH harness-verified ✓ (quirk #103) → arrival→house dialogue
    CHAIN ✓ → dialogue PORTRAITS un-MVP'd per-speaker+aligned ✓ → trace-studio v2 ✓ (ckpt 125-129) →
    the house + errands ROOM BACKDROPS RENDER ✓ (ckpt 130) → dialogue BOX POSITION ✓ (132) → TYPEWRITER-SKIP
    ✓ (133) → dialogue CADENCE TICK-1:1 ✓ (134, THEME 1) → dialogue PORTRAIT FADE-IN ✓ (135) +
    FADE-OUT dissolve ✓ (136) → arrival→house TRANSITION CHOREOGRAPHY TICK-1:1 ✓ (137, THEME 3) →
    the TRANSITION FADE (center-out + box-over-cover + house L0 +8t→0) ✓ (138) →
    the BUTTERFLY direction sprite ✓ (139, THEME 2 #0/#2 — frame_base from the map variant) →
    the cast "Arche running" run-off render ✓ (140, THEME 2 #5/#3 — RUN/DECEL/idle clips + run physics) →
    the run-off CAMERA-PAN OVERLAP ✓ (141 — fires concurrent with "Cool!", camera/run bit-exact) →
    the NPC COLOUR variant ✓ (142, THEME 2 #1 — 8bpp palette-index remap on the cloned variant bank) →
    the BUTTERFLY VERTICAL FLUTTER ✓ (143, THEME 2 #2 — the case-3 jump FSM + captured terrain-mover trigger) →
    the HOUSE CAST (Arche+Mom+Dad, position bit-exact) ✓ (144) → the FREEROAM HAND-OFF (controllable Arche
    walks the errands on live input) ✓ (144) → the ERRANDS tile-frames (arg_0c, backdrop bit-exact) ✓ (144) →
    the house→errands TRANSITION FADE + the missing errands FURNITURE ✓ (145) → the house Arche TURN
    (emote 0x401e60, cels 158→7→idle) ✓ (146) → the errands SHELF-PROPS z-order (background furniture
    occluded them) ✓ (146) → the errands FIREPLACE FIRE (res=1034 additive, PIXEL-EXACT
    differ_px==0 + the over-grade fix) ✓ (147) →
    the errands WALL-TINT (per-room PALETTE SWAP, FUN_00417bc0 cross-slot — floor reads res 0x76b's
    palette, pixel-exact) ✓ (148) →
    the dialogue BODY-ROW DISTRIBUTION (line-count-dependent vertical spacing, FUN_0048da70 — fewer
    rows ⇒ larger gap; the 3-line line now renders all 3 rows, was cut at 2) ✓ (149) →
    the freeroam DASH double-tap TRIGGER (run derives from the live ring — input_dash_double_tap +
    character_resolve_run, the 0x478ba0/0x479e70 detection; window 800ms read live; char-run-trigger
    RETIRED) ✓ (150) →
    the house Arche TURN as the BLOCKING beat (CS_BEAT_ACTOR_TURN = L6's lead, keyed to L5's confirm;
    0x401e60 sets in_ECX[8]=4 the actor-wait the thunk pumps; turn now 158@1579/7@1583/0@1587 == retail,
    the ckpt-146 ~7t lag GONE) ✓ (151) →
    the errands-scene OPENING DIALOGUE (questline 0x4dc510's 3 Arche lines via 0x49d6e0 — the movement
    tutorial — through the existing box system, concurrent with freeroam; tick-aligned L1@1770/L2@1800/
    L3@1830 == retail) ✓ (152) →
    the freeroam U/D-POSE move — COMMAND (cmd[3]: DOWN→10 crouch/slide, UP→0xb defensive;
    character_resolve_pose = 0x478ba0:248-259 + the input.h ring-id fix UP=1/DOWN=3) + PHYSICS
    (character_step brakes vel→0 at -800/tick while posed+grounded = apply states 2/5; "UP stops you
    faster", crouch, slide; ground-truthed off the self-starting Frida host, bit-exact vs cap-body +
    a port .osr) ✓ (153, quirk #114) →
    the freeroam U/D-POSE SPRITE (char-pose-anim) — held-axis injection added to the trace-studio proxy,
    cels RE'd off retail-pose.osr (CROUCH 31→32→31, UP 34→35→34), arche_pose_clip keyed on cmd_pose;
    port .osr res 0x570 == retail tick-for-tick ✓ (153b, quirk #114) →
    **next: the rest of the moveset (sword Z / attack X / door-enter = char-up-door-probe) + the freeroam
    HUD (scoped)**.
- **LATEST (ckpt 152): the errands-scene OPENING DIALOGUE is PORTED + TICK-ALIGNED (USER directive).**
  USER: "we're still missing the opening dialogue for the errands scene (starts once it switches to the
  errands scene where you have control). port that first."  The questline `0x4dc510`'s entry case
  (decompile :1086-1116) plays **3 Arche lines** via `0x49d6e0` — RE-confirmed to be the SAME dialogue
  display the town chain already drives, so this is 3 more lines through the existing
  box/typewriter/portrait/advance system, played **CONCURRENT with freeroam control**: (1) "So the first
  floor of our house is the store. Cool!", (2) "Okay, I need to help with the moving-in!", (3) "To move
  around, I press [<>] & [^v], and to talk to people and do stuff, I press [Z/X], right?" (the movement
  tutorial).  Faces 0x02/0x02/0x09 (`0x49d6e0` param_8); box anchors to Arche's freeroam spawn (clamped
  left = the wide bottom box retail shows at (32,192)-(440,304)).  **The port:** `cutscene.c` TOWN_ERRANDS
  1-room chain + `cutscene_errands_intro()`; `main.c` `g_errands_dlg_pending` arms it once the entry reveal
  recedes (retail plays it AFTER the fade-from-black), re-arming `g_cutscene` so it renders + advances via
  the existing path while `freeroam_step` keeps control live.  **VERIFIED off `port-errdlg.osr` vs
  `retail.osr` (`dlg_reconstruct.py`, the new tick-aligned `nav-errands-dlg`):** all 3 lines render at
  retail's ticks (L1@1770 / L2@1800 / L3@1830), name "Arche" @(332,184) color 0x455f7b + body @(168,222)
  color 0xa8b9cc + the 3-row layout on L3 == retail EXACTLY; the arm log fires "reveal complete".  1045
  host pass.  **USER VERIFIED (ckpt 152): "looks mostly correct" — flagged the missing inline arrow icons**
  (studio note tick 1823).  RESIDUAL (now RE'd + the NEXT task): L3's inline button icons show as raw codes
  (`@@\x81\xa9`/`@@\x81\xa8`/`@@X`) where retail draws **17x17 res=0 KEY-CAP sprites = ←/→/X keys** (the
  bound move/action keys; recon-crop confirmed).  `@@<code>` escapes (`0x40 0x40`+code); positions = the
  monospace body layout (x = box_x+TEXT_DX+char_idx·7).  **BLOCKED on the res=0 UI key-cap bank source —
  faithful+LEGAL = load it from the user's files (never embed captured pixels); bank unlocated = the SAME
  subsystem as the HUD + advance-arrow.**  USER chose: /clear + resolve it fresh (= the NEXT task, unblocks
  icons + HUD + arrow).  Full RE: `port-debt.md` (dialogue-arrow-art).  Retires the DIALOGUE half of
  `PORT-DEBT(cutscene-scene-chain)`.
  **USER-VERIFY (visual): click the studio shortcut** (`studio-current.txt` → `port-errdlg.osr` |
  `retail.osr`), scrub the errands ticks ~1758-1840 — Arche's opening dialogue plays line-for-line with
  retail (the only diff is the L3 button icons render as raw text codes).
- **Prior (ckpt 151): the house Arche TURN is now the BLOCKING beat retail uses — TICK-ALIGNED (the
  ckpt-146 ~7t lag is RESOLVED).**  Diagnosis first (off `port-dash.osr`, a fully matched nav): the house
  DIALOGUE was ALREADY tick-1:1 (`dialogue_timeline` ±1t) yet the turn was STILL 7t late — DISPROVING the
  finding's "auto-aligns once the cadence is tick-1:1" claim.  The real cause: ckpt 146 emitted the turn
  FIRE-AND-FORGET on the L5→L6 advance (the nav delayed L5 ~7t to land L6, dragging the turn late).  RE'd
  the decompile (`0x4d7d80:1163-1184`): `0x401e60(Arche,1)` sets cmd-2 + `in_ECX[8]=4` (the actor-WAIT
  beat) so the thunk `0x439680` PUMPS it to completion BEFORE L6 — a BLOCKING beat.  **The port:**
  `CS_BEAT_ACTOR_TURN` + house L6's lead beat (`HOUSE_L6_LEAD`, dur 8); `box_hold=8` keeps L5's box up
  (full text) through the turn then shrink-closes as L6 reopens (quirk #107; slide+dissolve gated to the
  run-off CAMERA_PAN lead).  Nav presses L5's confirm at retail's tick 1579 (`runoff_leads 15:15`).
  Removed the now-dead `turn_after_line` field.  **VERIFIED off `port-houseturn.osr` vs `retail.osr`:**
  turn 158@1579-1582/7@1583-1586/0@1587 == retail; house L0-L7 tick-1:1 (L5 +4t box-overlap); cover-start
  1669 == retail; arrival run-off unchanged (no regression).  1045 host pass.  `findings/arche-house-turn.md`.
  **USER-VERIFY (visual): click the studio shortcut** (`studio-current.txt` → `port-houseturn.osr` |
  `retail.osr`), scrub the house ticks ~1576-1610 — Arche turns to face her father AT retail's tick, with
  his text bubble still up through the turn.
- **Prior (ckpt 150): the freeroam DASH double-tap TRIGGER is PORTED + host-verified end-to-end —
  `PORT-DEBT(char-run-trigger)` RETIRED.**  A USER tap-tap-hold of a direction now makes freeroam Arche
  DASH (the last un-wired freeroam move; walk + jump already worked live, ckpt 144).  The dash PHYSICS was
  already bit-exact (ckpt 118, cap 48000); what was missing was the run FLAG.  RE'd off the decompile (not
  curve-fit): the char-AI `0x478ba0` builds the dash command `entity+0x14854` (5/6 = dash L/R) from the
  discrete press RING — snapshot prev, reset, scan for a direction DOUBLE-TAP (`FUN_00479e70`/`FUN_00479960`:
  two distinct pressed ring records of the same id within the window, a "used" mask so a single held press
  is NOT a double-tap), self-sustain while held (`local_608[0]==5/6`), end on release.  The window
  `*(*0x8a6e80+0xf8)` is a config field with no static default → **read LIVE from retail = 800 ms**
  (`runs/dash-window2`, the `*0x8a6e80` chain at the title), `run_mode` `*(*0x8a6e80+0x510)==0` = the active
  double-tap branch.  **The port:** `input_dash_double_tap` (`input.{c,h}`) + `character_resolve_run`
  (`character.{c,h}`, new `cmd_lr` field) + `freeroam_step` (`main.c`) feeding `character_step` on the live
  ring (`GetTickCount` clock).  1045 host pass (+10): the detector + `character_dash_via_double_tap` proves
  the input-ring → run → **RUN cap 48000** chain through the REAL physics (single press → WALK cap 24000).
  Off the seed-pinned parity path (the double-tap is wall-clock, like retail) — unit tests pin the logic.
  Commit `43a55f1`; quirk #113; `findings/dash-double-tap-trigger.md`.  **ALSO binary-verified off a port
  `.osr`** (`port-dash.osr`, drove the replay into freeroam + a double-tap): at tick 1866 Arche's cel flips
  WALK 0-3 → RUN 16-19 + the dst-x step ramps 2.4 → ~5 px/tick (= the run cap, 2× walk) — the freeroam_step
  wiring works in the real exe, not just the unit.  **USER-VERIFY (visual): click the studio shortcut**
  (`studio-current.txt` → `port-dash.osr` | `retail.osr`), scrub freeroam ticks ~1840-1887 (Arche walks then
  dashes; retail idle = a port-only demo).
- **Prior (ckpt 149): the dialogue BODY-TEXT ROW SPACING is line-count DISTRIBUTED (RE'd, bit-exact) —
  the USER's tick-770 bug (a 3-line line "We haven't been here since…" showed only 2 lines + too-tall
  spacing) is FIXED.**  ckpt d16ae1a had set a CONSTANT pitch (LINE_H=36 / TEXT_DY=29) fitted to the
  2-LINE case, which over-spaced every other line and pushed the 3rd row out of the box.  USER: "RE the
  exact logic, don't hardcode from empirical data; it's nuanced logic with cases like the box sizing."
  **RE'd `FUN_0048da70` (@0x48da70, the grid text renderer):** the body rows are VERTICALLY DISTRIBUTED
  in a fixed 3-row grid — `gap = min(max_gap, ((max_rows-rows)*pitch)/(rows+1))`, row Y = `box_y + base_y
  + (r+1)*gap + pitch*r`.  Constants RE'd off the decompile (NOT measured): `base_y=20`/`max_rows=3`
  (`FUN_0040df40` params), `max_gap=20` (`FUN_00410610:19` sets records+0x1c=0x14), `pitch=28` (RE-
  confirmed by formula consistency — it's BOTH the 3-row pitch AND the 2-row gap candidate (1·28)/3=9).
  So 1 row→gap 20 (@box+40); 2 rows→9 (@+29,+66, pitch 37); 3 rows→0 (@+20,+48,+76, pitch 28).  `rows`
  is the line's TOTAL count (over all grid records), so the layout is fixed when the text is set (proven:
  tick 661 shows 'A' alone already at the 3-row offset 20).  **The port (`dialogue.{c,h}`/`main.c`):**
  `dialogue_body_gap()` + `dialogue_body_row_dy()` replace the constant `TEXT_DY + r*LINE_H`.  1035 host
  pass (+1).  **VERIFIED off `port-dlgdist.osr` vs `retail.osr`** (`tools/trace_studio2/dlg_text_probe.py`,
  the body TextOutA row baselines): port == retail EXACTLY at every arrival line (3-row [20,48,76], 2-row
  [29,66], 1-row [40]); the 3-line line renders all 3 rows (feed montage: the text aligns 1:1; the
  residual is the inline book/item art = PORT-DEBT(dialogue-arrow-art)).  Commit `a91696f`.
  Writeup: `findings/dialogue-body-row-distribution.md`.  **USER-VERIFY: click the studio shortcut**
  (`studio-current.txt` → `port-dlgdist.osr` | `retail.osr`) — scrub the arrival dialogue (ticks 661-985):
  the 1/2/3-line lines all space + fit like retail.
- **Prior (ckpt 144): the HOUSE/ERRANDS arc — HOUSE CAST + FREEROAM HAND-OFF + ERRANDS tile-frames, all
  committed (3 commits) + 1027 host pass.**  USER directive: "the errands scene and the scene right before
  it (house) — map 1:1, implement arche's movement, arche+mom+dad missing on the scene right before errands;
  synthesize whatever trace you need."  All three delivered:
  - **HOUSE CAST (Arche+Mom+Dad) — position bit-exact** (`actor_spawn_room_cast`/`HOUSE_CAST`, `g_room_cast`):
    the port suppressed the cast in non-town rooms; now a per-room cast pool spawns the family (banks 0x8b/0xe3/
    0xb5, which PERSIST across the map reload) at their captured world positions + idle clips, rendered +
    animated only in non-town rooms.  Positions solved from the town cast's 1:1 projection (1 px = 100 world,
    house cam 89600/3200) → Arche (128000,39200)/Mother (131200,37200)/Father (134400,37200) — land EXACTLY on
    the cutscene.c house speaker coords (an independent cross-check).  draw_probe tick 1340: Arche @354,336 /
    Mother @386,320 / Father @418,320 == retail; residual = the idle-breathe frame PHASE only (effect-anim-phase).
  - **FREEROAM HAND-OFF — controllable Arche walks the errands** (`freeroam_begin`/`freeroam_step`,
    `g_freeroam_char` = the bit-exact mover): on chain-complete the port hands off to `character_step` on the
    live held axis (the +0x200==0 char-AI path, quirk #103), REPLACING the removed ckpt-120 MVP (retires
    PORT-DEBT(char-control-trigger)).  Spawn world (19200,52000) facing right (== retail freeroam-walk /
    control-path-gt; projects to screen (162,336) at the errands cam (0,16000)).  Walk cels 0-3 (right) / 4-7
    (left mirror, flip_table[0x8b]=4), idle 0/1.  VERIFIED off `port-freeroam.osr` (dialogue nav + a held-axis
    walk): Arche walks RIGHT 162→475px then LEFT →181px, facing flips — driven by the live axis.  **JUMP
    wired too (`5aa1092`): jump_held = axis_held[4] (the C-button level, KEYMAP DIK_C); off port-jump.osr her
    dst-Y arcs ground 336→apex 261→land 336 (the bit-exact windup/impulse/variable-height physics).  Walk +
    jump both work on live input; run/dash is the only un-wired move (the double-tap detection, char-run-trigger).
  - **ERRANDS tile-frames bit-exact** (`map_decode.c`): the auto-footprint floor/wall arms (0x1b97c/72/77)
    emitted `cfg->scene_frame` (=0) but the per-cell tile VARIANT is the cell's **`arg_0c`** (+0xc).  PROVEN: the
    8 errands 0x1b977 cells' arg_0c (4,5,5,8,5,5,6,7) == retail's res 1897 frames exactly; town/house don't use
    these tiles (so untested before).  Fixed → EVERY errands tile bank's per-frame draw count now == retail
    (res 1897/1898/1072/1073/1074); full-frame differ 143978→90939.
  - **ERRANDS CAST — the family + shop props (`ERRANDS_CAST`, commits `7ea34cb`+`71717df`).**  Father (bank
    0xe3) @480,320 + Mother (bank 0xb5) @624,128 + the 10 static shop props/NPCs (res 1027/bank 0x16c) —
    captured static room-cast members (the TOWN_EFFECT_DEFS pattern).  All 14 res-1027 draws now == retail
    (frame@pos); errands differ 90939→82639.  The errands BACKDROP + CAST are now bit-exact; the residual is
    the errands DIALOGUE/HUD only (the questline 0x4dc510, `cutscene-scene-chain`).
  **USER-VERIFY:** `osr_view.exe C:\oss-osr\port-errands.osr C:\oss-osr\retail.osr` (shortcut loaded) — scrub
  the HOUSE (ticks ~1340-1670: the family present) + the ERRANDS (ticks ~1740+: the shop backdrop + family +
  props 1:1, Arche idle); `osr_view.exe C:\oss-osr\port-freeroam.osr C:\oss-osr\retail.osr` shows Arche WALKING
  the errands (the port walks her on a held-axis trace, retail is idle there — the walk is the port-only demo).
  Montages on the feed.  **NEXT: the errands opening DIALOGUE + questline (0x4dc510) + the freeroam HUD +
  freeroam refinements (jump/run, camera-FOLLOW [Arche walks off-screen past ~wx 60000], distance-locked walk).**
- **Prior (ckpt 143): the BUTTERFLY VERTICAL FLUTTER is PORTED + bit-exact — THEME 2 note #2's vertical bob
  is RESOLVED; the 4 town butterflies now bob up/down matching retail tick-for-tick (port dst-Y == retail, 0 X
  regression).**  RE'd "trace the code" (the physics) + USER-approved captured stand-in for the trigger.
  **Mechanism (quirk #112):** the bob is the SHARED case-3 "jump" FSM (`0x442a70`, the SAME one `character.c`
  ports for Arche's jump), with the butterfly's per-archetype constants read off the install (`0x41f200` case
  `0xe29a` → `0x427d30(−32000,1000,4000)` + `0x427c30(1,16000,2000)`): a 4-tick windup → impulse **−32000** (+2000
  on the impulse tick ⇒ −30000) → rise grav **+1000 held / +4000 free** (variable-height like Arche's jump) →
  fall **+2000** capped **+16000**; `worldY += vvel/100`.  The flap TRIGGER is the terrain-aware wander mover
  `0x43f880` — it scans the collision grid DOWNWARD for the floor and flaps to hold ~8000 units above it (the
  irregular 16-38t cadence emerges from the floor scan), the SAME mover the freeroam arc needs.  **The port
  (`butterfly.{c,h}`):** the real case-3 physics in `butterfly_step`, driven by the captured per-tick
  `(state3, cmd2_held)` control (`butterfly_flap_ctrl.h`, 2 bits/butterfly, seed-pinned from
  `runs/butterfly-flutter`) — `PORT-DEBT(butterfly-flutter-trigger)` to retire with the freeroam mover.  1027
  host pass (+1 `butterfly_flutter`).  **VERIFIED:** the ported physics reproduces the captured `vvel` 0
  mismatches / 1824 ticks ×4; `port-flutter.osr` vs `retail.osr` (`draw_probe --res 0x3fa`, ticks 80-360) the
  butterfly dst-Y matches retail tick-for-tick, X byte-identical before/after the change (289==289 = the
  pre-existing horizontal lag).  Plot on the feed (port·retail dst-Y overlap).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-flutter.osr C:\oss-osr\retail.osr`** (shortcut loaded) — scrub the settled town
  (ticks 80-360): the butterflies bob up/down matching retail.  Writeup: `findings/butterfly-flutter.md`.
- **Prior (ckpt 142): the NPC COLOUR variant is PORTED + bit-exact — THEME 2 note #1 ("npc color variant
  gap", tick 274) is RESOLVED; the townswoman renders her blonde/pink variant instead of the brunette/blue
  base, pixel-identical to retail (crop differ_px 1387→0).**  RE'd "trace the code" (not curve-fit).
  **Mechanism:** the girl = the map townswoman `0xc440` with colour variant `param_11` = the map record field
  **`+0x2c`** = 1 (the ONLY town EFFECT with `+0x2c`≠0; proven via a spawn dump).  `0x41f200:1768` resolves a
  per-variant sprite BANK (0→`0xa5` base / 1→`0xa6` / 2→`0xa7` / 3→`0xa8`); the variant banks are **CLONES of
  the base 8bpp sheet + a palette-INDEX-remap** at the info-entry `+8` (`DAT_006748d0/ad8/ce0`, .rdata) that
  shifts the body palette indices 0x20-0x4f into the next 48-colour bank (the sheet's embedded palette holds 4
  banks).  **The fix (`src/asset_register.c`):** (1) `ar_sprite_decode` applies the variant's remap to the 8bpp
  pixels before the slice (`ar_npc_palette_remap`; data from `tools/extract/npc_palette_remap.py`, the
  world_tables pattern); (2) **the pass-order bug** — the clone (`FUN_004179b0`) CLEARS the dst info-entry `+8`,
  and the port batched all info-events THEN all clones, so the clone wiped the remap pointer; retail issues each
  DATA_SET AFTER its clone (proven 98/98), so `ar_reapply_group3_data_events` re-applies the data-sets after the
  clones.  1026 host pass (+2).  **VERIFIED off `C:\oss-osr\port-npc.osr` vs `retail.osr`:** crop tick 274
  (137,290 48×89) `differ_px==0`; full-frame 1518→108 (residual = the pre-existing butterfly noise, no
  regression).  Recon on the feed (blonde/pink).  **USER-VERIFY: `osr_view.exe C:\oss-osr\port-npc.osr
  C:\oss-osr\retail.osr`** (shortcut loaded with this pair) — scrub the settled town (ticks 80-360): the
  townswoman by the inn is blonde in a pink dress, matching retail.  Writeup: `findings/npc-colour-variant.md`.
- **Prior (ckpt 141): the run-off CAMERA-PAN OVERLAP is FIXED — the camera + Arche's screen position/run cels
  now match retail TICK-FOR-TICK during the run-off (framing offset 0px, was ~40px); the run-off fires
  CONCURRENT with "Cool!", not serialized after its confirm.**  RE'd "trace the code" — the ckpt-140 diagnosis
  ("~10t late") had the trigger tick wrong; the CAMERA ONSET pins it (quirk #111): the easer `0x43d1d0`
  accelerates from rest (`v+=10`/tick, **5 ticks to the first pixel**), and retail.osr's static caravan moves
  its first pixel at tick **977**, so the run-off command fired at **972** (= "Cool!"'s beat completion, full+14),
  NOT 976.  "Cool!"'s box BODY independently holds to tick 982 (its full+24 auto-hold), so the run plays behind
  the still-shown line.  **The port (`cutscene.c`/`actor_spawn.c`/`nav-theme3.jsonl`):** advance "Cool!" at 972
  (the run-off lead −10) firing the camera + run, LINGER the box showing "Cool!" to 982 (`ARRIVAL_RUNOFF_BOX_HOLD`,
  THEME 1 preserved), accelerate Arche from 972 but hold her run cel idle 7t so fr16 lands at 980
  (`ARCHE_RUNOFF_WINDUP_TICKS`); `ARRIVAL_L8_RUNOFF` 97→108 keeps L8 at ~1150.  **VERIFIED bit-exact off
  `C:\oss-osr\port-runoff2.osr` vs `retail.osr`:** caravan res 1004 position IDENTICAL every tick 977-1000;
  Arche res 0x570 screen-x + run cels IDENTICAL every tick 975-1010 (fr16@980, fr17@990, fr18@995, fr19@1000);
  the whole arrival→house dialogue chain unchanged (L0-L11 within ~1t).  1024 host pass.  Recon montage on the
  feed (tick 985, near-zero diff).  **USER-VERIFY: `osr_view.exe C:\oss-osr\port-runoff2.osr C:\oss-osr\retail.osr`**
  (shortcut loaded with this pair) — scrub ticks ~972→1010: Arche runs + the camera pans, framed identically to
  retail.  Residuals (documented debts, NOT the camera/run): the ~5-tick windup LEAN cels (retail fr 3/8/9 vs
  the port's idle = `cutscene-party-chars` emote) + the box-close SLIDE (`dialogue-runoff-box-slide` — retail
  slides the empty bubble off, the port shrinks it).  Writeup: `findings/arche-runoff-render.md`; quirk #111.
- **Prior (ckpt 140): the cast "ARCHE RUNNING" RUN-OFF RENDER is PORTED + frame-sequence bit-exact — THEME 2
  note #5 ("arche runs to the house in retail, stands still in port", USER tick 1027) is RESOLVED.**  On the
  L7→L8 inter-line beat ("Mom! Dad! c'mon!") Arche now plays her RUN cycle → DECEL → arrival-IDLE and runs to
  the house door, instead of standing static.  **Render is FAITHFUL (the clips are RE'd cel-sequence metadata
  read off `retail.osr`'s Arche draw stream, `draw_probe.py --res 0x570`): RUN cels 16,16,17,18,19,19,20,21
  (dur 5, loop — contact frames 16/19 held 2×), DECEL cels 8-11, arrival-IDLE 152-154.**  **Motion is the REAL
  ported run physics** (char-run, ckpt 118: two-phase accel → cap 48000, `world_x += vel/100`) toward the RE'd
  target world 73104 (= `0x402730(Arche,+32000)` = L8's spk_wx); only the DECEL-approach is the tagged
  `0x54f980`-mover stand-in.  1024 host pass (+`test_arche_runoff`).  Writeup: `findings/arche-runoff-render.md`.
- **Prior (ckpt 139): the BUTTERFLY DIRECTION SPRITE is PORTED + DRAWCALL-1:1 — THEME 2 note #0
  ("butterflies color gap") and the SPRITE half of #2 are CLOSED; the 4 town butterflies now render their
  correct colours/directions, bit-exact to retail.**  RE'd "trace the code" then live-read-verified (the
  Frida host, 2452 render calls, 0 deviations) — NOT curve-fit.  **Mechanism (quirk #110): the butterfly cel
  = `frame_base + 16·(facing==3) + flap`**, where (a) **`frame_base` = the per-instance map VARIANT field
  (+0x18)** = its BASE DIRECTION (0/4/8/12; the install `0x426d70(0,0x146,param_7)`, `param_7 =
  *(u16)(record+0x18)` via dispatcher `0x58d460:151`) — the port hardcoded 0 (all butterflies one variant);
  (b) the **facing mirror is `DAT_008a8440[0x146] = 16`** (frames 16-31 = the left-facing cels) — the port had
  it wrong as 4; (c) the facing toggle (1/3) was ALREADY ported (the heading FSM).  The render's angle path is
  DEAD for butterflies (angle_anim=0).  Fix: `src/actor_spawn.c` reads the variant for 0xe29a + flip 4→16 (a
  2-field change; the facing toggle does the rest).  **VERIFIED** off a fresh port `.osr` vs `retail.osr`
  (`draw_probe.py --res 0x3fa`): butterfly frames now match tick-for-tick (e.g. ticks 272-278 upper butterfly
  21,21,5,6,6,6,6 == retail; was the wrong 5,5,1,2,2) — 273/280 settled-town ticks frame-identical (the 7
  misses are a pre-existing horizontal-position lag of the entering-from-left butterfly, NOT a frame bug).
  1023 host pass (+butterfly frame_base=variant test).  **RESIDUAL (separate debts, NOT this chip):** the
  per-tick dst-Y bob (`butterfly-flutter`, note #2's vertical sawtooth) + the entering-butterfly horizontal
  position (`butterfly-bounds-writer`).  Montages on the feed.  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-bf.osr C:\oss-osr\retail.osr`** (shortcut loaded with this pair) — scrub the
  settled town (ticks 80-360): each butterfly shows its correct colour/direction; the only residual is a
  ≤2-5px vertical bob (the flutter).  Writeup: `findings/butterfly-direction-sprite.md`.
- **Prior (ckpt 138): the THEME-3 transition FADE punch-list (the ckpt-137 USER studio residuals) is
  CLOSED — the arrival→house fade matches retail: CENTER-OUT, the dialogue box renders OVER the cover, and
  house L0 lands tick-1:1 (the +8t is gone).**  Commit `cd1c547`; driven by reading the retail `.osr` DRAW
  STREAM (`tools/trace_studio2/draw_probe.py`), verified tick-1:1 off a fresh port capture.  1023 host pass.
  (1) **VARIANT → center-out (var 0):** retail's cover (tick 1234) + reveal (1261) both grow/recede from the
  MIDDLE (+4 rows/tick), NOT edges-in.  The variant is genuinely the RNG draw `(rand*3)>>15` (`0x439690:563`)
  but the port's LCG drifts at these arms (aligned at game_enter — establishing reveal rolls 0 — but the
  unported cast consumes the LCG in between); `main.c` forces the beat's `fade_var`=0 as a center-out
  STAND-IN, keeping the `rng_rand()` draw to match the per-arm count.  `PORT-DEBT(cutscene-fade-variant)`.
  (2) **Box OVER the cover:** the closing L9 box shrinks out in FRONT of the cover (already rendered after
  scene_fade — it just needed the cover to OVERLAP the close).  (3) **House L0 +8t = the cover-arm TIMING,
  NOT a hard wipe** (retail ages the cells too, alpha `bmode=1`): off retail.osr the cover arms ~10t after L9
  advances (1224→1234), not the script's WAIT 0x14=20 — the exit wait runs ~2×/sim-tick (vs the house
  wait's 1×, not yet pinned); `cutscene.c` calibrates `ARRIVAL_EXIT_WAIT=10`.  **VERIFIED:** cover var-0
  onset 1235 (retail 1234), reveal var-0, box over the cover, house L0 first-glyph 1370 (retail 1372, was
  1380); house L1/L2/L3 still tick-exact.  Montage on the feed.  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-theme3.osr C:\oss-osr\retail.osr`** — scrub ticks 1224→1300: the box runs
  ahead, the screen fades to black from the MIDDLE with the bubble on top, the room swaps under black, the
  house fades in from the middle.  **+ LETTERBOX follow-up (USER studio note tick 1293):** the cinematic
  letterbox is now town-arrival-only — gated `letterbox_render` on `g_loaded_room_key == CUTSCENE_ROOM_ARRIVAL`
  so the house/errands render bar-free (retail drops the bars at the room swap; `PORT-DEBT(ingame-letterbox)`
  reduced to the constant heights).
- **Prior (ckpt 137): the arrival→house TRANSITION CHOREOGRAPHY (THEME 3) is PORTED + TICK-1:1 — the
  cutscene now plays the non-dialogue BEATS between L7 and the house, tracking retail tick-for-tick through
  the house dialogue.**  RE'd exactly (USER: trace the code, don't measure — engine-quirk #109): the script
  `0x4d7d80` is a FLAT beat sequence the beat-runner `0x439690` pumps.  Added a beat model to `cutscene.c`
  (WAIT / CAMERA_PAN / FADE beats, a per-room lead/exit map; main.c performs the camera/fade/reload).
  Three beats ported: (1) **L7→L8 the "Arche runs ahead" gap** — a FIRE-AND-FORGET camera pan to (28000,12800)
  (the `0x43d1d0` easer, draw-verified -148px scroll) + the case-4 RUN-OFF wait (`0x402730` overwrites the
  beat type to 4; gated on the actor mover `0x54f980` = the cast, a tagged `cutscene-party-chars` stand-in,
  97t); (2) **the arrival EXIT** — wait20 + a fade-TO-black (cover) that GATES the room-key swap (so it
  happens under black, no early snap); (3) **the house ENTRY** — a fade-FROM-black reveal + wait50.  The
  fades RE the arm `0x439690:555-563` (MODE = `in_ECX[10]`, SPEED 1000, VARIANT = LCG `(rand*3)>>15`); a FADE
  beat completes when the live `scene_fade` grid SETTLES (retail's case-2 gate).  **VERIFIED off a port
  `.osr` vs retail.osr (dialogue timeline): L8 1150 (retail 1149) / L9 1190 (==) / house L1 1398 / L2 1448 /
  L3 1493 — all tick-exact.**  1023 host pass (+2: `cutscene_l8_lead_beats` + `cutscene_transition_fades`).
  RESIDUAL (RESOLVED ckpt 138): house L0 +8t — was the cover-arm timing (the exit wait), NOT a hard wipe;
  the "Arche running" sprite is the cast debt (THEME 2).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-theme3.osr C:\oss-osr\retail.osr`** — scrub the arrival→house transition
  (ticks 982→1500): Arche's box runs ahead (L8), the screen fades to black, the room swaps under it, then the
  house fades in; the dialogue tracks retail tick-for-tick.
- **Prior (ckpt 136): the dialogue PORTRAIT FADE-OUT dissolve is PORTED + DRAWCALL+LUT-EXACT — the
  ckpt-135 next-chip is CLOSED; on a speaker change the OUTGOING bust now dissolves out via the reverse
  ramp idx 18→2 then GONE, matching retail TICK-FOR-TICK on ALL THREE arrival speaker changes.**  The
  coordinated fix (engine-quirk #108): retail processes the advance ~2t BEFORE the new box opens, so
  (1) the matched nav presses the speaker-change advance 2t early (`−8` not `−6`, `dialogue_timeline.py`),
  (2) `cutscene.c` DELAYS the new box's re-pop 2t (`reopen_delay`/`CUTSCENE_REOPEN_DELAY`: the main box is
  hidden, the closing-box snapshot dissolves, then ARM_REOPEN fires — the box-frame stays at `advance_tick−6`
  so the ckpt-134 28/28 overlap is preserved), (3) the closing box runs the reverse ramp (`dialogue_arm_fadeout`
  + `dialogue_fadeout_step`: `portrait_fade` 450→0, idx 18→2 then `DIALOGUE_PORTRAIT_GONE`=draw-nothing).
  **VERIFIED off both seed-pinned `.osr` (`tools/trace_studio2/portrait_fade_probe.py`, the portrait blit's
  per-tick BLEND ref → ramp idx): port == retail tick-for-tick** — L0→L1 idx 18,16,..,2 over [688,696] gone
  697; L1→L2 [733,741] gone 742; L2→L3 [778,786] gone 787 (the per-side BLEND ref numbers differ by a
  constant — same `ramp_b` LUTs — the IDX is identical).  Recon montage on the feed: **differ_px=0 at the
  mid/late dissolve ticks (690/692/694/696/697)**, the only residual a 1px ≤1-LSB cross-side sheet sample on
  the OPAQUE pre-dissolve bust (present with or without the fade-out — the standing accepted noise).  1021
  host pass (+2: `dialogue_portrait_fadeout` + `cutscene_portrait_fadeout`).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-fadeout.osr C:\oss-osr\retail.osr`** — scrub the arrival dialogue speaker
  changes (ticks 688/733/778): the outgoing bust dissolves out as the box closes, tracking retail; the
  box-overlap/cadence/fade-in stay 1:1.
- **Prior (ckpt 135): the dialogue PORTRAIT FADE-IN is DRAWCALL+LUT-EXACT — the USER's ckpt-134 "slightly
  less dim" note is RESOLVED (tick 661 now PIXEL-identical, differ_px=0).**  NOT the ramp formula or the
  box-open (both already faithful): retail HOLDS the dimmest cross-fade step (ramp_b idx 0) for TWO opening
  ticks because the cross-fade state (`0x49c910` `uVar1`/`+0x2e`) arms one tick AFTER scale hits 1000 —
  `0x49c910` returns early WITHOUT the `f += 50` on the first fully-open tick.  retail L0: idx 0 at ticks
  660+661, then 2,4,..,18 (662-670), opaque 671; the port advanced a step early (idx 2 at 661).  Fix:
  `dialogue_box.fade_armed` (`dialogue.c` `dialogue_step` gates the increment on a prior arm tick; reset by
  `dialogue_arm`/`dialogue_reopen`, untouched by `dialogue_set_text`).  **VERIFIED LUT-byte-identical
  port↔retail, 13/13 portrait ticks, EVERY arrival line** (L0 @150,76 / L1 @70,88 / L2 @38,76 / L3 @70,88).
  1019 host pass.  **SURFACED the next chip — the PORTRAIT FADE-OUT dissolve:** on a speaker change retail
  dissolves the OUTGOING bust out via the reverse ramp idx 18→2 over `[advance_tick−8, advance_tick]`
  (CONSISTENT L0/L1/L2), the port holds it OPAQUE then cuts it.  **MECHANISM RESOLVED (ckpt 135):** retail
  processes the advance ~2 ticks BEFORE the new box opens (the box-frame is tick-aligned port↔retail at
  `advance_tick−6`, 28/28 holds; retail box res=0; `0x49c910` is __cdecl, state @+0x2e 1=in/2-3=out, f @+0x30).
  **Fix spec'd (the open chip):** nav `−8` + delay the box re-pop 2t (28/28 preserved) + closing-box reverse
  ramp.  Three commits (`9153763` fade-in fix, `c97320b`+ fade-out RE).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-portrait.osr C:\oss-osr\retail.osr`** — scrub the arrival dialogue: the
  portrait fade-in tracks retail tick-for-tick (the old box-overlap/cadence stay 1:1); the speaker-change
  fade-OUT is the visible remaining gap (feed: tick 661 differ 0 / tick 692 differ 13976).
- **Prior (ckpt 134): the dialogue CADENCE is TICK-1:1 — the MATCHED-CADENCE nav landed and the arrival
  dialogue (L0-L7) now tracks retail TICK-FOR-TICK (start/full/advance all bit-equal, 314/323 ticks of
  name+body identical).  THEME 1 of the intro-cutscene punch-list DONE.**  Three chips: (1) **tick-keyed
  input-trace** — `input_trace` entries may key on the SIM-TICK (`{"tick":N}`) not just the Flip frame, axis
  PER-ENTRY so one nav MIXES a flip-keyed boot prefix + tick-keyed in-game confirms (the port's Flip cadence
  differs from retail's, so only the shared sim-tick aligns the dialogue).  (2) **the box re-pop model** —
  the port re-armed (20-update pop-in) EVERY line; retail keeps the box on a SAME-speaker advance (gap 1t,
  `dialogue_set_text`, no re-pop) and on a SPEAKER CHANGE re-opens from HALF scale
  (`dialogue_reopen`; the exact open/close is the box-overlap chip below).  Plus KEEP the word-wrap space
  (`dialogue_expand_text`: retail renders the trailing space → body byte-identical).
  (3) **`dialogue_timeline.py`** reads the reveal curve off any `.osr` (body MAIN glyphs 0x3e537d per tick)
  + emits the matched nav.  **PROVEN: the port reveal/skip MECHANICS were already faithful** (retail.osr:
  1 char/5t, space 1t, instant skip — the port's exact model), so note #4 was a CADENCE artifact (the spam
  nav skipped instantly), NOT a reveal-rate bug.  (4) **the speaker-change box OVERLAP** (USER studio note,
  tick 696): retail overlaps the closing OLD box (front) over the opening NEW box (behind) ~9t —
  `dialogue_close_step` + a `cutscene.closing` box (`render_dialogue_box` ×2) + a deferred same-speaker
  re-text (`pending_keep`); this also fixed the advance-boundary residual.  **The overlap is DRAWCALL-EXACT**
  (USER: no approximation — the first cut was a curve-fit close + a guessed z-order; redone after reading the
  exact drawcalls with the NEW `tools/trace_studio2/draw_probe.py` ordered-drawcall region probe): z-order
  CORRECTED (new box in front), open spawn 200 +50/update, close -40/update removed <160, old box lingers
  full until new>half, advance fires advance_tick-6.  Every box-frame cell (pos+scale) matches retail across
  L0->L1/L1->L2/L2->L3/L5->L6 (28/28/29/33 ticks EXACT); per-tick (name,body) 322/323 (tick 884 = a
  retail-coalesced flip).  1019 host pass (+8).  **USER-VERIFY: `osr_view.exe C:\oss-osr\port-matched.osr
  C:\oss-osr\retail.osr`** — scrub the arrival dialogue (ticks 661-982): reveals/skips/advances + the box
  overlap on retail's ticks.  THEME 3 (Arche-runs gap) starts after L7 (tick 982).
- **Prior (ckpt 133): the dialogue TYPEWRITER-SKIP is PORTED — the confirm-while-typing desync blocker is
  CLOSED; the port now advances dialogue at the PRESS cadence (chain COMPLETE @hold 2571 vs the old 11365,
  4.4×).**  The USER-flagged ckpt-132 blocker.  RE'd from the beat-runner `0x439690:976-1011` — the box
  widget is in state 1 (typing) OR state 2 (waiting), an if/else-if, so retail's ONE confirm (ENTER/X = ring
  `0x24`) does exactly ONE thing per tick: press while TYPING → SKIP (`FUN_0043ce50(9)`→`FUN_0043ca40(9)`
  forces the text fully-shown, `FUN_0043bca0` returns 3 → state 1→2); press while COMPLETE → ADVANCE
  (`FUN_0043b980`).  The skip press is consumed, NOT also advancing — ~2 confirms/line, the SAME cadence as
  retail (the old advance-only port waited out every typewriter → lagged → desync).  **Ported** `dialogue_typing`
  + `dialogue_skip_reveal` (reveal→total) in `dialogue.{c,h}`; `cutscene_step` now SKIPs-or-ADVANCEs on the
  confirm (mutually exclusive); renamed `advance_pressed`→`confirm_pressed`.  **Also (ckpt 133): the live
  CONFIRM is ENTER/X not Z — RE'd the `0x46a880` producer** (the ring WRITER, the input.md "only remaining
  black box", now RESOLVED): ENTER (`0x1c`) is the FIXED `0x24` binding, X (config `+0x558`, the attack key)
  also posts `0x24`; Z is the `+0x590` sheathe button (→ ring 9, NOT confirm).  Fixed `input_live.c` KEYMAP
  (ENTER+X→`0x24`, dropped the wrong Z→`0x24`).  **Verified:** host test (skip-then-advance, X/ENTER confirm,
  Z does not) + the 4.4× completion collapse + a fresh `C:\oss-osr\port-skip.osr` tick-joins retail.osr
  **2027/2042 paired, 3 anchors RNG-OK** (the port stays on retail's tick axis through the dialogue).  1012
  host pass (+1).  Montage on the feed (arrival→house, skip-driven).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-skip.osr C:\oss-osr\retail.osr`** — boot→town is pixel-1:1; the DIALOGUE
  section will show nav-cadence divergence (the port spam-nav vs retail's nav) until a matched nav lands —
  that mismatch is the NEXT sweep step, NOT a skip bug.
- **Prior (ckpt 132): the dialogue BOX POSITION is PORTED FAITHFULLY (`0x49c640` over the `0x490b90`
  projection) — 17/18 town-intro lines bit-exact vs retail; the box now ANCHORS to the speaker.**  The
  USER-chosen faithful fix for the last ckpt-131 rendering gap.  Captured EVERY `0x49c640` input from retail
  (`tools/flow/box_pos_inputs_fields.json` + a new `argchain` agent read kind; `runs/box-pos-inputs`), which
  PROVED the formula: `box_x = clamp((sprite_w/200 - W/2) + scr_x, 0x20, 0x260-W)`,
  `box_y = (metric_14/100 + spk+0x1c) - H - 0x30 + scr_y`, where `(scr_x,scr_y)` is the speaker WORLD pos
  projected through the LIVE camera (`scr_x = wx/100 - cam->0x60/100`, …) — the SAME `mr_camera` the port
  already has.  Ported as `dialogue_box_position` (pure C, host-tested bit-exact vs all 10 captured cases);
  per-line speaker world pos + per-character body geometry baked into `cutscene.c` (HARNESS-CAPTURED — the
  live cast that owns them is PORT-DEBT(cutscene-party-chars)); `game_render_dialogue` projects through the
  live `g_game_camera_mr` each frame (the box TRACKS the camera).  A port `--call-trace` over the same nav
  reproduced retail's box position for arrival L1-L9 + ALL house lines bit-exact (both clamps fire: box_x 32
  left / 200 right).  **The ONE residual — arrival L10 Mother (port 62 vs retail 32) — is the camera not
  panning to 28000 to follow Arche running ahead at L9 (Arche is static = PORT-DEBT(cutscene-party-chars) +
  the camera-pan debt), NOT a box-logic bug** (quirk #106, the upstream-input pillar).  Retires the box half
  of PORT-DEBT(dialogue-trigger); the bubble TAIL x still rides box_x+188 (the same projection would derive
  it — a small follow-up).  1011 host pass (+1).  Montage on the feed (per-speaker anchoring).  **USER-VERIFY:
  `osr_view.exe C:\oss-osr\port-boxpos.osr C:\oss-osr\retail.osr`** (scrub the arrival/house dialogue; the box
  rides each speaker).
- **Prior (ckpt 131): the dialogue-rendering gaps up to the errands — FOUR bugs fixed, harness-verified;
  the box POSITION is the one remaining 1:1 gap.**  USER drove a frame-by-frame trace-studio pass.  (1) the
  errands BOTTOM FLOOR (bank 0x188 cloned from the wrong source → floor tiles culled; fixed the inline-clone
  {0x187←0x184}/{0x188←0x185}, `e8d5c0b`); (2) house + errands PROPS (the map-driven STRUCTURE band now
  re-spawns + renders per room — house flowers/"Items" sign, errands furniture; `1d826c3`); (3) the dialogue
  PORTRAITS — **the +13 pool/array offset made EVERY bust wrong** (the render indexed
  `g_ar_sprite_slots[pslot]` directly, but the face table returns a POOL index → use `ar_pool_get_slot`;
  `cbbab94`, verified the port now emits retail's exact slots res 0x7ef/0x5a3/0x7f9 + the right 160×176
  dims).  REMAINING: the box POSITION (port hardcodes (174,148); retail anchors to the speaker via
  `0x49c640`) — USER-chose to port it faithfully; ground truth captured (`tools/flow/portrait_pos_fields.json`,
  the per-line box obj +0xc/+0x10).  1010 host pass (+1).  Montages on the feed (L1 Father / L3 Mother
  port-vs-retail).  Studio note: the NOTE UI is dual-mode only (a single-file-scrub gap to build).
- **Prior (ckpt 130): ROOM-RENDER LANDS — the house (DATA 1023) + errands/freeroam (DATA 1025) ROOM
  BACKDROPS RENDER.** The whole reason trace-studio v2 was pulled forward: resume the room-render arc.  The
  cutscene room-key swap (arrival 0x334be → house 0x334c8 → errands 0x334dc) now RELOADS the backdrop, the
  per-room `map_decode` tilesets are ported BIT-EXACT, and both rooms render (montages on the feed).  5
  commits (`c2b1568` M1 → `87bf668` M4); 1009 host pass (+5).  **The main-goal room — the errands/freeroam
  scene — now renders its real backdrop; the controllable-Arche hand-off is the next arc.**
  - **M1 (`c2b1568`) — scene + parallax from the active room.**  `game_world_room_render_cfg(key)` resolves
    a room's DATA scene (GW_ROOM_SCENE) + the 0x587e00 prologue params (room[0x44]/room[0x43]) from the
    registry; `town_render_load` takes them; `main.c load_room(key)` drives the load.  Verified at the byte
    level: arrival→1022/(4,1), house→1023/(4,1), errands→1025/(9,4).
  - **M2 (`e228150`) — the house + errands map_decode arms, BIT-EXACT to a retail emit capture.**  The
    "main RISK" resolved: ported the 14 new FUN_00587e00 tile-id arms (the 10xxx/112xxx/113xxx/171xxx/172xxx
    families + the 113xxx auto-footprint floor/walls) + the param_4 tileset-bank prologue
    (`map_decode_cfg`).  Ground-truthed against a retail capture of the decode emit sequence
    (0x58c910/0x58ca80 across the chain, `runs/room-render-gt`) cross-referenced with the cell
    (tile id, shape) histograms — every emit_tile/emit_obj (bank, slot, flag, count) matches retail EXACTLY
    (a host probe decodes the real DATA 1023/1025).  RESOLVED the architecture Q: the room swap DOES re-run
    0x587e00 per room (3 decodes captured); param_3(local_918)=0x14 for all town-area rooms.
  - **M3 (`c3accc0`) — room-keyed backdrop reload + per-room camera.**  `reload_room_backdrop(key)` (free +
    load_room + camera snap) fires on the cutscene room swap; the house/errands SETTLED camera origins are
    harness-captured (house (89600,3200), errands (0,16000)).  Town cast suppressed for non-town rooms (the
    room cast is PORT-DEBT Phase 2b).  Retires PORT-DEBT(cutscene-room-render): the house lines play IN the
    house.  **USER-VERIFY: `osr_view.exe 'C:\oss-osr\port-rooms.osr'`** (scrub to the house ~tick 3185 +
    errands ~tick 5938; drill the draws).
  - **Tooling (`87fafd5`) — `--no-frame-limit`** uncaps the in-game 60 FPS gate (gated on g_game_active; the
    title nav stays capped) so the full ~13000-frame cutscene→errands replay captures in ~9 s not ~210 s.
  - **M4 (`87bf668`) — the errands-render CRASH fixed.**  Rendering DATA 1025 access-violated: an
    under-loaded tileset bank (PORT-DEBT(assetreg-clone-defer)) made `ar_sprite_slot_frame`'s unbounded
    `frames[frame_id]` read OOB → a garbage cel the blit deref'd.  Bound frame_id against `slot->f_38` (the
    slice frame count; retail's bank always has enough, so no behavior change there — the port culls a
    frame a bank genuinely lacks).  The errands now renders (gaps where a bank is under-loaded).
  - **OPEN / NEXT:** (1) the FREEROAM HAND-OFF — at the errands room, stop the sequencer + run
    `character_step` on live `axis_held` (the `+0x200==0` char-AI path, mover DONE bit-exact); (2) load the
    under-loaded errands tileset banks (the render gaps; assetreg-clone-defer); (3) the room CAST
    (Phase 2b); (4) a tick-aligned port↔retail studio diff needs a matched-cadence nav (the port
    cutscene-verify nav vs retail control-path nav reach the rooms at very different ticks).
- **Prior (ckpt 129): M6 — the TICK-JOIN STUDIO LANDS: both sides' `.osr` are paired by the
  deterministic `sim_tick` (the openrecet E3 identity join — NO flip-drift search) and `osr_view` grows a
  native PORT|RETAIL|DIFF three-panel + a diff heat ribbon. The studio is now a usable frame-by-frame 1:1
  port-vs-retail scrub. 1002 host pass (unchanged — tooling only); commits `2788ed9` (M6a) + `57260be`
  (M6b). Tick-join montages on the feed.**
  - **M6a — the JOIN verdict + the streaming reader (`tools/trace_studio2/`, runnable from WSL).**
    `osr.py` grew `stream_records`/`stream_frames`/`read_header` — a block-buffered iterator that yields only
    the small records (FRAMEBEG/ANCHOR/SEED) and SKIPS the bulky BLIT/TEXT/SHEET payloads by seek arithmetic,
    so it streams the full **1.9 GB `retail.osr` (37673 frames) in ~11 s with NO OOM** (retires the
    survey-flagged `parse()` OOM debt + the `run_proxy.sh` 256 MB band-aid; byte-identical to `parse()` on
    the small file). `pair.py` streams both sides → groups frames by `sim_tick`, takes the LAST flip per tick
    (the presented state — retail coalesces, quirk #99) → joins on the tick union, reporting paired count,
    honest port-only/retail-only GAPS, per-shared-anchor RNG assertions, and the flip-axis drift contrast.
    `--write-pairs` emits `pairs.json` (reference; the viewer recomputes natively). VERDICT on
    (port-m5, retail-snap): **PASS — 190 tick-paired, 2 honest port-only gaps (ticks 41/91, retail coalesced),
    all 3 shared anchors RNG-aligned** (newgame `0x404a0a8f`, prologue/game_enter `0x40d00581`); game_enter
    lands at port flip 1116 vs retail 1242, so naive same-flip pairing would silently drift +126 flips
    (~63 ticks) — the tick-join is immune.
  - **M6b — the native three-panel (`tools/osr_view`, `osr_view.exe <port.osr> <retail.osr>`).** Opens TWO
    scrub sessions (two DDraw devices + the DX11 swapchain coexist — recon is system-memory, no contention),
    builds the tick-join in C from the two frame indices (`build_join`, same semantics as `pair.py`, NO
    `pairs.json` dependency), and shows PORT | RETAIL | DIFF at the joined tick with a tick-indexed scrubber.
    `diff_image` = an amplified cross-side diff (faint silhouette where equal, yellow→red by magnitude) +
    `(differ_px, maxd)`; the diff HEAT RIBBON precomputes per-paired-tick `differ_px` in the background and
    draws an aggregate worst-per-column strip (green=exact, yellow→red=divergent, blue=honest gap) with
    click-to-seek + worst/next-diff nav. Single-file mode unchanged. Makefile now links `src/osr_emit.c`
    into all 3 osr_view targets (zdd.c hard-refs its M5 taps — fixes a latent M5 link break).
  - **VERIFIED headless (`osr_prof` dumps, the same `osr_scrub` engine the GUI wraps):** the join indices
    match `pair.py` exactly, two-session reconstruction works, and the cross-side diff is real — **sim_tick 0
    (the game_enter TRANSITION frame — near-black, the inspector showed it's a town composed then wiped by a
    late mid-frame scene-transition CLEAR, quirk #105) reconstructs `differ_px==0` PIXEL-IDENTICAL on both
    sides**; the settled town renders a few ticks in (e.g. tick 97 `differ_px=264` 0.09%, the animated
    butterflies — a small localized divergence the studio surfaces). USER-CONFIRMED the GUI ("studio looks
    good"); the post-tick-191 black PORT panel is the honest gap (port-m5 ends at tick 191).
- **ALSO ckpt 129: M7 — the DRILL-IN + the NOTE hand-off (USER-requested "M7 stuff + a note system").**
  Commits `953ee74` (notes) + `b568104` (inspector engine) + `6279274` (inspector GUI). On the feed.
  - **NOTES (the human→agent contract):** in osr_view the USER drags a crop rect on any panel + types a
    note → `osr_notes.jsonl` beside the `.osr` (tick, port/retail flip, crop xywh, differ, text); a list
    panel seeks/deletes; loads on startup. `tools/trace_studio2/notes.py` is the agent READ side — resolves
    each note's tick → frame indices via the join, and `--render` reconstructs the cropped port|retail|diff
    at that tick (`osr_prof` dump + crop), `--feed` pushes it. So a mark says "look HERE at THIS tick" and
    round-trips to a precise visual. VERIFIED end-to-end headless (a C-format sample → notes.py rendered the
    crops). Gap panels now LABEL "no frame at this tick" instead of bare black.
  - **DRAW DRILL, UNIFIED into the main panels (openrecet N3 + the USER's UX):** `osr_scrub` grew
    `frame_ndraws`/`frame_draws` (the ordered draw list + labels), `render_rgba_upto(K)`, `pick_draw(px,py)`.
    A **frame/draw-drill MODE toggle** + per-panel **show** checkboxes (NOT a separate window); in drill mode
    ONE K slider drives `render_rgba_upto` on BOTH sides synchronously → the DIFF panel shows where the two
    draw sequences diverge.  DRAG=crop, CLICK=pick (InvisibleButton owns both → the crop-drag window-move bug
    is FIXED).  ENGINE VERIFIED headless (`render_rgba_upto(all)==render_rgba`; clean build-up; cross-side
    up-to-K diff at tick 112 peaks ~1020px @K≈158 then settles to 380).  **USER-CONFIRMED** the scrub + the
    note round-trip (cropped Sana = pixel-identical).  Direction: eventually MATCH the draw SEQUENCE port↔
    retail for max faithfulness (tick 112 already shows port 616 vs retail 634 draws — a real mismatch).
  - **WORKFLOW (CLAUDE.md, USER-set):** inspect every divergence in the draw drill; GIVE the `osr_view.exe
    <port> <retail>` command on any visually-confirmable change; crop MARKS → `notes.py --render`; a studio
    shortcoming is a new studio FEATURE.  **NEXT:** the studio is usable + self-diagnosing → RESUME the
    now-unblocked room-render/freeroam port (verify each frame in the studio + draw-sequence-match).  Studio
    polish (openrecet survey 4/5/6: `.osr` slice tool, capture cache + one orchestrator cmd, draw-program
    semantic panel) pull-when-needed.  CAVEAT: port-m5 only reaches tick 191; the 190-paired region is the
    working demo.  Roadmap: `plans/trace-studio-v2.md`
    §openrecet-v3-survey (items 1/2/3/7 done; 4/5/6 remain).
- **ALSO ckpt 129: M8 — the GAME-STATE PANEL (USER-requested; openrecet orv3_state model).** A native,
  opt-in OSR_STATE record (`src/osr_format.h`; generic NAMED fields, extensible) captured both sides — port
  via `--osr-emit --osr-state`, retail via `OSS_OSR_STATE=1` — and shown in an osr_view "ENGINE STATE" panel
  per joined tick, port-vs-retail, diff-highlighted.  The **RNG census/survey is folded in**: `rng` (LCG
  state `DAT_008a4f94`/`rng_peek_state`) both sides + `rngcalls` (cumulative draw count, new `rng_call_count`)
  port-side; this LIVE per-tick rng diff SUPERSEDES `tools/rng_tick_diff.py` (archived → `tools/archive/`).
  Commits `ba0b801` (M8a format+port emit+osr.py+test, 1003 host pass) + `8da3dcb` (M8c viewer panel) +
  `2a6f424` (M8b proxy rng emit).  VERIFIED headless: a 150-frame port capture emits STATE=150
  (rng=0x4f5347 pinned, rngcalls=0), `osr.py STATES`/`SUMMARY` read it, `osr_scrub_frame_state` feeds the
  panel.  **EXTENSIBLE — add fields as we annotate** (`osr_emit_state_field` in `main.c` drive_present +
  the read in the proxy `eh_flip_cb`): player px/py, scene id, flags, dialogue state next.  OPEN: retail
  `rngcalls` (`PORT-DEBT(osr-state-rngcalls-retail)` — a `0x5bf505` trampoline); GUI panel + the retail
  capture are USER-verify.  Consumer ATTRIBUTION stays `tools/rng_consumer_census.py` (separate).  Pointer
  persisted in CLAUDE.md.
- **Prior (ckpt 128): M5 — the PORT `.osr` EMITTER LANDS: the port writes the SAME draw stream the
  retail proxy captures, and `--osr-replay` of the port's OWN `.osr` rebuilds its frames
  `differ_px==0` (newgame menu 700 / prologue 900 / town 1250 vs the port's live captures) — the
  port stream is SELF-CONTAINED. 1002 host pass (+2); commit `cc99f3a`. Montages on the feed.**
  - **`src/osr_emit.{c,h}`** (pure C, host-linkable) mirrors the proxy's hook map 1:1: FRAMEBEG/
    PRESENT at `drive_present` (present-then-framebeg, the same one-flip label offset), BLIT at the
    5 zdd primitives (`zdd_emit_blit` extended with dest/desc/srcw+srch), dedup'd per-CEL SHEETs
    via an injected lock-based surface reader (sheet_grab.h's hash shape; tombstoned eviction at
    the zdd_object dtor — the ckpt-126 lesson), CLEAR at `zdd_object_clear` (quirk #105), mode-4
    BLEND (blend_grab.h's exact LUT sizing), TEXT via a per-HDC shadow bound at
    `zdd_object_get_dc` (glyph ops + dialogue mirrors; banner-cel text correctly filtered — its
    pixels arrive via the composed cel's SHEET), FONT at the `ar_gdi_create_font` chokepoint,
    ANCHOR/SEED at the existing pin sites.  BLIT/CLEAR/TEXT filter to the PRIMARY dest
    (dst_handle 1 — retail's observed stream shape).  CLI: `--osr-emit <path>
    [--osr-scenario <name>]`; every sink gates internally (the call_trace discipline).
  - **Proven live (intro-1 nav, 1500 flips):** 316k blits / 173 sheets (0 grab fails) / 18k texts /
    11 fonts / 38 blends / 909 clears; `osr.py` reads it **100% named, 100% dhash/dst coverage**
    (above retail's 89-90%), all 4 anchors at the proven flips (game_enter@1116), both seed pins.
    Port dhash ≠ retail dhash stays expected (pitch is inside the hashed bytes —
    `PORT-DEBT(osr-sheet-dhash-xside)`); `(res,frame)` remains the cross-side join.
  - **NEXT — M6 the tick-join studio:** pair both sides' `.osr` by `sim_tick` (the identity JOIN,
    openrecet E3 — honest gaps, no drift search), then port|retail|diff panels + a diff ribbon in
    osr_view.  The port file already opens in osr_view/recon unchanged — one shared codec.
- **Prior (ckpt 127): M4d — the `--validate` GROUND-TRUTH GATE LANDS and PASSES: 71/71 real retail
  backbuffer snapshots reconstruct `differ_px==0` (boot→title→newgame-menu→prologue→town→dialogue→
  house-freeroam), and the gate's ONE initial failure root-caused + FIXED the USER-flagged menu
  artifact. 1000 host pass (+0 net: +1 snap test); flip-800 before/after montage on the feed.**
  - **OSR_SNAP (the gate).** The proxy Locks the REAL backbuffer at the flip hook (after the closing
    frame's draws, before its PRESENT) and streams a SNAP record (`OSS_OSR_SNAP_EVERY=200` +
    `OSS_OSR_SNAP_FLIPS=…`; draws-bearing frames only — an empty re-present's content is flip-chain-
    rotation-dependent, quirk #99; ~600 KB each, <1% fps cost). The recon's `on_snap` compares its
    accumulated dest at exactly that stream point (RGB565, per-pixel) and dumps `real_/recon_` BMP
    pairs on mismatch. Alpha + GDI text + the house freeroam are now PROVEN pixel-exact against
    retail's actual screen — not just eyeballed.
  - **The menu artifact = the missing scene-transition CLEAR (quirk #105), caught on the gate's first
    run (67/68).** Real flip 800 shows the newgame menu over BLACK: retail zero-fills the compose
    surface via `FUN_005b9410` at scene transitions (+ per-frame at the title) and the menu scene does
    NOT fully redraw — the accumulating recon kept stale title pixels, and the menu dialog's GROW
    animation stacked "onion-ring" borders (never a clip bug). NEW `OSR_CLEAR` record (proxy INT3 at
    `0x5b9410`, filtered to the tracked compose surface — the engine zero-fills offscreen panel sheets
    through the same fn), replayed as an ORDERED draw by the recon AND osr_view's scrub (a clear-only
    frame counts NON-empty). Re-captured (anchors byte-identical) → **71/71 clean**; osr_view renders
    flip 800 identical to retail.
  - ~~NEXT — M5 the port emitter~~ **DONE ckpt 128** (see LATEST above). No known capture gaps
    remain; --validate snaps police future staleness for free.
- **Prior (ckpt 125): TRACE STUDIO v2 — M4 RECONSTRUCT LANDS (the `.osr` → frames, on Windows). The port
  binary's `--osr-replay` mode rebuilds frames 1:1 from a captured draw stream through the port's OWN
  bit-exact sinks, and the capture now records the ALPHA blend descriptor it was missing. 5 commits; 998 host
  pass (+6). USER-CONFIRMED the town reconstruction looks correct.**
  - **M4a — the `.osr` STREAMING reader (`src/osr_replay.{c,h}`, host-tested).** A real capture is 1.5 GB /
    16M blits and the port is 32-bit, so it can't be slurped — it STREAMS records to an `osr_replay_sink`
    visitor. Validated against the REAL capture: a throwaway host harness streamed it and its per-type counts
    match `osr.py` EXACTLY (BLIT/TEXT/SHEET/FONT/BLEND). +6 host tests.
  - **M4b+M4c — the RECONSTRUCTOR (`src/osr_recon.c`, Win32; `--osr-replay <osr> --osr-out <dir>
    [--osr-replay-frames i,j]`).** SHEET → a DDraw source surface (loaded TOP-DOWN — the capture grabbed a
    Lock, not a bottom-up DIB); FONT → an HFONT; BLIT → the matching `zdd.c` primitive onto the dest with the
    source metrics/colorkey/state stamped from the record; TEXT → real GDI TextOutA on the dest DC; PRESENT →
    BMP snapshot. RAN on the real capture: 0 no-sheet / 0 no-font. **Colorkey fix:** the record's `ckey` is
    `colorkey_OUT` (already RGB565), so bind it raw — NOT via `set_color_key` (which re-converts → the
    magenta-leak). **No-clear accumulation:** the reconstructor does NOT clear between frames — retail flips a
    back-buffer chain so an empty re-present frame (quirk #99) retains the prior pixels (prologue 0%→99.7%);
    cleared once at start.
  - **M4-alpha — "capture everything we're missing" (USER): the mode-4 ALPHA blend is now captured + replayed.**
    The `.osr` recorded only the blend MODE, so alpha (~333k blits: prologue, sky/ground, fades) was black.
    NEW `OSR_BLEND` record (mode + 3 channels {shift, mask, exact-sized LUT}) + `osr_blit.blend_ref` (76→80 B);
    proxy `blend_grab.h` grabs the descriptor at the alpha detour. The descriptor is a HEAP object (not a
    global) → captured via VirtualQuery-guarded reads + CONTENT dedup. Re-captured: BLEND=38, 100% of alpha
    blits referenced, ~944 fps. Reconstructor rebuilds a `zdd_blend_desc` → `zdd_blit_orchestrate` (0
    alpha-skipped). Town flip 1250 + prologue 1200 on the feed.
  - **M6 native VIEWER LANDED (ckpt 125) — `tools/osr_view`, a Dear ImGui + DX11 native Windows PE** that
    reconstructs `.osr` frames (DDraw+GDI via the shared `recon_apply` core) and scrubs them INSTANTLY:
    **~1.4 ms/frame (~720 fps), ~0.3 s open**, USER-CONFIRMED. Three perf fixes (commit `5118724`):
    self-contained render (a SotES non-empty frame is a full redraw, 0.0% vs accumulated) + SYSTEM-memory
    surfaces (a video-mem readback Lock STALLED ~274 ms/frame on a GPU sync) + a block-buffered index (was a
    16M-record per-record stdio loop, ~50 s). ImGui from the flake (`pkgs.imgui`/`IMGUI_DIR`); profiler +
    frame-dumper `make -C tools/osr_view prof`. (Replaces the planned Python `:8780` studio.)
  - **HOUSE-FREEROAM BUG FIXED (ckpt 126) — it was the CAPTURE, not recon geometry.** The white "holes" +
    Arche-head fragments were STALE SHEET references: `sheet_grab.h` cached ptr→dhash forever, but the
    engine DESTROYS + REALLOCATES sheet surfaces at a room swap (zdd dtor `0x5b9390`, quirk #104) — a
    recycled pointer recorded the OLD surface's dhash (the holes = a town-era 100%-WHITE dialog-panel
    sheet). Fix: the proxy hooks the dtor and EVICTS `+0x2c`/`+0xac` from the sheet+surfid caches
    (tombstoned); also CLOSED the mode-2 RECTS gap — `osr_blit` grew `srcw/srch` (80→88 B; legacy 80-B
    captures still decode, zero-filled). Recaptured (anchors byte-identical, ~950 fps): flip 6390 clean
    (**USER-CONFIRMED**), 79→7 flagged blits (all benign res-0 labels), prologue/town `differ_px==0` vs
    the old reconstruction. 999 host pass (+1). Writeup: `plans/trace-studio-v2.md` "RESOLVED (ckpt 126)".
  - ~~NEXT — M4d the `--validate` gate~~ **DONE ckpt 127** (see LATEST above): 71/71 snaps clean,
    menu artifact root-caused (quirk #105) + fixed via `OSR_CLEAR`. Plan: `plans/trace-studio-v2.md`.
- **M3c (prior, ckpt 125): the SOURCE pixels + surface identity are captured —
  the draw stream is now self-contained enough to reconstruct frames (M4). NO COM vtable wrap was needed:
  the blit decompiles showed each cel/dest holds a real `IDirectDrawSurface7*` at `+0x2c` (the engine calls
  `dest->Blt(&dr, src, &sr, …)` via vtable +0x14), so the proxy interns those raw surface pointers + grabs
  source pixels straight from the blit detour. Per a fresh nav→town capture: `dst_handle` 100% set
  (1 distinct = the backbuffer), src `dhash` 100% set, **496 SHEETs / 420 distinct (9.4 MiB raw RGB565)**,
  header re-stamped `640×480 RGB565` (was UNKNOWN), all 3 anchors + both seed pins byte-identical to M3b,
  90% named, **912 fps in-game** (<4% vs M3b's 950 — well under the 10% budget, no Lock stall/crash).
  990 host pass (+1). Two commits this step (M3c-fmt SHEET record+test; M3c proxy+reader).**
  - **M3c — surface identity + SHEET (the source-pixel grab).** `src/osr_format.h` grew the variable-length
    `OSR_SHEET` record (24-B prefix `dhash/res/frame/w/h/pitch/pixfmt/codec/byte_len` + raw pixels) +
    `osr_enc_sheet_prefix` so the writer streams big pixel payloads into the ring with ONE copy.
    `tools/capture_proxy/surface_id.h` (NEW) interns surface ptr→stable handle; `sheet_grab.h` (NEW) Locks a
    source surface READONLY on first sighting, FNV-1a's it (mirroring `asset_register.c`'s w/h/bitcount+pixels
    seed order), emits one dedup'd SHEET per surface ptr, caches the dhash; `engine_pixfmt.h` (NEW) classifies
    `DDPIXELFORMAT`→`OSR_PIXFMT_*`. `engine_hooks.h` reads the dest surface (`*(void**)(arg0+0x2c)`) → handle
    + first-one fixes the header, and the src surface (`*(void**)(cel+0x2c)`) → SHEET → BLIT `dhash`.
    `osr_writer.h` re-stamps the header at offset 0 from the bg thread once the first desc lands.
    `osr.py` decodes SHEET (+`SHEETS` dump, dst/dhash coverage in SUMMARY). The captured sheets are coherent:
    640×480 backdrop/scroll layers, a sparkle particle series (22→20→…→6), 18× 160×176 portrait busts, 32×32
    town tiles. **KNOWN follow-ups (NOT bugs):** raw SHEET pixels (miniz deferred → `PORT-DEBT(osr-sheet-compression)`);
    the retail dhash won't byte-match the port's cross-side (native pitch/pixfmt differ → a legit render_diff
    `[decode]` signal, `PORT-DEBT(osr-sheet-dhash-xside)`); the alpha (mode-4) source is a GDI/`paint_ctx`
    blend whose `+0x2c` surface grab is best-effort.
  - **M3b (prior): the native BLIT draw-stream** — the ORDERED 5-primitive blit op list + the render-id
    identity per frame (INT3+VEH + E9-jmp trampoline detours, NO Frida) keyed to the load-stable
    `(resource_id,frame)` `render_diff.py` aligns on, FULL TURBO. Commits `cc63407` + `ee55e5b`/`50ec26b`.
  - **M3a (prior) — `.osr` format + the cheap records.** `src/osr_format.h` codec + `osr_writer.h` (bg-thread
    ring → `C:\` `.osr`) + FRAMEBEG/PRESENT/ANCHOR/SEED records + `osr.py`. PROVEN on a real boot (417 KB,
    11585 frames). Config: `OSS_OSR`/`OSS_OSR_PATH`/`OSS_SCENARIO`; `run_proxy.sh` collects + summarizes.
    Commit `8c42c02`.
- **TRACE STUDIO v2 — M1+M2 (prior, ckpt 125): a fully native, Frida-free capture proxy boots the real
  retail game seed-pinned + lockstep + headless TURBO to `game_enter` with every anchor emitted.
  `tools/capture_proxy/` (proxy `ddraw.dll`, all C/mingw32); ~790 fps in-game turbo (vs v1's ~60fps
  `--no-turbo` cap).**
  1. **M1 — auto-load + forward.** The retail exe imports one DDRAW symbol (`DirectDrawCreateEx`) with a
     FIXED base 0x400000 + relocations stripped, so a proxy `ddraw.dll` dropped next to the exe wins the DLL
     search order — no Frida, no injector. `ddraw_proxy.c` forwards to the real SysWOW64 ddraw.
  2. **M2a — native lifecycle.** IAT-patched turbo/lockstep clock (`clock.h`, port of the agent's
     `installTurboHooks`), env config (`proxy_config.h`), harness thread (`harness.h` — hide window,
     keep-alive, auto-dismiss the launcher dialog). Headless turbo boot to `DirectDrawCreateEx` in ~1 s.
  3. **M2b — the engine-VA detour layer (`va_detour.h` INT3 + a vectored exception handler; NO
     length-disassembler).** `engine_hooks.h` ports the agent's VA map: flip `0x5b8fc0` (+lockstep advance),
     sim-tick `0x43d1d0`, one-shot title seed-pin `0x56c070`→`DAT_008a4f94`, the newgame/prologue/game_enter
     anchors, the per-map RNG re-pin `0x41f200`. `engine_input.h` ports ring injection (hook `0x43c110`,
     fill the input ring) → drives the menu. **Live: newgame@flip652 → prologue@1000 → game_enter@1242
     (RNG re-pin fires) → sim_tick climbs ~1:1 with flips (lockstep engaged in-game).** Launcher:
     `tools/capture_proxy/run_proxy.sh` (deploy → run → collect → ALWAYS clean up ddraw.dll so v1 is safe).
  4. **NAV lesson:** a nav with EXACT flip frames is calibrated to one boot cadence (the agent's); the proxy's
     differs (newgame@652), so the ckpt-122 flip-keyed nav's submenu presses stalled — a cadence-TOLERANT nav
     (presses over windows) reaches game_enter robustly (fine for a boot: game_enter re-pins the seed).
  **M3d — GDI text → TEXT/FONT: LANDED (ckpt 125). The `.osr` is now a COMPLETE frame description.** The one
  remaining draw class — GDI text (the engine `TextOutA`s straight onto the backbuffer DC, outside the 5
  blits — `findings/text-glyph-pipeline.md` / quirk #63) — is captured by IAT-patching the engine's gdi32
  imports (`tools/capture_proxy/engine_gdi.h`, via `iat_hook.h`; an IAT swap is a full wrapper that SEES the
  return value, so `CreateFontIndirectA`'s new HFONT needs no onLeave framework): `TextOutA` → TEXT records
  (seq, dst_handle, x, y, font_ref, color, bk_mode, string), `CreateFontIndirectA` → dedup'd FONT (LOGFONTA),
  `SelectObject`/`SetTextColor`/`SetBkMode` track per-HDC font/colour/bk_mode. TEXT shares the per-frame draw
  `seq` with BLIT (replayer interleaves them) + targets the single backbuffer handle (M3c). `OSR_TEXT`/
  `OSR_FONT` codec records in `src/osr_format.h` (+ round-trip tests, 992 host pass / +2); `osr.py` decodes
  them (+`TEXTS` dump). PROVEN on a fresh nav→game_enter capture: **9 FONT** (Courier New h8..20) + **553k
  TEXT** (font_ref/dst_handle 100% set, 7 distinct colours), all anchors + both seed pins + BLIT/SHEET
  coverage byte-identical to M3c. The decode matches quirk #63 EXACTLY — font ref 3 = Courier New 7×18,
  per-glyph TextOutA at 7px advance, the 3-copy shadow (`0xa8b9cc`/`0xa8b9cc`/main `0x3e537d`), bk
  TRANSPARENT, dst=1.
  **NEXT — M4 reconstruct** (`opensummoners.exe --osr-replay` — rebuild a source surface per SHEET, replay
  each frame's BLITs through `zdd.c` + TEXT through `glyph_render_win32.c`'s real GDI; the `--validate`
  `differ_px==0` gate vs one real snapshot), then **M5** port emitter (`src/osr_emit.c` → same `.osr`), **M6**
  the tick-join studio (`:8780` scrub). Plan: `plans/trace-studio-v2.md`. Two M3c follow-ups stay tagged
  PORT-DEBT (raw SHEET pixels → miniz; cross-side dhash reconciliation) — neither blocks M4.
- **Prior (ckpt 124): the dialogue PORTRAITS are UN-MVP'd + ALIGNED — the bust RESOLVES per speaker AND
  the right face-table VARIANT per line (USER-CONFIRMED correct). `src/portrait.{c,h}` + the embedded
  face table; 982 host pass (+4); commits `ce1af81` (per-speaker) + `1a527cb` (per-line variant). Montages
  on the feed.**
  1. **The bug.** The portrait was hardcoded to slot 663 = a WRONG character (head 100000104); every
     speaker showed the same bust. Now `portrait.c` ports the `0x49d6e0` face-table lookup: speaker
     head-state (`+0x1d8`) + line `face_id` + the per-line VARIANT → the portrait pool-slot.
  2. **The data.** `portrait_face_data.{c,h}` (generated by `tools/extract/portrait_face_table.py`) embeds
     the retail face table `DAT_006b6568` (147 records, 3 variants each). Speaker→head-state HARNESS-CAPTURED
     (`runs/portrait-gt`): Arche=100000101, Father=100000211, Mother=100000212.
  3. **The VARIANT (the alignment fix).** The 3 variants are DIFFERENT busts/SIZES (e.g. Father A=676
     160x176 vs B=683 176x144), picked per line by the speaker's body-facing (`0x49d6e0:143`).  Using one
     column (B) rendered Father L1 as the squished 176x144 — the misaligned overlay the USER flagged.  RE'd
     the per-line variant for all 18 lines (harness-read the resolved `+0x84` off `0x439680`, no lag) →
     baked into `cutscene_line.pvar` (arrival A,B,B,B,B,B,B,B,A,B / house A,A,A,A,A,A,B,A).  Father L1 now
     matches retail (port|retail montage on the feed); **USER-CONFIRMED the portraits look correct.**
     OPEN (deferred, USER): a frame-by-frame trace-studio pass to verify the exact expression/pose per line
     (the ad-hoc comparison frames were not sim-tick aligned — the cross-fade blends).
     `PORT-DEBT(dialogue-portrait-facing)` reduced to: `pvar` is captured data, not yet DERIVED from a live
     (animated) cast facing.
  **Retires PORT-DEBT(dialogue-portrait-per-speaker).** Side-fix on the USER's request; the planned next
  arc is unchanged: **render the errands/house ROOMS** (`plans/controllable-arche-faithful.md` Phase 2a —
  the room-render path; scene ids confirmed house=1023/errands=1025) → then the freeroam hand-off.
- **Prior (ckpt 123): the town-intro CUTSCENE now CHAINS arrival→house — the room-key swap is ported +
  BEHAVIORALLY VERIFIED. `src/cutscene.{c,h}` grew from a single-script driver to a multi-ROOM sequencer;
  978 host pass (+4); commit `daa1f65`. Montage on the feed.**
  1. **The chain.** `cutscene.c` walks a ROOMS list modeling the room-key swap (`0x401d40` stage /
     `0x402030` commit): arrival `0x334be` (10 lines) → house `0x334c8` (8 lines) → ends at the errands
     boundary `0x334dc` (= the freeroam hand-off point). On a room's last line it COMMITs the next room key
     (`room_idx++`) + arms line 0; past the last room, completes. New `cutscene_room` + `cutscene_town_house`/
     `_chain` + `cutscene_room_key`.
  2. **The house script (RE'd, `0x4d7d80` case 0x334c8, lines 1029-1218):** 8 dialogue lines, text VAs
     0x86d390..0x86d1dc, speakers Arche/Mother/Father (actor ids 0x5f5e165/1d4/1d3 → dramatist names,
     confirmed vs the arrival's known speakers); the actor-emote beat `0x401e60` (lines 6-7) skipped
     (`cutscene-beat-runner`).
  3. **VERIFIED.** A seed-pinned replay (extended Z-spam, `runs/cutscene-verify`) advanced the LIVE chain
     through all 18 lines + logged "cutscene chain COMPLETE → errands boundary 0x334dc" @flip 11365; the
     house lines render with correct text + name. KNOWN (tagged debts, NOT bugs): the portrait stays the
     Father bust (`dialogue-portrait-per-speaker`) + the backdrop is still the town scene — **NEW
     `PORT-DEBT(cutscene-room-render)`** (the house map isn't loaded; the room key drives the unported
     `0x585ae0`/`0x586010` map-load path — 14 `+0x4024` consumers).
  **NEXT — the errands room = the freeroam (a SCOPE DECISION pending with the USER, see HANDOFF).** Two
  findings reshape the plan: (a) the errands room `0x334dc` is a flag-gated QUESTLINE (the separate
  dispatcher `0x4dc510`, its own dialogue API `0x4a5ee0`), not a linear cutscene; (b) NEITHER the house
  nor errands ROOM is rendered. The freeroam hand-off (`character_step` on live `axis_held`, the
  `+0x200==0` char-AI path) is only NON-FAKE once the errands room RENDERS (else Arche walks over the town
  backdrop = the ckpt-120 "wrong scene" the USER removed). So the faithful foundation is the **ROOM-RENDER
  path** (port the room-key→map-load/decode so house+errands backdrops render) → THEN the freeroam
  hand-off. Retired the SCRIPT-chain part of `cutscene-scene-chain`; debt + `cutscene-room-render`. Plan:
  `plans/controllable-arche-faithful.md` Phase 2.
- **Prior (ckpt 122): the CONTROL-TRANSFER PATH is HARNESS-VERIFIED — the static-vs-live conflict is
  RESOLVED (both prior readings half-right), the porting model is CORRECTED. Pure RE/harness (no port
  code); 974 host pass (unchanged). quirk #103; artifacts `runs/control-path-gt/` (montage on the feed).**
  The ckpt-121 "DO FIRST: harness-verify" step, done. Seed-pinned `--lockstep --no-turbo` retail drove
  the proven ckpt-112 nav (Z-spam from `game_enter`, extended to flip 8392) under a per-Flip field spec
  (`tools/flow/control_handoff_fields.json`) reading `room_state = *(*(0x8a9b50)+0x2784)`.
  1. **3-ROOM CHAIN, confirmed** — the committed room key `room_state+0x4024` swaps **`0x334be` arrival
     (flip 1430) → `0x334c8` house (3661) → `0x334dc` errands (4270)**, staged by `FUN_00401d40` (@3659/
     @4268), committed by `FUN_00402030`. The static room sequence is REAL.
  2. **LIGHT room-key swap, NOT a full reload** — ONE `game_enter`; `room_state`/leader `+0x200c`
     (`0xd1dcc58`)/entity (`+0x9f4`, code `0xc35a`)/`+0x158a4` hold CONSTANT across both swaps. So
     ckpt-112's "no reload, entities persist" was right; its "same scene" was wrong.
  3. **CONTROL IS `entity+0x200 == 0` (char-AI), NOT `+0x200=1`** (the ckpt-114 polarity open, RESOLVED).
     A held-axis walk in the errands room drove Arche bit-exact (held-RIGHT `wx 19200→73800` facing 1,
     held-LEFT →`14640` facing 3) with `+0x200`==0 + `+0x158a4` non-null throughout — matching `0x46cd70`'s
     dispatch (`+0x200==0` → char AI `0x478ba0`). The `0x41e070`/`0x4c6830` `+0x200=1` setters are a
     LATER/different control point (party / end-of-day), NOT the entry to player movement.
  4. **The errands room `0x334dc` IS the freeroam** — "PLAYER!" marker + HUD on screen (= ckpt-112's
     "PLAYER!@4500", correctly located). USER-confirmed: "a house with mom and dad, run some errands,
     short dialogue at the start." Z-spam STALLS there because it's gameplay — the stall IS the boundary.
  **NEXT (the active arc — PORT the verified chain):** extend `cutscene.c` to chain the 3 room scripts
  (arrival ✓ → house `0x334c8` 8 lines → errands `0x334dc` + its short opening dialogue) via the LIGHT
  room-key swap (`0x401d40` stage → `0x402030` commit; no full reload); at the errands room STOP the
  sequencer and run `character_step` on live `axis_held` (the `+0x200==0` char-AI path — the faithful
  replacement for the ckpt-120 `CHAR_CONTROL_ARM_FRAMES` MVP arm). DROP the `+0x200=1` model for the
  first freeroam. OPEN (don't block): the LATER `+0x200=1` transfer (post-errands → Sana, needs a
  walk-to-trigger nav); is char-AI suppressed during the cutscenes or just un-fed (held-input untested
  there). Plan corrected: `plans/controllable-arche-faithful.md` "Phase 2 VERIFIED". Throwaway specs/runs
  gitignored. Debt unchanged from ckpt 121.
- **Prior (ckpt 121): UN-MVP'd the movement — 3 of 4 steps LANDED (MVP wire REMOVED; FAITHFUL live
  keyboard input + the town-arrival DIALOGUE ADVANCE ported + verified). USER COMMITTED to the FULL
  FAITHFUL control-transfer chain (the big arc). 974 host pass (+11), 0 fail; 4 commits.** The USER's
  un-MVP directive (ckpt-120): remove the throwaway controllable-Arche scaffold, then build the faithful
  base. Steps 1-3 done; step 4 (the real hand-off) is now a multi-session arc (USER-chosen).
  1. **REMOVED the MVP** (`src/main.c`): the measured-trigger live-wire (wrong scene, replay-only,
     static render) is gone; the bit-exact mover (`character.{c,h}`) is UNTOUCHED — its faithful caller
     returns with the real hand-off (step 4).
  2. **FAITHFUL LIVE INPUT** (`src/input_live.{c,h}` — the port of the producer `0x46a880`): real keys
     fill `input_mgr.axis_held[0..6]` (clear-then-set) + post discrete ring EDGES each frame (arrows →
     walk/menu/dash, C → jump, Z → advance id 0x24). A single `main.c feed_input` gates REPLAY (the
     deterministic parity path) vs LIVE (interactive, focus-gated on `g_app_active_flag`) — mutually
     exclusive so live keys never perturb a capture. Host-tested (6); boot smoke clean. **USER
     VISUAL-VERIFY PENDING** (run windowed: navigate the title menu with arrows + Z).
  3. **DIALOGUE ADVANCE** (`src/cutscene.{c,h}` — the reduction of `0x4d7d80` case 0x334be + the
     beat-runner `0x439690`): the 10-line town-arrival family conversation Z-advances (Father→Arche→
     Mother), each line's text+name read from the user's exe by VA (dramatist rows 0/4/5; faces/voices
     RE'd). Z advances only when the line is fully typed (`dialogue_awaiting_advance` = `0x439690:1004`).
     Replay-verified end-to-end (`runs/cutscene-verify`, montage on the feed: f2600 "Ahh…"→f3150 "Yay"→
     f3700 "We haven…"). KNOWN: the portrait stays the Father bust (PORT-DEBT(dialogue-portrait-per-speaker),
     a render detail). Host-tested (5).
  4. **NEXT — the FULL FAITHFUL CONTROL TRANSFER (USER-committed, ckpt 121, the active arc).** The RE
     (two subagent maps of `0x4d7d80`) found the hand-off is NOT a few lines downstream: `FUN_00401d40`
     does a ROOM TRANSITION — arrival (10 lines) → load room **0x334c8** (house interior, 8 lines) →
     load room **0x334dc** (morning errands, a SEPARATE fn `FUN_004dc510`, advances story flag
     `0x5f76805`→0xd2) → back to **town 0x334be flag 0xd2** → Sana-walk-home → `entity+0x200=1` (the real
     `0x41e070`/`0x4c6830` idiom, inlined at `4d7d80.c:449-463`: `+0x200=1; FUN_0041e180(1);
     FUN_0041e280()`). **CONFLICTS with ckpt-112's live "one game_enter, no map reload"** → **DO FIRST:
     harness-verify the live room/control path** (field-spec the scene-controller room key
     `*(0x8a9b50+0x1038)[0]`/map `+0x4024` + Arche's `+0x200` + flag `0x5f76805`, Z-spam retail to the
     hand-off) — the transitions may be same-map (camera-only) or the live path shorter than the static
     chain. THEN port the room-transition system + the intervening rooms + the transfer. Plan +
     subagent findings: `plans/controllable-arche-faithful.md` (rewritten ckpt 121). The control
     MECHANISM is small + clear; reaching retail's exact LOCATION is the arc.
  Debt (new this ckpt): cutscene-beat-runner (camera/timer/action beats stay measured), cutscene-scene-chain
  (downstream rooms unported), dialogue-portrait-per-speaker, keybind-config; carried: char-control-trigger,
  cutscene-party-chars (Arche's static render / no walk-cycle), char-run-trigger, char-walk-tuning,
  char-collision-mover, char-input-autorepeat, char-jump-fall-grav-source, held-axis-array-b,
  effect-color-variant. Butterfly chip-1 drift visual-verify still pending.
- **Prior (ckpt 120): PHASE-4 chip 3c — the LIVE WIRE: Arche is CONTROLLABLE ON SCREEN. `character_step`
  gets its FIRST live caller; held-axis input drives Arche walking in the settled town. `src/main.c` only;
  963 pass (unchanged). USER VISUAL-VERIFY PENDING (pushed to the feed).** The chip-3a/b walk physics
  (host-validated bit-exact) now drive Arche's rendered sprite live — the movement-system MILESTONE.
  1. **The wire (`game_actor_update`, mirroring the butterfly pattern).** Arche is the cutscene-cast
     EFFECT actor (code `0xc35a`, slot 18, bank 0x8b); at a MEASURED control-transfer frame
     (`CHAR_CONTROL_ARM_FRAMES`=200 post-game_enter, PORT-DEBT(char-control-trigger)) `character_step`
     runs once/sim-tick off `g_game_drive.input.axis_held[0..3]` (held_trace-driven; aligns with
     `CHAR_AXIS_*`) + `[4]` (jump C) and its `world_x/world_y/facing` mirror into her render-state.
  2. **VERIFIED (the capture, `runs/livewire`).** Drove the port to the settled town (nav `edit.trace.port.jsonl`,
     game_enter@1116; camera pans to cur_x 12800 by ~flip 2097), then a held-LEFT-then-RIGHT trace
     (`walk2.jsonl`): Arche walks left to Barnard then back right, smooth accel/decel, no glitch.
     Montages pushed to the feed.
  3. **KNOWN DEFERRED (render polish, NOT the mover).** She SLIDES on the idle clip (no walk-cycle) and
     stays RIGHT-FACING when walking left — `facing==3` selects `frame_base+flip_table[0x8b]` and bank
     0x8b has no mirror-frame registered. Both are the **animated-render** debt PORT-DEBT(cutscene-party-chars)
     (the multi-part party-band render `0x4997b0`). `run`=0 (no live double-tap → PORT-DEBT(char-run-trigger));
     no live keyboard producer yet (WM_KEYDOWN no-op) — driven via held_trace replay (the capture path).
  4. **NEXT — the USER has SET ASIDE this MVP (ckpt-120 directive): go FAITHFUL.** The MVP wire proved the
     seams but is a throwaway scaffold (measured trigger + replay-only input + static render); building the
     animation system on it would be annoying to un-MVP. Instead, next session: **the FAITHFUL controllable
     Arche** — the real freeroam scene + LIVE input handling — per the new plan
     **`docs/plans/controllable-arche-faithful.md`**: (1) faithful LIVE input (port the `0x46a880` producer +
     ring → real keyboard fills `axis_held`/ring, alongside the replay) so movement is testable live; (2) the
     REAL control hand-off (dialogue chip 4 → `entity+0x200=1`) — REPLACES the MVP trigger; (3) the animation
     system on the faithful party-band render (retires cutscene-party-chars). The mover itself is DONE
     (bit-exact). **OPEN (USER):** the MVP commit (ckpt 120) stays in history as the seam-proving prototype —
     revertable for a clean slate if wanted; Phase 2 supersedes it regardless. Butterfly chip-1 drift verify
     still pending. Debt: PORT-DEBT(char-control-trigger / char-run-trigger / char-jump-fall-grav-source /
     char-walk-tuning / char-collision-mover / char-input-autorepeat / cutscene-party-chars),
     PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).
- **Prior (ckpt 119): PHASE-4 chip 3b — the jump WINDUP is PORTED + BIT-EXACT (the launch-anticipation
  delay between the jump trigger and the impulse). `src/character.{c,h}` + `test_character.c`; 963 pass
  (+1).** The jump execute enters the airborne state IMMEDIATELY but the body stays STATIONARY for
  exactly **4 sim-ticks** (a visible launch crouch, ~8 flips) before the impulse fires — RE'd from the
  `0x442a70:834-841` case-3 sub-state-0 branch (`counter++; if (4<counter) { vvel:=impulse; sub:=1 }`;
  `0x426f50(body,3)` resets sub+counter on entry) and ground-truthed bit-exact off the EXISTING
  `capjump-ring2` capture (NO fresh capture needed). Writeup: **engine-quirk #102** (the windup bullet).
  1. **The ground truth (the `bstate` field reveals it).** `capjump-ring2`'s `bstate` reads `body+0x38`
     = main | sub<<16; decoding it: flips **4602-4609 = (main 3, sub 0, vvel 0)** = the 4 stationary
     windup ticks, flip **4610 = (main 3, sub 1, vvel −76000)** = the impulse. The earlier `jump_arc.py`
     keyed on `vvel!=0`, so the windup was invisible to the arc extraction (but always in the data).
  2. **The port (`character.c` + `test_character.c`).** Added `jump_sub`/`jump_ctr` (mirror `body+0x3a`/
     `+0x3c`) + the windup branch: on the jump rising edge enter airborne sub-0; count up; on the tick
     the counter exceeds `CHAR_JUMP_WINDUP_THRESH`=4 apply the impulse + advance to sub-1. New
     `test_character_jump_windup` asserts the 4-tick count + the impulse tick bit-exact; the short-hop /
     held-rise arc tests got the windup prefix and still pass bit-exact. The real sub-states 1/2/3
     (transient/rise/fall anim bookkeeping) collapse to the port's vvel-sign branch; the main-state-4
     landing recovery is subsumed by the flat ground clamp.
  3. **NEXT (chip 3c — the milestone):** **The LIVE wire** — the chip-4 freeroam hand-off (dialogue
     chip 4 → the `entity+0x200` control transfer) gives `character_step` its first live caller in
     `game_actor_update` → Arche walks/jumps/dashes on screen + the chip-2 collision mover/probes get a
     live grounded actor → USER visual-verify. The live wire also retires PORT-DEBT(char-run-trigger /
     char-walk-tuning / char-collision-mover). **OPEN (USER):** butterfly chip-1 drift visual-verify
     still pending. Debt: PORT-DEBT(char-run-trigger / char-jump-fall-grav-source / char-walk-tuning /
     char-collision-mover / char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).
- **Prior (ckpt 118): PHASE-4 chip 3b — Arche's DASH (run) is PORTED + FIELD-EXACT, validated
  bit-exact vs a fresh RING-double-tap capture. `src/character.{c,h}` + `test_character.c`; 962 pass
  (+1).** The run is the captured two-phase delta on the walk (same `body+0x28` accumulator, same
  `0x445db0` clamp-ramp) — RE'd from the `0x442a70` case-0x75 run branch + the live const band, then
  validated tick-for-tick. Writeup: **engine-quirk #102** (amended, the run-physics bullet). Artifacts:
  `runs/runjump-gt/capdash2` (the dash capture), `dash-ring2.jsonl`/`dash-held.jsonl`/`dash_fields.json`.
  1. **The CAPTURE (the chip-3b blocker CLEARED — the ring double-tap fires).** `0x479e70` matches two
     direction ring events with `flag==1` in the window, marking them BY SLOT (not ts), so injecting
     `ids:[4,4]` (two id-4 events → slots 63/62, same ts) IS a valid double-tap. Held RIGHT (axis)
     sustains it. `capdash` failed to reach freeroam (a flaky 2-VA/held-hook stall); `capdash2` (lean
     1-VA spec + the PROVEN `freeroam-walk` nav) reached it and the dash fired clean: cmd0 2 (walk)→6
     (run), `hvel` ramps to **48000**, `wx` to **+480/tick**.
  2. **The run LAW (`0x442a70` case 0x75, the cmd[0]=5/6 branch).** Two captured consts differ from the
     walk: **cap `in_ECX[0x5664]`=48000** (dwx ±480 = 2× walk) + a **TWO-PHASE accel** — `in_ECX[0x565d]`
     =**3200** while `|hvel| < 24000` (the walk cap), then the walk accel **1600** up to 48000 (decompile
     `:998` tests `hvel < param_3` before param_3 is reassigned to 48000). Brake stays the walk −800;
     releasing the dash while holding dir decays 48000→24000 at −800 (the `0x445db0` over-cap path).
  3. **The port (`character_step(c, axis, jump_held, run)` — `run`=the resolved cmd[0]==5/6).** The
     accelerate branch picks the run cap + two-phase accel; `test_character_run_ramp` asserts the captured
     `(hvel, worldX)` bytes tick-for-tick (1600,3200, then 6400…22400,25600 at +3200, then 27200…48000 at
     +1600) AND the over-cap decay. The double-tap DETECTION (`0x479e70`) is deferred to the live wire →
     PORT-DEBT(char-run-trigger) (the port replays the held AXIS, not the discrete ring).
  4. **NEXT (chip 3b/3c, in order):** (a) RE + port the ~7-flip jump WINDUP (case-3 sub-state-0 counter>4
     — a launch lag invisible to the arc). (b) **The LIVE wire** — the chip-4 freeroam hand-off gives
     `character_step` its first live caller → Arche walks/jumps/dashes on screen + the chip-2 collision
     mover/probes get a live grounded actor → USER visual-verify (the milestone).
     **OPEN (USER):** butterfly chip-1 drift visual-verify still pending. Debt: PORT-DEBT(char-run-trigger
     / char-jump-variable-height / char-jump-fall-grav-source / char-walk-tuning / char-collision-mover /
     char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).
- **Prior (ckpt 117): PHASE-4 chip 3b — Arche's JUMP is PORTED + FIELD-EXACT, and the move-tuning
  consts are CAPTURED LIVE (resolving an earlier decompile mis-read). `src/character.{c,h}` grew the
  vertical airborne integrator; 961 pass (+3).** The short-hop arc is reproduced BIT-EXACT vs the ckpt-116
  capture; the variable-height high-jump RISE is now ALSO bit-exact validated (a held-C capture). Writeup:
  **engine-quirk #102** (amended, ckpt 117). Artifacts: `runs/runjump-gt/capconsts` + `capband` (the live
  const reads), `capheld` (the held jump), `jump_variable_height.png` (pushed to the feed).
  1. **The port (`character.{c,h}` + `test_character.c` ×2):** `character_step(c, axis, jump_held)` now
     runs the vertical integrator alongside the walk — launch impulse on the jump rising edge, then
     `worldY += vvel/100` with ASYMMETRIC, VARIABLE-HEIGHT gravity + a flat ground clamp (the open-air
     reduction of `0x442a70` case 3 + the vertical mover `0x54e5c0`). The 27-tick short-hop arc asserts
     bit-exact vs retail's captured bytes.
  2. **The consts CAPTURED off Arche's entity (the rigor step — `in_ECX[idx]` = entity byte `idx*4`):**
     impulse `[0x5667]`=**−80000**, rise grav HELD `[0x5668]`=**2000** (floaty high jump), rise grav FREE
     `[0x5669]`=**8000** (short hop), and the walk cap/accel/brake `[0x565b/c/e]`=**24000/1600/−800** —
     **confirming the ckpt-115 walk-tuning hypothesis**. The fall grav (**4000**, arc-pinned) is NOT in
     the move-tuning band → a global/derived gravity (4000 = 8000/2). **METHOD:** the 12 KB shared
     integrator `0x442a70`'s decompile reuses vars across vertical/horizontal terms — an earlier
     line-by-line read mis-mapped `[0x565b]/[0x565e]` to fall-grav/terminal (they're the WALK cap/brake).
     RE the structure, PIN the values + provenance with a live capture.
  3. **Both jump heights validated bit-exact:** the SHORT HOP (ring tap → `cmd[2]==0` → free grav 8000,
     full arc) and the HELD high-jump RISE (held-C → `cmd[2]=8` → grav 2000, 16 rise ticks, apex 2.2×
     higher). The held apex clamps on a **town CEILING** (wy≈41600 — a collision concern, not jump
     physics) and a **terminal fall velocity 64000** was found + ported.
  4. **NEXT (chip 3b/3c, in order):** (a) **Capture + port the DASH (run)** — a direction double-tap +
     hold → cmd[0]=5/6, the run cap (run accel `[0x565d]`=3200 already captured). (b) RE + port the
     ~7-flip jump WINDUP. (c) **The LIVE wire** — the chip-4 freeroam hand-off gives `character_step` its
     first live caller → Arche walks/jumps on screen → USER visual-verify.
     **OPEN (USER):** butterfly chip-1 drift visual-verify still pending. Debt: PORT-DEBT(char-run /
     char-jump-variable-height / char-jump-fall-grav-source / char-walk-tuning / char-collision-mover /
     char-input-autorepeat), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).
- **Prior (ckpt 116): PHASE-4 chip 3b — the run/jump BLOCKER is RESOLVED and Arche's JUMP is captured
  bit-exact in the TOWN. Pure ground-truth (no port code yet). 958 pass (unchanged).** The ckpt-115
  "dash/jump need a platforming/dungeon scene" hypothesis is **REFUTED**: jump/dash are sourced from
  the discrete EVENT RING, and the chip-3b captures only injected the HELD-AXIS — a harness gap, not a
  scene gate. Writeup: **engine-quirk #102** (amended). Artifacts: `runs/runjump-gt/capjump-ring2`
  (`jump_arc.py` + `jump_arc.png`, pushed to the feed); the jump-via-ring trace `jump-ring.jsonl`.
  1. **The RE (decompile-decisive).** The apply `0x442a70` executes the jump on **`cmd[2]==7`** (the
     execute), NOT the held-array **`cmd[2]==8`** (the hold-to-rise marker, consumed nowhere). `cmd[2]=7`
     comes from `0x478ba0:287` matching a discrete ring event via `0x479960(now,0,800,1,7,…)` — it scans
     the ring `input-mgr+0xc` (64×`{id,ts,flag}`) for `id==7, flag==1` in an 800 ms window. The
     chip-3b captures pressed C only through the held leaf `0x5ba520` (→ `+0x124` → `cmd[2]=8`), so the
     ring jump (id 7) was never posted → no execute. **Dash (cmd[0]=5/6) is the same gap** (`0x479e70`
     direction double-tap in the ring).
  2. **Empirically confirmed (the harness, not a guess).** Injecting `ids:[7]` (one ring press, the
     SAME channel as the Z-advance id `0x24`, ZERO harness changes) at a settled town freeroam frame
     makes Arche jump — a clean, deterministic parabola from grounded rest, **two byte-identical jumps**.
  3. **The jump arc (the bit-exact port target, per sim-tick):** vvel impulse **−80000**, then `wy +=
     vvel/100`; gravity is **ASYMMETRIC** — rise decel **+8000/tick**, fall accel **+4000/tick** (a
     floaty fall ≈ Arche's reputation; ~27 ticks airtime, apex `wy 52000→47200` = rise **4800**),
     ground-clamp `wy=52000` zeroes vvel.
  4. **NEXT (chip 3b/3c, in order):** (a) **PORT the jump** — extend `character.{c,h}` with `world_y/vvel`
     + the airborne integrator, host-tested vs the captured arc (like chip 3a). RE the apex/fall branch
     in the body+0x38==3 sub-FSM (`0x442a70:832-877`, the `-20000` threshold) + read the consts
     `in_ECX[0x5667/0x565b/0x565e]` and the variable-height hold (cmd[2]=8) so it's RE'd, not curve-fit.
     (b) **Capture + port the DASH** (inject 2 direction ring presses + hold → cmd[0]=5/6, the run cap).
     (c) **The LIVE wire** — the chip-4 freeroam hand-off gives `character_step` its first live caller →
     Arche walks/jumps on screen → USER visual-verify. **OPEN (USER):** butterfly chip-1 drift
     visual-verify still pending. Debt: PORT-DEBT(char-run-jump / char-input-autorepeat / char-walk-tuning
     / char-collision-mover), PORT-DEBT(held-axis-array-b), PORT-DEBT(effect-color-variant).
- **Prior (ckpt 115): PHASE-4 chip 3a — Arche's freeroam WALK is PORTED + FIELD-EXACT (host-tested
  vs the ground-truth capture). New `src/character.{c,h}`; 958 pass (+4).** The reduction of the AI
  `0x478ba0` (held-axis → command) + the `0x442a70` case-0x75 walk integrator, fit BIT-EXACT to Arche's
  real-body per-tick worldX (`runs/mover-caller`, ckpt 114). Mirrors butterfly chip 1 (a field-exact
  open-air reduction, host-tested; the LIVE payoff awaits the freeroam hand-off, chip 4 — the port
  can't reach freeroam yet).
  1. **The WALK law (the embedded host-test target = RETAIL's captured bytes, `test_character.c`):**
     velocity accumulator `body+0x28` (NOT `body+0x18` = the 0-the-whole-walk VERTICAL vel) ramps via
     the 150-B clamp-ramp `0x445db0`: **accel +1600/tick → cap 24000** (dwx = vel/100 ramps +16/tick:
     16,32,…,240), **brake −800/tick → 0** on release (dwx −8: 240,232,…,8,0); facing `body+0x2c` holds
     through the brake-to-stop and flips 1↔3 only at **v==0** when the opposite dir is commanded. worldX
     += vel/100 (a flat reduction of the collision mover `0x54db10`). Confirmed: the real body `0xe637b80`,
     committed by `0x485fc0+0x96e`→`0x442a70`, tracks `arche_body.wx` tick-for-tick (19200→44280 then
     LEFT mirror).
  2. **`src/character.{c,h}` (NEW) + `test_character.c` (4 tests, field-exact):** `character_step(c,
     axis_held[4])` runs the AI reduction (held L/R → latched walk dir, the `0x478ba0` reduction; UP/DOWN
     `cmd[3]` + run `5/6` + jump read but deferred) then the apply reduction (the velocity ramp + facing
     flip + commit). Accel-ramp, cap-sustain, brake-to-stop, and the LEFT-from-rest symmetry all assert
     against the embedded captured worldX sequences.
  3. **Annotated** `0x478ba0` (the durable `char_ai` entry: held axis U/D/L/R + cmd[0] + speed-mode +
     facing) in `retail_fields.json` — compounds the next flow_diff on the freeroam mover (no live port
     CALL_TRACE mirror yet; that lands with the chip-4 hand-off). Reduction tagged 4 PORT-DEBTs:
     PORT-DEBT(char-run-jump), (char-input-autorepeat) (the press→latch warmup constant = the wall-clock
     auto-repeat), (char-walk-tuning) (accel/cap/brake = the captured consts; real source `in_ECX[0x565b/c/e]`),
     (char-collision-mover) (the flat worldX commit + the untested reverse-decel rate).
  4. **NEXT (chip 3):** (a) **chip-3b INPUT RE'd + the full MOVESET recorded (ckpt 115, quirk #102), but
     run/jump MOTION won't fire in the town freeroam (4 captures).** Live scancodes, USER-confirmed roles:
     **C `0x2e` = JUMP** (→`+0x124`, cmd`[2]=8`, jump-buffered), **X `0x2d` = ATTACK/interact** (→`+0x128`,
     cmd`[4]=0xe`), Z = sword sheathe; **dash = direction DOUBLE-TAP, hold the 2nd** (cmd`[0]`=5/6 via the
     ring detector `0x479e70`, window `*0x8a6e80+0xf8`). The captures also INDEPENDENTLY re-confirmed the
     chip-3a walk port byte-for-byte (`hvel`=`body+0x28`=1600..24000). **THE BLOCKER:** every run/jump
     COMMAND gets set (cmd`[2]=8` on C) but produces **NO motion** — `wy`/`vvel` stay flat even on a clean
     C tap from idle/grounded, cmd`[0]` stays 2 on the dash ⇒ the integrator gates dash/jump on a
     precondition the **flat town/inn cutaway doesn't satisfy** (likely **dash/jump need a DUNGEON /
     platforming scene**). **NEXT:** capture in a platforming area, OR RE the integrator's jump/dash gate —
     don't guess-port unobservable motion. (b) The LIVE wire: the chip-4 freeroam hand-off gives
     `character_step` its first live caller → Arche walks on screen + the chip-2 collision mover/probes get
     a live grounded actor → USER visual-verify. **OPEN (USER):**
     butterfly chip-1 drift visual-verify still pending. Debt unchanged: PORT-DEBT(held-axis-array-b),
     PORT-DEBT(effect-color-variant).
- **Prior (ckpt 114): PHASE-4 chip 3 — Arche's FREEROAM MOVER is PINNED + the input→position
  architecture is fully RE'd. Pure ground-truth, no port code. 954 pass (unchanged).** Method:
  with the ckpt-113 held-axis harness driving the walk, `--call-trace` the integrator `0x442a70`
  over the walk window with arg/this field reads (`tools/flow/freeroam_mover_fields.json`) and
  filter to Arche (`in_ECX+0x1d4==0xc35a`; in_ECX IS the ~90 KB entity). Artifacts:
  `runs/mover-caller/` (`find_mover.py`). Writeup: **engine-quirk #101 final bullet**.
  1. **The architecture is TWO LAYERS, both SHARED with the actor system — mirrors the butterfly
     exactly, but the CHARACTER band + the FULL integrator.** AI `FUN_00478ba0` (held-axis →
     command block) + APPLY `FUN_00485fc0+0x96e`→`FUN_00442a70` (command → in-place position).
  2. **AI `0x478ba0`** (the RNG-free character update townsfolk share; counterpart of the
     butterfly's `0x47b990`): reads the held axis at `*(entity+0x158a4)+0x114/118/11c/120`
     (U/D/L/R, quirk #41 confirmed) → builds the **command block `entity+0x14854`**: LEFT→`[0]`=1
     (walk)/5 (run), RIGHT→2/6, DOWN→`[3]`=10, UP→`[3]`=0xb (walk/run via speed-mode
     `entity+0x158a0`). Also a 4-step collision LOOK-AHEAD into STACK scratch (the "shadow" calls).
     Explicit `0xc35a` special-casing. HELD walking reads the axis flag directly (no GetTickCount
     repeat dependence = a determinism win).
  3. **APPLY `0x485fc0+0x96e`** (the SAME pass that integrates the butterflies, `0x46cd70:71`):
     `FUN_00442a70(in_ECX+0x5215=cmd +0x14854, body, body, 0,0)` in-place on the REAL body — the
     ONLY committer of Arche's position (244 calls; `body_wx==new_wx`, tracking the walk
     **19200→40800→44280→24120**, facing 1=right→3=left). vel `body+0x18`=0 ⇒ direct position
     write; the per-tick step accelerates +16/tick → ~+240 cap. **Reconciles quirk #101 finding
     #3** (the `0x405e80`/… candidate guesses are superseded; `0x46cd70` DOES reach her as a band
     actor, just never via the party array).
  4. **Invocation site RESOLVED (static):** the `0x46cd70` band walk, `0x1160` band — pass 1
     dispatches on `entity+0x200` (`==0`→`0x478ba0`, `==1`→`0x47b990`), pass 2 calls `0x485fc0`. So
     Arche is an ordinary band actor (the port already mirrors this band order). **NEXT (chip 3):**
     (a) RE the run/jump scancodes (`0x8a6e80` keybind defaults) → capture walk/run/jump per-tick =
     bit-exact target; (b) PORT the `0x478ba0` AI + the FULL `0x442a70` integrator (chip-2 collision
     mover/probes get their first LIVE caller). **OPEN (USER):**
     butterfly chip-1 drift visual-verify still pending. Debt unchanged: PORT-DEBT(held-axis-array-b),
     PORT-DEBT(effect-color-variant).
- **Prior (ckpt 113): PHASE-4 chip 3 — the HELD-AXIS INJECTION HARNESS landed + LIVE-VALIDATED:
  Arche WALKS in retail freeroam. The chip-3 blocker (quirk #101 finding #4) is CLOSED. 954 pass
  (+8).** The freeroam mover reads the held-axis array (`input-mgr+0x114`), NOT the event ring, so
  `--input-trace` left Arche idle. RE'd the producer `0x46a880` (fills `+0x114` each frame from the
  DInput buffer via the leaf query `0x5ba520`; slots `[0]=UP [1]=DOWN [2]=LEFT [3]=RIGHT` —
  **quirk #41 CONFIRMED + the "[0]=V/[1]=H" guess corrected**).
  1. **Retail (`tools/frida/opensummoners-agent.js` + `frida_capture.py --held-trace`):** hook the
     LEAF query `0x5ba520`, force its return to pressed for injected scancodes → the real producer
     fills `+0x114` exactly as a physical keypress would. Hooking the leaf (vs writing the array) is
     decisive: it needs NO model of the engine's per-frame clear/produce path (release "just works")
     + survives the hidden window's loss of DInput focus. Read-back diagnostic hooks `0x46a880`
     (`{kind:'axis'}`). `mem_watch` gets `--held-trace` too (for pinning the mover next).
  2. **Port (`src/held_trace.{c,h}` NEW + host-tested ×8):** the LEVEL counterpart of `input_trace` —
     `{"frame":N,"keys":[scancode|name]}` rebuilds `mgr->axis_held[0..3]` each frame (clear-then-set).
     `main.c` wires `--held-trace` at the 4 drive replay sites; the title menu already consumes
     `axis_held[0/1]`. Composes with `--input-trace` (ring Z-advance + held walk together).
  3. **LIVE-VALIDATED (`runs/freeroam-walk`, the contrast vs the ckpt-112 ring run):** held RIGHT
     4560-4760 → `arche_body.wx` **19200→41760** (facing 1); held LEFT 4820-5020 → wx **45472→25320**
     (facing flips 1→3); the walk anim cycles + decelerates to a stop — vs the ring run's **static
     wx=19200**. The `axis` read-back shows `R=1`/`L=1` in the freeroam array. Walk montage pushed to
     the feed.
  4. **NEXT (chip-3 ground-truth, NOW UNBLOCKED):** (a) her `wx` WRITERS are PINNED (`runs/mover-pin`,
     `--hw` watchpoint): `0x442a70+0x2f` (the butterfly-known kinematic COMMIT — copies a precomputed
     `param_2` state into the body; 90%) + `0x54ded0+0x5da` (tile-grid collision clamp, 10%). vel=0 ⇒
     position-based, so the MOVER is `0x442a70`'s **caller** — DO NEXT: call-trace `0x442a70`'s `ret_va`
     over the walk → the party-leader caller that reads the held-axis. (b) capture walk/run/jump
     per-tick → bit-exact target → THEN
     port (party-leader update + the chip-2 mover/probes get their first LIVE caller). **USER-CONFIRMED:
     the trace is set up correctly + Arche walks right↔left as described.** Butterfly chip-1 drift verify
     still pending;
     incidental PORT-DEBT(effect-color-variant), PORT-DEBT(held-axis-array-b) (the port replay models
     array A only; array B at +0x140 deferred until a consumer needs it).
- **Prior (ckpt 112): PHASE-4 chip 3 GROUND-TRUTH (USER chose "ground-truth freeroam first") —
  the HOUSE FREEROAM is REACHED in retail + four chip-3-reshaping facts pinned. NO port code (pure
  RE). 946 pass.** Method: drive retail past the whole town-arrival cutscene (`--seed-pin --lockstep
  --no-turbo`), Z-spam (ring id `0x24`) every ~12 flips from game_enter; the ~15 dialogue beats clear
  and control transfers INSIDE the inn (the **"PLAYER!" prompt + HUD at flip 4500 / sim-tick 1556**,
  pushed to the feed). Detail: **engine-quirk #101**; artifacts `runs/freeroam-gt/` + throwaway specs
  `tools/flow/freeroam_{handoff,arche}_fields.json`.
  1. **Freeroam needs NO map reload** (one `game_enter`; the inn interior is the same cutaway scene)
     ⇒ the USER's "house freeroam" target is reachable by advancing dialogue.
  2. **The party leader `room_state+0x200c` is PERSISTENT (Arche since new-game), NOT cutscene-set** —
     a per-Flip chain `*(*(0x8a9b50+0x2784)+0x200c)` = a constant slot (entity code `0xc35a`) the whole
     town visit; the transfer flips a **per-actor controllable flag** (`entity+0x200=1` via the setters
     `0x41e070`/`0x4c6830`), not the leader.
  3. **Arche's freeroam mover is NOT `0x47b990`** (in freeroam it fired only for `0xc3dc`/`0xc440` =
     Father/Mother; `0x46cd70` never touches the party) — a SEPARATE party-leader path (corrects the
     chip-3 plan; the `0xc35a` case in `0x47b990` is the CUTSCENE-actor behaviour).
  4. **Freeroam movement reads the HELD-AXIS array `input-mgr+0x114` (quirk #41), NOT the event ring** —
     Z-advance worked, but injecting dir ids 3/4 every flip left Arche fully idle (only the idle anim
     cycling). ⇒ the harness needs a **held-axis injection** mode to drive/validate the walk.
  5. **NEXT:** (a) add held-axis injection to the harness; (b) pin Arche's freeroam mover (`mem_watch`
     her worldX writer once walking); (c) capture walk/run/jump per-tick → bit-exact target → THEN port.
     **OPEN (USER):** verify the freeroam frames on the feed; the butterfly chip-1 drift verify still
     pending. Incidental: PORT-DEBT(effect-color-variant).
- **Prior (ckpt 111): PHASE-4 chip 2 (TILE COLLISION) — the READ-SIDE CORE is PORTED + host-tested.
  946 pass (+6).** The chip RE-SCOPED on a discovery: the town's collision GRID is **already built**
  by `map_decode.c` (the `0x587e00` town arms deposit region B class/slope + C slope-type + D flag
  per cell, on the proven-1:1 render path) — "port the grid" was moot.
  1. **`src/collision.{c,h}` (NEW) — `collision_move_vertical` = `FUN_0054e990`**, the VERTICAL
     tile-grid mover (gravity/ground/ceiling clamp). FAITHFUL + PURE over (grid, body box, delta)
     — its `in_ECX` is the GRID, not the actor. Sweeps world-Y in ≤100 steps, scans the X-extent
     vs region-B class (10=wall, 1=slope-surface w/ the verbatim `d==0` contact predicate), clamps.
     Slopes via a caller callback (region-B `+0x8` = exe VA `0x5cc410`/`0x5cc430`) — town flat so
     unused → PORT-DEBT(collision-slopes).
  2. **`map_grid` read accessors** (`map_grid_obj_*`/`map_grid_flag`, region B `0x140030` / D
     `0x2c1040`; `idx=col*0x80+row`) + **`test_collision.c` (6 tests, exact clamps hand-derived):**
     drop→floor (15000), open-air (20000), off-axis wall, ceiling (9600), zero-delta, slope-cb.
  3. **DEFERRED → chip 3** (need a live grounded actor; probes are actor-entangled; user accepted
     "host-tested only, live payoff at Arche"): the AI probes `0x441ae0`/`0x47dbb0`; the integrator
     generalization (butterfly apply STAYS the field-exact open-air reduction); `0x4412d0`/`0x440e40`
     = ENTITY-vs-entity (corrects the plan's "tile grid" label).
  4. **NEXT — chip 3 (controllable Arche):** party band `0x4997b0` + DirectInput → the `0xc35a` case
     → walk/run/jump, giving the mover+probes their first LIVE caller. **OPEN (USER):** butterfly
     chip-1 drift visual-verify pushed (trace-studio `intro-1` ~frame 1580-1670 + feed montage) —
     field-exact per `compare.py`, not yet USER-confirmed on screen. Incidental:
     PORT-DEBT(effect-color-variant) (the INN townsgirl renders the wrong colour variant).
- **Prior (ckpt 110): PHASE-4 chip 1 LANDED — the butterfly OPEN-AIR PATROL MOTION is PORTED +
  FIELD-EXACT. The 4 butterflies now drift left↔right 1:1 (heading/facing field-exact); the RNG
  stream stays bit-exact. (940 pass, +1.)** `src/butterfly.{c,h}` grew from the ckpt-98 RNG-consumer
  stub to the full open-air FSM; `main.c` apply-wires it into the rendered EFFECT actors. Validated
  `runs/butterfly-fsm/compare.py` (port `--call-trace` vs the ckpt-109 capture, SIM-TICK axis).
  1. **Ported — the open-air REDUCTION of `0x47b990`/`0x43f880`/`0x485fc0`→`0x442a70`.** (a) bounds
     at register (`b1=spawn_wx+11200`, `b3=spawn_wx−8000`); (b) the `0xe29a` HEADING FSM — the 2
     RNG draws/tick moved into their real use (decrement cooldown `0x14248`; FLIP heading `0x14244`
     toward the far bound when `cd==0` AND `(|wx−bound|<0xc81` OR the 10% roll); the `0x47dbb0`
     collision term omitted = open-air clear; **draw count/order UNCHANGED**); (c) the horizontal
     APPLY integrator (`hvel` ramps ±10/tick→±100, **step-before-ramp** = the capture's form),
     run EVERY tick (both gate phases, mirroring `0x485fc0`) → the glide.
  2. **VALIDATION — field-exact on the sim-tick axis vs `runs/butterfly-fsm`.** HEADING: **0
     mismatches** — every flip tick matches retail for all 4 butterflies (bf0 [35,85,155,199,243],
     bf2 [3,47,101,149,189,269], …) ⇒ the LCG is **byte-aligned through tick 269** (exceeds ckpt-99's
     0-248). FACING: ≤1/286. worldX: field-exact BETWEEN reversals (bf0 exact to t37, bf3 to t51);
     the ONLY residual is a **≤170-unit ≈ ≤2px BOUNDED transient at turn-arounds** (non-accumulating
     — the exact flips phase-lock it). Host `butterfly_motion` (NEW) + `butterfly_pertick` (UNCHANGED
     ⇒ the RNG model intact). Durable annotation: `0x47b990` in `retail_fields.json`.
  3. **DEFERRED (PORT-DEBT).** (a) `butterfly-flutter` — the VERTICAL flutter sawtooth (`body+0x18`
     vel + the `cmd_2` flap sub-FSM → worldY bob) + the flap/reversal coupling (= the ≤2px residual);
     worldY holds the spawn value (butterflies glide flat, no bob). (b) `butterfly-bounds-writer` —
     the +11200/−8000 spawn derivation un-RE'd (values bit-exact for the town). (c) per-instance
     frame_base multicolor (the white variant). All retire with the full integrator (chip 2/3).
  4. **NEXT (chip 2 — TILE COLLISION).** Port the grid (`0x2c1030`/`0x2c1040`) + the swept probe
     `0x4412d0`/`0x440e40` + the directional probes `0x441ae0`/`0x47dbb0` → grounded actors stop
     clipping terrain (prereq for Arche, chip 3). Deferred intro chips unchanged (fountain-anchor 2×,
     sky-anchor, R8 grade). **USER VISUAL-VERIFY PENDING: a fresh butterfly trace-studio session.**
- **Prior (ckpt 109): PHASE-4 chip 0 — the butterfly FSM GROUND-TRUTHED per-tick** (`runs/butterfly-fsm/`,
  seed-pinned+lockstep, `game_enter@1434`, sim-ticks 0..285 × 4 butterflies; spec
  `tools/flow/butterfly_capture_fields.json`). Resolved both plan opens as bycatch: the apply = the
  band's 2nd EFFECT pass `0x485fc0`→`0x442a70` (every tick); the bounds = `spawn_wx ± 11200/−8000`
  (dead-constant). The model the chip-1 port (LANDED above) consumed. (939 pass.)
- **Prior (ckpt 108): the PHASE-4 MOVEMENT SYSTEM arc OPENED** (USER-chosen over the smaller
  intro-polish chips) — the butterfly drift (@1177/@1627) is the full ENTITY MOVEMENT FSM, the
  same code that drives controllable Arche + freeroam. Architecture mapped + plan written
  (`plans/movement-system.md`); ckpt 109 then ground-truthed it (above). (939 pass.)
- **Prior (ckpt 107): R7 FOUNTAIN spray RESOLVED + USER-CONFIRMED 1:1 — the water is BIT-EXACT
  (upper spray `differ_px==0`); fountain-box differ `4286 → 305` at stamp-equal t30, and 100% of
  the 305 is the butterflies (separate subsystem). Finishes a fable session cut short by a Windows
  update + the USER-caught fade. (939 pass.)** USER: "can confirm fountain is 1:1."
  1. **Anchor (the bulk).** New `0x557370` field-spec (`runs/r7-anchor-retail`) reads the
     fountain prop's box `+0xc/+0x10` = (6400,6400) + world (176000,41600) — byte-identical to
     the PORT's prop world. A port|retail water-droplet blit match (t30, 27 droplets each, same
     cel/camera) pins the EMPIRICAL anchor at **+1600 both axes** (`FOUNTAIN_EMIT_{X,Y}_OFF`,
     was X=+1245/no-Y → the spray sat high-left). **OPEN 2×:** the decompile (`0x557370:58`)
     reads `+0xc/2` = +3200 but the rendered spawn matches at +1600 = `+0xc/4` (root cause
     un-RE'd — `0x557550`/`0x426620` 2nd halving, or doubled `+0xc`).
  2. **Band-order one-tick lead (decompile-verified, `46cd70.c:103-169`).** `0x46cd70` steps the
     PARTICLE band `0x13e0` (`0x46e510`) BEFORE the CHARACTER band `0x11e0` (`0x54f980` emit), so
     a fresh droplet renders UNSTEPPED one tick; `particle_pool_step` moved before the emitters
     (RNG-free → stream intact). The 3-way velocity CYCLE was already correct (init 0 → `(k+1)%3`,
     case-for-case identical, aligned from tick 0 — `runs/r7-vel-retail`/`r7-anchor-retail`).
  3. **Fade — the FRESH-droplet blend (USER-caught, RESOLVED).** The sub_phase-0 droplets blew
     out white (pink-white) where retail is dim/transparent. `0x557550` case `0x18708` sets the
     fresh alpha via `FUN_004385c0(DAT_008a9330)` = **ramp_b[10]** (group B, mode 0 NORMAL); the
     step `0x46e510` only switches to `ramp_a[10−sub_phase]` (mode 1 ADD) from sub_phase 1. The
     port drew ALL water via ramp_a → the 3 overlapping spawn droplets ADD-accumulated to white.
     Fix: `particle_pool_render` renders sub_phase 0 via `ramp_b[10]`. Upper-spray differ 154→0,
     box 560→305. (Positions/count/timing were already 1:1; `fountain-collide` is a latent debt,
     NOT observable here — the "13 vs 6" was a 2-flip doubling artifact.)
  4. **Annotations** (the openrecet "annotate as you RE" step): `0x557370` (particle_spawn +
     anchor), `0x453960` (velocity_scatter), `0x5bd550` (blt_alpha) in `retail_fields.json`.
  5. **Remaining ~305 px = the 2 BUTTERFLIES only — PORT-DEBT(butterfly-wander).** res `0x3fa`,
     KEYED blit `0x5b9b70`, frozen at (484,320)/(532,320) in the box (movement FSM `0x43f880`
     unported) while retail's drift. A separate subsystem — the fountain water itself is done.
  6. **NEXT:** (a) `butterfly-wander` — the movement FSM `0x43f880` (clears the last box residual);
     (b) resolve the anchor 2× (RE `0x557550`/`0x426620`); (c) `sky-anchor` — the spec read the
     sky prop box=3200 (≠ quirk-#88 "+0xc==0"), validate +1600 at the settled town; (d) the
     ckpt-106 R8 typewriter grade + dialogue chip 4.
- **Prior (ckpt 106): R6 establishing-REVEAL RESOLVED — the frontier band is
  `differ_px==0` at EVERY stamp-equal tick (2..32); the residual was TWO stacked causes,
  each hiding the other's fix. R7 narrowed to the fountain ONLY. (939 pass.)**
  1. **Cause 1 — graded mask cels:** retail binds res `0x458` (alpha mask ramp) + `0x583`
     (opaque cel) via the PLAIN getter `0x4184a0(0)` — UNGRADED (quirk-#96 family).  The
     port graded them → masks one 5-bit step weak.  Slots 40/41 → the grade skip-list.
  2. **Cause 2 — the ckpt-105b fence was a misfix:** mask-level extraction (per-pixel
     `s5 = backdrop5 − out5`, mode per cell) proves retail's frame stamped tick u presents
     the POST-update-u grid (`s5(a)==a` exactly, 11 indexes); the fenced port presented
     u−1.  The 105b dt-scan that justified the fence ran over GRADED cels — the content
     error biased it one tick.  Fence removed (`scene_fade_step` unfenced every sim tick).
  3. **The grid model itself was always bit-exact:** new `0x499ab0` grid-dump annotation
     (`r40..r80` chain fields + port mirror in main.c) — 41 rows × 31 ticks, ZERO
     mismatches (`runs/r6-grid`, the staircase `timer=100u+50−50d`).  quirk #100;
     parity-ledger R6 has the method writeup (state-equality proof → cel-content
     extraction — reusable).
  4. **Bycatch:** grid+0x20/24/28 = an overlay AUDIO-fade ramp (town: 0→1000 at +10/tick,
     ducking ~12 DSound position updaters via `0x5bb870/80/90` — vol/pan/freq thunks),
     NOT video (quirk #100) — matters when town sound lands.  R7 NARROWED: at t30 100% of
     the whole-frame residual = the fountain box (smoke 0; retail half of the dual blit
     trace already captured, `runs/r7-blits-retail`).  Tooling: studio port-driver
     stdout must be a PIPE (WSL interop vsock fails exec when child stdout is a file).
  5. **NEXT:** (a) R7 — finish the dual blit trace (port half) → per-particle attribution;
     (b) R8 typewriter row-close grade; (c) dialogue chip 4 (Z-advance + script table);
     (d) probe-flag removal; (e) a deeper working trace (past dialogue → house freeroam).
- **Prior (ckpt 105): the SIM-TICK AXIS lands in the studio — the whole intro-1 worklist
  attributed at forced tick-equality; 3 trigger constants recalibrated (banner/pan/dialogue
  now `differ_px==0` at tick-equal); ckpt-104's @2463 "zero-mean" read RETRACTED. (939 pass.)**
  1. **Instrumentation:** the port stamps its easer-call count into `--capture-all` names
     (`port_frame_<flip>_t<tick>.bmp`, mirroring the retail agent's `0x43d1d0` count;
     `g_sim_tick_count` in main.c); the studio pairs/carries BOTH ticks (state.jsonl
     `port.sim_tick`, viewer red on mismatch, worklist rows print both). **Chase marks at
     FORCED tick-equality, never the pairing's drift** (the timestep-determinism rule, now
     tooled — `docs/trace-studio.md`).
  2. **The worklist attributed:** @1177 NPC-anim = differ 0 at tick-equal (pairing phase —
     CLOSED); @218 lizsoft fade = R3 boot stretching, logo differ 0 at matched fade (CLOSED);
     @2159 banner fade-out = NOT noise, a trigger offset (CLOSED by recalibration); @1122
     reveal = REAL ~1-tick frontier lead (R6 OPEN); @1177 fountain = REAL tick-independent
     ensemble offset (R7 OPEN); @2463 text reveal = REAL row-close pause-grade mismatch (R8
     OPEN — the ckpt-104 "zero-mean phase" verdict was a flip-axis pairing artifact).
  3. **Trigger recalibration (quirk #99 — flip-axis constants absorb retail's coalesced
     ticks):** banner arm 78→**82** (first alpha step t42; both fade edges differ_px==0 at
     tick-equal — the 2.5-tick alpha-ramp plateaus had hidden a 2-tick error from dt-probes;
     calibrate fades off per-present VALUE sequences), pan 184→**182** (cmd t92, first move
     t93; tick-equal pan residual = the named fountain/butterfly ensembles only), dialogue
     1298→**1282** (arm t642, first change t645 = retail; the pop/fade window differ_px==0 —
     was a constant 8 ticks late).
  4. **R6 phase A LANDED same-ckpt:** the reveal's first update is fenced one tick
     (main.c hold>=2) — the dt minimum moved +1→0, mark box improved (t9 10318→7251).
     Phase B OPEN: a per-row fade-LEVEL map mismatch inside the frontier band (interior
     rows port ~1 index clearer, clear-edge rows retail already cleared — NOT a uniform
     shift; per-row ratios measured in ledger R6).
  5. **NEXT:** (a) R6-B — dump res 0x458 per-index luminance, invert both sides' row maps
     to (marked-tick, stagger), solve the model delta; (b) R7 — dual blit trace at one
     matched tick → per-particle (res,frame,dst) attribution; (c) R8 — RE the typewriter
     char-class→grade map (the row-close grade); (d) dialogue chip 4 cont. (Z-advance +
     script table + module-aware arrow re-probe) + the probe-flag removal chip.
- **Prior (ckpt 104): the in-game DIALOGUE BUBBLE is PORTED + BIT-EXACT in-window — pop-in,
  speaker tail, name tab, name, portrait cross-fade, and the TYPEWRITER all pair `differ_px==0`
  on trace-studio intro-1 (the worklist's big mark @2429 CLOSED). (939 pass, +7.)**
  1. **`src/dialogue.{c,h}` (NEW, host-tested ×7) + `main.c` wiring** — the `0x439690`-builder
     widget model (quirk #97): 9-slice bubble res `0x456` at (174,148) 408×112 with the
     `+0x1c==1` SCALE pop-in (+50/update, content gated till 1000), speaker-anchored TAIL pair
     (box-bank frames 9/10 at clamp(speaker)−16, box bottom), name tab res `0x44a` f0, name
     white+`0x455f7b` 3-pass GDI, portrait res `0x7ef` keyed 1:1 at (150,76) with the `0x49c910`
     cross-fade (ramp_b, SNAPS opaque at fade 500), body rows (310,168)+28/row Courier 7×18
     `0x3e537d`/`0xa8b9cc`, typewriter 5 updates/char (space 1, comma 3i, row close +i — fitted,
     `dialogue-pause-grades`). Armed at game_enter+1298 (`dialogue-trigger`); line 1 text+name
     read from the user's exe by VA (`0x86d58c`/`0x6b6f80` — never embedded).
  2. **Verification (the studio loop, 2 recapture rounds):** round 1 caught the fade lag (the
     hold-at-19 model — retail snaps the new cel OPAQUE at fade 500) + the satellite misplace
     (851-px residual → the tail pair belongs at the SPEAKER x, not the box left edge); round 2:
     box region differ 0 at 22/25 sampled frames, the rest = ONE GLYPH at a reveal boundary.
  3. **The USER's new mark @2463 ("retail a couple frames ahead on the text reveal") MEASURED
     = the phase pillar, zero-mean:** reveal boundaries oscillate (retail +2 flips at 2462/2472/
     2544, simultaneous 2516-2580, port +2 at 2558/2568/2590) — retail's tick-coalescing under
     lockstep (the R5 mechanism) stepped through the sticky pairing drift. Cadence itself 1:1.
  4. **Format finds (quirk #98):** the 24bpp blobs (portraits/parallax) embed a plain BMP whose
     palette slot is XP-era packer heap garbage; the screen = the sheet through ONE RGB565
     quantize+bit-replicate round trip (565 surfaces). And res 1000 in sotesd = a parallax
     MOUNTAIN plane — the arrow bank's "res 1000" is another module (quirk #92 collision), so
     the arrow art is PORT-DEBT(dialogue-arrow-art) (it's hidden during typing = out-of-window).
  5. **NEXT:** (a) the remaining intro-1 marks: the reveal/fountain/NPC-anim phase trio (@1122/
     @1177 — likely one sim-tick-origin cause), the lizsoft-logo fade @218 (boot phase), the
     banner fade-out @2159 check; (b) chip 4 cont.: Z-advance + the ~15-line script table +
     the arrow-bank re-probe (module-aware); (c) the probe-flag removal chip (ckpt-103 leftover).
- **Prior (ckpt 103): the TRACE STUDIO is BUILT + LIVE — the openrecet-style scrub-and-mark
  viewer, pulled forward from Phase C by USER directive. The review loop is now: capture → USER
  scrubs + drops divergence notes in the browser → worklist.md → Claude fixes → `--only port`
  re-capture. (36 tool checks pass; first live session `intro-1` = 2598 paired frames.)**
  1. **`tools/trace_studio.py {capture,recapture,pair,serve,apply,sessions}`** (package
     `tools/trace_studio/`, SPA `tools/trace_studio_web/`; how-to `docs/trace-studio.md`,
     architecture `docs/plans/trace-studio.md`). Capture drives BOTH targets concurrently
     (port `--capture-all` → C:/ staging; retail `--no-turbo --lockstep --seed-pin
     --capture-frames all --max-flips`), pairs the flip axes ANCHOR-SEGMENTED with a sticky
     ±drift best-match, emits ordinal-named frame trees + all-intra scrub mp4s + state.jsonl +
     an anchor-RNG/per-segment verdict. Serve = 3 lockstep panels + differ_px ribbon + anchor
     track + marks with crop thumbnails + working-trace editor + re-capture jobs.
  2. **The pairing model is PROVEN on the live intro-1:** the prologue segment LOCKS at a
     constant −7 drift with 192/290 bit-exact (the per-side anchor arm offset, measured); the
     town hunts (0/1483) because content genuinely differs every frame — the missing dialogue
     box (the ckpt-102 front), frozen butterflies, pan offsets. The studio made the whole gap
     visible in one artifact. boot/title redness = documented R3; anchor-rng DESYNC before
     game_enter = expected (quirk #77, the verdict explains it inline).
  3. **Harness hardening from the live smokes:** anchors.jsonl stream + `--max-flips` with an
     agent-side emit ceiling (the ~900KB/frame firehose starved device.kill for minutes);
     leftover-game kill must go THROUGH FRIDA (elevated frida-server children get Access-denied
     from taskkill); pre-flight leftover kill; WSL-interop vsock footgun (run captures from an
     interactive shell). **R5 (USER-observed pan spikes) RESOLVED same-day by measurement on
     intro-1:** pan LOGIC identical (same ease histogram, same 434px, step-for-step timing);
     the residual is retail occasionally coalescing 2 ticks into one present even under
     lockstep (3×/pan; pervasive under the live real clock = the USER's spikes) — the phase
     pillar, zero logic divergence. parity-ledger R5 has the numbers.
  4. **Archive sweep:** `tools/archive/README.md` — the ad-hoc /tmp side-by-side video flow is
     SUPERSEDED by the studio; the frida_capture probe-flag graveyard is marked for mechanical
     removal (deferred so the live capture path stayed stable during the first review round).
  5. **NEXT:** (a) read the USER's intro-1 worklist marks and chase them (the dialogue-box chip
     ckpt-102 step 5 is the known big one); (b) the probe-flag removal chip; (c) optional:
     `--call-trace` studio session for per-frame flow fields in the state panel.
- **Prior (ckpt 102): the in-game DIALOGUE BOX subsystem is fully RE'd + the legal text-reader is
  BUILT/TESTED, and the box render is GROUND-TRUTHED — foundation for the town-intro dialogue →
  controllable Arche (Phase 3→4). (932 pass, +5; 2 commits, no pixels yet.)** Plan: `plans/dialogue-cutscene.md`.
  1. **Architecture (decompile-verified).** The town arrival is a linear cutscene coroutine: script
     `0x4d7d80` case `0x334be` configures a beat on the scene-controller, then `FUN_00439680`→ the
     blocking BEAT-RUNNER `0x439690` (pumps frames via `0x48c150`+flip until the beat completes; returns
     6 on Escape). Dialogue-line setup `0x49d6e0`: text→`+0x8a`, name→`+0x28a`, voice→`+0x2e8`, portrait
     id→`+0x84` (resolved via the face table `DAT_006b6568`), dirty `+0x78`, beat `+0x20=1`. Beat types
     (switch `:1128`): 1=dialogue (Z via `0x43b980`), 2=flag, 3=camera-at-target, 6=timer `+0x57c`, etc.
  2. **Render path — both primitives ALREADY in the port.** `0x48c150`→`0x48c820` (widget tree)→
     `0x48cf80` (9-slice frame = `src/newgame_box.c`) + `0x48e200` (GDI text = `src/glyph_render.c`).
  3. **exe_strings (COMMITTED, host-tested incl. the real exe).** The story text + names are content in
     the user's `sotes.exe` `.data` (line 1 @ VA `0x86d58c`); the Steam DRM leaves `.data` intact, so the
     port reads them at runtime by VA (`pe_string_at`/`exe_data_string`) — never embedded as source.
  4. **GROUND TRUTH captured (`runs/dialogue-probe`/`-portrait`, PNG confirms):** box frame res `0x456`
     (9-patch 32×32, **174,148, 408×112**, alpha fade-in), speaker-name header ("Arche's Father", Courier
     New **7×18**, color `0x455dbb`, ~(410,139)), **2** body rows (Courier New 7×18, typewriter ~1 char/10
     flips, main `0x3e537d` + light outline `0xa8b9cc`), advance arrow res `0x3e8` (animated, ~(542,240)),
     **large portrait bust** res `0x7ef` (160×176, magenta key) on the left. Town script = ~15 lines
     (Father/Arche/Mother/Sana, voices `0x3eb`–`0x3f4`).
  5. **NEXT (the immediate chip, all inputs ready):** port `src/dialogue.{c,h}` — register res `0x456`
     (box) + `0x7ef` (portrait); compose box (`newgame_box`) + name + 2 text rows (`glyph_render`) +
     portrait at the captured geometry; arm with a measured-constant trigger (PORT-DEBT, like the banner)
     after the banner; verify `differ_px==0` vs `runs/dialogue-probe`. Then typewriter + Z-advance + the
     full script; then the beat-runner driver (retires banner/camera-pan/letterbox debts); then Phase 4.
- **Prior (ckpt 101): the "Town of Tonkiness" area-title BANNER is PORTED + BIT-EXACT (differ_px=0) —
  USER-confirmed "banner looks good". (933 pass, +5.)**
  1. **It's `FUN_00494a60`, NOT the `0x5a00c0` overlay player** (ckpt-100 RE; `0x5a00c0` is the scrolling
     story-text/dialogue runner). The area card = a 3-slot renderer called from `0x48c150:176-178` (AFTER
     the scene-fade grid); only slot0 (`view+0x11c`) is the title. mode 1 = the scroll SPRITE (res `0x449`,
     slot 53 / the `0x8a7714` bank) with the area name **GDI-composed onto it** (Courier New h20 w10, white
     + `0x404040` outline) then blit at (160,64); animation = the `0x499ab0` phase machine (fade-in
     `alpha+=0x14`/sim-tick → hold 400 → fade-out).
  2. **PORTED (`src/banner.{c,h}`, NEW + host-tested) + `main.c` wiring** — the scroll fetch (slot 53), the
     GDI compose (`zdd_object_get_dc`→`TextOutA`→`release_dc`), the keyed/alpha blit, armed at game_enter+78,
     stepped+rendered in the sim-tick block after `scene_fade`. The port already had every primitive
     (GetDC/`glyph_row_draw`/`ar_make_font`/blits/`g_ramp_b`).
  3. **BIT-EXACT: `differ_px=0/36720`** over the whole banner (scroll+vines+text+sky), camera-matched (port
     1300 vs retail `runs/banner-verify` 1614). **Key fix:** the scroll decodes **UNGRADED** — retail binds
     it via the plain getter `0x418470(0)` (no `0x417c40` grade), so skipping the in-game 8bpp palette grade
     for slot 53 made the parchment bit-exact (a graded decode was ~10% too dark). parity-ledger #11;
     engine-quirk #96.
  4. **NEXT (open options):** (a) the `0x5a00c0` dialogue box + scrolling story-text overlay (the remaining
     non-tile layer); (b) the movement FSM `0x43f880` → butterflies drift + controllable Arche (Phase 4).
     Residual debt: PORT-DEBT(banner-trigger) (arm timer +78 + text are measured constants, real source =
     the scene script), (banner-grade) (the slot-53 grade-skip is by index; the faithful gate is the
     `0x417c40` getter), (banner-font-table) (only the font-6 length band).
- **Prior (ckpt 99): the SETTLED-town per-tick RNG stream is now bit-exact ALL THE WAY (not just the
  REVEAL window) — the 4 IRREGULAR ambient/event timers are ported, closing PORT-DEBT(fountain-rng-phase).
  (922 pass, +1.)**
  1. **The residual was FOUR self-clocked timers, not "all 0x5531b0"** (corrects the ckpt-98 guess).
     A seed-pinned timer-state capture (`runs/ambient-timer`, the `0x5531b0`/`0x467380`/`0x54f980`
     `+0x5c`/`+0x20c` field-spec reads) pinned each one — all clean **unit-decrement**: the two
     `0x5531b0` ambient SOUND emitters **`0x11370`** (fires tick 33) + **`0x1136f`** (189), the wagon
     **`0x1872d`** idle-wander (134), and the **`0x467380`** (`0xe2a5`) event timer (183). The
     census's earlier C-values (141/189/33) were **off-by-one** — the `rng_state` field is the state
     *before* the draw, so the value is `rval(step(state))`; reading `cd` directly gave 189/33/134/184.
  2. **PORTED (`src/ambient.{c,h}`, NEW).** Four consume-to-advance timers in `0x46cd70` band order
     (proven by the capture seq): EFFECT `butterfly_step → ambient_effect_step` (`0x467380`), then
     CHARACTER `fountain → sky → ambient_character_step` (`0x1136f`, `0x11370`, wagon). Each inits
     `(rand*300)>>15` @t0 and fires (3-4 draws) on cue; the values feed sounds / the wagon wander / an
     `0xe2a5` sub-effect (none ported) but the COUNTS + TIMING keep the stream aligned. Replaced the
     ckpt-98 blanket 3-draw consume.
  3. **VALIDATED 3 ways:** offline LCG replay **0/297** vs the capture's `0x46cd70` checkpoints (full
     298-tick window); **LIVE port bit-exact ticks 0-248** (two `--call-trace` windows; the port's
     per-tick `0x46cd70` rng matches retail tick-for-tick through ALL FOUR fires 33/134/183/189); host
     test `ambient_pertick`. **Retires** the RNG residual of fountain-rng-phase +
     the RNG half of PORT-DEBT(actor-protagonist-clip) (the wagon's idle-wander draws). New tooling: the
     reusable `argfield` field source (read a struct field off a stack-arg pointer).
  4. **NEXT (open options):** (a) the `0x5a00c0` banner/textbox overlay (visible gap, high leverage —
     gates letterbox heights + camera-pan trigger); (b) the movement FSM `0x43f880` → butterflies drift
     + controllable Arche (Phase 4). Residual debt: PORT-DEBT(ambient-event-cd) (the `0x467380` cd-init
     = the seed-pinned 184; real source = the unported `0xe2a5` spawn arm `0x431cb0`).
- **Prior (ckpt 98): the town's PER-TICK RNG STREAM is bit-exact through the establishing-REVEAL
  window — the ckpt-73 "non-deterministic RNG" was the MISSING butterfly draws, not nondeterminism.
  (921 pass, +2.)**
  1. **The complete per-tick model (engine-quirk #95), validated bit-exact at 293/298 ticks.** `0x46cd70`
     walks the bands; only two draw RNG in the town: the **EFFECT band `0x47b990`** (called ONLY for the
     4 BUTTERFLIES — update-mode 1; the townsfolk take the RNG-free `0x478ba0`) and the **CHARACTER band
     `0x54f980`** (the fountain/sky emitters, already ported). Clean COUNT model: `tick 0 = 23`, then
     `6 (fountain) + 8·[N even] + 8·[N≡5 mod 6]`. The 5 misses are named irregulars (the ambient timer
     `0x5531b0` ×3, the flit re-fire at tick 162, +2 unknown).
  2. **PORTED (`src/butterfly.{c,h}`, NEW).** `butterfly_step` runs the `0x47b990` `0xe29a` draw model
     once/sim-tick BEFORE the emitters: the every-other-tick gate `0x14232`, the flit-pick timer `0x14236`
     (fires every 80 work-ticks), and the heading+flag draws — each butterfly's `0xc874` move-freq captured
     by `actor_spawn_effect_from_map` from the spawn replay. The flit MOTION (`0x43f880`, the 5.5 KB
     movement FSM) is deferred (consume-to-advance), so the butterflies hold position but the stream aligns.
  3. **LIVE-CONFIRMED bit-exact:** the running port's per-tick LCG state matches retail tick-for-tick
     (`0x9c2b551d, 0xb92fc6fa, 0x5c22a348, …` for ticks 0-11, = `runs/rng-census-repin`). Host test
     `butterfly_pertick` (gate/timer/count) + the offline 34/34 replay. **Retires the RNG half of two
     debts:** PORT-DEBT(fountain-rng-phase) narrows to the irregular `0x5531b0` ambient (the regular
     consumers are aligned); PORT-DEBT(butterfly-wander) is no longer RNG-blocked — the drift now waits on
     the movement FSM `0x43f880` (Phase 4).
  4. **NEXT (open options):** (a) the ambient timer `0x5531b0` + the 2 unknowns → the SETTLED-town
     fountain bit-exact (closes fountain-rng-phase); (b) the `0x5a00c0` banner/textbox; (c) the movement
     FSM `0x43f880` → the butterflies drift + controllable Arche (Phase 4).
- **Prior (ckpt 97): the room-load RNG burst is COMPLETE — the establishing-REVEAL iris VARIANT is
  DRAWN at the correct post-spawn phase, not pinned. (920 pass.)**
  1. **The town's first in-game frame draws a 19-object EFFECT spawn burst** (engine-quirk #94): 15 MAP
     objects (`effect_from_map`, 171 draws) + **4 SCRIPT cutscene-cast** (`cutscene_cast`, 42 draws —
     Arche `0xc35a` draws 12 via her `0x427360`, the other 3 ten each), THEN the iris-variant draw
     `(rand*3)>>15`. The port consumed only the 15 map objects → the iris drew variant 2 (sweep);
     now `cutscene_cast` consumes its 42 → variant **0 (center-out)** = retail's town value.
  2. **Only the COUNT matters** (the MSVC LCG state after N steps is value-independent). Proven offline
     (after 213 spawn draws the iris rand=7211→variant 0; `0x4f5347`+214 draws=`0x9c2b551d`= retail's
     tick-0 state — the spawn is now byte-aligned), host-tested (`actor_spawn_cutscene_iris`), and live
     (`scene_fade_arm … variant=0 … DRAWN`). Retires the RNG half of PORT-DEBT(scene-fade-rng-phase).
- **Prior (ckpt 96): the town BUTTERFLIES are PORTED — and `0xe29a` was NEVER "wandering
  villagers" (a ~13-checkpoint mislabel). USER-confirmed the retail capture + identification. (919 pass.)**
  1. **The chase (USER-pointed: tiny ~3px butterflies by the flowerbeds, "over the dark wood, below
     the ARMS/sword sign, above the dog", retail flips 2028/2138).** No capture had hooked the particle
     band at the SETTLED town, so I drove retail there (`--seed-pin --lockstep`) + captured PNGs/traces.
     The particle band (`0x493480`) renders ONLY the ported `0x18704`+`0x18708`; the EFFECT band only
     townsfolk/cast. A **blit trace** found the butterfly at the screen pos = **res `0x3fa`, 14×8**; an
     **emit trace** (`0x492670` cel_res+ret_va) named the producer `0x493ba0` at world positions
     matching the **`0xe29a`** census **1:1** → the "wandering villagers" ARE the butterflies.
  2. **Asset:** res `0x3fa` = bank `0x146` (slot 313, 32×32, sotesd.dll DATA — already group3-registered,
     just unused), clip **`0x65ddf0`** (decoded: 3-frame flap, dur 4, loop). Two colour variants
     (yellow+white) = per-instance frame_base 0/4/8/12.
  3. **Port (`src/actor_spawn.c`):** added `0xe29a` to `TOWN_EFFECT_DEFS` (bank `0x146`, dst 0/0, layer
     12) + `BUTTERFLY_CLIP`; the spawn selects the per-code clip before the `0x426ec0` phase draws (draw
     COUNT unchanged → no townsfolk-phase regression). Was *excluded* (draws consumed, not spawned); now
     spawns + renders via `actor_render_static`. **Verified:** 919 pass; port blit trace emits res `0x3fa`
     frames 0/1/2 on-screen (frame 1600 @dx 116/180, 1850 @dx 491/555) — 2 yellow butterflies flapping by
     the ARMS sign / flowers / dog, matching retail. Pushed to feed. quirk #93;
     `in-game-intro.md` "The town BUTTERFLIES". PORT-DEBT(butterfly-wander): per-instance direction/colour
     + RNG flit drift (the 5 `0x427670` draws are consumed, motion not applied) — Phase 2.
  4. **Lesson:** to ID a small/ambient actor, capture the rendered RESOURCE (blit `res`/emit `cel_res`),
     not just the actor code+bank — a "wandering NPC" can be a butterfly. The hold census missed it
     (off-screen-left at flip 1500; the camera only pans to the inn half during the arrival).
- **Prior (ckpt 95): the establishing REVEAL is PORTED — the center-out vertical iris that opens
  the town from black. USER: "the iris looks reasonable." (919 pass, +5.)**
  1. **A self-contained scene-transition FADE-GRID** (`src/scene_fade.{c,h}`, NEW): a 10×120 grid of
     64×4px cells over the screen, each `state 0 opaque → 1 fading → 2 clear`. **render** = `0x48e920`
     (after the letterbox, `0x48c150:175`); **update** = the INLINE loop `0x499ab0:125-177` + the iris
     **pattern setters** `0x49a890` (variant 0 center-out) / `0x49a740` (1 edges-in) / `0x49aae0`+
     `0x49aa00` (2 sweep); **arm** = `0x439690:555-583`. Live town params (`runs/reveal-grid`, the
     `0x48e920` field spec): W=10 H=120 count=1200, **mode 1, speed 1000, variant 0** (the variant is
     the LCG draw `(rand*3)>>15`).
  2. **CORRECTS quirk #90:** `0x49af40` is **NOT** the grid update — reading it, it's the HUD/portrait/
     HP-bar animator (walks the party array `room+0x4030`). The −8px/sim-tick = mode-1's **2 rows/tick**
     × 4px (not `0x49af40` 2×). Fixed in `engine-quirks.md` #90 + `retail_fields.json`.
  3. **Wired + verified.** `enter_game` arms it; `scene_fade_step` runs once/sim-tick after the camera
     easer; `scene_fade_render` after the letterbox. opaque sink = letterbox cel (res `0x583`); **alpha
     sink = the true `0x5bd550` composite** of res `0x458` frame[level] (the per-level gray mask) via the
     descriptor **`g_pd_boot_group_e[19]`** (= `*(0x8a93b8)`, weight 1000 mode-2 subtract-blend) — found
     by disassembling `0x48e920` to recover the ECX Ghidra dropped (the first keyed-blit cut drew the gray
     opaquely → USER "white outside/black inside/no transparency"; now the town shows through, darkening
     to the edge). **Port blit trace:** black tiles 1490→650→320 over frames 1118→1200, center-out,
     settling to the 64px letterbox by ~sim-tick 25 (= retail's 240→64). Host-tested (`test_scene_fade.c`,
     5). PORT-DEBT(scene-fade-rng-phase): the iris VARIANT is RNG + the spawn-RNG phase isn't aligned yet
     → pinned to the live town 0; the load-window start offset + the variant land in Phase 2.
  4. **BMP capture footgun fixed (USER caught it):** the in-game capture was never broken — passing a
     WSL `--capture-dir /tmp/…` the Windows exe can't `fopen`; default (game dir) works. Added a hint on
     `fopen` failure.
- **Prior (ckpt 94): ARCHE RENDERS — the in-game intro cast is COMPLETE. USER-confirmed on
  the live port window: "everyone is rendering correctly now." (914 pass.)**
  1. **The whole "party band" Phase-2 framing was unnecessary for the arrival scene.** A live
     census (`runs/cutscene-cast`) showed Arche (`0xc35a`) is drawn by the SAME `0x493ba0` EFFECT
     path as the rest of the cast — row0 bank `0x8b`, clip `0x62a8c8` (decoded byte-identical to the
     idle clip), world (41600, 45600), dst (−30,−24), facing 1, layer 13. Her ONLY blocker was bank
     registration.
  2. **Her body banks `0x8b`–`0x8e` (slots 126–129) are EXE-EMBEDDED sprites res `0x570`–`0x573`**
     — pinned by a field-spec chain read of the live retail slots (`runs/arche-res`/`arche-params`,
     validated vs known slots). **The trap (quirk #92):** those ids are `WAVE` *sounds* in sotesd.dll
     but `DATA` *sprites* in **sotes.exe**'s own `.rsrc` — a numeric collision that derailed ckpt 90.
     So they load from the user's `sotes.exe` at runtime (`FindResourceA`), NEVER embedded (USER
     directive). New `ar_register_party_exe_sprites` registers slots 126–129 with `settings =
     g_sotes_exe`; `actor_spawn_cutscene_cast` gains an Arche row (`bank_override 0x8b`, since her
     dramatist bank is 0). She renders via `actor_render_static` (one keyed cel).
  3. **Bit-level confirm:** port blit trace (settled frame 2200) emits res `0x570` frame 1 at screen
     (258, 304) — exactly world (41600,45600) − settled cam + dst. PORT-DEBT(cutscene-party-chars)
     **narrowed:** the static-cast Arche, not yet the party band `0x4997b0`; her multi-part body
     `0x8c`–`0x8e`, the walk-in roll-in, and the live-actor handle registry (dialogue) remain Phase 2/3.
- **Prior (ckpt 93): the DRAMATIST RESOLVE + arrival-cast spawn is PORTED — Arche's MOTHER
  (`0xc440` bank `0xb5`) now renders her own sheet. USER-confirmed: "all characters except for
  arche are there and positioned correctly." (914 pass, +3.)**
  1. **New `src/party.{c,h}`** ports the static "Get Dramatist Info" table `DAT_006b6ea8`
     (79 rows `{handle, code, bank}`, numeric facts only — names stay in the proof/dump tool,
     not embedded) + **`party_resolve_spawn`** = `0x41f200:54-69` (handle→code/bank lookup; the
     spawn path passes the activator's param_4=3, so the row code overrides only when code_in==0)
     + **`party_archetype_default_bank`** = the per-case `if (sVar17==0) sVar17 = <facing default>`
     arm (the RE'd subset `0xc3dc`/`0xc3e6`/`0xc3f0`/`0xc440`/… read off the decompile). Host-tested.
  2. **`actor_spawn_cutscene_cast` rewritten** to spawn the family by their RE'd `0x41f0e0` params
     and resolve each through `party_resolve_spawn`: **Dr. Barnard** (by code → `0xeb`), **Father**
     (handle → `0xe3`), **Mother** (handle → OVERRIDE `0xb5`). Mom's `0xb5` is registered in group3
     (idx 168), so she renders — fixing her absence (the port had only the far-right map townswoman
     `0xa6`). Positions = the wagon's settled anchor 41600 + the RE'd offsets (reproduces the census
     exactly). The frozen `CUTSCENE_CAST_DEFS` snapshot is retired.
  3. **The one remaining gap: ARCHE the GIRL** (`0xc35a`, dramatist bank 0) — she is the party
     LEADER (party band `0x4997b0`), her body banks `0x8b`–`0x8e` (idx 126-129, UNREGISTERED) are
     party-loaded by the unported new-game path. **NEXT (Phase 2):** the party band `0x4997b0` +
     per-character sprite-load (register `0x8b`–`0x8e` + her clip `0x62a8c8` via the multi-part
     `0x493ba0` arm) → Arche renders → gateway to controllable Arche. Then Phase 3: the walk-in
     dialogue movement + portrait/textbox (`0x5a00c0`). PORT-DEBT(cutscene-party-chars).
- **Prior (ckpt 90): two golden-review gaps chased — the establishing REVEAL is RE'd
  (a fade-grid, NOT the letterbox) and the town-intro cutscene NPCs are PORTED; the woman +
  little girl are PLAYER-PARTY characters, render path now SCOPED (PARTLY WRONG — see ckpt 91).**
  1. **The establishing REVEAL = a per-cell FADE-GRID transition, not the letterbox bars
     (committed, quirk #90).** Pixel envelope: top/bottom black ramps ~240→64 at −8px/sim-tick.
     **Refuted by live field capture** — both `0x499ab0`'s and the grid-fill `0x48c150`'s bar
     heights read **constant 64** the whole reveal (scroll 0). The real producer is **`0x48e920`**
     (a 64×4 per-cell black-tile fade-grid, the center-out iris; `ret_va 0x48e9c3` emits
     ~1010→0 tiles), rendered from `0x48c150:175`, updated 2×/tick by **`0x49af40`** from the
     cinematic step `0x499ab0`. Explains the long-open ckpt-66/67 "dark top gradient". PORT:
     unported (the reveal chip — port `0x49af40`+`0x48e920`+trigger). `findings/in-game-intro.md`
     "The establishing REVEAL is a per-cell FADE-GRID".
  2. **The town-intro cutscene NPCs (in front of the wagon) PORTED — `actor_spawn_cutscene_cast`
     (committed).** USER flagged 4 missing characters at the pan end. RE'd the spawn (cutscene
     `0x4d7d80` → anchor-relative `0x41f0e0` → `0x41f200`, positioned vs the wagon's anchor
     `0x65`) + captured the settled census (`runs/cutscene-cast`). Ported the 3 EFFECT spawns
     to `g_effects`; **`0xc3dc`+`0xc3f0` (banks `0xe3`/`0xeb`) RENDER** (the 2 NPCs near the
     horse), facing fixed with the in-scene `DAT_008a8440` flip read (=4). **`0xc35a` (the
     woman) CULLS + the little girl is absent — both are PLAYER-PARTY characters.** 911 pass.
  3. **The PARTY render path SCOPED (next arc, USER-chosen).** The party renderer **`0x4997b0`**
     (150 B) just iterates the 8 party actors (`room_state+0x4030`, reset by `0x560e60`) and
     renders each via **`0x493ba0`** — the renderer the port ALREADY reuses. The woman is a
     keyed blit **res `0x477`**; the little girl (the controllable-character actor) renders via
     a richer path (no keyed blit). **Blocker = bank registration** (`game_sprites[]`), with a
     mismatch to resolve (census bank `0x8b`→res `0x4fb` ≠ the rendered `0x477`; `0xc35a`'s
     `+0x48` is a party indirection). **NEXT:** find the bank for res `0x477` → spawn the woman;
     RE the party spawn + wire `0x4997b0` for the girl/protagonist (gateway to the
     controllable-character milestone, Phase C). `findings/in-game-intro.md` "The PARTY-character
     render path is SCOPED". PORT-DEBT(cutscene-party-chars).
- **Prior (ckpt 89): the SKY-AMBIENT particles (`0x18704` = CHIMNEY SMOKE) are PORTED +
  USER-1:1, and the placement is now TRACE-FAITHFUL (anchor + facing fixed from retail).**
  Chip 4: the town's second particle system, built on the ckpt-88 pool/alpha path.
  **PORTED (`src/particle.{c,h}`):** emitter `0x112e2` (`0x54f980:150`, spawns 1 every 6th
  tick), config `0x557550:630` (bank `0x1aa` frame 8, clip `0x644b58` = 6-frame **ONESHOT**
  decoded from the exe, layer 6, `0x453960` velocity scatter), step `0x46e510:683` (vel_y
  decel→-5000, integrate, **expire on the oneshot done flag**, ramp_b fade), and the **ramp_b
  alpha** path (`game_present_blit` decodes `param8 = (ramp_sel<<8)|idx` → ramp_a water /
  **ramp_b** `0x8a9308` sky). **WIRED (`main.c`):** finds both `0x112e2` props, emits each
  sim-tick into the shared `g_fountain_pp`. **USER-confirmed "smoke looks 1:1"** + the USER
  independently spotted the same chimney smoke in retail. **TRACE VERIFICATION (USER directive)
  caught + fixed 2 RNG-independent bugs:** (a) **anchor** — I'd HARDCODED +1600; the faithful
  `0x557370` mode-1 anchor is render-state +0xc/2, and the invisible `0x112e2` trigger has
  +0xc==0 → **anchor 0** (spawn at the prop's exact world pos); removed the constant. (b)
  **facing** — `runs/sky-facing` shows every particle has +0x2c==**1** → x `+= +vel_x/100`
  (no flip) → the sky **drifts LEFT** (matching retail); the port spawned facing 0 → drifted
  right; fixed `particle_spawn_{water,sky}` to facing 1. After both: port sky world X
  `[51440..113369]` ≈ retail `[50690..114356]`, Y matching. **911 pass** (+5), ledger
  unchanged. quirk #88; `findings/in-game-intro.md` "The SKY-AMBIENT particles". **The town's
  `+0x13e0` band now renders BOTH its codes (`0x18704`+`0x18708`) — no particle remainder.**
- **Full-intro side-by-side VIDEO (ckpt 89, USER-requested).** Frame-matched (anchor-aligned)
  retail|port across title→newgame→prologue→town, 64 pairs (`/tmp/intro_sidebyside.mp4` +
  a feed montage). **title/menu 1:1, prologue aligned, town establishing 1:1** (backdrop +
  fountain + decorations + townsfolk + chimney smoke all match). The one clear divergence the
  sequence surfaces: retail's **"Town of Tonkiness" area banner** (~retail flip 1600+) is
  MISSING in the port = the `0x5a00c0` scripted-overlay debt (PORT-DEBT `ingame-nontile-layers`;
  a TIMED element, absent at the hold — consistent with ckpt 82). **NEXT — the USER's
  golden-video review (ckpt 89) flagged 5 concrete items (quirk #89):** (1) the **establishing
  REVEAL** — a VERTICAL FADE opening from the MIDDLE of the screen outward (CONFIRMED off the
  golden; this IS the long-open "dark top gradient" of ckpt 66/67 — a vertical-iris reveal, not
  a static tint; the port jumps straight to the letterboxed scene). (2) the **`0x5a00c0`
  banner/scripted overlay** ("Town of Tonkiness", timed ~retail flip 1600+). (3) **ground
  BUTTERFLIES** by the flowerbeds at the SETTLED town (~flip 2150) — likely a `0x557550` "leaf"
  particle code (`0x18707`/`0x18709`) the hold-only census missed; RE via a render trace at the
  settled town. (4) the Start-Game menu **SCALE transition** (scales IN on appear + OUT on confirm; port pops
  it in/out). (5) **phase-match
  the particle RNG** (PORT-DEBT `fountain-rng-phase`, Phase 2). PORT bug to verify: the
  menu-cursor pulse looked fast (dev-build frame rate? — TODO).
- **Prior (ckpt 88): the FOUNTAIN SPRAY (`0x18708`) is PORTED + USER-confirmed** — the
  particle subsystem RE'd (1024-slot `+0x13e0` pool, alloc `0x557370` / config `0x557550` /
  step `0x46e510` / `0x493480` alpha render); translucent water via ramp_a
  (`g_ramp_a[10-sub_phase]`). `src/particle.{c,h}` NEW; quirk #87.
- **Prior (ckpt 87): the townsfolk IDLE ANIMATION PHASE is PORTED — they now breathe
  from a per-actor RNG start frame instead of frozen on frame 0.** Builds on the ckpt-86
  anchor; the first user-visible payoff of the spawn-RNG arc. **The model (engine-quirk
  #86, decompile- + census-verified):** for each of the 15 map EFFECT objects in
  dispatch (layer) order, `0x41f200` consumes `8 + extra` draws (`0x426fd0`(1) + prologue
  (7); extra = 5 for the 4 `0xe29a` wanderers via `0x427670`, 1 for `0xe2a5` via
  `0x431cb0`, else 0) then `0x426ec0`'s 2 idle-phase draws (`frame=(rand*20)>>15`,
  `timer=(rand*14)>>15`). All 11 rendered townsfolk fall in the first 15 effects (the 4
  script effects spawn after), so they are unaffected by the rest. **PORTED:**
  `actor_spawn_effect_from_map` replays the per-object draws in map order (consume-to-
  advance; only the `0x426ec0` pair is used, only for the rendered townsfolk — the
  wanderers/unknowns still consume their draws), embeds the shared idle clip `IDLE_CLIP`
  (= `0x6290e0`: base 0, 20f, dur 14, looping, delta {0,1,2,1,…} — decoded from the exe),
  and sets `rs->clip`/`frame`/`timer`; `game_actor_update` now also advances `g_effects`
  per sim-tick (RNG-free `anim_clip_advance`) so they breathe in lockstep. **Verified:**
  host test locks the replay to a reference LCG (frame/timer per slot); the draw model is
  census-verified (counts 134/38/20/19) + decompile-verified (the `0xe29a`/`0xe2a5`
  cases); offline from `0x4f5347` the 11 start frames are {1,17,17,17,3,14,4,16,18,12,10}.
  **898 pass, ledger 199/194 unchanged** (bare-VA slice of `0x41f200`). quirk #86;
  `findings/in-game-intro.md` "The town SPAWN RNG anchor" + the idle-phase note.
  **VALIDATION PENDING (not yet differ_px==0):** a bit-exact cross-check of the port's
  `+0x72` per townsperson vs retail (a `0x426ec0` onLeave capture, or render_diff at a
  matched sim-tick) — the chain is complete but the live pixel/`+0x72` diff is the next
  step. **THEN Chip 3 — the FOUNTAIN SPRAY** (band `+0x13e0`/`0x493480`; the `0x427b70`/
  `0x427670`(20) particle init + per-tick `0x47b990`/`0x453960`).
- **Prior (ckpt 86): the town SPAWN RNG ANCHOR is LANDED + LIVE-VERIFIED — the keystone
  for the two remaining RNG residuals (idle PHASE + fountain SPRAY).** Phase-2 matching
  half, the foundational chip (no visual change yet — that lands when the spawn consumers
  are ported, Chip 2). The title→town RNG is non-deterministic run-to-run even under the
  boot seed-pin (quirk #77; game_enter seed was `0x46fe3f46` this run vs `0x83600390`
  last), so the town SPAWN draws started from an unpredictable phase. **FIX:** re-pin
  `DAT_008a4f94` on BOTH sides at the spawn start. **Ground truth (engine-quirk #86, the
  seed-pinned `0x5bf505` census `runs/rng-census-repin`):** the town-LOAD frame draws a
  fixed **238-draw burst over 19 EFFECT objects** (`0x58d460`→`0x41f200`, map order; the
  port renders 11 but ALL 19 consume RNG). Per object: `0x426fd0`(1)+`0x41f200`(7 jitter
  +particle-params)+optional `0x427670`(5, 4 objects)+`0x426ec0`(2 idle frame/timer).
  **The re-pin point is the FIRST `0x41f200`, NOT game_enter** — a pre-spawn one-off
  `0x4c5e00`(1 draw) sits between them (so a game_enter pin would desync the port by one
  draw). **Agent:** `installRngAnchor()` arms at the game_enter anchor, writes the seed at
  the first `0x41f200` onEnter (`rng_anchor` event). **Port:** `game_rng_seed()` helper +
  `rng_srand` re-seed at `enter_game` top (faithful — all pre-effect-spawn code is
  RNG-free). **VERIFIED LIVE:** `re-pinned @ frame 1419: 0x71cc78f1 -> 0x004f5347`
  (`before` ≠ game_enter seed ⇒ the intervening draw is real); spawn draw counts
  byte-identical pre/post (134/38/20/19) ⇒ values reset, control flow untouched. **898
  pass, ledger 199/194 unchanged** (harness+seam, no fn ported). quirk #86;
  `findings/in-game-intro.md` "The town SPAWN RNG anchor". **NEXT (Chip 2):** port
  `0x41f200`'s 19-object RNG consumption in order, give the 11 townsfolk the idle clip
  `0x6290e0` + set `+0x72`/`+0x70` from the aligned `0x426ec0` draws → idle phases 1:1;
  then the fountain (Chip 3).
- **Prior (ckpt 85): townsfolk FACING is PORTED + USER-1:1 — it's a deterministic MAP
  field, NOT RNG (corrects the ckpt-84 guess).** Phase-2 matching half, first chip. RE'd
  the three ckpt-84 RNG residuals: **facing is RNG-FREE** — the dispatcher `0x58d460:96`
  computes `cVar12 = (puVar1[4]!=0)?3:1` from the map sub-record `puVar1[4]` and forwards it
  as **param_8** to `0x41f200`/`0x431e30` → render-state `+0x2c`; `0x44d160` mirrors the cel
  (`frame += flip`) + reflects `off_x` only on `facing==3`, where **`flip = *(s16)(DAT_008a8440
  [bank])` = the sprite group's frames-per-direction** (`0x8a8440` confirmed live a POINTER
  array → heap descriptors; 4 or 16 for the town banks). Live census (the `0x493ba0` spec +
  a new `rs_facing` field + a one-shot read of `DAT_008a8440`): of the 11 map townsfolk **7
  are facing 3** (`c3be/c3dd/c3e6/c422/c42c/c441/c468`), 4 normal. **PORTED:** `TOWN_EFFECT_
  DEFS` gains `facing`+`flip`; `actor_spawn_effect_fill_flip_table` fills the bank-indexed
  stand-in for the global `DAT_008a8440`, wired into every `game_actor_walk` render call;
  **898 pass**, builds clean. **USER-confirmed: "npc orientation matches retail yes."** quirk
  #85; `findings/in-game-intro.md` "Townsfolk facing is a MAP field". (Townsfolk still
  frozen-frame — the idle anim PHASE is RNG, next.) PORT-DEBT `effect-sprite-table` extended.
  **THE REMAINING TWO RESIDUALS ARE RNG → need the game_enter RNG ANCHOR:** (1) **idle PHASE**
  — `0x426ec0` sets `rs+0x72 = (rand()*clip.frame_count)>>15` (every townsperson runs clip
  `0x6290e0` at a random start frame); (2) **the FOUNTAIN SPRAY** (band `+0x13e0`/`0x493480`)
  — `0x41f200`'s 8 rand draws are position-jitter (`0x426e00` `+0x58`/`+0x60`) + a particle
  sub-spawn (`0x427b70`); helper `0x427670` (20 draws) + per-tick `0x47b990`/`0x453960` drive
  the spray. **NEXT:** re-pin `DAT_008a4f94` at `game_enter` both sides → port the spawn RNG
  consumers in order → idle phase + fountain land 1:1.
- **Prior (ckpt 84): the EFFECT townsfolk are PORTED — positions USER-confirmed 1:1; the
  residual is now PINNED to the RNG pillar (Phase 2 begins).** Landed the EFFECT band (the
  standing villagers in the square) positioned 1:1, frozen on the idle clip's frame 0 (the
  wagon/STRUCTURE precedent). **The render REUSES `actor_render_static`** — for a plain
  townsperson `0x493ba0`'s static arm reduces to the ported describe (`0x44d160`) + emit
  (`0x492670`): exactly ONE mode-0 keyed cel each (verified vs the hold blit trace — no
  `0x4917b0` shadow, no `0x8a9358` color-remap). **Placement FULLY MAP-DRIVEN:** `world =
  (map (x,y) − dst) × 100` (the +30 world offset cancels the −30 render dst → screen = map −
  cam; derived cel-for-cel vs the census `rs_x`). The 11 map townsfolk = 10 `0xc3xx` + `0xe2a5`.
  PORTED `actor_spawn_effect_from_map` (`g_effects`, walked at layer 13) + the captured def
  table (PORT-DEBT `effect-sprite-table`). **898 pass** (+1); ledger 199/194 unchanged
  (bare-VA slices of `0x41f200`/`0x493ba0`). **USER-confirmed: "the NPCs are rendering at the
  correct positions."** quirk #84; `findings/in-game-intro.md` "The EFFECT townsfolk PORTED".
  **THE RNG RESIDUAL (USER directive — pivot to Phase 2):** the scene is NOT yet 1:1 every
  frame because of THREE RNG-driven elements: (1) **townsfolk FACING** — some render flipped
  (`0x44d160`'s `facing==3` mirror; the port spawns facing 0 + `flip_table NULL` → no mirror;
  facing is likely an RNG draw at spawn); (2) **townsfolk idle PHASE** — frozen frame 0; the
  clip `0x6290e0` (20f dur 14) + stepper are ported but the per-actor START phase is staggered
  (likely RNG); (3) **the FOUNTAIN PARTICLE SPRAY** — the entire `+0x13e0` band (`0x493480`,
  res `0x408`) is MISSING (USER pointed out the purple/blue spray + leafy particles); RNG
  positions. PORT-DEBT `effect-anim-phase`/`effect-wanderers`.
- **RNG-CONSUMER CENSUS DONE (ckpt 84) — integrated into the flow trace.** Per the USER
  ("integrate, not a bespoke probe"): added `0x5bf505` (the LCG) as a `retail_fields.json`
  entry (the auto `ret_va` names the consumer site) + `tools/rng_consumer_census.py`. Captured
  1142 town-scene draws (game_enter@1434), cross-checked vs the decompile (`0x41f200` has
  exactly 8 rand calls = the 8 sites). **SPAWN (room-load):** `FUN_0041f200` (the EFFECT
  activator, 134/8 sites = townsfolk facing+phase) + helpers `0x426ec0`/`0x427670`/`0x426fd0`.
  **HOLD (per-tick):** `FUN_0054f980` (behaviour/wander 425), `FUN_0047b990` (the `+0x1160`
  EFFECT update = fountain particles/wander 320), `FUN_00453960` (154). Retires the ckpt-73
  defer-all-RNG. `findings/in-game-intro.md` "The scene-wide RNG-consumer census". **NEXT = the
  MATCHING half:** RE each consumer's draws (facing/phase/particles) → an RNG ANCHOR at
  game_enter both sides (re-seed the town scene; the port can't replay the whole boot RNG chain)
  → annotate producers (`rngcalls` + port `CALL_TRACE_BEGIN`) + `flow_diff` → port in order → 1:1.
- **Prior (ckpt 83): the establishing-hold CAST is PINNED to FOUR map-object bands — the
  Phase-1 producer map is complete (no port yet, RE+census milestone).** Resolves ckpt-82's
  "pin each cast cel to its actor" via a field-spec **band census** (`retail_fields.json`
  gained the 5 non-main band render entries `0x4937c0`/`0x493480`/`0x492fc0`/`0x493230`/
  `0x493ba0` + emit prims `0x492670`/`0x4917b0` with `renderid` on the cel). The driver
  `0x48c150` runs 8 emit passes → one present `0x48eac0`; the cel↔producer tie is at EMIT
  (the node carries no back-ref). **The 18 visible keyed cels = 4 bands, all DATA-1022 map
  objects:** **`+0x2560` STRUCTURE (`0x493230`, single-cel)** = the **TREE** (`0xec55` bank
  `0x15f`→res `0x481`) + bg decorations (`0xec6a`→`0x16c`/`0x403`) + fg hedges (`0xec60`→
  `0x164`/`0x426`, layer 15); **`+0x1160` EFFECT (`0x493ba0`, multi-part)** = the townsfolk
  (10 distinct `0xc3xx` + `0xe29a`×4 + `0xe2a5`); **`+0x11e0` CHARACTER (`0x491ae0`)** =
  collision volumes + props + wagon (**already ported**); **`+0x13e0` (`0x493480`)** = bank
  `0x1aa`→res `0x408` particles (alpha, not keyed — deferred). **STRUCTURE is fully
  map-driven:** pos = map (x,y)×100, **frame_base = map variant@+0x18** (verified
  cel-for-cel). EFFECT townsfolk map 1:1 by code/count + a deterministic spawn offset
  (≈+3000 x, from `0x41f200`); `0xc35a`/`0xc3dc`/`0xc3f0` are script/party-spawned (not in
  the map). **Hold = 16 static townsfolk + 39 static structure objects (only the anim frame
  steps, det. per #76) + 4 wandering `0xe29a` (RNG, Phase 2).** Corrects the docs' "the cast
  is the +0x11e0 band" model; the tree is STRUCTURE `0xec55`, not a banner/`0x5a00c0`/tile.
  quirk #84; `findings/in-game-intro.md` "The establishing-hold cast is FOUR map-object
  bands".
- **LANDED (ckpt 83b): the STRUCTURE band is PORTED + USER-confirmed 1:1.** RE'd the
  STRUCTURE activator `0x438a60` (per-code bank def table) + dispatcher `0x58d460`
  (layer = record +0x30 ? 15 : 8, frame_base = variant +0x18 — all verified vs census).
  PORTED `actor_spawn_struct_from_map` (60000-range, fully map-driven) + the bank table
  `actor_spawn_struct_bank_for_code`; the render REUSES `actor_render_static` (the
  `0x493230` static single-cel blit is bit-identical), wired via `game_actor_walk`
  (g_structs) at the structure layers 8/15. **Live: 39 objects spawned + emitted; tree bank
  0x15f registered.** **render_diff/position-verified BIT-EXACT:** tree `(0x481,f0)`@(496,64)
  320×320, the 5 hedges `(0x426)`, the 4 `0x403` props/deco — all identical port↔retail,
  zero `[rect]`/`[decode]`/`[state]`. **USER-confirmed on the feed: "the decorations are
  there and positioned 1:1".** parity-ledger #9; **897 pass** (+1, `actor_spawn_struct`);
  ledger 199/194 unchanged (bare-VA slices). **NEXT (Phase 1 cont.):** the EFFECT townsfolk
  — the multi-part char renderer `0x493ba0` (built on the ported `0x44d160`) + the
  `0x41f200` spawn (map pos + the ≈+3000 x offset); 16 static land at a matched sim-tick,
  the 4 wandering `0xe29a` need Phase 2 (RNG). Then the `0x4962a0` invisibles + the `0x13e0`
  particles. Also pending: byte-confirm the wagon via `render_diff` `(res 0x3ec, frame)`.
  Artifacts (`/tmp`): `cast_census/`, `port_hold_trace.jsonl`, `retail_hold_1500.png`.
- **Prior (ckpt 81): the caravan's HORSES now TROT — the per-tick actor anim is wired +
  BIT-VERIFIED live.** Read `0x54f980` case-`0x1872d` (`:911-970`): its two halves are
  separable (quirk #82) — **`:911-928` is the frame-stepper, UNCONDITIONAL** (gated only on
  clip `+0x6c`, byte-identical to `anim_clip_advance`, reads no RNG/clock → the horses ALWAYS
  trot), and **`:929-970` is the behaviour**, which `break`s out unless primary AND the global
  scene-lock `*(0x8a9b50+0x27a8)==0`, then draws the LCG for idle/wander (the deferred RNG
  layer #77). So the trot is portable in isolation. PORTED: `actor_render_state` gains the anim
  block `timer`(+0x70)/`done`(+0x74); **`actor_anim_advance`** (a thin adapter to the single
  ported stepper `anim_clip_advance`); **`actor_pool_update`** = the `0x46cd70:123-169` band
  walk (advance every active render-state with a clip — static actors no-op); `main.c
  game_actor_update` runs it on the SAME sim-tick gate as the camera easer (`hold & 1`), BEFORE
  `camera_follow_step` (retail `0x439690` order :1108→:1123), with a `CALL_TRACE_BEGIN(0x46cd70)`
  mirror. **LIVE (port blit trace, settled cam 12800, one 144-Flip cycle):** the wagon is **3
  keyed cels res `0x3ec`** (corrects ckpt-80's mis-noted `0x058f`) at x160/288/416; the body
  cel (x416) steps **5→2→3→4→5** every 36 Flips while the two fixed cels hold frames 0/1;
  `0x46cd70` mirror reports `advanced:1`/tick. **USER-confirmed** (horses idle subtly — ear
  flicks; the wagon is PARKED, not moving — "which is how it's supposed to be"; so `WAGON_CLIP`
  is an IDLE loop, not locomotion — quirk #82). **896 pass** (+3); ledger 199/194 unchanged
  (bare-VA slices). quirk #82; PORT-DEBT `actor-protagonist-clip` narrowed to the RNG behaviour
  + the cutscene roll-in. `findings/in-game-intro.md` "The horses TROT". **NEXT (corrected
  ckpt 82):** the code-adjacent "siblings" `0x1872e`/`0x1872f`/`0x18730` are **OUT-OF-SCENE**
  — `0x1872e`←`0x539e80` room 410240 (area 410), `0x1872f`←`0x5034b0` room 230110 (area 230),
  `0x18730` = child of non-town char `0x11350`; the town script `0x4d7d80` spawns ONLY the
  wagon. Code-adjacency ≠ same scene (quirk #83).
- **GROUND TRUTH (ckpt 82): the hold residual is the CHARACTER CAST + foreground TREE, NOT a
  "banner".** Re-captured the retail blit trace + a PNG at the scene-LOCKED establishing hold
  (flip 1500, cam 128000). The 108 keyed blits = **54 visible via present `0x48eac0`
  (`ret_va 0x48ecc2`)** — res `0x481` 320×320 = the foreground TREE + ~5-7 multi-part townsfolk
  CHARACTERS (banks `0x426`×5/`0x459`/`0x462`/`0x46a`/`0x46b`/`0x472`/`0x47b`) + props — and **54
  INVISIBLE via `0x4962a0`** parked at y=572 (off-screen, NO render-id; a HUD?). **No "Town of
  Tonkiness" banner blit exists at the hold** (zero `0x5a00c0`-range `ret_va`s → the docs' banner
  + the `0x5a00c0`-overlay producer are BOTH refuted, like the letterbox was). The cast is NOT
  the main map-object band (bank `0x16c`/`0x175`, off-screen-left at cam 128000) — it's the 8
  PARTY actors (`0x59f2c0`→`0x560e60`, `ret_va 0x59f578`) and/or a scene-actor band. Per quirk
  #82 the hold is scene-locked ⇒ deterministic. `findings/in-game-intro.md` "The hold residual
  is the CHARACTER CAST + foreground tree". **USER DIRECTIVE: get the intro scene 1:1 on EVERY
  frame, THEN pinpoint + port every RNG consumer in the scene 1:1.** That decomposes exactly:
  **Phase 1 (this residual)** = the character/multi-part static render (spawn + sprite tables +
  poses → the locked establishing frames go differ_px==0; generalise the `0x491ae0` arm past the
  wagon + the lazy `+0x48` fill, PORT-DEBT `actor-sprite-table`); **Phase 2** = every in-scene
  LCG consumer (`0x54f980:929+` wander, …), matched by rng+rngcalls both sides. **NEXT:** pin
  each cast cel to its actor (annotate the emit `0x492670`/the band-walk feeding `0x48eac0` with
  the actor code, recapture) → RE the cast spawn → port the multi-part render → verify. Also
  pending: byte-confirm the wagon via `render_diff` keyed on `(res 0x3ec, frame)`.
- **Prior (ckpt 80): the town intro `0x1872d` is PORTED + SPAWN-RE'd + WIRED + LIVE-VERIFIED —
  and it's the arrival WAGON, not "the protagonist" (corrects #79/#80).** Three parts: (1) **the
  render arm** — `actor_render_protagonist` ports `0x491ae0`'s case-`0x1872d` (a 3-cel composite;
  part 2 is byte-identical to `FUN_0044d160`/`actor_render_describe`, wrapped with two fixed
  bank-`0x175` cels @ x-256/x-128). (2) **the SPAWN, fully RE'd:** `0x1872d` is NOT a map object
  (code outside 70000) — it's spawned by the **town intro cutscene script `FUN_004d7d80`** (`case
  0x334be`=room 210110 / area `0xd2`, gated on event flags `0x5f76805`/`0x606aa4f`) →
  **`FUN_00431d10(0,0x1872d,anchor=0x65,x=0x3200,0,0)`** (the by-code `+0x11e0` spawn helper:
  free-slot scan + anchor-relative placement) → **`0x431e30` case-`0x1872d`** which installs sprite
  row 0 via **`FUN_00426db0(0,0x175,0,…)`** (the long-missing `+0x48` FILL PRIMITIVE, now RE'd —
  retires part of `actor-sprite-table`), clip `&DAT_00671c48`, layer 9, facing 99. (3) **WIRED +
  LIVE-VERIFIED:** `actor_spawn_protagonist` + the `game_actor_walk` dispatch → the port logs
  `8 nodes from 33 actors (bank 0x175 registered)`; a with-`0x1872d` vs no-`0x1872d` rebuild diff at
  the settled camera (cam 12800) **isolates exactly its pixels = a horse-drawn CARAVAN** (bbox
  x180-543), NOT a person → **`0x1872d` is the town intro arrival CARRIAGE** (**USER-CONFIRMED on the
  feed: "that definitely matches retail"**). The 3-cel composite = wagon-left | wagon-body |
  **HORSES**: the first render froze the body on frame 0 (redrew the wagon-left cel → "cut in half"),
  so decoded the clip **`&DAT_00671c48`** from the user's exe (`base_sprite 2, 4 frames, looping,
  delta {0,1,2,3}` → body cels 2..5 = the horses) and pointed the render-state at a reconstructed
  `WAGON_CLIP` → the body now draws sprite 2 = the horses. **893 pass** (+4); ledger 199/194 unchanged
  (bare-VA slices). quirk #81; PORT-DEBT `actor-protagonist-clip` (the horses are FROZEN on frame 2 —
  the per-tick stepper that trots them + the cutscene roll-in are deferred).
  `findings/in-game-intro.md` "The 0x1872d SPAWN + the arrival WAGON". **NEXT:** the per-tick anim
  (trot the horses) + the scripted roll-in; then the caravan's siblings `0x1872e`/`0x1872f` (likely
  the characters — spawned by `0x539e80`/`0x5034b0`); byte-confirm via `render_diff` (res `0x3ec` —
  ckpt 81 corrects the `0x058f` here from the live blit trace).
- **Prior (ckpt 79): the town CHARACTER band is RE'd, SPAWNED, RENDERED + WIRED — and it's
  mostly PROPS, not NPCs (USER-confirmed live).** Per the methodology ("capture each slot's
  `+0x48` live"), extended the `0x491ae0` field spec + captured every active `+0x11e0` actor at the
  town hold (flip 1480/1500/1520, `--seed-pin --lockstep`). **Census (corrects #78/#79):** of 33
  main-band actors, **27 are INVISIBLE** (all-zero `+0x48` → self-skip; collision/trigger/spawn
  volumes — the `0x111d6`/`0x112e6`/… physics-body codes), and **only 6 DRAW** — `0x1129e`×3 /
  `0x1129f` / `0x112e5` are **static PROPS** (bank `0x16c` = town-objects sheet res `0x403`: a
  barrel, the fountain — NOT people), + **`0x1872d` the animated protagonist** (bank `0x175`, the
  one PERSON; OUTSIDE the 70000 range = a SEPARATE spawn; needs the `0x491ae0` multi-part arm —
  **the bulk of the 36-blit residual**). Corrections landed: `0x426620` **ZEROES** `+0x48` (its
  `type*0x80+0x21c04` is the **collision-grid** lookup #79 misnamed); the sprite table fills
  **LAZILY** (`0x40afe0`/`0x41e600`, type-keyed def table, un-RE'd); and the prop offset from
  `map_x*100` is **DETERMINISTIC per-code (NOT RNG)** — the fountain `0x112e5` is `+0/+0` and
  matches retail exactly. **PORTED + WIRED + LIVE-VERIFIED:** `src/actor_spawn.{c,h}`
  (`actor_spawn_from_map`: 32 CHARACTER objects → `{actor,render-state}` at `(x,y)*100`, the 3
  prop rows from the captured stand-in, PORT-DEBT `actor-sprite-table`) + `town_render_step_ex`
  actor seam + `main.c` (`game_actor_walk` → `actor_render_static`, `game_cel_dims` cull,
  `game_present_blit` `PRESENT_KEYED` → `zdd_object_blt_keyed`). The port logs `5/32 actors
  emitted (bank 0x16c registered)` and the props render at the correct spots (USER-confirmed on
  the feed). **889 pass** (+6); ledger 199/194. quirk #80; `findings/in-game-intro.md` "The town
  actor RENDER CENSUS". **NEXT:** the `0x1872d` protagonist multi-part animated arm (the actual
  person + the bulk of the 36 residual) as its own arc; then `render_diff` vs retail flip 1500.
- **Prior (ckpt 78): the town actor SPAWN is RE'd + BYTE-VERIFIED — no live drive needed**
  (unblocks the ckpt-77 ported renderer; docs-only, 883 pass unchanged). **The chain**
  (corrects the ckpt-76 guess `0x42eb20`/"`0x587e00` layer pass"): `0x586010:698` →
  **`FUN_0058d460`** (room object-population pass) → **`FUN_00431e30`** (character activator).
  `0x58d460` walks the map's **86 object-placement layers** (`mapobj+0x38` headers, `+0x10`=type
  code, `+0x04/+0x08`=x/y) and dispatches each by **type RANGE** into four pre-alloc bands off
  `DAT_008a9b50` (EFFECT 50k→`+0x1160`, STRUCTURE 60k→`+0x2560`, **CHARACTER 70k→`+0x11e0` via
  `0x431e30`**, DEVICE 80k→`+0x13e0`; each a `"<kind> Object Count Over"`-guarded free-slot scan).
  `0x431e30` (thiscall, ECX=slot) per-type switch: `+0x1d0=1`/`+0x1d4=type`/`+0xfc=9`/`+0xe8=0`,
  zeroes `+0x48` sprite table, stores world (x,y); a helper (`0x426620`) installs the sprite from a
  def table (`type*0x80+0x21c04`). **The proof** (resolves "codes never assigned as constants"):
  the town codes ARE the map object type fields — `map_data.py --objects` on DATA 1022 → 15 effect
  + 39 structure + **32 character** + 0 device; the 32 char codes + multiplicities are IDENTICAL to
  the ckpt-76 live census (0x112e6 ×10, 0x111d6 ×7, …), with world positions. The 33rd live actor =
  the 1 animated NPC (`0x1872d`, separate path). `docs/proofs/map-object-layer-format.md`; quirk #79.
  **NEXT (the port):** the **code → `+0x48` sprite-table** mapping (the only datum not in the map —
  `0x431e30`'s def-table install; RE the 13 town codes OR capture each slot's `+0x48` live) → minimal
  spawn (32 objects from `map_data` → render-state pos + sprite + dir + layer 9) → wire into
  `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual drops) → human pixel-verify.
  `findings/in-game-intro.md` "The town actor SPAWN".
- **Prior (ckpt 77): the town ACTOR RENDER SIDE is PORTED + host-tested** (the default arm
  that draws 32/33 town actors), ahead of the spawn. Pure, no harness.
  **Ported (commit `0533603`):** `draw_pool_emit_actor` = `FUN_00492670` (the actor analog
  of `draw_pool_emit`; node mode = `bool(alpha!=0)`); **`actor_render.{c,h}` (NEW)** —
  `actor_render_describe` = `FUN_0044d160` (the static/animated/mirrored/angle sprite
  descriptor over the per-direction table `actor+0x48`) + `actor_render_static` = the
  `0x491ae0` **default arm** (`caseD_11257`: 32/33 town actors hit it); `map_present`
  **MODE 0** (the opaque-actor keyed path `FUN_005b9b70`, cull dims from the cel via a new
  `present_dims_fn`). actor + render-state are LOGICAL structs (the spawn fills them);
  `actor_sprite_row` (0x14) pinned. **Validated:** the render-state offsets match the ckpt-76
  live `0x491ae0` field spec exactly (`rs_x`/`rs_y`/`rs_clip`/`rs_frame` = +4/+8/+0x6c/+0x72);
  logic host-tested bit-exact. **883 pass** (+18); ledger 199/194. **NEXT (the gating arc —
  needs the harness, then the human for pixel-verify):** the **SPAWN** (the `+0x11e0` band
  activator — NOT `0x560e60`/`0x584710`; it's the entity subsystem `0x42eb20`/`0x4282f0`/…
  over the DATA 1022 layer entries) → the `0x1872d` animated arm (the 1 key NPC) → **wire**
  the band walk into `game_render` → `render_diff` vs retail flip 1500 (the 36-blit residual
  drops). PORT-DEBT `present-actor-modes` (narrowed: mode 0 done, wiring blocked on spawn) +
  `actor-occlusion`. `findings/in-game-intro.md` "The town ACTOR render side".
- **Prior (ckpt 76): the town NPC/actor RENDER PATH is RE'd LIVE + the trace tooling hardened.**
  User: "implement the NPCs / consult the runtime trace / harden + document the trace tooling."
  Did the RE + instrumentation half (render-side port follows). **Tooling:** added the reusable
  **`thischain`** field source (ECX-rooted pointer hops — probes any actor by its live `__thiscall`
  `this`) + **annotated** `0x491ae0` (actor render entry), `0x560e60`, `0x584710` in
  `retail_fields.json`. **RE (live, retail town hold flip 1500):** the MAIN actor band is
  `DAT_008a9b50+0x11e0` (0x80 slots, rendered by `0x491ae0`, updated by `0x54f980`; one of six
  bands `0x48c150`/`0x46cd70` walk). **33 active actors: 32 STATIC** (clip==0), **1 animated**
  (`0x1872d`). **32/33 behaviour codes fall through to `0x491ae0`'s DEFAULT arm → `FUN_0044d160`**
  (the static-actor descriptor) → `0x492670` emit into the draw_pool as **mode 0/1** (= the
  deferred PORT-DEBT `present-actor-modes`).  The code drives the AI (`0x54f980`), NOT the render —
  **one function draws the town**.  Render banks res `0x403`/`0x426`/… = the ckpt-75 36-divergence
  residual.  **Band is a PRE-ALLOCATED 128-slot pool** (`0x586010:487` `FUN_0058cf60(0x40)`×128);
  the per-room **spawn = ACTIVATE+configure**, running after `0x586010`'s `"Init Objects"` marker —
  a **data-driven entity-by-id** subsystem (codes never literal; NOT `0x560e60`=8 party / NOT
  `0x584710`).  **NEXT:** find the `+0x11e0` activator (hook post-"Init Objects"), then port the
  render side (`FUN_0044d160`+`0x492670`+present 0/1) + wire + pixel-verify vs retail flip 1500.
  865 pass (no C touched); engine-quirk #78; `findings/in-game-intro.md` "The town ACTORS".
- **Prior (ckpt 75): the establishing-shot cinematic LETTERBOX is PORTED + blit-trace 1:1.**
  RE'd the producer from the captured retail blit trace: it's NOT the `0x5a00c0` overlay but
  **`0x48c150:124-162`** (the per-frame world driver), two grid-fill loops that tile a 64×4
  opaque cel (res **`0x583`** = main-pool slot 41) across the screen — BOTTOM bar
  (`in_ECX+0x44` rows, ret `0x48c48a`, dy 416-476) then TOP bar (`in_ECX+0x48` rows, ret
  `0x48c4fe`, dy 0-60), both 64 → the quirk-#74 black bars (rows 0-63 + 416-479), 640×352
  cinematic window. **Ported `letterbox.{c,h}`** (the grid-fill, host-tested 4 + bit-exact
  vs the trace) + `main.c game_letterbox_blit` (resolves slot 41 frame 0, `zdd_object_blt_onto`,
  drawn after the present). **Re-diff: the town-frame divergences dropped 356→36** — all 320
  `0x583` blits now match retail on identity+geometry+state; the 36 left are exactly the
  deferred RNG-driven actor/banner/tree banks. **Port frame pixel-verified** (rows 0-63 +
  416-479 `(0,0,0)`, row 64 = sky), USER-CONFIRMED on the feed. 865 pass; parity-ledger #8;
  PORT-DEBT `ingame-letterbox` (the 64/64 heights stand in for the unported `0x5a00c0` op
  that writes `+0x44`/`+0x48` — the geometry is bit-exact). **Next chip: the "Town of
  Tonkiness" BANNER + foreground TREE (`0x5a00c0` overlay player).**
- **Where we are (ckpt 73): the actor-band residual is PINNED to the RNG pillar — and the
  shared LCG stream is NON-DETERMINISTIC run-to-run EVEN UNDER `--seed-pin`.** Ran the ckpt-72
  directed live check: drove retail **twice** (`--seed-pin --lockstep --no-turbo`, same
  in-game trace), snapshotting the LCG state `DAT_008a4f94` at the per-sim-tick actor-update
  boundary (`0x46cd70`, new `rng` field). **Result: `rng` matches 0/8643 in-game
  sim-ticks** — the stream is at a different phase every tick despite the pinned seed and the
  deterministic sim-tick index. **Smoking gun:** at `prologue_enter` BOTH runs are on the
  IDENTICAL flip 946 yet rng differs (`0x84654e6f` vs `0xa79a2d6e`) → at the same flip the
  engine has drawn a *different number* of LCG values. Mechanism: a consumer draws per-PRESENT
  and the presents-per-sim-tick count is non-deterministic (quirk #75), so the stream phase
  desyncs and never re-converges. Since `0x54f980`'s behaviour cases draw this exact LCG
  (`FUN_005bf505`, ~40 sites: idle waits `+0x5c`, the idle→wander branch pick, move offsets →
  `0x450ef0`), the actors pick different waits/dirs/positions run-to-run = the #75 ~6.7k-px
  band. **CONCLUSION:** an RNG-reading subsystem needs its OWN **RNG anchor** (snapshot/restore
  `DAT_008a4f94` at the game_enter sim-tick, both sides) — the camera's `g_sim_tick` anchor is
  insufficient for it (works only because the camera reads no RNG). Parity bar for the actor
  band = "data-1:1 given a matched RNG state" (retail-vs-retail isn't observed-1:1 here).
  (The `a0_clip/a0_frame` fields matched 8643/8643 but TRIVIALLY — main-band slot 0 was inert
  the whole run; the `rng` divergence is the real result.) Tool: `tools/rng_tick_diff.py`.
  Engine-quirk #77; `in-game-intro.md`. **DIRECTION (user): defer ALL RNG-order parity**
  until every in-scene RNG consumer is RE'd, then match consumption order (rng+`rngcalls`
  both sides — the flow trace now carries `rngcalls`, the unified consumption signal,
  openrecet-style; commit `4c587c0`). **Next chips = implement the scene's VISUAL elements**
  (LETTERBOX #74 → `0x5a00c0` banner/tree → NPC render/spawn); RNG behaviour parity comes after.
- **TOWN FRAME DIFFED via the new blit trace (ckpt 74) — the port's backdrop is PIXEL-FAITHFUL;
  the gaps are MISSING layers, pinpointed.** render_diff (hold: port 1200 ↔ retail 1500, both
  cam=128000) → 606 retail / 250 port blits, **356 divergences ALL `[sprite]` (missing), ZERO
  `[rect]`/`[decode]`/`[state]`, ZERO port-extra** — every port blit matched retail on identity
  + geometry + state. The missing draws: **320× bank `0x583`** (a 64×4 full-screen grid, frame 0,
  deterministic, `dy=416` = the letterbox row → the **establishing-shot cinematic overlay**,
  quirk #74 — **PORTED ckpt 75, see LATEST above; the 320 blits now match retail**) + ~36 actor/overlay banks
  (`0x426`/`0x403`/… NPCs + tree + banner — RNG-driven, accepted-divergent until the scene RNG is
  RE'd). `findings/ddraw-blit-trace.md` "The TOWN frame".
- **Prior (ckpt 72): the ACTOR ANIMATION cycle RE'd + the frame-stepper ported — rides the
  sim-tick clock, no separate counter.** The per-tick UPDATE pass (`0x439690:1108`→`0x46cd70`
  once/tick→`0x54f980` per actor) runs one byte-identical inline stepper on the render-state
  anim fields (`+0x6c` clip/`+0x70` timer/`+0x72` frame/`+0x74` done): `timer++`; at `>=clip.dur`
  → `frame++`,`timer=0`; at `>=clip.count` → loop or one-shot hold. Clip = a fixed **0x154-B
  32-frame** descriptor, (re)set on STATE CHANGE (`0x40afe0`/`0x41e600`). **PORTED (host-tested
  bit-exact, 854 pass): `src/anim_clip.{c,h}`.** The stepper reads no GetTickCount/Flip/RNG → it
  is deterministic under the camera's `g_sim_tick` anchor; ckpt 73 then proved the leftover band
  diff is the RNG-driven BEHAVIOUR, not the stepper. Engine-quirk #76.
- **Prior (ckpt 71): TIMESTEP DETERMINISM established — the SIM-TICK is the only
  valid frame-of-reference; the "house off by 3px" was FLIP-misalignment, not a bug.**
  The in-game sim is a wall-clock GetTickCount frame-limiter (`FUN_00439690:776-859`): one
  logical sim tick per outer iteration (easer `0x43d1d0` once at `:1123`) but a VARIABLE
  number of Flips per tick → **the Flip index is non-deterministic run-to-run** (two identical
  retail runs disagree on 47-86% of flips by ≤3px; `--lockstep-epsilon-ms 0` is worse, so it's
  intrinsic, not the epsilon). The **sim-tick index** (easer-call count) is bit-identical.
  The user's whole-foreground 3px trail (background Δ0) is the signature of flip-misalignment
  — the 0.5×/0.25× parallax hides the same camera offset the 1× foreground exposes; the tile
  math is provably identical at equal `cam_x60`. **FIX (committed):** the agent counts easer
  calls (`g_sim_tick`), tags every captured frame (`frame_<flip>_t<simtick>.png` + manifest)
  + call-trace event, and RESETS the counter at the `game_enter` scene-load anchor (synchronize
  at every non-deterministic load) → cross-run deterministic (99 ticks, 0 cam-mismatches; pan
  starts at tick 92 both runs). `tools/sim_tick_diff.py` matches two run-dirs by sim_tick/cam
  (dx=0) vs flip (the 3px trail). Engine-quirk #75; `in-game-intro.md` "Timestep determinism".
  **DECISION (user):** anchor each subsystem for determinism rather than a global timestep
  hack (fallback if it gets unwieldy). The camera is synced (sim-tick); the actor anim cycle is
  now RE'd + ported (ckpt 72 above — it rides the same sim-tick clock, no new pin needed). The
  intra-tick-identical observation is now explained: the stepper reads no flip/clock/RNG.
  Standing rule: never diff on the Flip index — anchor on the sim tick. NB `--turbo` is NOT faster in-game (Frida/LAN overhead dominates, not Sleep) and
  breaks the no-turbo-timed input traces; use `--no-turbo --lockstep` until traces are re-timed.
- **Prior (ckpt 70): the intro-PAN camera is WIRED LIVE — the town now pans.**
  `main.c game_render` steps a live `camera_view` each frame (`camera_follow_step` =
  `FUN_0043d1d0`, with the `CALL_TRACE_BEGIN(0x43d1d0)` flow-trace mirror) and projects the
  backdrop through its *current* scroll instead of the static const. `enter_game`
  spawn-snaps it (`camera_apply_snap` → cur=tgt=128000/12800); a hold timer fires the
  scripted pan (`camera_apply_pan` → tgt=12800/12800, speed 300) at hold-end. The two
  target-setters are bit-exact ports of `0x439690`'s SNAP/PAN command arms (`:599-664`),
  host-tested (clamp to `[0, map-vp]`; snap-jumps-cur / pan-keeps-cur). **Visually confirmed
  on the feed:** hold (cam x=128000) → mid-pan → settled (cam x=12800, town left edge).
  **848 pass / 0 fail / 6 skip** (+2). Also added `MAP_RENDER_CAM_TOWN_3F2_SETTLED` (x=y=12800).
- **CADENCE + TRIGGER measured → the pan is now TRAJECTORY-1:1 (ckpt 70b).** A retail
  field-spec trace (`--seed-pin --lockstep --no-turbo`, easer `0x43d1d0` + Flip hooked,
  contiguous Flip whitelist) pinned both stand-ins to ground truth: the easer fires **once
  per 2 Flips** (the sim runs at half the Flip rate; `cam_x60` is a STEP function, −300/2flips
  at cruise) and the pan command fires at **`game_enter + 184` Flips** (Flip 1616 HOLD, 1617
  PAN). `game_camera_step` now gates the sim to every 2nd frame (`hold & 1`), trigger at
  `GAME_CAMERA_HOLD_FRAMES=184`. **The port now passes through the IDENTICAL `cam_x60`
  sequence as retail** (128000,127990,127970,127940,…,cruise −300/2flips — verified by
  diffing the captured `0x43d1d0` mirror). **RESIDUAL (PORT-DEBT `ingame-camera-pan`):** a
  ~2-3 Flip startup-jitter PHASE offset (retail's sim accumulator is wall-clock-paced — a
  4-Flip plateau at 1618-1621 a clean 2:1 step can't reproduce; ≤1 step ≈ 3px, transient,
  zero at hold+settled) + the cutscene-script TRIGGER source — both downstream of the
  in-game sim / `0x5a00c0` port.
- **Methodology (reinforced ckpt 69):** "annotate" = the **flow-trace field spec**
  (`retail_fields.json` named functions+fields + port `CALL_TRACE_BEGIN` mirrors) — a CORE
  step of finishing any RE/port; thiscall/struct tagging is a SEPARATE static-readability
  lane. Never an ad-hoc symbol-rename. (CLAUDE.md "Annotate as you RE".)
- **Prior (ckpt 68): 24bpp parallax LUT grade LANDED — sky colour USER-CONFIRMED.**
  Found retail grades the 24bpp sky/mountain banks (`0x55`/`0x58`/`0x59`) at **DECODE**, not via
  the palette path (`0x417c40` early-exits to the plain getter when a bank has no palette): its
  **flag-3 branch** (the 24bpp case) stamps the slot's brightness descriptor (`f_08=1`, scales
  1000 = tint case 0, `f_18`=tone-curve LUT) before the getter, and the lazy `ar_sprite_decode`
  runs `ar_sheet_decode_pixels` (already ported, quirk #46). The port's parallax sink used the
  plain getter so never stamped it → sky decoded raw/too-bright. **Fix:** `game_arm_parallax_grade`
  in `main.c` replicates the stamp in `game_parallax_blit`. Verified: raw sky `(66,150,255)`→LUT
  →565 = `(33,125,239)`; **blue `239` matches retail's main sky band exactly**, and the user
  confirmed the grade looks correct on the feed. (The old finding's raw `(132,186,255)`/retail
  `(103,165,231)` numbers were wrong — actual raw is `(66,150,255)`.) **OPEN (deferred):** a
  "dark top gradient" the user sees in the establishing-scene frame (but not in settled gameplay)
  — likely a **per-scene CINEMATIC effect tied to the establishing shot**, to be confirmed by
  probing ground truth alongside the intro PAN RE.
- **Prior (ckpt 67):** backdrop TILES `differ_px==0` via the 8bpp palette grade (`color_grade.{c,h}`);
  the "establishing shot" proven a leftward **PAN not a zoom** (only `+0x60` moves; dx=0, same
  scale; `MAP_RENDER_CAM_TOWN_3F2`). **840 pass / 0 fail / 6 skip.** Ledger **194/1490 touched /
  189 tested**. Both GUI builds clean.
- **NOT a full `differ_px==0` frame yet — named residuals** (NOT logic bugs): the **NPC actors**
  (present modes 0/1/2, blocked on the entity/spawn system — PORT-DEBT `present-actor-modes`); the
  **foreground tree** + **"Town of Tonkiness" banner** (`0x5a00c0`, PORT-DEBT `ingame-nontile-layers`);
  the intro **pan** is wired + cadence/trigger-matched (ckpt 70b) — it passes through retail's
  exact `cam_x60` sequence; residual is a ~2-3 Flip startup-jitter PHASE (PORT-DEBT
  `ingame-camera-pan`), zero at the hold + settled ends (`MAP_RENDER_CAM_TOWN_3F2_SETTLED`, x=y=12800).
- **PAN BACKDROP DIFF DONE — verified pixel-1:1 (ckpt 70b).** Captured fresh retail pan frames
  (`--no-turbo --seed-pin --lockstep`) + their `cam_x60`, matched port frames by `cam_x60` (port
  Flips 1304/1344/1384/1422/1462 ↔ retail 1617/1660/1700/1740/1780, shared cam 127990/125690/
  120050/114350/108350), and diffed: the **backdrop is Δ0** (shift-search peaks sharply at
  `dx=dy=0`; pan-start `x=80` column all Δ0). The remaining diff is the **named missing layers
  ONLY** — exactly the signal we wanted. NEW retail ground-truth (quirk #74): the establishing
  shot is **LETTERBOXED** (solid-black bars rows 0-63 + 416-479, a 640×352 cinematic window; the
  "dark top" the user saw, with a matching bottom bar). Parity-ledger entry #7.
- **Next move (the named layers, simplest first):** (a) the **cinematic LETTERBOX** (quirk #74)
  — **DONE ckpt 75** (`letterbox.{c,h}`, the `0x48c150:124-162` grid-fill; 356→36 diff); (b) the **"Town of Tonkiness" banner** + **foreground tree/veg**
  (`0x5a00c0` scripted-scene overlay player — also where the pan TRIGGER source lives, closing
  `ingame-nontile-layers` + the trigger half of `ingame-camera-pan`); (c) the **actor renderers**
  (present modes 0/1/2, need the entity/spawn system first). Writeups: `findings/in-game-intro.md`
  "The pan CADENCE + TRIGGER measured" + the diff verification; quirk #74.
- **Tooling front (ckpt 74): the DDraw BLIT TRACE landed + cross-side-verified — we now have
  the two-drill-in coverage the user asked for.** `render_diff` names the wrong DRAW (and how:
  `[sprite]`/`[decode]`/`[rect]`/`[state]`); `flow_diff` names the wrong LOGIC. B3 (`docs/plans/
  trace-tooling-phase-b.md`, `findings/ddraw-blit-trace.md`): `src/render_id.{c,h}` is the
  cross-side identity — a cel→`(resource_id, frame)` registry (openrecet's `tex_name` trick:
  drop the alloc-dependent pointer, key on the load-stable asset name) **plus `dhash`**, an
  FNV-1a fingerprint of the DECODED sheet pixels (the improvement over openrecet's name-only
  scheme — a software blitter has the pixels in CPU mem at decode, so it catches RIGHT sprite /
  WRONG decode, the palette/24bpp residual class). Port emits at the 5 blit primitives
  (`zdd_emit_blit`); the Frida agent mirrors it (resolver `0x418470` registry + the new
  `renderid`/`thisderef` field sources, each auto-installing — no ad-hoc flag). **LIVE-VERIFIED:**
  retail emits the IDENTICAL `resource_id` (0x91b) as the port for the title background; ECX/arg
  reads correct; render_diff named all 59 title-phase blits by identity. Next layers: retail-side
  decode-hash (so `[decode]` fires cross-side), the cdecl `0x5bd550` retail spec, a same-scene
  aligned in-game diff. AGENT-WORKFLOW.md trimmed to subagent-only (process audit). 813 pass.
- **Prior tooling: Phase-B B2 (the field-bearing flow trace) LANDED + live-verified**
  (`docs/plans/trace-tooling-phase-b.md`): `call_trace` carries `seq` + `CALL_TRACE_BEGIN/FIELD/END`;
  the Frida agent reads same-named retail fields per `tools/flow/retail_fields.json` (now incl.
  the `cam_*` camera chain + the ckpt-67 `tint`/`lutgate1/2`/`lut*` colour-grade probes +
  the ckpt-69 `camera_follow_step` producer entry);
  `tools/flow_diff.py` names the first `[chain]`/`[data]` divergence. Remaining Phase B: **B1**
  unified `scenario-test.py`, **B3** DDraw blit-command trace + `render_diff.py`.
  **`mem_watch.py` (ckpt 69):** now resolves **chain heap addresses**
  (`--watch-chain ROOTVA:HOPS:OFF:SIZE[:LABEL[:ARM_AT_FLIP]]`) + a **`--hw` hardware
  watchpoint** (frida-17 per-thread DR) — the fitting tool for a hot heap field (found the
  camera easer through its heap fn-pointer dispatch in one run).
- **Standing bar:** every divergence is `differ_px == 0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic
  (`parity-model.md`); seed-pinned both sides, compared by anchor/tick.

> Hand-edited in `docs/FRONT.md`, injected here verbatim so it can't drift.

## Where to read next

- `STATUS.md` (this file) — 60-second orientation.
- `../CLAUDE.md` — the dense auto-loaded entry point (conventions, parity model, paths).
- `FRONT.md` — the hand-edited current front (source of the block above).
- `HANDOFF.md` — rolling current-checkpoint detail (module layout + open threads).
- `parity-model.md` — the multi-pillar model; `parity-ledger.md` — confirmed-1:1 guard.
- `port-debt.md` — synthetic/MVP shortcuts owed back.
- `port-ledger.md` / `.json` — per-function port status (derived).
- `port-frontier.md` — what to port next: unported fns reachable from ported
  code, zero-dep leaves ranked (derived; `tools/gen_frontier.py`).
- `ROADMAP.md` — milestones + subsystem map with difficulty / target module.
- `PROGRESS.md` — dated narrative changelog.
- `findings/INDEX.md` — map of subsystem RE writeups.
- `findings/engine-quirks.md` — the running quirk log (cite in commits).
- `AGENT-WORKFLOW.md` — when to spawn subagents vs stay in the main loop.
- `PLAN.md` — goal, constraints, phased roadmap.
