<!--
  The ONE hand-edited status block.  tools/gen_port_ledger.py injects everything
  between the FRONT:BEGIN/END markers verbatim into docs/STATUS.md's "Current front"
  section, so STATUS can never drift from reality.  Update THIS when the active front
  moves; keep it a 60-second read.  Everything else in STATUS is derived from code.
  Port state / next-move detail belongs here or in HANDOFF.md, NOT in engine-quirks.md.
  Rolling per-checkpoint narrative → PROGRESS.md; deep RE → findings/.  Keep this SHORT.
-->
<!-- FRONT:BEGIN -->
- **Phase — the freeroam sim**, built bit-exact vs errands re-drives. The prior arcs
  (town intro → arrival/house/errands cutscene; the HUD panel ckpt 167–174) render 1:1.
- **Landed ckpt 175 — COLLISION:** the movers `0x54ded0`/`0x54db10` tile halves ported
  (`collision.c`) + `character_step` restructured to the retail `0x442a70` tick order
  (support probe → vertical mover/ledge-fall → ramp → worldX commit); slope ramps
  read live off the user's exe; the `LAB_00589520` "occlusion marks" = invisible
  collision WALLS (map_decode fix). **Arche stops/climbs/descends the errands stairs
  tick-exact** (dwx==dwy==0 through the whole climb; both walls flush) — retires the
  studio-note-2441 divergence + debts `char-collision-mover`/`collision-slopes`/
  `decode-occlusion-mark`. The 4-tick standing TURN-AROUND (`char-turn-state`) is
  now PORTED (ckpt 177 below). RE: `findings/freeroam-collision.md`.
- **HUD blocker (parked) — the PORTRAIT.** `hud_ctx+0x1b4` (leader_uid) reads 0x0 on
  every scripted replay (a replay-fidelity gap, not a port bug); resolve via the
  `+0x1b4` setter or a live/manual play. `findings/freeroam-hud.md §6-9`.
- **Landed ckpt 176 — dialogue mark t1197 FIXED + char-turn RE corrected.** (a1) The
  "port skips dialogue early" mark is NOT an advance-gate bug — the advance PRESSES
  already match retail (port DLGT + dialogue_timeline off the stairs pair).  It was a
  box RENDER linger: retail keeps the arrival L9 box up through tick 1200, the port
  cleared it at 1192 → `ARRIVAL_EXIT_BOX_HOLD=8` (== the house-exit box-hold pattern),
  drawcall-verified L9 adv 1192→1200 == retail.  `findings/dialogue-advance-early.md`.
  (c) char-turn RE CORRECTED: the ckpt-175 "case-2" pointer was WRONG (that's the
  DOWN/crouch, already ported); the real reversal turn is the STATE-1 horizontal FSM
  `0x442a70:1011-1090`.  See `port-debt.md`.
- **Landed ckpt 177 — `char-turn-state` PORTED (retires the collision residual).**
  The from-rest REVERSAL now plays retail's 8-tick pivot: `CHAR_TURN_HOLD=4`
  STATIONARY windup ticks (facing HELD, the fr-6 turn cel) → flip facing + walk ramp
  (fr-7 → +152 fr-159 lingers 4 render ticks) → walk (fr 160).  `character.c`
  `turn_ctr`/`turn_frame` + `ARCHE_TURN_CLIP` {6,7} (both dirs via the +152 mirror,
  == the house turn 158→7).  GROUND TRUTH retail-stairs res 0x570 (fr 6×4 → 159×4 →
  160); `draw_probe port-stairs2` == retail, `state_diff` RIGHT 0-div/301t (no regress)
  + LEFT ramp-shape bit-exact (the −960 gap GONE).  RESIDUAL: a constant 1-tick
  reversal-ONSET phase (−240) — the port latches the reverse press 1t before retail's
  warmup gate (→ `char-input-autorepeat`, not the turn), within retail's ±1-2t
  coalescing slop.  RE: `findings/freeroam-turn-around.md`.  **USER: verify the pivot
  animation — studio shortcut = `port-stairs2 | retail-stairs` @ tick ~2950.**
