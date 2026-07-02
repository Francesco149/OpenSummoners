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
  `decode-occlusion-mark`. Sole residual: the 4-tick standing TURN-AROUND
  (`char-turn-state`). RE: `findings/freeroam-collision.md`.
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
- **Next move (the residual dialogue drift + the 2nd USER mark):**
  (a2) **the chain still plays EARLY (OPEN)** — a BEAT-duration gap, NOT the dialogue
  gate: under the dense-confirm re-drive the house starts ~−10t, errands ~−26t (the
  advances match, but the room-transition FADE beats settle short).  Suspects: the
  scene_fade grid settle-rate + the exit/entry WAITs (measured stand-ins off the OLD
  retail.osr).  RE the fade PERFORMER's per-tick step off retail-stairs before
  adjusting — do NOT curve-fit.  `findings/dialogue-advance-early.md` "Component 2/3".
  (b) **missing house props** (mark t2278: the stove's steaming COOKING POT + the
  kitchen HUTCH with dishes, upstairs) — the unported object-spawn PLACEHOLDER pass
  (`0x58c8c0` is a 4-B getter; the real spawn family is `0x58c8d0`/`0x58cb30`; the
  res_explorer already renders these host-side).  **CAVEAT (ckpt 176): the t2278 raw
  differ (22498) is CONFOUNDED by the char-turn (c) offset — the port is ~960 wx ahead
  on the LEFT walk, so the whole crop is shifted; the port DOES render most of the scene
  (res 1071/1072/1082/1722/1026 all present at t2278).  Assess (b) AFTER (c), or at a
  PRE-reversal tick, to isolate the truly-missing placeholder objects.**  Mom's pose
  in-crop differs too — check her clip once the props land.  (Visual-verify: deferred.)
  (c) `char-turn-state` — the STATE-1 reversal turn FSM `0x442a70:1011-1090` (deep,
  entangled with the bit-exact walk; verify via `state_diff` synth-stairs, no regress).
  (d) HUD: door-indicator spawn source / bottom strips; `mover-actor-scan` when
  collidable actors matter.
- **Open PORT-DEBT (this front):** `char-turn-state`, `mover-actor-scan`,
  `char-drop-through`, `char-reverse-decel`; HUD: `hud-party-context`,
  `hud-door-actors`, `hud-slide`/`hud-item-hslide`. See `port-debt.md`.
- **Standing bar:** every divergence is `differ_px==0` or a named/understood residual
  (`parity-ledger.md`); attribute to a pillar before suspecting logic (`parity-model.md`);
  seed-pinned both sides, compared by anchor/tick — never the flip axis.
- **Read next:** changelog → `PROGRESS.md`; deep RE → `findings/` (esp. `freeroam-hud.md`,
  `freeroam-brake-onset.md`); module layout + open threads → `memories/HANDOFF.md`
  (last rewritten ckpt 155 — stale on the HUD arc).
<!-- FRONT:END -->