- **Landed ckpt 178 — ALL-MAPS SWEEP: the 587e00 tile dispatch is COMPLETE.**
  `tools/extract/map_sweep.py` (all 376 map resources vs the port's own sources):
  tile-id cell coverage 5.0% → **100.0%** (87/87 ids; every family transcribed
  emit-for-emit; quirks #119-#121).  The trailing `FUN_0058cb30` placeholder pass
  is PORTED (map codes 90010/90011 → region-E link anchors — NOT spawns).
  STRUCTURE def table complete (0xf295→0x77; 0xeead→the runtime bank 0x88/89/8a
  per param_4 5/6/8, RE'd off the 587e00 prologue).  res_explorer: PLACEHOLDER
  band named + region-E inspect; CHARACTER label kept (the ENGINE's own band
  name — holds props/emitters too, answers the USER naming question); param_4
  cfg 1-8.  Maps 1057/1047 render fully (feed); town regression clean.  1095
  host tests.  Still stubbed: EFFECT/CHARACTER sprite tables beyond the town
  captures, the decode-prologue header/palette installs, the region-E spawn
  CONSUMER (next link for (b) below).
- **Landed ckpt 179 — the arrival→house dialogue DRIFT FIXED (`ARRIVAL_EXIT_WAIT` 10→20).**
  The chain played −11t early because the arrival-exit WAIT was curve-fit to 10 (a bogus
  "2× anomaly") when it's the script's `0x14=20` mapped **1:1**.  The scene_fade PERFORMER
  is bit-exact (settle rates match) — the "settle-rate" hypothesis is DISPROVEN; the old
  "room-load black-hold" read was a CONFOUND of the −11t global shift.  The "2×" was a
  measurement error: ckpt-137 timed "first CENTER alpha" but retail's cover grows TOP-DOWN
  (sweep, not center-out), so the center lags the arm.  GROUND TRUTH off `retail-stairs.osr`
  (res-1110 box + 64×4 fade coverage; retail fades capture as res=0 subtract-blend, no
  alpha split): house L0 box open −11t→**−1t**, **house box close now tick-EXACT (1650)**,
  errands −20t→−6t.  `findings/dialogue-advance-early.md` "Component 2/3 RESOLVED"; quirk
  #122; 1095 host pass.  **USER: verify the transition — studio `port-waitfix | retail-stairs`
  @ ~1200-1350 (the arrival→house cover/reveal); feed `waitfix_cmp.png`.**
- **Landed ckpt 180 — the errands t2278 mark DIAGNOSED + the kitchen CABINET FIXED.**  The
  "missing POT" is a `port-waitfix` STALE-TRACE artifact: the pot (res1074) renders BIT-EXACT
  in the walk-aligned `port-stairs2` vs retail-stairs @t2278 (proof: port-stairs2==retail
  bit-for-bit — pot 376/376, wall-shelf 108/108, Arche 270,296).  The ckpt-179 waitfix
  desynced port-waitfix's tick-keyed freeroam walk (+176px camera lag, 4 props measured) — so
  **the freeroam studio pair is `port-stairs2 | retail-stairs`, NOT port-waitfix** (waitfix is
  valid only for the dialogue window ≲tick 2000).  The real gap = the kitchen CABINET (res1023
  fr4 + fr2 upstairs hutch, DATA-1025 CHARACTER code 0x112d1 map layer[18]/[31]) — FIXED as 2
  ERRANDS_CAST entries (world 70400,25600 / 70400,6400; verified in `port-cabinet.osr`: dims
  78×120/82×115 == retail, 172px offset == map, z pre-structure).  `findings/errands-render-gaps.md`
  §6.  **USER: verify the cabinet — studio `port-cabinet | retail-stairs`, scrub to the camera
  CLAMP (~t2400+, both pinned at the right edge; the cabinet at screen ~256).**
- **Next move (the errands −6t residual + the 2nd USER mark):**
  (a2') **errands entry −6t (OPEN, smaller)** — the house dialogue is now tick-exact, so
  the residual is entirely the house→errands transition (house-close 1650 → errands-open
  port 1693/retail 1699).  `HOUSE_EXIT` has no preceding WAIT, so it's the errands ENTRY
  reveal arming ~6t early (main.c arms on chain-complete, no errands room-load latency →
  `PORT-DEBT(cutscene-errands-entry-latency)`).  Measure the house→errands cover/reveal
  envelope off retail-stairs before adjusting — do NOT curve-fit.
  (b) **house props (mark t2278) — DIAGNOSED + the cabinet FIXED (ckpt 180).**  The POT
  (res1074) is NOT missing: it renders BIT-EXACT in the walk-aligned `port-stairs2` vs
  retail-stairs @t2278 — the "missing" was a `port-waitfix` STALE-TRACE artifact (the
  ckpt-179 waitfix desynced the tick-keyed `synth-stairs` held-walk → +176px camera phase
  lag).  The real gap = the KITCHEN CABINET (res1023 fr4 @280,96 + fr2 upstairs hutch),
  errands-map (DATA 1025) CHARACTER code 0x112d1 map layer[18]/[31] — FIXED as 2 ERRANDS_CAST
  entries (world 70400,25600 / 70400,6400; verified 172px offset == map).  (region-E is
  WAYPOINTS, not props — the old "prop spawn consumer" framing was wrong.)  **⇒ freeroam
  studio pair = `port-stairs2 | retail-stairs`, NOT port-waitfix (stale past ~tick 2000).**
  `findings/errands-render-gaps.md` §6.  Still open at t2278: Mom's pose clip.
  (c) `char-turn-state` — **DONE ckpt 177** (`findings/freeroam-turn-around.md`).
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`, `char-input-autorepeat` (the residual
  1-tick reversal onset); HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).
<!-- FRONT:END -->
